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
#include "log.h"

static ChatSession _chat_session_new(const char * const recipient,
    gboolean recipient_supports);
static void _chat_session_free(ChatSession session);

struct chat_session_t {
    char *recipient;
    gboolean recipient_supports;
};

static GHashTable *sessions;

void
chat_sessions_init(void)
{
    sessions = g_hash_table_new_full(g_str_hash, g_str_equal, g_free,
        (GDestroyNotify)_chat_session_free);
}

void
chat_sessions_clear(void)
{
    g_hash_table_remove_all(sessions);
}

void
chat_session_start(const char * const recipient, gboolean recipient_supports)
{
    char *key = strdup(recipient);
    ChatSession session = _chat_session_new(key, recipient_supports);
    g_hash_table_insert(sessions, key, session);
}

void
chat_session_end(const char * const recipient)
{
    g_hash_table_remove(sessions, recipient);
}

gboolean
chat_session_recipient_supports(const char * const recipient)
{
    ChatSession session = g_hash_table_lookup(sessions, recipient);

    if (session == NULL) {
        log_error("No chat session found for %s.", recipient);
        return FALSE;
    } else {
        return session->recipient_supports;
    }
}

static ChatSession
_chat_session_new(const char * const recipient, gboolean recipient_supports)
{
    ChatSession new_session = malloc(sizeof(struct chat_session_t));
    new_session->recipient = strdup(recipient);
    new_session->recipient_supports = recipient_supports;

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
