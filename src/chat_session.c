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

static ChatSession _chat_session_new(char *recipient);
static void _chat_session_free(ChatSession session);

struct chat_session_t {
    GTimer *last_composing_sent;
    char *recipient;
    chat_state_t state;
};

static GHashTable *sessions;

void
chat_session_init(void)
{
    sessions = g_hash_table_new_full(NULL, g_str_equal, g_free, 
        (GDestroyNotify)_chat_session_free);
}

ChatSession
chat_session_new(char *recipient)
{
    ChatSession session = _chat_session_new(recipient);
    g_hash_table_insert(sessions, recipient, session);
    return session;
}

int
chat_session_size(void)
{
    return g_hash_table_size(sessions);
}

ChatSession
chat_session_get(char *recipient)
{
    return (ChatSession) g_hash_table_lookup(sessions, recipient);
}

chat_state_t
chat_session_get_state(ChatSession session)
{
    return session->state;
}

char *
chat_session_get_recipient(ChatSession session)
{
    return session->recipient;
}

static ChatSession
_chat_session_new(char *recipient)
{
    ChatSession new_session = malloc(sizeof(struct chat_session_t));
    new_session->last_composing_sent = NULL;
    new_session->recipient = malloc(strlen(recipient) + 1);
    strcpy(new_session->recipient, recipient);
    new_session->state = ACTIVE;
    
    return new_session;
}

static void
_chat_session_free(ChatSession session)
{
    if (session->last_composing_sent != NULL) {
        g_timer_destroy(session->last_composing_sent);
        session->last_composing_sent = NULL;
    }
    g_free(session->recipient);
    g_free(session);
}
