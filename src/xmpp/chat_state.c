/*
 * chat_state.c
 *
 * Copyright (C) 2012 - 2017 James Booth <boothj5@gmail.com>
 *
 * This file is part of Profanity.
 *
 * Profanity is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Profanity is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Profanity.  If not, see <https://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give permission to
 * link the code of portions of this program with the OpenSSL library under
 * certain conditions as described in each individual source file, and
 * distribute linked combinations including the two.
 *
 * You must obey the GNU General Public License in all respects for all of the
 * code used other than OpenSSL. If you modify file(s) with this exception, you
 * may extend this exception to your version of the file(s), but you are not
 * obligated to do so. If you do not wish to do so, delete this exception
 * statement from your version. If you delete this exception statement from all
 * source files in the program, then also delete it here.
 *
 */

#include <stdlib.h>
#include <assert.h>

#include <glib.h>

#include "config/preferences.h"
#include "ui/window_list.h"
#include "ui/win_types.h"
#include "xmpp/xmpp.h"
#include "xmpp/chat_state.h"
#include "xmpp/chat_session.h"

#define PAUSED_TIMEOUT 10.0
#define INACTIVE_TIMEOUT 30.0

static void _send_if_supported(const char *const barejid, void (*send_func)(const char *const));

ChatState*
chat_state_new(void)
{
    ChatState *new_state = malloc(sizeof(struct prof_chat_state_t));
    new_state->type = CHAT_STATE_GONE;
    new_state->timer = g_timer_new();

    return new_state;
}

void
chat_state_free(ChatState *state)
{
    if (state && state->timer!= NULL) {
        g_timer_destroy(state->timer);
    }
    free(state);
}

void
chat_state_handle_idle(const char *const barejid, ChatState *state)
{
    gdouble elapsed = g_timer_elapsed(state->timer, NULL);

    // TYPING -> PAUSED
    if (state->type == CHAT_STATE_COMPOSING && elapsed > PAUSED_TIMEOUT) {
        state->type = CHAT_STATE_PAUSED;
        g_timer_start(state->timer);
        if (prefs_get_boolean(PREF_STATES) && prefs_get_boolean(PREF_OUTTYPE)) {
            _send_if_supported(barejid, message_send_paused);
        }
        return;
    }

    // PAUSED|ACTIVE -> INACTIVE
    if ((state->type == CHAT_STATE_PAUSED || state->type == CHAT_STATE_ACTIVE) && elapsed > INACTIVE_TIMEOUT) {
        state->type = CHAT_STATE_INACTIVE;
        g_timer_start(state->timer);
        if (prefs_get_boolean(PREF_STATES)) {
            _send_if_supported(barejid, message_send_inactive);
        }
        return;

    }

    // INACTIVE -> GONE
    if (state->type == CHAT_STATE_INACTIVE) {
        if (prefs_get_gone() != 0 && (elapsed > (prefs_get_gone() * 60.0))) {
            ChatSession *session = chat_session_get(barejid);
            if (session) {
                // never move to GONE when resource override
                if (!session->resource_override) {
                    if (prefs_get_boolean(PREF_STATES)) {
                        _send_if_supported(barejid, message_send_gone);
                    }
                    chat_session_remove(barejid);
                    state->type = CHAT_STATE_GONE;
                    g_timer_start(state->timer);
                }
            } else {
                if (prefs_get_boolean(PREF_STATES)) {
                    message_send_gone(barejid);
                }
                state->type = CHAT_STATE_GONE;
                g_timer_start(state->timer);
            }
            return;
        }
    }
}

void
chat_state_handle_typing(const char *const barejid, ChatState *state)
{
    // ACTIVE|INACTIVE|PAUSED|GONE -> COMPOSING
    if (state->type != CHAT_STATE_COMPOSING) {
        state->type = CHAT_STATE_COMPOSING;
        g_timer_start(state->timer);
        if (prefs_get_boolean(PREF_STATES) && prefs_get_boolean(PREF_OUTTYPE)) {
            _send_if_supported(barejid, message_send_composing);
        }
    }
}

void
chat_state_active(ChatState *state)
{
    state->type = CHAT_STATE_ACTIVE;
    g_timer_start(state->timer);
}

void
chat_state_gone(const char *const barejid, ChatState *state)
{
    if (state->type != CHAT_STATE_GONE) {
        if (prefs_get_boolean(PREF_STATES)) {
            _send_if_supported(barejid, message_send_gone);
        }
        state->type = CHAT_STATE_GONE;
        g_timer_start(state->timer);
    }
}

void
chat_state_idle(void)
{
    jabber_conn_status_t status = connection_get_status();
    if (status == JABBER_CONNECTED) {
        GSList *recipients = wins_get_chat_recipients();
        GSList *curr = recipients;

        while (curr) {
            char *barejid = curr->data;
            ProfChatWin *chatwin = wins_get_chat(barejid);
            chat_state_handle_idle(chatwin->barejid, chatwin->state);
            curr = g_slist_next(curr);
        }

        if (recipients) {
            g_slist_free(recipients);
        }
    }
}

void
chat_state_activity(void)
{
    jabber_conn_status_t status = connection_get_status();
    ProfWin *current = wins_get_current();

    if ((status == JABBER_CONNECTED) && (current->type == WIN_CHAT)) {
        ProfChatWin *chatwin = (ProfChatWin*)current;
        assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
        chat_state_handle_typing(chatwin->barejid, chatwin->state);
    }
}

static void
_send_if_supported(const char *const barejid, void (*send_func)(const char *const))
{
    gboolean send = TRUE;
    GString *jid = g_string_new(barejid);
    ChatSession *session = chat_session_get(barejid);
    if (session) {
        if (session->send_states) {
            g_string_append(jid, "/");
            g_string_append(jid, session->resource);
        } else {
            send = FALSE;
        }
    }

    if (send) {
        send_func(jid->str);
    }

    g_string_free(jid, TRUE);
}
