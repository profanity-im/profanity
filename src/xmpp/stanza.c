/*
 * stanza.c
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

#include <glib.h>
#include <strophe.h>

#include "common.h"
#include "log.h"
#include "xmpp/connection.h"
#include "xmpp/stanza.h"
#include "xmpp/capabilities.h"

#include "muc.h"

static int _field_compare(FormField *f1, FormField *f2);

#if 0
xmpp_stanza_t *
stanza_create_bookmarks_pubsub_request(xmpp_ctx_t *ctx)
{
    xmpp_stanza_t *iq, *pubsub, *items;

    /* TODO: check pointers for NULL */
    iq = xmpp_stanza_new(ctx);
    pubsub = xmpp_stanza_new(ctx);
    items = xmpp_stanza_new(ctx);

    xmpp_stanza_set_name(iq, STANZA_NAME_IQ);
    xmpp_stanza_set_type(iq, STANZA_TYPE_GET);

    xmpp_stanza_set_name(pubsub, STANZA_NAME_PUBSUB);
    xmpp_stanza_set_ns(pubsub, STANZA_NS_PUBSUB);

    xmpp_stanza_set_name(items, STANZA_NAME_ITEMS);
    xmpp_stanza_set_attribute(items, "node", "storage:bookmarks");

    xmpp_stanza_add_child(pubsub, items);
    xmpp_stanza_add_child(iq, pubsub);
    xmpp_stanza_release(items);
    xmpp_stanza_release(pubsub);

    return iq;
}
#endif

xmpp_stanza_t *
stanza_create_bookmarks_storage_request(xmpp_ctx_t *ctx)
{
    xmpp_stanza_t *iq, *query, *storage;

    /* TODO: check pointers for NULL */
    iq = xmpp_stanza_new(ctx);
    query = xmpp_stanza_new(ctx);
    storage = xmpp_stanza_new(ctx);

    xmpp_stanza_set_name(iq, STANZA_NAME_IQ);
    xmpp_stanza_set_type(iq, STANZA_TYPE_GET);
    xmpp_stanza_set_ns(iq, "jabber:client");

    xmpp_stanza_set_name(query, STANZA_NAME_QUERY);
    xmpp_stanza_set_ns(query, "jabber:iq:private");

    xmpp_stanza_set_name(storage, STANZA_NAME_STORAGE);
    xmpp_stanza_set_ns(storage, "storage:bookmarks");

    xmpp_stanza_add_child(query, storage);
    xmpp_stanza_add_child(iq, query);
    xmpp_stanza_release(storage);
    xmpp_stanza_release(query);

    return iq;
}

