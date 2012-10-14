/* 
 * chat_session.c
 *
 * Copyright (C) 2012 James Booth <boothj5@gmail.com>
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

static ChatSession _chat_session_new(const char * const recipient);
static void _chat_session_free(ChatSession session);

struct chat_session_t {
    char *recipient;
    chat_state_t state;
    gboolean sent;
};

static GHashTable *sessions;

void
chat_session_init(void)
{
    sessions = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, 
        (GDestroyNotify)_chat_session_free);
}

void
chat_session_start(const char * const recipient)
{
    char *key = strdup(recipient);
    ChatSession session = _chat_session_new(key);
    g_hash_table_insert(sessions, key, session);
}

void
chat_session_end(const char * const recipient)
{
    g_hash_table_remove(sessions, recipient);
}

chat_state_t
chat_session_get_state(const char * const recipient)
{
    ChatSession session = g_hash_table_lookup(sessions, recipient);
    if (session == NULL) {
        return SESSION_ERR;
    } else {
        return session->state;
    }
}

void
chat_session_set_state(const char * const recipient, chat_state_t state)
{
    ChatSession session = g_hash_table_lookup(sessions, recipient);
    session->state = state;
}

gboolean
chat_session_get_sent(const char * const recipient)
{
    ChatSession session = g_hash_table_lookup(sessions, recipient);
    return session->sent;
}

void
chat_session_sent(const char * const recipient)
{
    ChatSession session = g_hash_table_lookup(sessions, recipient);
    session->sent = TRUE;
}

static ChatSession
_chat_session_new(const char * const recipient)
{
    ChatSession new_session = malloc(sizeof(struct chat_session_t));
    new_session->recipient = strdup(recipient);
    new_session->state = ACTIVE;
    new_session->sent = FALSE;
    
    return new_session;
}

static void
_chat_session_free(ChatSession session)
{
    if (session != NULL) {
        g_free(session->recipient);
        g_free(session);
    }
    session = NULL;
}
