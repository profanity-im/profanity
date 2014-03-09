/*
 * presence.c
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

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <glib/gprintf.h>

#include "common.h"
#include "config/preferences.h"
#include "log.h"
#include "muc.h"
#include "profanity.h"
#include "server_events.h"
#include "xmpp/capabilities.h"
#include "xmpp/connection.h"
#include "xmpp/stanza.h"
#include "xmpp/xmpp.h"

static Autocomplete sub_requests_ac;

#define HANDLE(ns, type, func) xmpp_handler_add(conn, func, ns, \
                                                STANZA_NAME_PRESENCE, type, ctx)

static int _unavailable_handler(xmpp_conn_t * const conn,
    xmpp_stanza_t * const stanza, void * const userdata);
static int _subscribe_handler(xmpp_conn_t * const conn,
    xmpp_stanza_t * const stanza, void * const userdata);
static int _subscribed_handler(xmpp_conn_t * const conn,
    xmpp_stanza_t * const stanza, void * const userdata);
static int _unsubscribed_handler(xmpp_conn_t * const conn,
    xmpp_stanza_t * const stanza, void * const userdata);
static int _available_handler(xmpp_conn_t * const conn,
    xmpp_stanza_t * const stanza, void * const userdata);
static int _muc_user_handler(xmpp_conn_t * const conn,
    xmpp_stanza_t * const stanza, void * const userdata);
static int _presence_error_handler(xmpp_conn_t * const conn,
    xmpp_stanza_t * const stanza, void * const userdata);

static char* _get_caps_key(xmpp_stanza_t * const stanza);
static void _send_room_presence(xmpp_conn_t *conn, xmpp_stanza_t *presence);
void _send_caps_request(char *node, char *caps_key, char *id, char *from);

void
presence_sub_requests_init(void)
{
    sub_requests_ac = autocomplete_new();
}

void
presence_add_handlers(void)
{
    xmpp_conn_t * const conn = connection_get_conn();
    xmpp_ctx_t * const ctx = connection_get_ctx();

    HANDLE(NULL,               STANZA_TYPE_ERROR,        _presence_error_handler);
    HANDLE(STANZA_NS_MUC_USER, NULL,                     _muc_user_handler);
    HANDLE(NULL,               STANZA_TYPE_UNAVAILABLE,  _unavailable_handler);
    HANDLE(NULL,               STANZA_TYPE_SUBSCRIBE,    _subscribe_handler);
    HANDLE(NULL,               STANZA_TYPE_SUBSCRIBED,   _subscribed_handler);
    HANDLE(NULL,               STANZA_TYPE_UNSUBSCRIBED, _unsubscribed_handler);
    HANDLE(NULL,               NULL,                     _available_handler);
}

static void
_presence_subscription(const char * const jid, const jabber_subscr_t action)
{
    assert(jid != NULL);

    xmpp_ctx_t * const ctx = connection_get_ctx();
    xmpp_conn_t * const conn = connection_get_conn();
    const char *type = NULL;

    Jid *jidp = jid_create(jid);

    autocomplete_remove(sub_requests_ac, jidp->barejid);

    switch (action)
    {
        case PRESENCE_SUBSCRIBE:
            log_debug("Sending presence subscribe: %s", jid);
            type = STANZA_TYPE_SUBSCRIBE;
            break;
        case PRESENCE_SUBSCRIBED:
            log_debug("Sending presence subscribed: %s", jid);
            type = STANZA_TYPE_SUBSCRIBED;
            break;
        case PRESENCE_UNSUBSCRIBED:
            log_debug("Sending presence usubscribed: %s", jid);
            type = STANZA_TYPE_UNSUBSCRIBED;
            break;
        default:
            log_warning("Attempt to send unknown subscription action: %s", jid);
            break;
    }

    xmpp_stanza_t *presence = xmpp_stanza_new(ctx);
    char *id = generate_unique_id("sub");
    xmpp_stanza_set_id(presence, id);
    xmpp_stanza_set_name(presence, STANZA_NAME_PRESENCE);
    xmpp_stanza_set_type(presence, type);
    xmpp_stanza_set_attribute(presence, STANZA_ATTR_TO, jidp->barejid);
    xmpp_send(conn, presence);
    xmpp_stanza_release(presence);

    jid_destroy(jidp);
}

static GSList *
_presence_get_subscription_requests(void)
{
    return autocomplete_get_list(sub_requests_ac);
}

static gint
_presence_sub_request_count(void)
{
    return autocomplete_length(sub_requests_ac);
}

void
presence_free_sub_requests(void)
{
    autocomplete_free(sub_requests_ac);
}

void
presence_clear_sub_requests(void)
{
    autocomplete_clear(sub_requests_ac);
}

static char *
_presence_sub_request_find(char * search_str)
{
    return autocomplete_complete(sub_requests_ac, search_str);
}

static gboolean
_presence_sub_request_exists(const char * const bare_jid)
{
    gboolean result = FALSE;
    GSList *requests_p = autocomplete_get_list(sub_requests_ac);
    GSList *requests = requests_p;

    while (requests != NULL) {
        if (strcmp(requests->data, bare_jid) == 0) {
            result = TRUE;
            break;
        }
        requests = g_slist_next(requests);
    }

    if (requests_p != NULL) {
        g_slist_free_full(requests_p, free);
    }

    return result;
}

static void
_presence_reset_sub_request_search(void)
{
    autocomplete_reset(sub_requests_ac);
}

static void
_presence_update(const resource_presence_t presence_type, const char * const msg,
    const int idle)
{
    if (jabber_get_connection_status() != JABBER_CONNECTED) {
        log_warning("Error setting presence, not connected.");
        return;
    }

    if (msg != NULL) {
        log_debug("Updating presence: %s, \"%s\"",
            string_from_resource_presence(presence_type), msg);
    } else {
        log_debug("Updating presence: %s",
            string_from_resource_presence(presence_type));
    }

    xmpp_ctx_t * const ctx = connection_get_ctx();
    xmpp_conn_t * const conn = connection_get_conn();
    const int pri =
        accounts_get_priority_for_presence_type(jabber_get_account_name(),
                                                presence_type);
    const char *show = stanza_get_presence_string_from_type(presence_type);

    connection_set_presence_message(msg);
    connection_set_priority(pri);

    xmpp_stanza_t *presence = stanza_create_presence(ctx);
    char *id = generate_unique_id("presence");
    xmpp_stanza_set_id(presence, id);
    stanza_attach_show(ctx, presence, show);
    stanza_attach_status(ctx, presence, msg);
    stanza_attach_priority(ctx, presence, pri);
    stanza_attach_last_activity(ctx, presence, idle);
    stanza_attach_caps(ctx, presence);
    xmpp_send(conn, presence);
    _send_room_presence(conn, presence);
    xmpp_stanza_release(presence);

    // set last presence for account
    const char *last = show;
    if (last == NULL) {
        last = STANZA_TEXT_ONLINE;
    }
    accounts_set_last_presence(jabber_get_account_name(), last);
}

static void
_send_room_presence(xmpp_conn_t *conn, xmpp_stanza_t *presence)
{
    GList *rooms_p = muc_get_active_room_list();
    GList *rooms = rooms_p;

    while (rooms != NULL) {
        const char *room = rooms->data;
        const char *nick = muc_get_room_nick(room);

        if (nick != NULL) {
            char *full_room_jid = create_fulljid(room, nick);

            xmpp_stanza_set_attribute(presence, STANZA_ATTR_TO, full_room_jid);
            log_debug("Sending presence to room: %s", full_room_jid);
            xmpp_send(conn, presence);
            free(full_room_jid);
        }

        rooms = g_list_next(rooms);
    }

    if (rooms_p != NULL) {
        g_list_free(rooms_p);
    }
}

static void
_presence_join_room(char *room, char *nick, char * passwd)
{
    Jid *jid = jid_create_from_bare_and_resource(room, nick);

    log_debug("Sending room join presence to: %s", jid->fulljid);
    xmpp_ctx_t *ctx = connection_get_ctx();
    xmpp_conn_t *conn = connection_get_conn();
    resource_presence_t presence_type =
        accounts_get_last_presence(jabber_get_account_name());
    const char *show = stanza_get_presence_string_from_type(presence_type);
    char *status = jabber_get_presence_message();
    int pri = accounts_get_priority_for_presence_type(jabber_get_account_name(),
        presence_type);

    xmpp_stanza_t *presence = stanza_create_room_join_presence(ctx, jid->fulljid, passwd);
    stanza_attach_show(ctx, presence, show);
    stanza_attach_status(ctx, presence, status);
    stanza_attach_priority(ctx, presence, pri);
    stanza_attach_caps(ctx, presence);

    xmpp_send(conn, presence);
    xmpp_stanza_release(presence);

    muc_join_room(jid->barejid, jid->resourcepart);
    jid_destroy(jid);
}

static void
_presence_change_room_nick(const char * const room, const char * const nick)
{
    assert(room != NULL);
    assert(nick != NULL);

    log_debug("Sending room nickname change to: %s, nick: %s", room, nick);
    xmpp_ctx_t *ctx = connection_get_ctx();
    xmpp_conn_t *conn = connection_get_conn();
    resource_presence_t presence_type =
        accounts_get_last_presence(jabber_get_account_name());
    const char *show = stanza_get_presence_string_from_type(presence_type);
    char *status = jabber_get_presence_message();
    int pri = accounts_get_priority_for_presence_type(jabber_get_account_name(),
        presence_type);

    char *full_room_jid = create_fulljid(room, nick);
    xmpp_stanza_t *presence =
        stanza_create_room_newnick_presence(ctx, full_room_jid);
    stanza_attach_show(ctx, presence, show);
    stanza_attach_status(ctx, presence, status);
    stanza_attach_priority(ctx, presence, pri);
    stanza_attach_caps(ctx, presence);

    xmpp_send(conn, presence);
    xmpp_stanza_release(presence);

    free(full_room_jid);
}

static void
_presence_leave_chat_room(const char * const room_jid)
{
    assert(room_jid != NULL);

    log_debug("Sending room leave presence to: %s", room_jid);
    xmpp_ctx_t *ctx = connection_get_ctx();
    xmpp_conn_t *conn = connection_get_conn();
    char *nick = muc_get_room_nick(room_jid);

    if (nick != NULL) {
        xmpp_stanza_t *presence = stanza_create_room_leave_presence(ctx, room_jid,
            nick);
        xmpp_send(conn, presence);
        xmpp_stanza_release(presence);
    }
}

static int
_presence_error_handler(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza,
    void * const userdata)
{
    char *id = xmpp_stanza_get_id(stanza);
    char *from = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_FROM);
    xmpp_stanza_t *error_stanza = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_ERROR);
    char *type = NULL;
    if (error_stanza != NULL) {
        type = xmpp_stanza_get_attribute(error_stanza, STANZA_ATTR_TYPE);
    }

    // stanza_get_error never returns NULL
    char *err_msg = stanza_get_error_message(stanza);

    GString *log_msg = g_string_new("presence stanza error received");
    if (id != NULL) {
        g_string_append(log_msg, " id=");
        g_string_append(log_msg, id);
    }
    if (from != NULL) {
        g_string_append(log_msg, " from=");
        g_string_append(log_msg, from);
    }
    if (type != NULL) {
        g_string_append(log_msg, " type=");
        g_string_append(log_msg, type);
    }
    g_string_append(log_msg, " error=");
    g_string_append(log_msg, err_msg);

    log_info(log_msg->str);

    g_string_free(log_msg, TRUE);

    handle_presence_error(from, type, err_msg);

    free(err_msg);

    return 1;
}


static int
_unsubscribed_handler(xmpp_conn_t * const conn,
    xmpp_stanza_t * const stanza, void * const userdata)
{
    char *from = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_FROM);
    Jid *from_jid = jid_create(from);
    log_debug("Unsubscribed presence handler fired for %s", from);

    handle_subscription(from_jid->barejid, PRESENCE_UNSUBSCRIBED);
    autocomplete_remove(sub_requests_ac, from_jid->barejid);

    jid_destroy(from_jid);

    return 1;
}

static int
_subscribed_handler(xmpp_conn_t * const conn,
    xmpp_stanza_t * const stanza, void * const userdata)
{
    char *from = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_FROM);
    Jid *from_jid = jid_create(from);
    log_debug("Subscribed presence handler fired for %s", from);

    handle_subscription(from_jid->barejid, PRESENCE_SUBSCRIBED);
    autocomplete_remove(sub_requests_ac, from_jid->barejid);

    jid_destroy(from_jid);

    return 1;
}

static int
_subscribe_handler(xmpp_conn_t * const conn,
    xmpp_stanza_t * const stanza, void * const userdata)
{
    char *from = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_FROM);
    log_debug("Subscribe presence handler fired for %s", from);

    Jid *from_jid = jid_create(from);
    if (from_jid == NULL) {
        return 1;
    }

    handle_subscription(from_jid->barejid, PRESENCE_SUBSCRIBE);
    autocomplete_add(sub_requests_ac, from_jid->barejid);

    jid_destroy(from_jid);

    return 1;
}

static int
_unavailable_handler(xmpp_conn_t * const conn,
    xmpp_stanza_t * const stanza, void * const userdata)
{
    const char *jid = xmpp_conn_get_jid(conn);
    char *from = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_FROM);
    log_debug("Unavailable presence handler fired for %s", from);

    Jid *my_jid = jid_create(jid);
    Jid *from_jid = jid_create(from);
    if (my_jid == NULL || from_jid == NULL) {
        jid_destroy(my_jid);
        jid_destroy(from_jid);
        return 1;
    }

    char *status_str = stanza_get_status(stanza, NULL);

    if (strcmp(my_jid->barejid, from_jid->barejid) !=0) {
        if (from_jid->resourcepart != NULL) {
            handle_contact_offline(from_jid->barejid, from_jid->resourcepart, status_str);

        // hack for servers that do not send full jid with unavailable presence
        } else {
            handle_contact_offline(from_jid->barejid, "__prof_default", status_str);
        }
    } else {
        if (from_jid->resourcepart != NULL) {
            connection_remove_available_resource(from_jid->resourcepart);
        }
    }

    free(status_str);
    jid_destroy(my_jid);
    jid_destroy(from_jid);

    return 1;
}

static int
_available_handler(xmpp_conn_t * const conn,
    xmpp_stanza_t * const stanza, void * const userdata)
{
    // handler still fires if error
    if (g_strcmp0(xmpp_stanza_get_type(stanza), STANZA_TYPE_ERROR) == 0) {
        return 1;
    }

    // handler still fires if other types
    if ((g_strcmp0(xmpp_stanza_get_type(stanza), STANZA_TYPE_UNAVAILABLE) == 0) ||
            (g_strcmp0(xmpp_stanza_get_type(stanza), STANZA_TYPE_SUBSCRIBE) == 0) ||
            (g_strcmp0(xmpp_stanza_get_type(stanza), STANZA_TYPE_SUBSCRIBED) == 0) ||
            (g_strcmp0(xmpp_stanza_get_type(stanza), STANZA_TYPE_UNSUBSCRIBED) == 0)) {
        return 1;
    }

    // handler still fires for muc presence
    if (stanza_is_muc_presence(stanza)) {
        return 1;
    }

    const char *jid = xmpp_conn_get_jid(conn);
    char *from = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_FROM);
    log_debug("Available presence handler fired for %s", from);

    Jid *my_jid = jid_create(jid);
    Jid *from_jid = jid_create(from);
    if (my_jid == NULL || from_jid == NULL) {
        jid_destroy(my_jid);
        jid_destroy(from_jid);
        return 1;
    }

    char *show_str = stanza_get_show(stanza, "online");
    char *status_str = stanza_get_status(stanza, NULL);
    int idle_seconds = stanza_get_idle_time(stanza);
    GDateTime *last_activity = NULL;

    char *caps_key = NULL;
    if (stanza_contains_caps(stanza)) {
        caps_key = _get_caps_key(stanza);
    }

    if (idle_seconds > 0) {
        GDateTime *now = g_date_time_new_now_local();
        last_activity = g_date_time_add_seconds(now, 0 - idle_seconds);
        g_date_time_unref(now);
    }

    // get priority
    int priority = 0;
    xmpp_stanza_t *priority_stanza =
        xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_PRIORITY);

    if (priority_stanza != NULL) {
        char *priority_str = xmpp_stanza_get_text(priority_stanza);
        if (priority_str != NULL) {
            priority = atoi(priority_str);
        }
    }

    resource_presence_t presence = resource_presence_from_string(show_str);
    Resource *resource = NULL;

    // hack for servers that do not send fulljid with initial presence
    if (from_jid->resourcepart == NULL) {
        resource = resource_new("__prof_default", presence,
            status_str, priority, caps_key);
    } else {
        resource = resource_new(from_jid->resourcepart, presence,
            status_str, priority, caps_key);
    }

    // self presence
    if (strcmp(my_jid->barejid, from_jid->barejid) == 0) {
        connection_add_available_resource(resource);

    // contact presence
    } else {
        handle_contact_online(from_jid->barejid, resource,
            last_activity);
    }

    free(caps_key);
    free(status_str);
    free(show_str);
    jid_destroy(my_jid);
    jid_destroy(from_jid);

    if (last_activity != NULL) {
        g_date_time_unref(last_activity);
    }

    return 1;
}

void
_send_caps_request(char *node, char *caps_key, char *id, char *from)
{
    xmpp_ctx_t *ctx = connection_get_ctx();
    xmpp_conn_t *conn = connection_get_conn();

    if (node != NULL) {
        log_debug("Node string: %s.", node);
        if (!caps_contains(caps_key)) {
            log_debug("Capabilities not cached for '%s', sending discovery IQ.", from);
            xmpp_stanza_t *iq = stanza_create_disco_info_iq(ctx, id, from, node);
            xmpp_send(conn, iq);
            xmpp_stanza_release(iq);
        } else {
            log_debug("Capabilities already cached, for %s", caps_key);
        }
    } else {
        log_debug("No node string, not sending discovery IQ.");
    }
}

static char *
_get_caps_key(xmpp_stanza_t * const stanza)
{
    char *from = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_FROM);
    char *hash_type = stanza_caps_get_hash(stanza);
    char *node = stanza_get_caps_str(stanza);
    char *caps_key = NULL;
    char *id = NULL;

    log_debug("Presence contains capabilities.");

    if (node == NULL) {
        return NULL;
    }

    // xep-0115
    if ((hash_type != NULL) && (strcmp(hash_type, "sha-1") == 0)) {
        log_debug("Hash type %s supported.", hash_type);
        caps_key = strdup(node);
        char *id = generate_unique_id("caps");

        _send_caps_request(node, caps_key, id, from);

    // unsupported hash or legacy capabilities
    } else {
        if (hash_type != NULL) {
            log_debug("Hash type %s unsupported.", hash_type);
        } else {
            log_debug("No hash type, using legacy capabilities.");
        }

        guint from_hash = g_str_hash(from);
        char from_hash_str[9];
        g_snprintf(from_hash_str, sizeof(from_hash_str), "%08x", from_hash);
        caps_key = strdup(from_hash_str);
        GString *id_str = g_string_new("capsreq_");
        g_string_append(id_str, from_hash_str);
        id = id_str->str;

        _send_caps_request(node, caps_key, id, from);

        g_string_free(id_str, TRUE);
    }

    g_free(node);

    return caps_key;
}

static int
_muc_user_handler(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza,
    void * const userdata)
{
    // handler still fires if error
    if (g_strcmp0(xmpp_stanza_get_type(stanza), STANZA_TYPE_ERROR) == 0) {
        return 1;
    }

    char *from = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_FROM);
    Jid *from_jid = jid_create(from);
    if (from_jid == NULL || from_jid->resourcepart == NULL) {
        return 1;
    }

    char *room = from_jid->barejid;
    char *nick = from_jid->resourcepart;

    // handle self presence
    if (stanza_is_muc_self_presence(stanza, jabber_get_fulljid())) {
        char *type = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_TYPE);
        char *new_nick = stanza_get_new_nick(stanza);

        if ((type != NULL) && (strcmp(type, STANZA_TYPE_UNAVAILABLE) == 0)) {

            // leave room if not self nick change
            if (new_nick != NULL) {
                muc_set_room_pending_nick_change(room, new_nick);
            } else {
                handle_leave_room(room);
            }

        // handle self nick change
        } else if (muc_is_room_pending_nick_change(room)) {
            muc_complete_room_nick_change(room, nick);
            handle_room_nick_change(room, nick);

        // handle roster complete
        } else if (!muc_get_roster_received(room)) {
            handle_room_roster_complete(room);

        }

    // handle presence from room members
    } else {
        char *type = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_TYPE);
        char *status_str;

        char *caps_key = NULL;
        if (stanza_contains_caps(stanza)) {
            caps_key = _get_caps_key(stanza);
        }

        log_debug("Room presence received from %s", from_jid->fulljid);

        status_str = stanza_get_status(stanza, NULL);

        if ((type != NULL) && (strcmp(type, STANZA_TYPE_UNAVAILABLE) == 0)) {

            // handle nickname change
            if (stanza_is_room_nick_change(stanza)) {
                char *new_nick = stanza_get_new_nick(stanza);
                if (new_nick != NULL) {
                    muc_set_roster_pending_nick_change(room, new_nick, nick);
                    free(new_nick);
                }
            } else {
                handle_room_member_offline(room, nick, "offline", status_str);
            }
        } else {
            char *show_str = stanza_get_show(stanza, "online");
            if (!muc_get_roster_received(room)) {
                muc_add_to_roster(room, nick, show_str, status_str, caps_key);
            } else {
                char *old_nick = muc_complete_roster_nick_change(room, nick);

                if (old_nick != NULL) {
                    muc_add_to_roster(room, nick, show_str, status_str, caps_key);
                    handle_room_member_nick_change(room, old_nick, nick);
                    free(old_nick);
                } else {
                    if (!muc_nick_in_roster(room, nick)) {
                        handle_room_member_online(room, nick, show_str, status_str, caps_key);
                    } else {
                        handle_room_member_presence(room, nick, show_str, status_str, caps_key);
                    }
                }
            }

           free(show_str);
        }

        free(status_str);
        free(caps_key);
    }

    jid_destroy(from_jid);

    return 1;
}

void
presence_init_module(void)
{
    presence_subscription = _presence_subscription;
    presence_get_subscription_requests = _presence_get_subscription_requests;
    presence_sub_request_count = _presence_sub_request_count;
    presence_sub_request_find = _presence_sub_request_find;
    presence_sub_request_exists = _presence_sub_request_exists;
    presence_reset_sub_request_search = _presence_reset_sub_request_search;
    presence_update = _presence_update;
    presence_join_room = _presence_join_room;
    presence_change_room_nick = _presence_change_room_nick;
    presence_leave_chat_room = _presence_leave_chat_room;
}