#if 0
xmpp_stanza_t *
stanza_create_bookmarks_pubsub_add(xmpp_ctx_t *ctx, const char * const jid,
    const gboolean autojoin, const char * const nick)
{
    xmpp_stanza_t *stanza = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(stanza, STANZA_NAME_IQ);
    char *id = create_unique_id("bookmark_add");
    xmpp_stanza_set_id(stanza, id);
    xmpp_stanza_set_type(stanza, STANZA_TYPE_SET);

    xmpp_stanza_t *pubsub = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(pubsub, STANZA_NAME_PUBSUB);
    xmpp_stanza_set_ns(pubsub, STANZA_NS_PUBSUB);
    xmpp_stanza_add_child(stanza, pubsub);

    xmpp_stanza_t *publish = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(publish, STANZA_NAME_PUBLISH);
    xmpp_stanza_set_attribute(publish, STANZA_ATTR_NODE, "storage:bookmarks");
    xmpp_stanza_add_child(pubsub, publish);

    xmpp_stanza_t *item = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(item, STANZA_NAME_ITEM);
    xmpp_stanza_add_child(publish, item);

    xmpp_stanza_t *conference = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(conference, STANZA_NAME_CONFERENCE);
    xmpp_stanza_set_ns(conference, "storage:bookmarks");
    xmpp_stanza_set_attribute(conference, STANZA_ATTR_JID, jid);

    if (autojoin) {
        xmpp_stanza_set_attribute(conference, STANZA_ATTR_AUTOJOIN, "true");
    } else {
        xmpp_stanza_set_attribute(conference, STANZA_ATTR_AUTOJOIN, "false");
    }

    if (nick != NULL) {
        xmpp_stanza_t *nick_st = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(nick_st, STANZA_NAME_NICK);
        xmpp_stanza_set_text(nick_st, nick);
        xmpp_stanza_add_child(conference, nick_st);
    }

    xmpp_stanza_add_child(item, conference);

    xmpp_stanza_t *publish_options = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(publish_options, STANZA_NAME_PUBLISH_OPTIONS);
    xmpp_stanza_add_child(pubsub, publish_options);

    xmpp_stanza_t *x = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(x, STANZA_NAME_X);
    xmpp_stanza_set_ns(x, STANZA_NS_DATA);
    xmpp_stanza_set_attribute(x, STANZA_ATTR_TYPE, "submit");
    xmpp_stanza_add_child(publish_options, x);

    xmpp_stanza_t *form_type = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(form_type, STANZA_NAME_FIELD);
    xmpp_stanza_set_attribute(form_type, STANZA_ATTR_VAR, "FORM_TYPE");
    xmpp_stanza_set_attribute(form_type, STANZA_ATTR_TYPE, "hidden");
    xmpp_stanza_t *form_type_value = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(form_type_value, STANZA_NAME_VALUE);
    xmpp_stanza_t *form_type_value_text = xmpp_stanza_new(ctx);
    xmpp_stanza_set_text(form_type_value_text, "http://jabber.org/protocol/pubsub#publish-options");
    xmpp_stanza_add_child(form_type_value, form_type_value_text);
    xmpp_stanza_add_child(form_type, form_type_value);
    xmpp_stanza_add_child(x, form_type);

    xmpp_stanza_t *persist_items = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(persist_items, STANZA_NAME_FIELD);
    xmpp_stanza_set_attribute(persist_items, STANZA_ATTR_VAR, "pubsub#persist_items");
    xmpp_stanza_t *persist_items_value = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(persist_items_value, STANZA_NAME_VALUE);
    xmpp_stanza_t *persist_items_value_text = xmpp_stanza_new(ctx);
    xmpp_stanza_set_text(persist_items_value_text, "true");
    xmpp_stanza_add_child(persist_items_value, persist_items_value_text);
    xmpp_stanza_add_child(persist_items, persist_items_value);
    xmpp_stanza_add_child(x, persist_items);

    xmpp_stanza_t *access_model = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(access_model, STANZA_NAME_FIELD);
    xmpp_stanza_set_attribute(access_model, STANZA_ATTR_VAR, "pubsub#access_model");
    xmpp_stanza_t *access_model_value = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(access_model_value, STANZA_NAME_VALUE);
    xmpp_stanza_t *access_model_value_text = xmpp_stanza_new(ctx);
    xmpp_stanza_set_text(access_model_value_text, "whitelist");
    xmpp_stanza_add_child(access_model_value, access_model_value_text);
    xmpp_stanza_add_child(access_model, access_model_value);
    xmpp_stanza_add_child(x, access_model);

    return stanza;
}
#endif

xmpp_stanza_t *
stanza_create_chat_state(xmpp_ctx_t *ctx, const char * const recipient,
    const char * const state)
{
    xmpp_stanza_t *msg, *chat_state;

    msg = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(msg, STANZA_NAME_MESSAGE);
    xmpp_stanza_set_type(msg, STANZA_TYPE_CHAT);
    xmpp_stanza_set_attribute(msg, STANZA_ATTR_TO, recipient);
    char *id = create_unique_id(NULL);
    xmpp_stanza_set_id(msg, id);
    free(id);

    chat_state = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(chat_state, state);
    xmpp_stanza_set_ns(chat_state, STANZA_NS_CHATSTATES);
    xmpp_stanza_add_child(msg, chat_state);
    xmpp_stanza_release(chat_state);

    return msg;
}

xmpp_stanza_t *
stanza_create_message(xmpp_ctx_t *ctx, const char * const recipient,
    const char * const type, const char * const message,
    const char * const state)
{
    xmpp_stanza_t *msg, *body, *text;

    msg = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(msg, STANZA_NAME_MESSAGE);
    xmpp_stanza_set_type(msg, type);
    xmpp_stanza_set_attribute(msg, STANZA_ATTR_TO, recipient);
    char *id = create_unique_id(NULL);
    xmpp_stanza_set_id(msg, id);
    free(id);

    body = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(body, STANZA_NAME_BODY);

    text = xmpp_stanza_new(ctx);
    xmpp_stanza_set_text(text, message);
    xmpp_stanza_add_child(body, text);
    xmpp_stanza_release(text);
    xmpp_stanza_add_child(msg, body);
    xmpp_stanza_release(body);

    if (state != NULL) {
        xmpp_stanza_t *chat_state = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(chat_state, state);
        xmpp_stanza_set_ns(chat_state, STANZA_NS_CHATSTATES);
        xmpp_stanza_add_child(msg, chat_state);
        xmpp_stanza_release(chat_state);
    }

    return msg;
}

