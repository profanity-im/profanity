/*
 * chat_session.c
 *
 * Copyright (C) 2012 - 2014 James Booth <boothj5@gmail.com>
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
 * along with Profanity.  If not, see <http://www.gnu.org/licenses/>.
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
#include <string.h>

#include <glib.h>

#include "chat_session.h"
#include "config/preferences.h"
#include "log.h"
#include "xmpp/xmpp.h"

#define PAUSED_TIMOUT 10.0
#define INACTIVE_TIMOUT 30.0

typedef enum {
    CHAT_STATE_STARTED,
    CHAT_STATE_ACTIVE,
    CHAT_STATE_PAUSED,
    CHAT_STATE_COMPOSING,
    CHAT_STATE_INACTIVE,
    CHAT_STATE_GONE
} chat_state_t;

typedef struct chat_session_t {
    char *barejid;
    char *resource;
    gboolean supported;
    chat_state_t state;
    GTimer *active_timer;
    gboolean sent;
} ChatSession;

static GHashTable *sessions;

static ChatSession*
_chat_session_new(const char * const barejid, gboolean supported)
{
    ChatSession *new_session = malloc(sizeof(struct chat_session_t));
    new_session->barejid = strdup(barejid);
    new_session->resource = NULL;
    new_session->supported = supported;
    new_session->state = CHAT_STATE_STARTED;
    new_session->active_timer = g_timer_new();
    new_session->sent = FALSE;

    return new_session;
}

static void
_chat_session_free(ChatSession *session)
{
    if (session != NULL) {
        free(session->barejid);
        if (session->active_timer != NULL) {
            g_timer_destroy(session->active_timer);
            session->active_timer = NULL;
        }
        free(session);
    }
}

void
chat_sessions_init(void)
{
    sessions = g_hash_table_new_full(g_str_hash, g_str_equal, g_free,
        (GDestroyNotify)_chat_session_free);
}

void
chat_sessions_clear(void)
{
    if (sessions != NULL)
        g_hash_table_remove_all(sessions);
}

gboolean
chat_session_on_message_send(const char * const barejid)
{
    gboolean send_state = FALSE;
    if (prefs_get_boolean(PREF_STATES)) {
        ChatSession *session = g_hash_table_lookup(sessions, barejid);
        if (!session) {
            session = _chat_session_new(barejid, TRUE);
            g_hash_table_insert(sessions, strdup(barejid), session);

        }
        if (session->supported) {
            session->state = CHAT_STATE_ACTIVE;
            g_timer_start(session->active_timer);
            session->sent = TRUE;
            send_state = TRUE;
        }
    }

    return send_state;
}

void
chat_session_on_incoming_message(const char * const barejid, gboolean supported)
{
    ChatSession *session = g_hash_table_lookup(sessions, barejid);
    if (!session) {
        session = _chat_session_new(barejid, supported);
        g_hash_table_insert(sessions, strdup(barejid), session);
    } else {
        session->supported = supported;
    }
}

void
chat_session_on_window_open(const char * const barejid)
{
    if (prefs_get_boolean(PREF_STATES)) {
        ChatSession *session = g_hash_table_lookup(sessions, barejid);
        if (!session) {
            session = _chat_session_new(barejid, TRUE);
            g_hash_table_insert(sessions, strdup(barejid), session);
        }
    }
}

void
chat_session_on_window_close(const char * const barejid)
{
    if (prefs_get_boolean(PREF_STATES)) {
        ChatSession *session = g_hash_table_lookup(sessions, barejid);
        // send <gone/> chat state before closing
        if (session->supported) {
            session->state = CHAT_STATE_GONE;
            message_send_gone(barejid);
            session->sent = TRUE;
            g_hash_table_remove(sessions, barejid);
        }
    }
}

void
chat_session_on_cancel(const char * const jid)
{
    if (prefs_get_boolean(PREF_STATES)) {
        ChatSession *session = g_hash_table_lookup(sessions, jid);
        if (session) {
            session->supported = FALSE;
        }
    }
}

void
chat_session_on_activity(const char * const barejid)
{
    ChatSession *session = g_hash_table_lookup(sessions, barejid);
    if (session) {
        if (session->supported) {
            if (session->state != CHAT_STATE_COMPOSING) {
                session->sent = FALSE;
            }

            session->state = CHAT_STATE_COMPOSING;
            g_timer_start(session->active_timer);

            if (!session->sent || session->state == CHAT_STATE_PAUSED) {
                message_send_composing(barejid);
                session->sent = TRUE;
            }
        }
    }
}

void
chat_session_on_inactivity(const char * const barejid)
{
    ChatSession *session = g_hash_table_lookup(sessions, barejid);
    if (session && session->supported) {
        if (session->active_timer != NULL) {
            gdouble elapsed = g_timer_elapsed(session->active_timer, NULL);

            if ((prefs_get_gone() != 0) && (elapsed > (prefs_get_gone() * 60.0))) {
                if (session->state != CHAT_STATE_GONE) {
                    session->sent = FALSE;
                }
                session->state = CHAT_STATE_GONE;

            } else if (elapsed > INACTIVE_TIMOUT) {
                if (session->state != CHAT_STATE_INACTIVE) {
                    session->sent = FALSE;
                }
                session->state = CHAT_STATE_INACTIVE;

            } else if (elapsed > PAUSED_TIMOUT) {
                if (session->state == CHAT_STATE_COMPOSING) {
                    session->sent = FALSE;
                    session->state = CHAT_STATE_PAUSED;
                }
            }
        }

        if (session->sent == FALSE) {
            if (session->state == CHAT_STATE_GONE) {
                message_send_gone(barejid);
                session->sent = TRUE;
            } else if (session->state == CHAT_STATE_INACTIVE) {
                message_send_inactive(barejid);
                session->sent = TRUE;
            } else if (session->state == CHAT_STATE_PAUSED && prefs_get_boolean(PREF_OUTTYPE)) {
                message_send_paused(barejid);
                session->sent = TRUE;
            }
        }
    }
}