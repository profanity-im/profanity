/*
 * xmpp_iq.c
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

#include <stdlib.h>
#include <string.h>

#include <strophe.h>

#include "common.h"
#include "config.h"
#include "contact_list.h"
#include "log.h"
#include "stanza.h"
#include "xmpp.h"

#define HANDLE(ns, type, func) xmpp_handler_add(conn, func, ns, STANZA_NAME_IQ, type, ctx)

static int _iq_handle_error(xmpp_conn_t * const conn,
    xmpp_stanza_t * const stanza, void * const userdata);
static int _iq_handle_roster_set(xmpp_conn_t * const conn,
    xmpp_stanza_t * const stanza, void * const userdata);
static int _iq_handle_roster_result(xmpp_conn_t * const conn,
    xmpp_stanza_t * const stanza, void * const userdata);
static int _iq_handle_ping_get(xmpp_conn_t * const conn,
    xmpp_stanza_t * const stanza, void * const userdata);
static int _iq_handle_version_get(xmpp_conn_t * const conn,
    xmpp_stanza_t * const stanza, void * const userdata);
static int _iq_handle_discoinfo_get(xmpp_conn_t * const conn,
    xmpp_stanza_t * const stanza, void * const userdata);
static int _iq_handle_discoinfo_result(xmpp_conn_t * const conn,
    xmpp_stanza_t * const stanza, void * const userdata);

void
iq_add_handlers(xmpp_conn_t * const conn, xmpp_ctx_t * const ctx)
{
    HANDLE(NULL,                STANZA_TYPE_ERROR,  _iq_handle_error);
    HANDLE(XMPP_NS_ROSTER,      STANZA_TYPE_SET,    _iq_handle_roster_set);
    HANDLE(XMPP_NS_ROSTER,      STANZA_TYPE_RESULT, _iq_handle_roster_result);
    HANDLE(XMPP_NS_DISCO_INFO,  STANZA_TYPE_GET,    _iq_handle_discoinfo_get);
    HANDLE(XMPP_NS_DISCO_INFO,  STANZA_TYPE_RESULT, _iq_handle_discoinfo_result);
    HANDLE(STANZA_NS_VERSION,   STANZA_TYPE_GET,    _iq_handle_version_get);
    HANDLE(STANZA_NS_PING,      STANZA_TYPE_GET,    _iq_handle_ping_get);
}

static int
_iq_handle_error(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza,
    void * const userdata)
{
    const char *id = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_ID);

    if (id != NULL) {
        log_error("IQ error received, id: %s.", id);
    } else {
        log_error("IQ error recieved.");
    }

    return 1;
}

static int
_iq_handle_roster_set(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza,
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
_iq_handle_roster_result(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza,
    void * const userdata)
{
    const char *id = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_ID);

    // handle initial roster response
    if (g_strcmp0(id, "roster") == 0) {
        xmpp_stanza_t *query = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_QUERY);
        xmpp_stanza_t *item = xmpp_stanza_get_children(query);

        while (item != NULL) {
            const char *jid = xmpp_stanza_get_attribute(item, STANZA_ATTR_JID);
            const char *name = xmpp_stanza_get_attribute(item, STANZA_ATTR_NAME);
            const char *sub = xmpp_stanza_get_attribute(item, STANZA_ATTR_SUBSCRIPTION);

            gboolean pending_out = FALSE;
            const char *ask = xmpp_stanza_get_attribute(item, STANZA_ATTR_ASK);
            if (g_strcmp0(ask, "subscribe") == 0) {
                pending_out = TRUE;
            }

            gboolean added = contact_list_add(jid, name, "offline", NULL, sub,
                pending_out);

            if (!added) {
                log_warning("Attempt to add contact twice: %s", jid);
            }

            item = xmpp_stanza_get_next(item);
        }

        /* TODO: Save somehow last presence show and use it for initial
         *       presence rather than PRESENCE_ONLINE. It will be helpful
         *       when I set dnd status and reconnect for some reason */
        // send initial presence
        jabber_update_presence(PRESENCE_ONLINE, NULL, 0);
    }

    return 1;
}

static int
_iq_handle_ping_get(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza,
    void * const userdata)
{
    xmpp_ctx_t *ctx = (xmpp_ctx_t *)userdata;
    const char *id = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_ID);
    const char *to = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_TO);
    const char *from = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_FROM);

    if ((from == NULL) || (to == NULL)) {
        return 1;
    }

    xmpp_stanza_t *pong = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(pong, STANZA_NAME_IQ);
    xmpp_stanza_set_attribute(pong, STANZA_ATTR_TO, from);
    xmpp_stanza_set_attribute(pong, STANZA_ATTR_FROM, to);
    xmpp_stanza_set_attribute(pong, STANZA_ATTR_TYPE, STANZA_TYPE_RESULT);

    if (id != NULL) {
        xmpp_stanza_set_attribute(pong, STANZA_ATTR_ID, id);
    }

    xmpp_send(conn, pong);
    xmpp_stanza_release(pong);

    return 1;
}