xmpp_stanza_t *
stanza_create_roster_remove_set(xmpp_ctx_t *ctx, const char * const barejid)
{
    xmpp_stanza_t *iq = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(iq, STANZA_NAME_IQ);
    xmpp_stanza_set_type(iq, STANZA_TYPE_SET);

    xmpp_stanza_t *query = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(query, STANZA_NAME_QUERY);
    xmpp_stanza_set_ns(query, XMPP_NS_ROSTER);

    xmpp_stanza_t *item = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(item, STANZA_NAME_ITEM);
    xmpp_stanza_set_attribute(item, STANZA_ATTR_JID, barejid);
    xmpp_stanza_set_attribute(item, STANZA_ATTR_SUBSCRIPTION, "remove");

    xmpp_stanza_add_child(query, item);
    xmpp_stanza_release(item);
    xmpp_stanza_add_child(iq, query);
    xmpp_stanza_release(query);

    return iq;

}

xmpp_stanza_t *
stanza_create_roster_set(xmpp_ctx_t *ctx, const char * const id,
    const char * const jid, const char * const handle, GSList *groups)
{
    xmpp_stanza_t *iq = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(iq, STANZA_NAME_IQ);
    xmpp_stanza_set_type(iq, STANZA_TYPE_SET);
    if (id != NULL) {
        xmpp_stanza_set_id(iq, id);
    }

    xmpp_stanza_t *query = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(query, STANZA_NAME_QUERY);
    xmpp_stanza_set_ns(query, XMPP_NS_ROSTER);

    xmpp_stanza_t *item = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(item, STANZA_NAME_ITEM);
    xmpp_stanza_set_attribute(item, STANZA_ATTR_JID, jid);

    if (handle != NULL) {
        xmpp_stanza_set_attribute(item, STANZA_ATTR_NAME, handle);
    } else {
        xmpp_stanza_set_attribute(item, STANZA_ATTR_NAME, "");
    }

    while (groups != NULL) {
        xmpp_stanza_t *group = xmpp_stanza_new(ctx);
        xmpp_stanza_t *groupname = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(group, STANZA_NAME_GROUP);
        xmpp_stanza_set_text(groupname, groups->data);
        xmpp_stanza_add_child(group, groupname);
        xmpp_stanza_release(groupname);
        xmpp_stanza_add_child(item, group);
        xmpp_stanza_release(group);
        groups = g_slist_next(groups);
    }

    xmpp_stanza_add_child(query, item);
    xmpp_stanza_release(item);
    xmpp_stanza_add_child(iq, query);
    xmpp_stanza_release(query);

    return iq;
}

xmpp_stanza_t *
stanza_create_invite(xmpp_ctx_t *ctx, const char * const room,
    const char * const contact, const char * const reason)
{
    xmpp_stanza_t *message, *x;

    message = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(message, STANZA_NAME_MESSAGE);
    xmpp_stanza_set_attribute(message, STANZA_ATTR_TO, contact);
    char *id = create_unique_id(NULL);
    xmpp_stanza_set_id(message, id);
    free(id);

    x = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(x, STANZA_NAME_X);
    xmpp_stanza_set_ns(x, STANZA_NS_CONFERENCE);

    xmpp_stanza_set_attribute(x, STANZA_ATTR_JID, room);
    if (reason != NULL) {
        xmpp_stanza_set_attribute(x, STANZA_ATTR_REASON, reason);
    }

    xmpp_stanza_add_child(message, x);
    xmpp_stanza_release(x);

    return message;
}

xmpp_stanza_t *
stanza_create_room_join_presence(xmpp_ctx_t * const ctx,
    const char * const full_room_jid, const char * const passwd)
{
    xmpp_stanza_t *presence = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(presence, STANZA_NAME_PRESENCE);
    xmpp_stanza_set_attribute(presence, STANZA_ATTR_TO, full_room_jid);
    char *id = create_unique_id("join");
    xmpp_stanza_set_id(presence, id);
    free(id);

    xmpp_stanza_t *x = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(x, STANZA_NAME_X);
    xmpp_stanza_set_ns(x, STANZA_NS_MUC);

    // if a password was given
    if (passwd != NULL) {
        xmpp_stanza_t *pass = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(pass, "password");
        xmpp_stanza_t *text = xmpp_stanza_new(ctx);
        xmpp_stanza_set_text(text, passwd);
        xmpp_stanza_add_child(pass, text);
        xmpp_stanza_add_child(x, pass);
        xmpp_stanza_release(text);
        xmpp_stanza_release(pass);
    }

    xmpp_stanza_add_child(presence, x);
    xmpp_stanza_release(x);

    return presence;
}

