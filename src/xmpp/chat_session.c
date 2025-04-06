/*
 * chat_session.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
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

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <glib.h>

#include "log.h"
#include "config/preferences.h"
#include "xmpp/xmpp.h"
#include "xmpp/stanza.h"
#include "xmpp/chat_session.h"

static GHashTable* sessions;

/**
 * @brief Create a new chat session object and insert into the session hash table.
 *
 * @param barejid Bare JID of the contact.
 * @param resource Resource identifier.
 * @param resource_override Whether to override automatic resource updates.
 * @param send_states Whether to send chat state notifications.
 */
static void
_chat_session_new(const char* const barejid, const char* const resource, gboolean resource_override,
                  gboolean send_states)
{
    assert(barejid != NULL);
    assert(resource != NULL);

    ChatSession* new_session = calloc(1, sizeof(*new_session));
    new_session->barejid = strdup(barejid);
    new_session->resource = strdup(resource);
    new_session->resource_override = resource_override;
    new_session->send_states = send_states;

    g_hash_table_replace(sessions, strdup(barejid), new_session);
}

/**
 * @brief Free memory allocated for a ChatSession struct.
 *
 * @param session Pointer to ChatSession struct.
 */
static void
_chat_session_free(ChatSession* session)
{
    if (session) {
        free(session->resource);
        free(session->barejid);
        free(session);
    }
}

/**
 * @brief Initialize the chat session hash table.
 */
void
chat_sessions_init(void)
{
    if (sessions) {
        g_hash_table_destroy(sessions);
    }

    sessions = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)_chat_session_free);
}

/**
 * @brief Clear all chat sessions and free memory.
 */
void
chat_sessions_clear(void)
{
    if (sessions) {
        g_hash_table_destroy(sessions);
        sessions = NULL;
    }
}


/**
 * @brief Forcefully override the resource used for a session.
 *
 * Useful when a specific resource must be locked (e.g., for encrypted chats).
 *
 * @param barejid The bare JID of the contact.
 * @param resource The resource to use for future communication.
 */
void
chat_session_resource_override(const char* const barejid, const char* const resource)
{
    _chat_session_new(barejid, resource, TRUE, TRUE);
}

/**
 * @brief Get the current ChatSession for a given bare JID.
 *
 * @param barejid The bare JID to look up.
 * @return Pointer to ChatSession or NULL if not found.
 */
ChatSession*
chat_session_get(const char* const barejid)
{
    return sessions ? g_hash_table_lookup(sessions, barejid) : NULL;
}

/**
 * @brief Get the full JID (bare + resource) for a contact.
 *
 * If no session exists, returns the bare JID.
 *
 * @param barejid The bare JID of the contact.
 * @return Newly allocated full JID string.
 */
char*
chat_session_get_jid(const char* const barejid)
{
    ChatSession* session = chat_session_get(barejid);
    char* jid = NULL;
    if (session) {
        auto_jid Jid* jidp = jid_create_from_bare_and_resource(session->barejid, session->resource);
        jid = strdup(jidp->fulljid);
    } else {
        jid = strdup(barejid);
    }

    return jid;
}

/**
 * @brief Determine whether to send a chat state for a contact.
 *
 * Returns "active" if states are enabled and should be sent.
 *
 * @param barejid The bare JID of the contact.
 * @return A constant string representing the chat state ("active") or NULL.
 */
const char*
chat_session_get_state(const char* const barejid)
{
    ChatSession* session = chat_session_get(barejid);
    const char* state = NULL;
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

/**
 * @brief Mark a contact as having gone offline (resource no longer available).
 *
 * If the session has no override, it will be removed.
 *
 * @param barejid The bare JID of the contact.
 * @param resource The resource that went offline.
 */
void
chat_session_recipient_gone(const char* const barejid, const char* const resource)
{
    assert(barejid != NULL);
    assert(resource != NULL);

    ChatSession* session = g_hash_table_lookup(sessions, barejid);
    if (session && g_strcmp0(session->resource, resource) == 0) {
        if (!session->resource_override) {
            chat_session_remove(barejid);
        }
    }
}

/**
 * @brief Mark a contact as currently typing.
 *
 * @param barejid The bare JID of the contact.
 * @param resource The resource sending the typing notification.
 */
void
chat_session_recipient_typing(const char* const barejid, const char* const resource)
{
    chat_session_recipient_active(barejid, resource, TRUE);
}

/**
 * @brief Mark a contact as having paused typing.
 *
 * @param barejid The bare JID of the contact.
 * @param resource The resource sending the paused notification.
 */
void
chat_session_recipient_paused(const char* const barejid, const char* const resource)
{
    chat_session_recipient_active(barejid, resource, TRUE);
}


/**
 * @brief Mark a contact as inactive (not typing).
 *
 * @param barejid The bare JID of the contact.
 * @param resource The resource sending the inactive notification.
 */
void
chat_session_recipient_inactive(const char* const barejid, const char* const resource)
{
    chat_session_recipient_active(barejid, resource, TRUE);
}

/**
 * @brief Mark a contact as active, optionally updating their chat state setting.
 *
 * This is used internally by the above functions to unify state updates.
 *
 * @param barejid The bare JID of the contact.
 * @param resource The resource sending presence/chat state.
 * @param send_states Whether we should send chat states to this contact.
 */
void
chat_session_recipient_active(const char* const barejid, const char* const resource,
                              gboolean send_states)
{
    assert(barejid != NULL);
    assert(resource != NULL);

    ChatSession* session = g_hash_table_lookup(sessions, barejid);
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

/**
 * @brief Remove a specific chat session by bare JID.
 *
 * @param barejid The bare JID (user@domain) of the contact.
 */
void
chat_session_remove(const char* const barejid)
{
    g_hash_table_remove(sessions, barejid);
}
