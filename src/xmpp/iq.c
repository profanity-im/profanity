/*
 * iq.c
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

#include "config.h"

#ifdef HAVE_GIT_VERSION
#include "gitversion.h"
#endif

#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <strophe.h>

#include "log.h"
#include "muc.h"
#include "profanity.h"
#include "config/preferences.h"
#include "server_events.h"
#include "xmpp/capabilities.h"
#include "xmpp/connection.h"
#include "xmpp/stanza.h"
#include "roster_list.h"
#include "xmpp/xmpp.h"

#define HANDLE(ns, type, func) xmpp_handler_add(conn, func, ns, STANZA_NAME_IQ, type, ctx)

static int _error_handler(xmpp_conn_t * const conn,
    xmpp_stanza_t * const stanza, void * const userdata);
static int _ping_get_handler(xmpp_conn_t * const conn,
    xmpp_stanza_t * const stanza, void * const userdata);
static int _version_get_handler(xmpp_conn_t * const conn,
    xmpp_stanza_t * const stanza, void * const userdata);
static int _disco_info_get_handler(xmpp_conn_t * const conn,
    xmpp_stanza_t * const stanza, void * const userdata);
static int _disco_info_result_handler(xmpp_conn_t * const conn,
    xmpp_stanza_t * const stanza, void * const userdata);
static int _version_result_handler(xmpp_conn_t * const conn,
    xmpp_stanza_t * const stanza, void * const userdata);
static int _disco_items_result_handler(xmpp_conn_t * const conn,
    xmpp_stanza_t * const stanza, void * const userdata);
static int _disco_items_get_handler(xmpp_conn_t * const conn,
    xmpp_stanza_t * const stanza, void * const userdata);
static int _ping_timed_handler(xmpp_conn_t * const conn,
    void * const userdata);

void
iq_add_handlers(void)
{
    xmpp_conn_t * const conn = connection_get_conn();
    xmpp_ctx_t * const ctx = connection_get_ctx();

    HANDLE(NULL,                STANZA_TYPE_ERROR,  _error_handler);

    HANDLE(XMPP_NS_DISCO_INFO,  STANZA_TYPE_GET,    _disco_info_get_handler);
    HANDLE(XMPP_NS_DISCO_INFO,  STANZA_TYPE_RESULT, _disco_info_result_handler);

    HANDLE(XMPP_NS_DISCO_ITEMS, STANZA_TYPE_GET,    _disco_items_get_handler);
    HANDLE(XMPP_NS_DISCO_ITEMS, STANZA_TYPE_RESULT, _disco_items_result_handler);

    HANDLE(STANZA_NS_VERSION,   STANZA_TYPE_GET,    _version_get_handler);
    HANDLE(STANZA_NS_VERSION,   STANZA_TYPE_RESULT, _version_result_handler);

    HANDLE(STANZA_NS_PING,      STANZA_TYPE_GET,    _ping_get_handler);

    if (prefs_get_autoping() != 0) {
        int millis = prefs_get_autoping() * 1000;
        xmpp_timed_handler_add(conn, _ping_timed_handler, millis, ctx);
    }
}

static void
_iq_set_autoping(const int seconds)
{
    xmpp_conn_t * const conn = connection_get_conn();
    xmpp_ctx_t * const ctx = connection_get_ctx();

    if (jabber_get_connection_status() == JABBER_CONNECTED) {
        xmpp_timed_handler_delete(conn, _ping_timed_handler);

        if (seconds != 0) {
            int millis = seconds * 1000;
            xmpp_timed_handler_add(conn, _ping_timed_handler, millis,
                ctx);
        }
    }
}

static void
_iq_room_list_request(gchar *conferencejid)
{
    xmpp_conn_t * const conn = connection_get_conn();
    xmpp_ctx_t * const ctx = connection_get_ctx();
    xmpp_stanza_t *iq = stanza_create_disco_items_iq(ctx, "confreq", conferencejid);
    xmpp_send(conn, iq);
    xmpp_stanza_release(iq);
}

static void
_iq_disco_info_request(gchar *jid)
{
    xmpp_conn_t * const conn = connection_get_conn();
    xmpp_ctx_t * const ctx = connection_get_ctx();
    xmpp_stanza_t *iq = stanza_create_disco_info_iq(ctx, "discoinforeq", jid, NULL);
    xmpp_send(conn, iq);
    xmpp_stanza_release(iq);
}

static void
_iq_disco_items_request(gchar *jid)
{
    xmpp_conn_t * const conn = connection_get_conn();
    xmpp_ctx_t * const ctx = connection_get_ctx();
    xmpp_stanza_t *iq = stanza_create_disco_items_iq(ctx, "discoitemsreq", jid);
    xmpp_send(conn, iq);
    xmpp_stanza_release(iq);
}

static void
_iq_send_software_version(const char * const fulljid)
{
    xmpp_conn_t * const conn = connection_get_conn();
    xmpp_ctx_t * const ctx = connection_get_ctx();
    xmpp_stanza_t *iq = stanza_create_software_version_iq(ctx, fulljid);
    xmpp_send(conn, iq);
    xmpp_stanza_release(iq);
}

static int
_error_handler(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza,
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
_pong_handler(xmpp_conn_t *const conn, xmpp_stanza_t * const stanza,
    void * const userdata)
{
    char *id = xmpp_stanza_get_id(stanza);
    char *type = xmpp_stanza_get_type(stanza);

    if (id != NULL && type != NULL) {
        // show warning if error
        if (strcmp(type, STANZA_TYPE_ERROR) == 0) {
            log_warning("Server ping (id=%s) responded with error", id);

            // turn off autoping if error type is 'cancel'
            xmpp_stanza_t *error = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_ERROR);
            if (error != NULL) {
                char *errtype = xmpp_stanza_get_type(error);
                if (errtype != NULL) {
                    if (strcmp(errtype, "cancel") == 0) {
                        log_warning("Server ping (id=%s) error type 'cancel', disabling autoping.", id);
                        handle_autoping_cancel();
                        xmpp_timed_handler_delete(conn, _ping_timed_handler);
                    }
                }
            }
        }
    }

    // remove this handler
    return 0;
}

static int
_ping_timed_handler(xmpp_conn_t * const conn, void * const userdata)
{
    xmpp_ctx_t *ctx = (xmpp_ctx_t *)userdata;

    if (jabber_get_connection_status() == JABBER_CONNECTED) {

        xmpp_stanza_t *iq = stanza_create_ping_iq(ctx);
        char *id = xmpp_stanza_get_id(iq);

        // add pong handler
        xmpp_id_handler_add(conn, _pong_handler, id, ctx);

        xmpp_send(conn, iq);
        xmpp_stanza_release(iq);
    }

    return 1;
}

static int
_version_result_handler(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza,
    void * const userdata)
{
    const char *jid = xmpp_stanza_get_attribute(stanza, "from");

    xmpp_stanza_t *query = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_QUERY);
    if (query == NULL) {
        return 1;
    }

    char *ns = xmpp_stanza_get_ns(query);
    if (g_strcmp0(ns, STANZA_NS_VERSION) != 0) {
        return 1;
    }

    char *name_str = NULL;
    char *version_str = NULL;
    char *os_str = NULL;
    xmpp_stanza_t *name = xmpp_stanza_get_child_by_name(query, "name");
    xmpp_stanza_t *version = xmpp_stanza_get_child_by_name(query, "version");
    xmpp_stanza_t *os = xmpp_stanza_get_child_by_name(query, "os");

    if (name != NULL) {
        name_str = xmpp_stanza_get_text(name);
    }
    if (version != NULL) {
        version_str = xmpp_stanza_get_text(version);
    }
    if (os != NULL) {
        os_str = xmpp_stanza_get_text(os);
    }

    PContact contact;
    Jid *jidp = jid_create(jid);
    if (muc_room_is_active(jidp->barejid)) {
        contact = muc_get_participant(jidp->barejid, jidp->resourcepart);
    } else {
        contact = roster_get_contact(jidp->barejid);
    }

    Resource *resource = p_contact_get_resource(contact, jidp->resourcepart);
    const char *presence = string_from_resource_presence(resource->presence);
    handle_software_version_result(jid, presence, name_str, version_str, os_str);

    jid_destroy(jidp);

    return 1;
}

static int
_ping_get_handler(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza,
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
_version_get_handler(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza,
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
#ifdef HAVE_GIT_VERSION
            g_string_append(version_str, "dev.");
            g_string_append(version_str, PROF_GIT_BRANCH);
            g_string_append(version_str, ".");
            g_string_append(version_str, PROF_GIT_REVISION);
#else
            g_string_append(version_str, "dev");
#endif
        }
        xmpp_stanza_set_text(version_txt, version_str->str);
        xmpp_stanza_add_child(version, version_txt);

        xmpp_stanza_add_child(query, name);
        xmpp_stanza_add_child(query, version);
        xmpp_stanza_add_child(response, query);

        xmpp_send(conn, response);

        g_string_free(version_str, TRUE);
        xmpp_stanza_release(name_txt);
        xmpp_stanza_release(version_txt);
        xmpp_stanza_release(name);
        xmpp_stanza_release(version);
        xmpp_stanza_release(query);
        xmpp_stanza_release(response);
    }

    return 1;
}

static int
_disco_items_get_handler(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza,
    void * const userdata)
{
    xmpp_ctx_t *ctx = (xmpp_ctx_t *)userdata;
    const char *from = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_FROM);

    if (from != NULL) {
        xmpp_stanza_t *response = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(response, STANZA_NAME_IQ);
        xmpp_stanza_set_id(response, xmpp_stanza_get_id(stanza));
        xmpp_stanza_set_attribute(response, STANZA_ATTR_TO, from);
        xmpp_stanza_set_type(response, STANZA_TYPE_RESULT);
        xmpp_stanza_t *query = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(query, STANZA_NAME_QUERY);
        xmpp_stanza_set_ns(query, XMPP_NS_DISCO_ITEMS);
        xmpp_stanza_add_child(response, query);
        xmpp_send(conn, response);

        xmpp_stanza_release(response);
    }

    return 1;
}


static int
_disco_info_get_handler(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza,
    void * const userdata)
{
    xmpp_ctx_t *ctx = (xmpp_ctx_t *)userdata;
    const char *from = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_FROM);

    xmpp_stanza_t *incoming_query = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_QUERY);
    const char *node_str = xmpp_stanza_get_attribute(incoming_query, STANZA_ATTR_NODE);

    if (from != NULL) {
        xmpp_stanza_t *response = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(response, STANZA_NAME_IQ);
        xmpp_stanza_set_id(response, xmpp_stanza_get_id(stanza));
        xmpp_stanza_set_attribute(response, STANZA_ATTR_TO, from);
        xmpp_stanza_set_type(response, STANZA_TYPE_RESULT);
        xmpp_stanza_t *query = caps_create_query_response_stanza(ctx);
        if (node_str != NULL) {
            xmpp_stanza_set_attribute(query, STANZA_ATTR_NODE, node_str);
        }
        xmpp_stanza_add_child(response, query);
        xmpp_send(conn, response);

        xmpp_stanza_release(query);
        xmpp_stanza_release(response);
    }

    return 1;
}

static void
_identity_destroy(DiscoIdentity *identity)
{
    if (identity != NULL) {
        free(identity->name);
        free(identity->type);
        free(identity->category);
        free(identity);
    }
}

static void
_item_destroy(DiscoItem *item)
{
    if (item != NULL) {
        free(item->jid);
        free(item->name);
        free(item);
    }
}

static int
_disco_info_result_handler(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza,
    void * const userdata)
{
    log_debug("Recieved diso#info response");
    const char *id = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_ID);
    const char *from = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_FROM);

    if (g_strcmp0(id, "discoinforeq") == 0) {
        GSList *identities = NULL;
        GSList *features = NULL;

        xmpp_stanza_t *query = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_QUERY);

        if (query != NULL) {
            xmpp_stanza_t *child = xmpp_stanza_get_children(query);
            while (child != NULL) {
                const char *stanza_name = xmpp_stanza_get_name(child);
                if (g_strcmp0(stanza_name, STANZA_NAME_FEATURE) == 0) {
                    const char *var = xmpp_stanza_get_attribute(child, STANZA_ATTR_VAR);
                    if (var != NULL) {
                        features = g_slist_append(features, strdup(var));
                    }
                } else if (g_strcmp0(stanza_name, STANZA_NAME_IDENTITY) == 0) {
                    const char *name = xmpp_stanza_get_attribute(child, STANZA_ATTR_NAME);
                    const char *type = xmpp_stanza_get_attribute(child, STANZA_ATTR_TYPE);
                    const char *category = xmpp_stanza_get_attribute(child, STANZA_ATTR_CATEGORY);

                    if ((name != NULL) || (category != NULL) || (type != NULL)) {
                        DiscoIdentity *identity = malloc(sizeof(struct disco_identity_t));

                        if (name != NULL) {
                            identity->name = strdup(name);
                        } else {
                            identity->name = NULL;
                        }
                        if (category != NULL) {
                            identity->category = strdup(category);
                        } else {
                            identity->category = NULL;
                        }
                        if (type != NULL) {
                            identity->type = strdup(type);
                        } else {
                            identity->type = NULL;
                        }

                        identities = g_slist_append(identities, identity);
                    }
                }

                child = xmpp_stanza_get_next(child);
            }

            handle_disco_info(from, identities, features);
            g_slist_free_full(features, free);
            g_slist_free_full(identities, (GDestroyNotify)_identity_destroy);
        }
    } else if ((id != NULL) && (g_str_has_prefix(id, "capsreq"))) {
        log_debug("Response to query: %s", id);
        xmpp_stanza_t *query = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_QUERY);
        char *node = xmpp_stanza_get_attribute(query, STANZA_ATTR_NODE);
        if (node == NULL) {
            return 1;
        }

        char *caps_key = NULL;

        // xep-0115
        if (g_strcmp0(id, "capsreq") == 0) {
            log_debug("xep-0115 supported capabilities");
            caps_key = strdup(node);

            // validate sha1
            gchar **split = g_strsplit(node, "#", -1);
            char *given_sha1 = split[1];
            char *generated_sha1 = caps_create_sha1_str(query);

            if (g_strcmp0(given_sha1, generated_sha1) != 0) {
                log_info("Generated sha-1 does not match given:");
                log_info("Generated : %s", generated_sha1);
                log_info("Given     : %s", given_sha1);
                g_free(generated_sha1);
                g_strfreev(split);
                free(caps_key);

                return 1;
            }
            g_free(generated_sha1);
            g_strfreev(split);

        // non supported hash, or legacy caps
        } else {
            log_debug("Unsupported hash, or legacy capabilities");
            caps_key = strdup(id + 8);
            log_debug("Caps key: %s", caps_key);
        }

        // already cached
        if (caps_contains(caps_key)) {
            log_info("Client info already cached.");
            free(caps_key);
            return 1;
        }

        log_debug("Client info not cached");

        const char *category = NULL;
        const char *type = NULL;
        const char *name = NULL;
        const char *software = NULL;
        const char *software_version = NULL;
        const char *os = NULL;
        const char *os_version = NULL;
        GSList *features = NULL;

        xmpp_stanza_t *identity = xmpp_stanza_get_child_by_name(query, "identity");
        if (identity != NULL) {
            category = xmpp_stanza_get_attribute(identity, "category");
            type = xmpp_stanza_get_attribute(identity, "type");
            name = xmpp_stanza_get_attribute(identity, "name");
        }

        xmpp_stanza_t *softwareinfo = xmpp_stanza_get_child_by_ns(query, STANZA_NS_DATA);
        if (softwareinfo != NULL) {
            DataForm *form = stanza_create_form(softwareinfo);
            FormField *formField = NULL;

            if (g_strcmp0(form->form_type, STANZA_DATAFORM_SOFTWARE) == 0) {
                GSList *field = form->fields;
                while (field != NULL) {
                    formField = field->data;
                    if (formField->values != NULL) {
                        if (strcmp(formField->var, "software") == 0) {
                            software = formField->values->data;
                        } else if (strcmp(formField->var, "software_version") == 0) {
                            software_version = formField->values->data;
                        } else if (strcmp(formField->var, "os") == 0) {
                            os = formField->values->data;
                        } else if (strcmp(formField->var, "os_version") == 0) {
                            os_version = formField->values->data;
                        }
                    }
                    field = g_slist_next(field);
                }
            }

            stanza_destroy_form(form);
        }

        xmpp_stanza_t *child = xmpp_stanza_get_children(query);
        while (child != NULL) {
            if (g_strcmp0(xmpp_stanza_get_name(child), "feature") == 0) {
                features = g_slist_append(features, strdup(xmpp_stanza_get_attribute(child, "var")));
            }

            child = xmpp_stanza_get_next(child);
        }

        caps_add(caps_key, category, type, name, software, software_version,
            os, os_version, features);

        free(caps_key);
    }

    return 1;
}

static int
_disco_items_result_handler(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza,
    void * const userdata)
{

    log_debug("Recieved diso#items response");
    const char *id = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_ID);
    const char *from = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_FROM);
    const char *stanza_name = NULL;
    const char *item_jid = NULL;
    const char *item_name = NULL;
    GSList *items = NULL;

    if ((g_strcmp0(id, "confreq") == 0) || (g_strcmp0(id, "discoitemsreq") == 0)) {
        log_debug("Response to query: %s", id);
        xmpp_stanza_t *query = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_QUERY);

        if (query != NULL) {
            xmpp_stanza_t *child = xmpp_stanza_get_children(query);
            while (child != NULL) {
                stanza_name = xmpp_stanza_get_name(child);
                if ((stanza_name != NULL) && (g_strcmp0(stanza_name, STANZA_NAME_ITEM) == 0)) {
                    item_jid = xmpp_stanza_get_attribute(child, STANZA_ATTR_JID);
                    if (item_jid != NULL) {
                        DiscoItem *item = malloc(sizeof(struct disco_item_t));
                        item->jid = strdup(item_jid);
                        item_name = xmpp_stanza_get_attribute(child, STANZA_ATTR_NAME);
                        if (item_name != NULL) {
                            item->name = strdup(item_name);
                        } else {
                            item->name = NULL;
                        }
                        items = g_slist_append(items, item);
                    }
                }

                child = xmpp_stanza_get_next(child);
            }
        }
    }

    if (g_strcmp0(id, "confreq") == 0) {
        handle_room_list(items, from);
    } else if (g_strcmp0(id, "discoitemsreq") == 0) {
        handle_disco_items(items, from);
    }

    g_slist_free_full(items, (GDestroyNotify)_item_destroy);

    return 1;
}

void
iq_init_module(void)
{
    iq_room_list_request = _iq_room_list_request;
    iq_disco_info_request = _iq_disco_info_request;
    iq_disco_items_request = _iq_disco_items_request;
    iq_send_software_version = _iq_send_software_version;
    iq_set_autoping = _iq_set_autoping;
}