xmpp_stanza_t *
stanza_create_room_newnick_presence(xmpp_ctx_t *ctx,
    const char * const full_room_jid)
{
    xmpp_stanza_t *presence = xmpp_stanza_new(ctx);
    char *id = create_unique_id("sub");
    xmpp_stanza_set_id(presence, id);
    free(id);
    xmpp_stanza_set_name(presence, STANZA_NAME_PRESENCE);
    xmpp_stanza_set_attribute(presence, STANZA_ATTR_TO, full_room_jid);

    return presence;
}

xmpp_stanza_t *
stanza_create_room_leave_presence(xmpp_ctx_t *ctx, const char * const room,
    const char * const nick)
{
    GString *full_jid = g_string_new(room);
    g_string_append(full_jid, "/");
    g_string_append(full_jid, nick);

    xmpp_stanza_t *presence = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(presence, STANZA_NAME_PRESENCE);
    xmpp_stanza_set_type(presence, STANZA_TYPE_UNAVAILABLE);
    xmpp_stanza_set_attribute(presence, STANZA_ATTR_TO, full_jid->str);
    char *id = create_unique_id("leave");
    xmpp_stanza_set_id(presence, id);
    free(id);

    g_string_free(full_jid, TRUE);

    return presence;
}

xmpp_stanza_t *
stanza_create_presence(xmpp_ctx_t * const ctx)
{
    xmpp_stanza_t *presence = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(presence, STANZA_NAME_PRESENCE);

    return presence;
}

xmpp_stanza_t *
stanza_create_software_version_iq(xmpp_ctx_t *ctx, const char * const fulljid)
{
    xmpp_stanza_t *iq = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(iq, STANZA_NAME_IQ);
    xmpp_stanza_set_type(iq, STANZA_TYPE_GET);
    xmpp_stanza_set_id(iq, "sv");
    xmpp_stanza_set_attribute(iq, "to", fulljid);

    xmpp_stanza_t *query = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(query, STANZA_NAME_QUERY);
    xmpp_stanza_set_ns(query, STANZA_NS_VERSION);

    xmpp_stanza_add_child(iq, query);
    xmpp_stanza_release(query);

    return iq;
}

xmpp_stanza_t *
stanza_create_roster_iq(xmpp_ctx_t *ctx)
{
    xmpp_stanza_t *iq = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(iq, STANZA_NAME_IQ);
    xmpp_stanza_set_type(iq, STANZA_TYPE_GET);
    xmpp_stanza_set_id(iq, "roster");

    xmpp_stanza_t *query = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(query, STANZA_NAME_QUERY);
    xmpp_stanza_set_ns(query, XMPP_NS_ROSTER);

    xmpp_stanza_add_child(iq, query);
    xmpp_stanza_release(query);

    return iq;
}

xmpp_stanza_t *
stanza_create_disco_info_iq(xmpp_ctx_t *ctx, const char * const id, const char * const to,
    const char * const node)
{
    xmpp_stanza_t *iq = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(iq, STANZA_NAME_IQ);
    xmpp_stanza_set_type(iq, STANZA_TYPE_GET);
    xmpp_stanza_set_attribute(iq, STANZA_ATTR_TO, to);
    xmpp_stanza_set_id(iq, id);

    xmpp_stanza_t *query = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(query, STANZA_NAME_QUERY);
    xmpp_stanza_set_ns(query, XMPP_NS_DISCO_INFO);
    if (node != NULL) {
        xmpp_stanza_set_attribute(query, STANZA_ATTR_NODE, node);
    }

    xmpp_stanza_add_child(iq, query);
    xmpp_stanza_release(query);

    return iq;
}

xmpp_stanza_t *
stanza_create_disco_items_iq(xmpp_ctx_t *ctx, const char * const id,
    const char * const jid)
{
    xmpp_stanza_t *iq = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(iq, STANZA_NAME_IQ);
    xmpp_stanza_set_type(iq, STANZA_TYPE_GET);
    xmpp_stanza_set_attribute(iq, STANZA_ATTR_TO, jid);
    xmpp_stanza_set_id(iq, id);

    xmpp_stanza_t *query = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(query, STANZA_NAME_QUERY);
    xmpp_stanza_set_ns(query, XMPP_NS_DISCO_ITEMS);

    xmpp_stanza_add_child(iq, query);
    xmpp_stanza_release(query);

    return iq;
}

gboolean
stanza_contains_chat_state(xmpp_stanza_t *stanza)
{
    return ((xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_ACTIVE) != NULL) ||
            (xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_COMPOSING) != NULL) ||
            (xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_PAUSED) != NULL) ||
            (xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_GONE) != NULL) ||
            (xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_INACTIVE) != NULL));
}

