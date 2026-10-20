/*
 * chat_session.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
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

static void
_chat_session_new(const gchar* const barejid, const gchar* const resource, gboolean resource_override,
                  gboolean send_states)
{
    assert(barejid != NULL);
    assert(resource != NULL);

    ChatSession* new_session = g_new0(ChatSession, 1);
    new_session->barejid = g_strdup(barejid);
    new_session->resource = g_strdup(resource);
    new_session->resource_override = resource_override;
    new_session->send_states = send_states;

    g_hash_table_replace(sessions, g_strdup(barejid), new_session);
}

static void
_chat_session_free(ChatSession* session)
{
    if (session) {
        g_free(session->resource);
        g_free(session->barejid);
        g_free(session);
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
    if (sessions) {
        g_hash_table_destroy(sessions);
        sessions = NULL;
    }
}

void
chat_session_resource_override(const gchar* const barejid, const gchar* const resource)
{
    _chat_session_new(barejid, resource, TRUE, TRUE);
}

ChatSession*
chat_session_get(const gchar* const barejid)
{
    return sessions ? g_hash_table_lookup(sessions, barejid) : NULL;
}

gchar*
chat_session_get_jid(const gchar* const barejid)
{
    ChatSession* session = chat_session_get(barejid);
    gchar* jid = NULL;
    if (session) {
        auto_jid Jid* jidp = jid_create_from_bare_and_resource(session->barejid, session->resource);
        jid = g_strdup(jidp->fulljid);
    } else {
        jid = g_strdup(barejid);
    }

    return jid;
}

const gchar*
chat_session_get_state(const gchar* const barejid)
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

void
chat_session_recipient_gone(const gchar* const barejid, const gchar* const resource)
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

void
chat_session_recipient_typing(const gchar* const barejid, const gchar* const resource)
{
    chat_session_recipient_active(barejid, resource, TRUE);
}

void
chat_session_recipient_paused(const gchar* const barejid, const gchar* const resource)
{
    chat_session_recipient_active(barejid, resource, TRUE);
}

void
chat_session_recipient_inactive(const gchar* const barejid, const gchar* const resource)
{
    chat_session_recipient_active(barejid, resource, TRUE);
}

void
chat_session_recipient_active(const gchar* const barejid, const gchar* const resource,
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

void
chat_session_remove(const gchar* const barejid)
{
    g_hash_table_remove(sessions, barejid);
}