static int
_iq_handle_version_get(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza,
    void * const userdata)
{
    xmpp_ctx_t *ctx = (xmpp_ctx_t *)userdata;
    const char *id = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_ID);
    const char *from = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_FROM);

    if (from != NULL) {
        xmpp_stanza_t *response = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(response, STANZA_NAME_IQ);
        if (id != NULL) {
            xmpp_stanza_set_id(response, id);
        }
        xmpp_stanza_set_attribute(response, STANZA_ATTR_TO, from);
        xmpp_stanza_set_type(response, STANZA_TYPE_RESULT);

        xmpp_stanza_t *query = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(query, STANZA_NAME_QUERY);
        xmpp_stanza_set_ns(query, STANZA_NS_VERSION);

        xmpp_stanza_t *name = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(name, "name");
        xmpp_stanza_t *name_txt = xmpp_stanza_new(ctx);
        xmpp_stanza_set_text(name_txt, "Profanity");
        xmpp_stanza_add_child(name, name_txt);

        xmpp_stanza_t *version = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(version, "version");
        xmpp_stanza_t *version_txt = xmpp_stanza_new(ctx);
        GString *version_str = g_string_new(PACKAGE_VERSION);
        if (strcmp(PACKAGE_STATUS, "development") == 0) {
            g_string_append(version_str, "dev");
        }
        xmpp_stanza_set_text(version_txt, version_str->str);
        xmpp_stanza_add_child(version, version_txt);

        xmpp_stanza_add_child(query, name);
        xmpp_stanza_add_child(query, version);
        xmpp_stanza_add_child(response, query);

        xmpp_send(conn, response);

        xmpp_stanza_release(response);
    }

    return 1;
}

static int
_iq_handle_discoinfo_get(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza,
    void * const userdata)
{
    xmpp_ctx_t *ctx = (xmpp_ctx_t *)userdata;
    const char *from = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_FROM);

    xmpp_stanza_t *incoming_query = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_QUERY);
    const char *node_str = xmpp_stanza_get_attribute(incoming_query, STANZA_ATTR_NODE);

    if (from != NULL && node_str != NULL) {
        xmpp_stanza_t *response = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(response, STANZA_NAME_IQ);
        xmpp_stanza_set_id(response, xmpp_stanza_get_id(stanza));
        xmpp_stanza_set_attribute(response, STANZA_ATTR_TO, from);
        xmpp_stanza_set_type(response, STANZA_TYPE_RESULT);
        xmpp_stanza_t *query = caps_create_query_response_stanza(ctx);
        xmpp_stanza_set_attribute(query, STANZA_ATTR_NODE, node_str);
        xmpp_stanza_add_child(response, query);
        xmpp_send(conn, response);

        xmpp_stanza_release(response);
    }

    return 1;
}

static int
_iq_handle_discoinfo_result(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza,
    void * const userdata)
{
    const char *id = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_ID);

    if ((id != NULL) && (g_str_has_prefix(id, "disco"))) {
        xmpp_stanza_t *query = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_QUERY);
        char *node = xmpp_stanza_get_attribute(query, STANZA_ATTR_NODE);
        if (node == NULL) {
            return 1;
        }

        char *caps_key = NULL;

        // xep-0115
        if (g_strcmp0(id, "disco") == 0) {
            caps_key = strdup(node);

            // validate sha1
            gchar **split = g_strsplit(node, "#", -1);
            char *given_sha1 = split[1];
            char *generated_sha1 = caps_create_sha1_str(query);

            if (g_strcmp0(given_sha1, generated_sha1) != 0) {
                log_info("Invalid SHA1 recieved for caps.");
                FREE_SET_NULL(generated_sha1);
                g_strfreev(split);

                return 1;
            }
            FREE_SET_NULL(generated_sha1);
            g_strfreev(split);

        // non supported hash, or legacy caps
        } else {
            caps_key = strdup(id + 6);
        }

        // already cached
        if (caps_contains(caps_key)) {
            log_info("Client info already cached.");
            return 1;
        }

        xmpp_stanza_t *identity = xmpp_stanza_get_child_by_name(query, "identity");

        if (identity == NULL) {
            return 1;
        }

        const char *category = xmpp_stanza_get_attribute(identity, "category");
        if (category == NULL) {
            return 1;
        }

        if (strcmp(category, "client") != 0) {
            return 1;
        }

        const char *name = xmpp_stanza_get_attribute(identity, "name");
        if (name == 0) {
            return 1;
        }

        caps_add(caps_key, name);

        free(caps_key);

        return 1;
    } else {
        return 1;
    }
}