xmpp_stanza_t *
stanza_create_ping_iq(xmpp_ctx_t *ctx)
{
    xmpp_stanza_t *iq = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(iq, STANZA_NAME_IQ);
    xmpp_stanza_set_type(iq, STANZA_TYPE_GET);
    char *id = create_unique_id("ping");
    xmpp_stanza_set_id(iq, id);
    free(id);

    xmpp_stanza_t *ping = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(ping, STANZA_NAME_PING);

    xmpp_stanza_set_ns(ping, STANZA_NS_PING);

    xmpp_stanza_add_child(iq, ping);
    xmpp_stanza_release(ping);

    return iq;
}

gboolean
stanza_get_delay(xmpp_stanza_t * const stanza, GTimeVal *tv_stamp)
{
    // first check for XEP-0203 delayed delivery
    xmpp_stanza_t *delay = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_DELAY);
    if (delay != NULL) {
        char *xmlns = xmpp_stanza_get_attribute(delay, STANZA_ATTR_XMLNS);
        if ((xmlns != NULL) && (strcmp(xmlns, "urn:xmpp:delay") == 0)) {
            char *stamp = xmpp_stanza_get_attribute(delay, STANZA_ATTR_STAMP);
            if ((stamp != NULL) && (g_time_val_from_iso8601(stamp, tv_stamp))) {
                return TRUE;
            }
        }
    }

    // otherwise check for XEP-0091 legacy delayed delivery
    // stanp format : CCYYMMDDThh:mm:ss
    xmpp_stanza_t *x = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_X);
    if (x != NULL) {
        char *xmlns = xmpp_stanza_get_attribute(x, STANZA_ATTR_XMLNS);
        if ((xmlns != NULL) && (strcmp(xmlns, "jabber:x:delay") == 0)) {
            char *stamp = xmpp_stanza_get_attribute(x, STANZA_ATTR_STAMP);
            if ((stamp != NULL) && (g_time_val_from_iso8601(stamp, tv_stamp))) {
                return TRUE;
            }
        }
    }

    return FALSE;
}

char *
stanza_get_status(xmpp_stanza_t *stanza, char *def)
{
    xmpp_ctx_t *ctx = connection_get_ctx();
    xmpp_stanza_t *status =
        xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_STATUS);

    if (status != NULL) {
        // xmpp_free and free may be different functions so convert all to
        // libc malloc
        char *s1, *s2 = NULL;
        s1 = xmpp_stanza_get_text(status);
        if (s1 != NULL) {
            s2 = strdup(s1);
            xmpp_free(ctx, s1);
        }
        return s2;
    } else if (def != NULL) {
        return strdup(def);
    } else {
        return NULL;
    }
}

char *
stanza_get_show(xmpp_stanza_t *stanza, char *def)
{
    xmpp_ctx_t *ctx = connection_get_ctx();
    xmpp_stanza_t *show =
        xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_SHOW);

    if (show != NULL) {
        // xmpp_free and free may be different functions so convert all to
        // libc malloc
        char *s1, *s2 = NULL;
        s1 = xmpp_stanza_get_text(show);
        if (s1 != NULL) {
            s2 = strdup(s1);
            xmpp_free(ctx, s1);
        }
        return s2;
    } else if (def != NULL) {
        return strdup(def);
    } else {
        return NULL;
    }
}

gboolean
stanza_is_muc_presence(xmpp_stanza_t * const stanza)
{
    if (stanza == NULL) {
        return FALSE;
    }
    if (strcmp(xmpp_stanza_get_name(stanza), STANZA_NAME_PRESENCE) != 0) {
        return FALSE;
    }

    xmpp_stanza_t *x = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_X);

    if (x == NULL) {
        return FALSE;
    }

    char *ns = xmpp_stanza_get_ns(x);
    if (ns == NULL) {
        return FALSE;
    }
    if (strcmp(ns, STANZA_NS_MUC_USER) != 0) {
        return FALSE;
    }

    return TRUE;
}

