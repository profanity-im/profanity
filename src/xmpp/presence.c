/*
 * presence.c
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

#include "config.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <glib/gprintf.h>

#ifdef HAVE_LIBMESODE
#include <mesode.h>
#endif

#ifdef HAVE_LIBSTROPHE
#include <strophe.h>
#endif

#include "profanity.h"
#include "log.h"
#include "common.h"
#include "config/preferences.h"
#include "event/server_events.h"
#include "plugins/plugins.h"
#include "ui/ui.h"
#include "xmpp/connection.h"
#include "xmpp/capabilities.h"
#include "xmpp/session.h"
#include "xmpp/stanza.h"
#include "xmpp/iq.h"
#include "xmpp/xmpp.h"
#include "xmpp/muc.h"

static Autocomplete sub_requests_ac;

static int _presence_handler(xmpp_conn_t *const conn, xmpp_stanza_t *const stanza, void *const userdata);

static void _presence_error_handler(xmpp_stanza_t *const stanza);
static void _unavailable_handler(xmpp_stanza_t *const stanza);
static void _subscribe_handler(xmpp_stanza_t *const stanza);
static void _subscribed_handler(xmpp_stanza_t *const stanza);
static void _unsubscribed_handler(xmpp_stanza_t *const stanza);
static void _muc_user_handler(xmpp_stanza_t *const stanza);
static void _available_handler(xmpp_stanza_t *const stanza);

void _send_caps_request(char *node, char *caps_key, char *id, char *from);
static void _send_room_presence(xmpp_stanza_t *presence);
static void _send_presence_stanza(xmpp_stanza_t *const stanza);

void
presence_sub_requests_init(void)
{
    sub_requests_ac = autocomplete_new();
}

void
presence_handlers_init(void)
{
    xmpp_conn_t * const conn = connection_get_conn();
    xmpp_ctx_t * const ctx = connection_get_ctx();
    xmpp_handler_add(conn, _presence_handler, NULL, STANZA_NAME_PRESENCE, NULL, ctx);
}

void
presence_subscription(const char *const jid, const jabber_subscr_t action)
{
    assert(jid != NULL);

    Jid *jidp = jid_create(jid);
    autocomplete_remove(sub_requests_ac, jidp->barejid);

    const char *type = NULL;
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
            break;
    }
    if (!type) {
        log_error("Attempt to send unknown subscription action: %s", jid);
        return;
    }

    xmpp_ctx_t * const ctx = connection_get_ctx();
    xmpp_stanza_t *presence = xmpp_presence_new(ctx);

    char *id = create_unique_id("sub");
    xmpp_stanza_set_id(presence, id);
    free(id);

    xmpp_stanza_set_type(presence, type);
    xmpp_stanza_set_to(presence, jidp->barejid);
    jid_destroy(jidp);

    _send_presence_stanza(presence);

    xmpp_stanza_release(presence);
}

GList*
presence_get_subscription_requests(void)
{
    return autocomplete_create_list(sub_requests_ac);
}

gint
presence_sub_request_count(void)
{
    return autocomplete_length(sub_requests_ac);
}

void
presence_clear_sub_requests(void)
{
    autocomplete_clear(sub_requests_ac);
}

char*
presence_sub_request_find(const char *const search_str, gboolean previous)
{
    return autocomplete_complete(sub_requests_ac, search_str, TRUE, previous);
}

gboolean
presence_sub_request_exists(const char *const bare_jid)
{
    gboolean result = FALSE;

    GList *requests = autocomplete_create_list(sub_requests_ac);
    GList *curr = requests;
    while (curr) {
        if (strcmp(curr->data, bare_jid) == 0) {
            result = TRUE;
            break;
        }
        curr = g_list_next(curr);
    }
    g_list_free_full(requests, free);

    return result;
}

void
presence_reset_sub_request_search(void)
{
    autocomplete_reset(sub_requests_ac);
}

void
presence_send(const resource_presence_t presence_type, const int idle, char *signed_status)
{
    if (connection_get_status() != JABBER_CONNECTED) {
        log_warning("Error setting presence, not connected.");
        return;
    }

    char *msg = connection_get_presence_msg();
    if (msg) {
        log_debug("Updating presence: %s, \"%s\"", string_from_resource_presence(presence_type), msg);
    } else {
        log_debug("Updating presence: %s", string_from_resource_presence(presence_type));
    }

    const int pri = accounts_get_priority_for_presence_type(session_get_account_name(), presence_type);
    connection_set_priority(pri);

    xmpp_ctx_t * const ctx = connection_get_ctx();
    xmpp_stanza_t *presence = xmpp_presence_new(ctx);

    char *id = create_unique_id("presence");
    xmpp_stanza_set_id(presence, id);
    free(id);

    const char *show = stanza_get_presence_string_from_type(presence_type);
    stanza_attach_show(ctx, presence, show);

    stanza_attach_status(ctx, presence, msg);

    if (signed_status) {
        xmpp_stanza_t *x = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(x, STANZA_NAME_X);
        xmpp_stanza_set_ns(x, STANZA_NS_SIGNED);

        xmpp_stanza_t *signed_text = xmpp_stanza_new(ctx);
        xmpp_stanza_set_text(signed_text, signed_status);

        xmpp_stanza_add_child(x, signed_text);
        xmpp_stanza_release(signed_text);

        xmpp_stanza_add_child(presence, x);
        xmpp_stanza_release(x);
    }

    stanza_attach_priority(ctx, presence, pri);

    if (idle > 0) {
        stanza_attach_last_activity(ctx, presence, idle);
    }

    stanza_attach_caps(ctx, presence);

    _send_presence_stanza(presence);
    _send_room_presence(presence);

    xmpp_stanza_release(presence);

    // set last presence for account
    const char *last = show;
    if (last == NULL) {
        last = STANZA_TEXT_ONLINE;
    }

    char *account = session_get_account_name();
    accounts_set_last_presence(account, last);
    accounts_set_last_status(account, msg);
}

static void
_send_room_presence(xmpp_stanza_t *presence)
{
    GList *rooms = muc_rooms();
    GList *curr = rooms;
    while (curr) {
        const char *room = curr->data;
        const char *nick = muc_nick(room);

        if (nick) {
            char *full_room_jid = create_fulljid(room, nick);
            xmpp_stanza_set_to(presence, full_room_jid);
            log_debug("Sending presence to room: %s", full_room_jid);
            free(full_room_jid);

            _send_presence_stanza(presence);
        }

        curr = g_list_next(curr);
    }
    g_list_free(rooms);
}

void
presence_join_room(const char *const room, const char *const nick, const char *const passwd)
{
    Jid *jid = jid_create_from_bare_and_resource(room, nick);
    log_debug("Sending room join presence to: %s", jid->fulljid);

    resource_presence_t presence_type = accounts_get_last_presence(session_get_account_name());
    const char *show = stanza_get_presence_string_from_type(presence_type);
    char *status = connection_get_presence_msg();
    int pri = accounts_get_priority_for_presence_type(session_get_account_name(), presence_type);

    xmpp_ctx_t *ctx = connection_get_ctx();
    xmpp_stanza_t *presence = stanza_create_room_join_presence(ctx, jid->fulljid, passwd);
    stanza_attach_show(ctx, presence, show);
    stanza_attach_status(ctx, presence, status);
    stanza_attach_priority(ctx, presence, pri);
    stanza_attach_caps(ctx, presence);

    _send_presence_stanza(presence);

    xmpp_stanza_release(presence);
    jid_destroy(jid);
}

void
presence_change_room_nick(const char *const room, const char *const nick)
{
    assert(room != NULL);
    assert(nick != NULL);

    log_debug("Sending room nickname change to: %s, nick: %s", room, nick);
    resource_presence_t presence_type = accounts_get_last_presence(session_get_account_name());
    const char *show = stanza_get_presence_string_from_type(presence_type);
    char *status = connection_get_presence_msg();
    int pri = accounts_get_priority_for_presence_type(session_get_account_name(), presence_type);
    char *full_room_jid = create_fulljid(room, nick);

    xmpp_ctx_t *ctx = connection_get_ctx();
    xmpp_stanza_t *presence = stanza_create_room_newnick_presence(ctx, full_room_jid);
    stanza_attach_show(ctx, presence, show);
    stanza_attach_status(ctx, presence, status);
    stanza_attach_priority(ctx, presence, pri);
    stanza_attach_caps(ctx, presence);

    _send_presence_stanza(presence);

    xmpp_stanza_release(presence);
    free(full_room_jid);
}

void
presence_leave_chat_room(const char *const room_jid)
{
    assert(room_jid != NULL);

    char *nick = muc_nick(room_jid);
    if (!nick) {
        log_error("Could not get nickname for room: %s", room_jid);
        return;
    }

    log_debug("Sending room leave presence to: %s", room_jid);

    xmpp_ctx_t *ctx = connection_get_ctx();
    xmpp_stanza_t *presence = stanza_create_room_leave_presence(ctx, room_jid, nick);

    _send_presence_stanza(presence);

    xmpp_stanza_release(presence);
}

static int
_presence_handler(xmpp_conn_t *const conn, xmpp_stanza_t *const stanza, void *const userdata)
{
    log_debug("Presence stanza handler fired");

    char *text = NULL;
    size_t text_size;
    xmpp_stanza_to_text(stanza, &text, &text_size);

    gboolean cont = plugins_on_presence_stanza_receive(text);
    xmpp_free(connection_get_ctx(), text);
    if (!cont) {
        return 1;
    }

    const char *type = xmpp_stanza_get_type(stanza);

    if (g_strcmp0(type, STANZA_TYPE_ERROR) == 0) {
        _presence_error_handler(stanza);
    }

    if (g_strcmp0(type, STANZA_TYPE_UNAVAILABLE) == 0) {
        _unavailable_handler(stanza);
    }

    if (g_strcmp0(type, STANZA_TYPE_SUBSCRIBE) == 0) {
        _subscribe_handler(stanza);
    }

    if (g_strcmp0(type, STANZA_TYPE_SUBSCRIBED) == 0) {
        _subscribed_handler(stanza);
    }

    if (g_strcmp0(type, STANZA_TYPE_UNSUBSCRIBED) == 0) {
        _unsubscribed_handler(stanza);
    }

    xmpp_stanza_t *mucuser = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_MUC_USER);
    if (mucuser) {
        _muc_user_handler(stanza);
    }

    _available_handler(stanza);

    return 1;
}

static void
_presence_error_handler(xmpp_stanza_t *const stanza)
{
    const char *xmlns = NULL;
    xmpp_stanza_t *x = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_X);
    if (x) {
        xmlns = xmpp_stanza_get_ns(x);
    }

    const char *from = xmpp_stanza_get_from(stanza);
    xmpp_stanza_t *error_stanza = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_ERROR);

    // handle MUC join errors
    if (g_strcmp0(xmlns, STANZA_NS_MUC) == 0) {
        const char *error_cond = NULL;
        xmpp_stanza_t *reason_st = xmpp_stanza_get_child_by_ns(error_stanza, STANZA_NS_STANZAS);
        if (reason_st) {
            error_cond = xmpp_stanza_get_name(reason_st);
        }
        if (error_cond == NULL) {
            error_cond = "unknown";
        }

        Jid *fulljid = jid_create(from);
        log_info("Error joining room: %s, reason: %s", fulljid->barejid, error_cond);
        if (muc_active(fulljid->barejid)) {
            muc_leave(fulljid->barejid);
        }
        cons_show_error("Error joining room %s, reason: %s", fulljid->barejid, error_cond);
        jid_destroy(fulljid);

        return;
    }

    GString *log_msg = g_string_new("presence stanza error received");
    const char *id = xmpp_stanza_get_id(stanza);
    if (id) {
        g_string_append(log_msg, " id=");
        g_string_append(log_msg, id);
    }
    if (from) {
        g_string_append(log_msg, " from=");
        g_string_append(log_msg, from);
    }

    const char *type = NULL;
    if (error_stanza) {
        type = xmpp_stanza_get_type(error_stanza);
    }
    if (type) {
        g_string_append(log_msg, " type=");
        g_string_append(log_msg, type);
    }

    // stanza_get_error never returns NULL
    char *err_msg = stanza_get_error_message(stanza);
    g_string_append(log_msg, " error=");
    g_string_append(log_msg, err_msg);

    log_info(log_msg->str);

    g_string_free(log_msg, TRUE);

    if (from) {
        ui_handle_recipient_error(from, err_msg);
    } else {
        ui_handle_error(err_msg);
    }

    free(err_msg);
}

static void
_unsubscribed_handler(xmpp_stanza_t *const stanza)
{
    const char *from = xmpp_stanza_get_from(stanza);
    if (!from) {
        log_warning("Unsubscribed presence handler received with no from attribute");
        return;
    }
    log_debug("Unsubscribed presence handler fired for %s", from);

    Jid *from_jid = jid_create(from);
    sv_ev_subscription(from_jid->barejid, PRESENCE_UNSUBSCRIBED);
    autocomplete_remove(sub_requests_ac, from_jid->barejid);

    jid_destroy(from_jid);
}

static void
_subscribed_handler(xmpp_stanza_t *const stanza)
{
    const char *from = xmpp_stanza_get_from(stanza);
    if (!from) {
        log_warning("Subscribed presence handler received with no from attribute");
        return;
    }
    log_debug("Subscribed presence handler fired for %s", from);

    Jid *from_jid = jid_create(from);
    sv_ev_subscription(from_jid->barejid, PRESENCE_SUBSCRIBED);
    autocomplete_remove(sub_requests_ac, from_jid->barejid);

    jid_destroy(from_jid);
}

static void
_subscribe_handler(xmpp_stanza_t *const stanza)
{
    const char *from = xmpp_stanza_get_from(stanza);
    if (!from) {
        log_warning("Subscribe presence handler received with no from attribute", from);
    }
    log_debug("Subscribe presence handler fired for %s", from);

    Jid *from_jid = jid_create(from);
    if (from_jid == NULL) {
        return;
    }

    sv_ev_subscription(from_jid->barejid, PRESENCE_SUBSCRIBE);
    autocomplete_add(sub_requests_ac, from_jid->barejid);

    jid_destroy(from_jid);
}

static void
_unavailable_handler(xmpp_stanza_t *const stanza)
{
    inp_nonblocking(TRUE);

    xmpp_conn_t *conn = connection_get_conn();
    const char *jid = xmpp_conn_get_jid(conn);
    const char *from = xmpp_stanza_get_from(stanza);
    if (!from) {
        log_warning("Unavailable presence received with no from attribute");
    }
    log_debug("Unavailable presence handler fired for %s", from);

    Jid *my_jid = jid_create(jid);
    Jid *from_jid = jid_create(from);
    if (my_jid == NULL || from_jid == NULL) {
        jid_destroy(my_jid);
        jid_destroy(from_jid);
        return;
    }

    if (strcmp(my_jid->barejid, from_jid->barejid) !=0) {
        char *status_str = stanza_get_status(stanza, NULL);
        if (from_jid->resourcepart) {
            sv_ev_contact_offline(from_jid->barejid, from_jid->resourcepart, status_str);

        // hack for servers that do not send full jid with unavailable presence
        } else {
            sv_ev_contact_offline(from_jid->barejid, "__prof_default", status_str);
        }
        free(status_str);
    } else {
        if (from_jid->resourcepart) {
            connection_remove_available_resource(from_jid->resourcepart);
        }
    }

    jid_destroy(my_jid);
    jid_destroy(from_jid);
}

static void
_handle_caps(const char *const jid, XMPPCaps *caps)
{
    // hash supported, xep-0115, cache against ver
    if (g_strcmp0(caps->hash, "sha-1") == 0) {
        log_info("Hash %s supported", caps->hash);
        if (caps->ver) {
            if (caps_cache_contains(caps->ver)) {
                log_info("Capabilities cache hit: %s, for %s.", caps->ver, jid);
                caps_map_jid_to_ver(jid, caps->ver);
            } else {
                log_info("Capabilities cache miss: %s, for %s, sending service discovery request", caps->ver, jid);
                char *id = create_unique_id("caps");
                iq_send_caps_request(jid, id, caps->node, caps->ver);
                free(id);
            }
        }

    // unsupported hash, xep-0115, associate with JID, no cache
    } else if (caps->hash) {
        log_info("Hash %s not supported: %s, sending service discovery request", caps->hash, jid);
        char *id = create_unique_id("caps");
        iq_send_caps_request_for_jid(jid, id, caps->node, caps->ver);
        free(id);

   // no hash, legacy caps, cache against node#ver
   } else if (caps->node && caps->ver) {
        log_info("No hash specified: %s, legacy request made for %s#%s", jid, caps->node, caps->ver);
        char *id = create_unique_id("caps");
        iq_send_caps_request_legacy(jid, id, caps->node, caps->ver);
        free(id);
    } else {
        log_info("No hash specified: %s, could not create ver string, not sending service discovery request.", jid);
    }
}

static void
_available_handler(xmpp_stanza_t *const stanza)
{
    inp_nonblocking(TRUE);

    // handler still fires if error
    if (g_strcmp0(xmpp_stanza_get_type(stanza), STANZA_TYPE_ERROR) == 0) {
        return;
    }

    // handler still fires if other types
    if ((g_strcmp0(xmpp_stanza_get_type(stanza), STANZA_TYPE_UNAVAILABLE) == 0) ||
            (g_strcmp0(xmpp_stanza_get_type(stanza), STANZA_TYPE_SUBSCRIBE) == 0) ||
            (g_strcmp0(xmpp_stanza_get_type(stanza), STANZA_TYPE_SUBSCRIBED) == 0) ||
            (g_strcmp0(xmpp_stanza_get_type(stanza), STANZA_TYPE_UNSUBSCRIBED) == 0)) {
        return;
    }

    // handler still fires for muc presence
    if (stanza_is_muc_presence(stanza)) {
        return;
    }

    int err = 0;
    XMPPPresence *xmpp_presence = stanza_parse_presence(stanza, &err);

    if (!xmpp_presence) {
        const char *from = NULL;
        switch(err) {
            case STANZA_PARSE_ERROR_NO_FROM:
                log_warning("Available presence handler fired with no from attribute.");
                break;
            case STANZA_PARSE_ERROR_INVALID_FROM:
                from = xmpp_stanza_get_from(stanza);
                log_warning("Available presence handler fired with invalid from attribute: %s", from);
                break;
            default:
                log_warning("Available presence handler fired, could not parse stanza.");
                break;
        }
        return;
    } else {
        char *jid = jid_fulljid_or_barejid(xmpp_presence->jid);
        log_debug("Presence available handler fired for: %s", jid);
    }

    xmpp_conn_t *conn = connection_get_conn();
    const char *my_jid_str = xmpp_conn_get_jid(conn);
    Jid *my_jid = jid_create(my_jid_str);

    XMPPCaps *caps = stanza_parse_caps(stanza);
    if ((g_strcmp0(my_jid->fulljid, xmpp_presence->jid->fulljid) != 0) && caps) {
        log_info("Presence contains capabilities.");
        char *jid = jid_fulljid_or_barejid(xmpp_presence->jid);
        _handle_caps(jid, caps);
    }
    stanza_free_caps(caps);

    Resource *resource = stanza_resource_from_presence(xmpp_presence);

    if (g_strcmp0(xmpp_presence->jid->barejid, my_jid->barejid) == 0) {
        connection_add_available_resource(resource);
    } else {
        char *pgpsig = NULL;
        xmpp_stanza_t *x = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_SIGNED);
        if (x) {
            pgpsig = xmpp_stanza_get_text(x);
        }
        sv_ev_contact_online(xmpp_presence->jid->barejid, resource, xmpp_presence->last_activity, pgpsig);
        xmpp_ctx_t *ctx = connection_get_ctx();
        xmpp_free(ctx, pgpsig);
    }

    jid_destroy(my_jid);
    stanza_free_presence(xmpp_presence);
}

void
_send_caps_request(char *node, char *caps_key, char *id, char *from)
{
    if (!node) {
        log_debug("No node string, not sending discovery IQ.");
        return;
    }

    log_debug("Node string: %s.", node);
    if (caps_cache_contains(caps_key)) {
        log_debug("Capabilities already cached, for %s", caps_key);
        return;
    }

    log_debug("Capabilities not cached for '%s', sending discovery IQ.", from);
    xmpp_ctx_t *ctx = connection_get_ctx();
    xmpp_stanza_t *iq = stanza_create_disco_info_iq(ctx, id, from, node);

    iq_send_stanza(iq);

    xmpp_stanza_release(iq);
}

static void
_muc_user_self_handler(xmpp_stanza_t *stanza)
{
    const char *from = xmpp_stanza_get_from(stanza);
    Jid *from_jid = jid_create(from);

    log_debug("Room self presence received from %s", from_jid->fulljid);

    char *room = from_jid->barejid;

    const char *type = xmpp_stanza_get_type(stanza);
    if (g_strcmp0(type, STANZA_TYPE_UNAVAILABLE) == 0) {

        // handle nickname change
        const char *new_nick = stanza_get_new_nick(stanza);
        if (new_nick) {
            muc_nick_change_start(room, new_nick);

        // handle left room
        } else {
            GSList *status_codes = stanza_get_status_codes_by_ns(stanza, STANZA_NS_MUC_USER);

            // room destroyed
            if (stanza_room_destroyed(stanza)) {
                const char *new_jid = stanza_get_muc_destroy_alternative_room(stanza);
                char *password = stanza_get_muc_destroy_alternative_password(stanza);
                char *reason = stanza_get_muc_destroy_reason(stanza);
                sv_ev_room_destroyed(room, new_jid, password, reason);
                free(password);
                free(reason);

            // kicked from room
            } else if (g_slist_find_custom(status_codes, "307", (GCompareFunc)g_strcmp0)) {
                const char *actor = stanza_get_actor(stanza);
                char *reason = stanza_get_reason(stanza);
                sv_ev_room_kicked(room, actor, reason);
                free(reason);

            // banned from room
            } else if (g_slist_find_custom(status_codes, "301", (GCompareFunc)g_strcmp0)) {
                const char *actor = stanza_get_actor(stanza);
                char *reason = stanza_get_reason(stanza);
                sv_ev_room_banned(room, actor, reason);
                free(reason);

            // normal exit
            } else {
                sv_ev_leave_room(room);
            }

            g_slist_free_full(status_codes, free);
        }
    } else {
        gboolean config_required = stanza_muc_requires_config(stanza);
        const char *actor = stanza_get_actor(stanza);
        char *reason = stanza_get_reason(stanza);
        char *nick = from_jid->resourcepart;
        char *show_str = stanza_get_show(stanza, "online");
        char *status_str = stanza_get_status(stanza, NULL);
        const char *jid = NULL;
        const char *role = NULL;
        const char *affiliation = NULL;
        xmpp_stanza_t *x = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_MUC_USER);
        if (x) {
            xmpp_stanza_t *item = xmpp_stanza_get_child_by_name(x, STANZA_NAME_ITEM);
            if (item) {
                jid = xmpp_stanza_get_attribute(item, "jid");
                role = xmpp_stanza_get_attribute(item, "role");
                affiliation = xmpp_stanza_get_attribute(item, "affiliation");
            }
        }
        sv_ev_muc_self_online(room, nick, config_required, role, affiliation, actor, reason, jid, show_str, status_str);
        free(show_str);
        free(status_str);
        free(reason);
    }

    jid_destroy(from_jid);
}

static void
_muc_user_occupant_handler(xmpp_stanza_t *stanza)
{
    const char *from = xmpp_stanza_get_from(stanza);
    Jid *from_jid = jid_create(from);

    log_debug("Room presence received from %s", from_jid->fulljid);

    char *room = from_jid->barejid;
    char *nick = from_jid->resourcepart;
    char *status_str = stanza_get_status(stanza, NULL);

    const char *type = xmpp_stanza_get_type(stanza);
    if (g_strcmp0(type, STANZA_TYPE_UNAVAILABLE) == 0) {

        // handle nickname change
        const char *new_nick = stanza_get_new_nick(stanza);
        if (new_nick) {
            muc_occupant_nick_change_start(room, new_nick, nick);

        // handle left room
        } else {
            GSList *status_codes = stanza_get_status_codes_by_ns(stanza, STANZA_NS_MUC_USER);

            // kicked from room
            if (g_slist_find_custom(status_codes, "307", (GCompareFunc)g_strcmp0)) {
                const char *actor = stanza_get_actor(stanza);
                char *reason = stanza_get_reason(stanza);
                sv_ev_room_occupent_kicked(room, nick, actor, reason);
                free(reason);

            // banned from room
            } else if (g_slist_find_custom(status_codes, "301", (GCompareFunc)g_strcmp0)) {
                const char *actor = stanza_get_actor(stanza);
                char *reason = stanza_get_reason(stanza);
                sv_ev_room_occupent_banned(room, nick, actor, reason);
                free(reason);

            // normal exit
            } else {
                sv_ev_room_occupant_offline(room, nick, "offline", status_str);
            }

            g_slist_free_full(status_codes, free);
        }

    // room occupant online
    } else {
        // send disco info for capabilities, if not cached
        XMPPCaps *caps = stanza_parse_caps(stanza);
        if (caps) {
            log_info("Presence contains capabilities.");
            _handle_caps(from, caps);
        }
        stanza_free_caps(caps);

        const char *actor = stanza_get_actor(stanza);
        char *show_str = stanza_get_show(stanza, "online");
        char *reason = stanza_get_reason(stanza);
        const char *jid = NULL;
        const char *role = NULL;
        const char *affiliation = NULL;
        xmpp_stanza_t *x = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_MUC_USER);
        if (x) {
            xmpp_stanza_t *item = xmpp_stanza_get_child_by_name(x, STANZA_NAME_ITEM);
            if (item) {
                jid = xmpp_stanza_get_attribute(item, "jid");
                role = xmpp_stanza_get_attribute(item, "role");
                affiliation = xmpp_stanza_get_attribute(item, "affiliation");
            }
        }
        sv_ev_muc_occupant_online(room, nick, jid, role, affiliation, actor, reason, show_str, status_str);
        free(show_str);
        free(reason);
    }

    jid_destroy(from_jid);
    free(status_str);
}

static void
_muc_user_handler(xmpp_stanza_t *const stanza)
{
    inp_nonblocking(TRUE);

    const char *type = xmpp_stanza_get_type(stanza);
    // handler still fires if error
    if (g_strcmp0(type, STANZA_TYPE_ERROR) == 0) {
        return;
    }

    const char *from = xmpp_stanza_get_from(stanza);
    if (!from) {
        log_warning("MUC User stanza received with no from attribute");
        return;
    }

    Jid *from_jid = jid_create(from);
    if (from_jid == NULL || from_jid->resourcepart == NULL) {
        log_warning("MUC User stanza received with invalid from attribute: %s", from);
        jid_destroy(from_jid);
        return;
    }
    jid_destroy(from_jid);

    if (stanza_is_muc_self_presence(stanza, connection_get_fulljid())) {
        _muc_user_self_handler(stanza);
    } else {
        _muc_user_occupant_handler(stanza);
    }
}

static void
_send_presence_stanza(xmpp_stanza_t *const stanza)
{
    char *text;
    size_t text_size;
    xmpp_stanza_to_text(stanza, &text, &text_size);

    xmpp_conn_t *conn = connection_get_conn();
    char *plugin_text = plugins_on_presence_stanza_send(text);
    if (plugin_text) {
        xmpp_send_raw_string(conn, "%s", plugin_text);
        free(plugin_text);
    } else {
        xmpp_send_raw_string(conn, "%s", text);
    }
    xmpp_free(connection_get_ctx(), text);
}
