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
 */

#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "chat_session.h"

#include "config/preferences.h"
#include "log.h"

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

struct chat_session_t {
    char *recipient;
    gboolean recipient_supports;
    chat_state_t state;
    GTimer *active_timer;
    gboolean sent;
};

typedef struct chat_session_t *ChatSession;

static GHashTable *sessions;

static void _chat_session_free(ChatSession session);

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

void
chat_session_start(const char * const recipient, gboolean recipient_supports)
{
    ChatSession new_session = malloc(sizeof(struct chat_session_t));
    new_session->recipient = strdup(recipient);
    new_session->recipient_supports = recipient_supports;
    new_session->state = CHAT_STATE_STARTED;
    new_session->active_timer = g_timer_new();
    new_session->sent = FALSE;
    g_hash_table_insert(sessions, strdup(recipient), new_session);
}

gboolean
chat_session_exists(const char * const recipient)
{
    ChatSession session = g_hash_table_lookup(sessions, recipient);

    if (session != NULL) {
        return TRUE;
    } else {
        return FALSE;
    }
}

void
chat_session_set_composing(const char * const recipient)
{
    ChatSession session = g_hash_table_lookup(sessions, recipient);

    if (session != NULL) {
        if (session->state != CHAT_STATE_COMPOSING) {
            session->sent = FALSE;
        }
        session->state = CHAT_STATE_COMPOSING;
        g_timer_start(session->active_timer);
    }
}

void
chat_session_no_activity(const char * const recipient)
{
    ChatSession session = g_hash_table_lookup(sessions, recipient);

    if (session != NULL) {
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
    }
}

void
chat_session_set_sent(const char * const recipient)
{
    ChatSession session = g_hash_table_lookup(sessions, recipient);

    if (session != NULL) {
        session->sent = TRUE;
    }
}

gboolean
chat_session_get_sent(const char * const recipient)
{
    ChatSession session = g_hash_table_lookup(sessions, recipient);

    if (session == NULL) {
        return FALSE;
    } else {
        return session->sent;
    }
}

void
chat_session_end(const char * const recipient)
{
    g_hash_table_remove(sessions, recipient);
}

gboolean
chat_session_is_inactive(const char * const recipient)
{
    ChatSession session = g_hash_table_lookup(sessions, recipient);

    if (session == NULL) {
        return FALSE;
    } else {
        return (session->state == CHAT_STATE_INACTIVE);
    }
}

gboolean
chat_session_is_active(const char * const recipient)
{
    ChatSession session = g_hash_table_lookup(sessions, recipient);

    if (session == NULL) {
        return FALSE;
    } else {
        return (session->state == CHAT_STATE_ACTIVE);
    }
}

void
chat_session_set_active(const char * const recipient)
{
    ChatSession session = g_hash_table_lookup(sessions, recipient);

    if (session != NULL) {
        session->state = CHAT_STATE_ACTIVE;
        g_timer_start(session->active_timer);
        session->sent = TRUE;
    }
}

gboolean
chat_session_is_paused(const char * const recipient)
{
    ChatSession session = g_hash_table_lookup(sessions, recipient);

    if (session == NULL) {
        return FALSE;
    } else {
        return (session->state == CHAT_STATE_PAUSED);
    }
}

gboolean
chat_session_is_gone(const char * const recipient)
{
    ChatSession session = g_hash_table_lookup(sessions, recipient);

    if (session == NULL) {
        return FALSE;
    } else {
        return (session->state == CHAT_STATE_GONE);
    }
}

void
chat_session_set_gone(const char * const recipient)
{
    ChatSession session = g_hash_table_lookup(sessions, recipient);

    if (session != NULL) {
        session->state = CHAT_STATE_GONE;
    }
}

gboolean
chat_session_get_recipient_supports(const char * const recipient)
{
    ChatSession session = g_hash_table_lookup(sessions, recipient);

    if (session == NULL) {
        return FALSE;
    } else {
        return session->recipient_supports;
    }
}

void
chat_session_set_recipient_supports(const char * const recipient,
    gboolean recipient_supports)
{
    ChatSession session = g_hash_table_lookup(sessions, recipient);

    if (session != NULL) {
        session->recipient_supports = recipient_supports;
    }
}

static void
_chat_session_free(ChatSession session)
{
    if (session != NULL) {
        free(session->recipient);
        if (session->active_timer != NULL) {
            g_timer_destroy(session->active_timer);
            session->active_timer = NULL;
        }
        free(session);
    }
}