gboolean
stanza_is_muc_self_presence(xmpp_stanza_t * const stanza,
    const char * const self_jid)
{
    // no stanza, or not presence stanza
    if ((stanza == NULL) || (g_strcmp0(xmpp_stanza_get_name(stanza), STANZA_NAME_PRESENCE) != 0)) {
        return FALSE;
    }

    // muc user namespaced x element
    xmpp_stanza_t *x = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_MUC_USER);
    if (x != NULL) {

        // check for status child element with 110 code
        xmpp_stanza_t *x_children = xmpp_stanza_get_children(x);
        while (x_children != NULL) {
            if (g_strcmp0(xmpp_stanza_get_name(x_children), STANZA_NAME_STATUS) == 0) {
                char *code = xmpp_stanza_get_attribute(x_children, STANZA_ATTR_CODE);
                if (g_strcmp0(code, "110") == 0) {
                    return TRUE;
                }
            }
            x_children = xmpp_stanza_get_next(x_children);
        }

        // check for item child element with jid property
        xmpp_stanza_t *item = xmpp_stanza_get_child_by_name(x, STANZA_NAME_ITEM);
        if (item != NULL) {
            char *jid = xmpp_stanza_get_attribute(item, STANZA_ATTR_JID);
            if (jid != NULL) {
                if (g_str_has_prefix(self_jid, jid)) {
                    return TRUE;
                }
            }
        }

        // check if 'from' attribute identifies this user
        char *from = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_FROM);
        if (from != NULL) {
            Jid *from_jid = jid_create(from);
            if (muc_room_is_active(from_jid->barejid)) {
                char *nick = muc_get_room_nick(from_jid->barejid);
                if (g_strcmp0(from_jid->resourcepart, nick) == 0) {
                    return TRUE;
                }
            }

            // check if a new nickname maps to a pending nick change for this user
            if (muc_is_room_pending_nick_change(from_jid->barejid)) {
                char *new_nick = from_jid->resourcepart;
                if (new_nick != NULL) {
                    char *nick = muc_get_room_nick(from_jid->barejid);
                    char *old_nick = muc_get_old_nick(from_jid->barejid, new_nick);
                    if (g_strcmp0(old_nick, nick) == 0) {
                        return TRUE;
                    }
                }
            }

            jid_destroy(from_jid);
        }
    }

    // self presence not found
    return FALSE;
}

gboolean
stanza_is_room_nick_change(xmpp_stanza_t * const stanza)
{
    // no stanza, or not presence stanza
    if ((stanza == NULL) || (g_strcmp0(xmpp_stanza_get_name(stanza), STANZA_NAME_PRESENCE) != 0)) {
        return FALSE;
    }

    // muc user namespaced x element
    xmpp_stanza_t *x = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_MUC_USER);
    if (x != NULL) {

        // check for status child element with 303 code
        xmpp_stanza_t *x_children = xmpp_stanza_get_children(x);
        while (x_children != NULL) {
            if (g_strcmp0(xmpp_stanza_get_name(x_children), STANZA_NAME_STATUS) == 0) {
                char *code = xmpp_stanza_get_attribute(x_children, STANZA_ATTR_CODE);
                if (g_strcmp0(code, "303") == 0) {
                    return TRUE;
                }
            }
            x_children = xmpp_stanza_get_next(x_children);
        }
    }

    return FALSE;
}

char *
stanza_get_new_nick(xmpp_stanza_t * const stanza)
{
    if (!stanza_is_room_nick_change(stanza)) {
        return NULL;
    } else {
        xmpp_stanza_t *x = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_X);
        xmpp_stanza_t *x_children = xmpp_stanza_get_children(x);

        while (x_children != NULL) {
            if (strcmp(xmpp_stanza_get_name(x_children), STANZA_NAME_ITEM) == 0) {
                char *nick = xmpp_stanza_get_attribute(x_children, STANZA_ATTR_NICK);
                if (nick != NULL) {
                    return strdup(nick);
                }
            }
            x_children = xmpp_stanza_get_next(x_children);
        }

        return NULL;
    }
}

int
stanza_get_idle_time(xmpp_stanza_t * const stanza)
{
    xmpp_stanza_t *query = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_QUERY);

    if (query == NULL) {
        return 0;
    }

    char *ns = xmpp_stanza_get_ns(query);
    if (ns == NULL) {
        return 0;
    }

    if (strcmp(ns, STANZA_NS_LASTACTIVITY) != 0) {
        return 0;
    }

    char *seconds_str = xmpp_stanza_get_attribute(query, STANZA_ATTR_SECONDS);
    if (seconds_str == NULL) {
        return 0;
    }

    int result = atoi(seconds_str);
    if (result < 1) {
        return 0;
    } else {
        return result;
    }
}

gboolean
stanza_contains_caps(xmpp_stanza_t * const stanza)
{
    xmpp_stanza_t *caps = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_C);

    if (caps == NULL) {
        return FALSE;
    }

    if (strcmp(xmpp_stanza_get_ns(caps), STANZA_NS_CAPS) != 0) {
        return FALSE;
    }

    return TRUE;
}

char *
stanza_caps_get_hash(xmpp_stanza_t * const stanza)
{
    xmpp_stanza_t *caps = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_C);

    if (caps == NULL) {
        return NULL;
    }

    if (strcmp(xmpp_stanza_get_ns(caps), STANZA_NS_CAPS) != 0) {
        return NULL;
    }

    char *result = xmpp_stanza_get_attribute(caps, STANZA_ATTR_HASH);

    return result;

}

