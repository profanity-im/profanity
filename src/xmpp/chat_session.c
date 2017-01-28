/*
 * chat_session.c
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
#include <string.h>
#include <assert.h>

#include <glib.h>

#include "log.h"
#include "config/preferences.h"
#include "xmpp/xmpp.h"
#include "xmpp/stanza.h"
#include "xmpp/chat_session.h"

static GHashTable *sessions;

static void
_chat_session_new(const char *const barejid, const char *const resource, gboolean resource_override,
    gboolean send_states)
{
    assert(barejid != NULL);
    assert(resource != NULL);

    ChatSession *new_session = malloc(sizeof(struct chat_session_t));
    new_session->barejid = strdup(barejid);
    new_session->resource = strdup(resource);
    new_session->resource_override = resource_override;
    new_session->send_states = send_states;

    g_hash_table_replace(sessions, strdup(barejid), new_session);
}

static void
_chat_session_free(ChatSession *session)
{
    if (session) {
        free(session->barejid);
        free(session->resource);
        free(session);
    }
}

void
chat_sessions_init(void)
{
    if (sessions) {
        g_hash_table_destroy(sessions);
    }

    sessions = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)_chat_session_free);
}

void
chat_sessions_clear(void)
{
    if (sessions)
        g_hash_table_remove_all(sessions);
}

void
chat_session_resource_override(const char *const barejid, const char *const resource)
{
    _chat_session_new(barejid, resource, TRUE, TRUE);
}

ChatSession*
chat_session_get(const char *const barejid)
{
    return g_hash_table_lookup(sessions, barejid);
}

char*
chat_session_get_jid(const char *const barejid)
{
    ChatSession *session = chat_session_get(barejid);
    char *jid = NULL;
    if (session) {
        Jid *jidp = jid_create_from_bare_and_resource(session->barejid, session->resource);
        jid = strdup(jidp->fulljid);
        jid_destroy(jidp);
    } else {
        jid = strdup(barejid);
    }

    return jid;
}

char*
chat_session_get_state(const char *const barejid)
{
    ChatSession *session = chat_session_get(barejid);
    char *state = NULL;
    if (session) {
        if (prefs_get_boolean(PREF_STATES) && session->send_states) {
            state = STANZA_NAME_ACTIVE;
        }
    } else {
        if (prefs_get_boolean(PREF_STATES)) {
            state = STANZA_NAME_ACTIVE;
        }
    }

    return state;
}

void
chat_session_recipient_gone(const char *const barejid, const char *const resource)
{
    assert(barejid != NULL);
    assert(resource != NULL);

    ChatSession *session = g_hash_table_lookup(sessions, barejid);
    if (session && g_strcmp0(session->resource, resource) == 0) {
        if (!session->resource_override) {
            chat_session_remove(barejid);
        }
    }
}

void
chat_session_recipient_typing(const char *const barejid, const char *const resource)
{
    chat_session_recipient_active(barejid, resource, TRUE);
}

void
chat_session_recipient_paused(const char *const barejid, const char *const resource)
{
    chat_session_recipient_active(barejid, resource, TRUE);
}

void
chat_session_recipient_inactive(const char *const barejid, const char *const resource)
{
    chat_session_recipient_active(barejid, resource, TRUE);
}

void
chat_session_recipient_active(const char *const barejid, const char *const resource,
    gboolean send_states)
{
    assert(barejid != NULL);
    assert(resource != NULL);

    ChatSession *session = g_hash_table_lookup(sessions, barejid);
    if (session) {
        // session exists with resource, update chat_states
        if (g_strcmp0(session->resource, resource) == 0) {
            session->send_states = send_states;
        // session exists with different resource and no override, replace
        } else if (!session->resource_override) {
            _chat_session_new(barejid, resource, FALSE, send_states);
        }

    // no session, create one
    } else {
        _chat_session_new(barejid, resource, FALSE, send_states);
    }
}

void
chat_session_remove(const char *const barejid)
{
    g_hash_table_remove(sessions, barejid);
}
