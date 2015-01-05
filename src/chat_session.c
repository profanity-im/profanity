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
#include <assert.h>

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
    gboolean send_states;
    chat_state_t state;
    GTimer *active_timer;
    gboolean sent;
    gboolean includes_message;
} ChatSession;

static GHashTable *sessions;

static ChatSession*
_chat_session_new(const char * const barejid, const char * const resource, gboolean send_states)
{
    ChatSession *new_session = malloc(sizeof(struct chat_session_t));
    new_session->barejid = strdup(barejid);
    if (resource) {
        new_session->resource = strdup(resource);
    } else {
        new_session->resource = NULL;
    }
    new_session->send_states = send_states;
    new_session->state = CHAT_STATE_STARTED;
    new_session->active_timer = g_timer_new();
    new_session->sent = FALSE;
    new_session->includes_message = FALSE;

    return new_session;
}

static void
_chat_session_free(ChatSession *session)
{
    if (session != NULL) {
        free(session->barejid);
        free(session->resource);
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
chat_session_exists(const char * const barejid)
{
    ChatSession *session = g_hash_table_lookup(sessions, barejid);

    return (session != NULL);
}

char*
chat_session_get_resource(const char * const barejid)
{
    ChatSession *session = g_hash_table_lookup(sessions, barejid);
    assert(session != NULL);

    return session->resource;
}

gboolean
chat_session_send_states(const char * const barejid)
{
    ChatSession *session = g_hash_table_lookup(sessions, barejid);
    assert(session != NULL);

    return session->send_states;
}

void
chat_session_on_incoming_message(const char * const barejid, const char * const resource, gboolean send_states)
{
    ChatSession *session = g_hash_table_lookup(sessions, barejid);
    GString *log_msg = g_string_new("");

    // no session exists, create one
    if (!session) {
        g_string_append(log_msg, "Creating chat session for ");
        g_string_append(log_msg, barejid);
        if (resource) {
            g_string_append(log_msg, "/");
            g_string_append(log_msg, resource);
        }
        session = _chat_session_new(barejid, resource, send_states);
        g_hash_table_insert(sessions, strdup(barejid), session);

    // session exists for different resource, replace session
    } else if (g_strcmp0(session->resource, resource) != 0) {
        g_string_append(log_msg, "Replacing chat session for ");
        g_string_append(log_msg, barejid);
        if (session->resource) {
            g_string_append(log_msg, "/");
            g_string_append(log_msg, session->resource);
        }
        g_string_append(log_msg, " with session for ");
        g_string_append(log_msg, barejid);
        if (resource) {
            g_string_append(log_msg, "/");
            g_string_append(log_msg, resource);
        }
        g_hash_table_remove(sessions, session);
        session = _chat_session_new(barejid, resource, send_states);
        session->includes_message = TRUE;
        g_hash_table_insert(sessions, strdup(barejid), session);

    // session exists for resource, update state
    } else {
        g_string_append(log_msg, "Updating chat session for ");
        g_string_append(log_msg, session->barejid);
        if (session->resource) {
            g_string_append(log_msg, "/");
            g_string_append(log_msg, session->resource);
        }
        session->send_states = send_states;
        session->includes_message = TRUE;
    }
    if (send_states) {
        g_string_append(log_msg, ", chat states supported");
    } else {
        g_string_append(log_msg, ", chat states not supported");
    }

    log_debug(log_msg->str);
    g_string_free(log_msg, TRUE);
}

void
chat_session_on_message_send(const char * const barejid)
{
    ChatSession *session = g_hash_table_lookup(sessions, barejid);

    // if no session exists, create one with no resource, and send states
    if (!session) {
        log_debug("Creating chat session for %s, chat states supported", barejid);
        session = _chat_session_new(barejid, NULL, TRUE);
        g_hash_table_insert(sessions, strdup(barejid), session);
    }

    session->state = CHAT_STATE_ACTIVE;
    g_timer_start(session->active_timer);
    session->sent = TRUE;
    session->includes_message = TRUE;
}

void
chat_session_on_window_close(const char * const barejid)
{
    ChatSession *session = g_hash_table_lookup(sessions, barejid);

    if (session) {
        if (prefs_get_boolean(PREF_STATES) && session->send_states && session->includes_message) {
            GString *jid = g_string_new(session->barejid);
            if (session->resource) {
                g_string_append(jid, "/");
                g_string_append(jid, session->resource);
            }
            message_send_gone(jid->str);
            g_string_free(jid, TRUE);
        }
        GString *log_msg = g_string_new("Removing chat session for ");
        g_string_append(log_msg, barejid);
        if (session->resource) {
            g_string_append(log_msg, "/");
            g_string_append(log_msg, session->resource);
        }
        log_debug(log_msg->str);
        g_string_free(log_msg, TRUE);
        g_hash_table_remove(sessions, barejid);
    }
}

void
chat_session_on_activity(const char * const barejid)
{
    ChatSession *session = g_hash_table_lookup(sessions, barejid);

    if (!session) {
        log_debug("Creating chat session for %s, chat states supported", barejid);
        session = _chat_session_new(barejid, NULL, TRUE);
        g_hash_table_insert(sessions, strdup(barejid), session);
    }

    if (session->state != CHAT_STATE_COMPOSING) {
        session->sent = FALSE;
    }

    session->state = CHAT_STATE_COMPOSING;
    g_timer_start(session->active_timer);

    if (!session->sent || session->state == CHAT_STATE_PAUSED) {
        if (prefs_get_boolean(PREF_STATES) && prefs_get_boolean(PREF_OUTTYPE) && session->send_states) {
            GString *jid = g_string_new(session->barejid);
            if (session->resource) {
                g_string_append(jid, "/");
                g_string_append(jid, session->resource);
            }
            message_send_composing(jid->str);
            g_string_free(jid, TRUE);
        }
        session->sent = TRUE;
    }
}

void
chat_session_on_inactivity(const char * const barejid)
{
    ChatSession *session = g_hash_table_lookup(sessions, barejid);
    if (!session) {
        return;
    }

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
        GString *jid = g_string_new(session->barejid);
        if (session->resource) {
            g_string_append(jid, "/");
            g_string_append(jid, session->resource);
        }
        if (session->state == CHAT_STATE_GONE) {
            if (prefs_get_boolean(PREF_STATES) && session->send_states && session->includes_message) {
                message_send_gone(jid->str);
            }
            session->sent = TRUE;
            GString *log_msg = g_string_new("Removing chat session for ");
            g_string_append(log_msg, barejid);
            if (session->resource) {
                g_string_append(log_msg, "/");
                g_string_append(log_msg, session->resource);
            }
            log_debug(log_msg->str);
            g_string_free(log_msg, TRUE);
            g_hash_table_remove(sessions, barejid);
        } else if (session->state == CHAT_STATE_INACTIVE) {
            if (prefs_get_boolean(PREF_STATES) && session->send_states) {
                message_send_inactive(jid->str);
            }
            session->sent = TRUE;
        } else if (session->state == CHAT_STATE_PAUSED && prefs_get_boolean(PREF_OUTTYPE)) {
            if (prefs_get_boolean(PREF_STATES) && session->send_states) {
                message_send_paused(jid->str);
            }
            session->sent = TRUE;
        }
        g_string_free(jid, TRUE);
    }
}

void
chat_session_on_cancel(const char * const jid)
{
    Jid *jidp = jid_create(jid);
    if (jidp) {
        ChatSession *session = g_hash_table_lookup(sessions, jidp->barejid);
        if (session) {
            GString *log_msg = g_string_new("Removing chat session for ");
            g_string_append(log_msg, jidp->barejid);
            if (session->resource) {
                g_string_append(log_msg, "/");
                g_string_append(log_msg, session->resource);
            }
            log_debug(log_msg->str);
            g_string_free(log_msg, TRUE);
            g_hash_table_remove(sessions, jidp->barejid);
        }
    }
}