char *
stanza_get_caps_str(xmpp_stanza_t * const stanza)
{
    xmpp_stanza_t *caps = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_C);

    if (caps == NULL) {
        return NULL;
    }

    if (strcmp(xmpp_stanza_get_ns(caps), STANZA_NS_CAPS) != 0) {
        return NULL;
    }

    char *node = xmpp_stanza_get_attribute(caps, STANZA_ATTR_NODE);
    char *ver = xmpp_stanza_get_attribute(caps, STANZA_ATTR_VER);

    if ((node == NULL) || (ver == NULL)) {
        return NULL;
    }

    GString *caps_gstr = g_string_new(node);
    g_string_append(caps_gstr, "#");
    g_string_append(caps_gstr, ver);
    char *caps_str = caps_gstr->str;
    g_string_free(caps_gstr, FALSE);

    return  caps_str;
}

char *
stanza_get_error_message(xmpp_stanza_t *stanza)
{
    xmpp_ctx_t *ctx = connection_get_ctx();
    xmpp_stanza_t *error_stanza = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_ERROR);

    // return nothing if no error stanza
    if (error_stanza == NULL) {
        return strdup("unknown");
    } else {
        xmpp_stanza_t *text_stanza = xmpp_stanza_get_child_by_name(error_stanza, STANZA_NAME_TEXT);

        // check for text
        if (text_stanza != NULL) {
            gchar *err_msg = xmpp_stanza_get_text(text_stanza);
            if (err_msg != NULL) {
                char *result =  strdup(err_msg);
                xmpp_free(ctx, err_msg);
                return result;
            }

        // otherwise check each defined-condition RFC-6120 8.3.3
        } else {
            gchar *defined_conditions[] = {
                STANZA_NAME_BAD_REQUEST,
                STANZA_NAME_CONFLICT,
                STANZA_NAME_FEATURE_NOT_IMPLEMENTED,
                STANZA_NAME_FORBIDDEN,
                STANZA_NAME_GONE,
                STANZA_NAME_INTERNAL_SERVER_ERROR,
                STANZA_NAME_ITEM_NOT_FOUND,
                STANZA_NAME_JID_MALFORMED,
                STANZA_NAME_NOT_ACCEPTABLE,
                STANZA_NAME_NOT_ALLOWED,
                STANZA_NAME_NOT_AUTHORISED,
                STANZA_NAME_POLICY_VIOLATION,
                STANZA_NAME_RECIPIENT_UNAVAILABLE,
                STANZA_NAME_REDIRECT,
                STANZA_NAME_REGISTRATION_REQUIRED,
                STANZA_NAME_REMOTE_SERVER_NOT_FOUND,
                STANZA_NAME_REMOTE_SERVER_TIMEOUT,
                STANZA_NAME_RESOURCE_CONSTRAINT,
                STANZA_NAME_SERVICE_UNAVAILABLE,
                STANZA_NAME_SUBSCRIPTION_REQUIRED,
                STANZA_NAME_UNEXPECTED_REQUEST
            };

            int i;
            for (i = 0; i < ARRAY_SIZE(defined_conditions); i++) {
                xmpp_stanza_t *cond_stanza = xmpp_stanza_get_child_by_name(error_stanza, defined_conditions[i]);
                if (cond_stanza != NULL) {
                    char *result = strdup(xmpp_stanza_get_name(cond_stanza));
                    return result;
                }
            }

        }
    }

    // if undefined-condition or no condition, return nothing
    return strdup("unknown");
}

DataForm *
stanza_create_form(xmpp_stanza_t * const stanza)
{
    DataForm *result = NULL;
    xmpp_ctx_t *ctx = connection_get_ctx();

    xmpp_stanza_t *child = xmpp_stanza_get_children(stanza);

    if (child != NULL) {
        result = malloc(sizeof(DataForm));
        result->form_type = NULL;
        result->fields = NULL;
    }

    //handle fields
    while (child != NULL) {
        char *var = xmpp_stanza_get_attribute(child, "var");

        // handle FORM_TYPE
        if (g_strcmp0(var, "FORM_TYPE") == 0) {
            xmpp_stanza_t *value = xmpp_stanza_get_child_by_name(child, "value");
            char *value_text = xmpp_stanza_get_text(value);
            result->form_type = strdup(value_text);
            xmpp_free(ctx, value_text);

        // handle regular fields
        } else {
            FormField *field = malloc(sizeof(FormField));
            field->var = strdup(var);
            field->values = NULL;
            xmpp_stanza_t *value = xmpp_stanza_get_children(child);

            // handle values
            while (value != NULL) {
                char *text = xmpp_stanza_get_text(value);
                if (text != NULL) {
                    field->values = g_slist_insert_sorted(field->values, strdup(text), (GCompareFunc)strcmp);
                    xmpp_free(ctx, text);
                }
                value = xmpp_stanza_get_next(value);
            }

            result->fields = g_slist_insert_sorted(result->fields, field, (GCompareFunc)_field_compare);
        }

        child = xmpp_stanza_get_next(child);
    }

    return result;
}

