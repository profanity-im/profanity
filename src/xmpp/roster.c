/*
 * roster.c
 *
 * Copyright (C) 2012, 2013 James Booth <boothj5@gmail.com>
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

#include <string.h>

#include <glib.h>
#include <strophe.h>

#include "contact_list.h"
#include "log.h"
#include "xmpp/connection.h"
#include "xmpp/stanza.h"
#include "xmpp/xmpp.h"

#define HANDLE(type, func) xmpp_handler_add(conn, func, XMPP_NS_ROSTER, STANZA_NAME_IQ, type, ctx)

static int _roster_handle_set(xmpp_conn_t * const conn,
    xmpp_stanza_t * const stanza, void * const userdata);
static int _roster_handle_result(xmpp_conn_t * const conn,
    xmpp_stanza_t * const stanza, void * const userdata);

void
roster_add_handlers(void)
{
    xmpp_conn_t * const conn = connection_get_conn();
    xmpp_ctx_t * const ctx = connection_get_ctx();
    HANDLE(STANZA_TYPE_SET,    _roster_handle_set);
    HANDLE(STANZA_TYPE_RESULT, _roster_handle_result);
}

void
roster_request(void)
{
    xmpp_conn_t * const conn = connection_get_conn();
    xmpp_ctx_t * const ctx = connection_get_ctx();
    xmpp_stanza_t *iq = stanza_create_roster_iq(ctx);
    xmpp_send(conn, iq);
    xmpp_stanza_release(iq);
}

static int
_roster_handle_set(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza,
    void * const userdata)
{
    xmpp_stanza_t *query = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_QUERY);
    xmpp_stanza_t *item =
        xmpp_stanza_get_child_by_name(query, STANZA_NAME_ITEM);

    if (item == NULL) {
        return 1;
    }

    const char *jid = xmpp_stanza_get_attribute(item, STANZA_ATTR_JID);
    const char *sub = xmpp_stanza_get_attribute(item, STANZA_ATTR_SUBSCRIPTION);
    if (g_strcmp0(sub, "remove") == 0) {
        contact_list_remove(jid);
        return 1;
    }

    gboolean pending_out = FALSE;
    const char *ask = xmpp_stanza_get_attribute(item, STANZA_ATTR_ASK);
    if ((ask != NULL) && (strcmp(ask, "subscribe") == 0)) {
        pending_out = TRUE;
    }

    contact_list_update_subscription(jid, sub, pending_out);

    return 1;
}

static int
_roster_handle_result(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza,
    void * const userdata)
{
    const char *id = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_ID);

    // handle initial roster response
    if (g_strcmp0(id, "roster") == 0) {
        xmpp_stanza_t *query = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_QUERY);
        xmpp_stanza_t *item = xmpp_stanza_get_children(query);

        while (item != NULL) {
            const char *barejid = xmpp_stanza_get_attribute(item, STANZA_ATTR_JID);
            const char *name = xmpp_stanza_get_attribute(item, STANZA_ATTR_NAME);
            const char *sub = xmpp_stanza_get_attribute(item, STANZA_ATTR_SUBSCRIPTION);

            gboolean pending_out = FALSE;
            const char *ask = xmpp_stanza_get_attribute(item, STANZA_ATTR_ASK);
            if (g_strcmp0(ask, "subscribe") == 0) {
                pending_out = TRUE;
            }

            gboolean added = contact_list_add(barejid, name, sub, NULL, pending_out);

            if (!added) {
                log_warning("Attempt to add contact twice: %s", barejid);
            }

            item = xmpp_stanza_get_next(item);
        }

        contact_presence_t conn_presence = accounts_get_login_presence(jabber_get_account_name());
        presence_update(conn_presence, NULL, 0);
    }

    return 1;
}