void
stanza_destroy_form(DataForm *form)
{
    if (form != NULL) {
        if (form->fields != NULL) {
            GSList *curr_field = form->fields;
            while (curr_field != NULL) {
                FormField *field = curr_field->data;
                free(field->var);
                if ((field->values) != NULL) {
                    g_slist_free_full(field->values, free);
                }
                curr_field = curr_field->next;
            }
            g_slist_free_full(form->fields, free);
        }

        free(form->form_type);
        free(form);
    }
}

void
stanza_attach_priority(xmpp_ctx_t * const ctx, xmpp_stanza_t * const presence,
    const int pri)
{
    if (pri != 0) {
        xmpp_stanza_t *priority, *value;
        char pri_str[10];

        snprintf(pri_str, sizeof(pri_str), "%d", pri);
        priority = xmpp_stanza_new(ctx);
        value = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(priority, STANZA_NAME_PRIORITY);
        xmpp_stanza_set_text(value, pri_str);
        xmpp_stanza_add_child(priority, value);
        xmpp_stanza_release(value);
        xmpp_stanza_add_child(presence, priority);
        xmpp_stanza_release(priority);
    }
}

void
stanza_attach_show(xmpp_ctx_t * const ctx, xmpp_stanza_t * const presence,
    const char * const show)
{
    if (show != NULL) {
        xmpp_stanza_t *show_stanza = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(show_stanza, STANZA_NAME_SHOW);
        xmpp_stanza_t *text = xmpp_stanza_new(ctx);
        xmpp_stanza_set_text(text, show);
        xmpp_stanza_add_child(show_stanza, text);
        xmpp_stanza_add_child(presence, show_stanza);
        xmpp_stanza_release(text);
        xmpp_stanza_release(show_stanza);
    }
}

void
stanza_attach_status(xmpp_ctx_t * const ctx, xmpp_stanza_t * const presence,
    const char * const status)
{
    if (status != NULL) {
        xmpp_stanza_t *status_stanza = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(status_stanza, STANZA_NAME_STATUS);
        xmpp_stanza_t *text = xmpp_stanza_new(ctx);
        xmpp_stanza_set_text(text, status);
        xmpp_stanza_add_child(status_stanza, text);
        xmpp_stanza_add_child(presence, status_stanza);
        xmpp_stanza_release(text);
        xmpp_stanza_release(status_stanza);
    }
}

void
stanza_attach_last_activity(xmpp_ctx_t * const ctx,
    xmpp_stanza_t * const presence, const int idle)
{
    if (idle > 0) {
        xmpp_stanza_t *query = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(query, STANZA_NAME_QUERY);
        xmpp_stanza_set_ns(query, STANZA_NS_LASTACTIVITY);
        char idle_str[10];
        snprintf(idle_str, sizeof(idle_str), "%d", idle);
        xmpp_stanza_set_attribute(query, STANZA_ATTR_SECONDS, idle_str);
        xmpp_stanza_add_child(presence, query);
        xmpp_stanza_release(query);
    }
}

void
stanza_attach_caps(xmpp_ctx_t * const ctx, xmpp_stanza_t * const presence)
{
    xmpp_stanza_t *caps = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(caps, STANZA_NAME_C);
    xmpp_stanza_set_ns(caps, STANZA_NS_CAPS);
    xmpp_stanza_t *query = caps_create_query_response_stanza(ctx);

    char *sha1 = caps_create_sha1_str(query);
    xmpp_stanza_set_attribute(caps, STANZA_ATTR_HASH, "sha-1");
    xmpp_stanza_set_attribute(caps, STANZA_ATTR_NODE, "http://www.profanity.im");
    xmpp_stanza_set_attribute(caps, STANZA_ATTR_VER, sha1);
    xmpp_stanza_add_child(presence, caps);
    xmpp_stanza_release(caps);
    xmpp_stanza_release(query);
    g_free(sha1);
}

const char *
stanza_get_presence_string_from_type(resource_presence_t presence_type)
{
    switch(presence_type)
    {
        case RESOURCE_AWAY:
            return STANZA_TEXT_AWAY;
        case RESOURCE_DND:
            return STANZA_TEXT_DND;
        case RESOURCE_CHAT:
            return STANZA_TEXT_CHAT;
        case RESOURCE_XA:
            return STANZA_TEXT_XA;
        default:
            return NULL;
    }
}

static int
_field_compare(FormField *f1, FormField *f2)
{
    return strcmp(f1->var, f2->var);
}
