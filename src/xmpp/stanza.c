/*
 * stanza.c
 *
 * Copyright (C) 2012 - 2015 James Booth <boothj5@gmail.com>
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

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <glib.h>

#ifdef HAVE_LIBMESODE
#include <mesode.h>
#endif
#ifdef HAVE_LIBSTROPHE
#include <strophe.h>
#endif

#include "common.h"
#include "log.h"
#include "xmpp/connection.h"
#include "xmpp/stanza.h"
#include "xmpp/capabilities.h"
#include "xmpp/form.h"

#include "muc.h"

#if 0
xmpp_stanza_t*
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

xmpp_stanza_t*
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
xmpp_stanza_t*
stanza_create_bookmarks_pubsub_add(xmpp_ctx_t *ctx, const char *const jid,
    const gboolean autojoin, const char *const nick)
{
    xmpp_stanza_t *stanza = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(stanza, STANZA_NAME_IQ);
    char *id = create_unique_id("bookmark_add");
    xmpp_stanza_set_id(stanza, id);
    free(id);
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

    if (nick) {
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

xmpp_stanza_t*
stanza_enable_carbons(xmpp_ctx_t *ctx)
{
    xmpp_stanza_t *iq = xmpp_stanza_new(ctx);
    char *id = create_unique_id("carbons");

    xmpp_stanza_set_name(iq, STANZA_NAME_IQ);
    xmpp_stanza_set_type(iq, STANZA_TYPE_SET);
    xmpp_stanza_set_id(iq, id);
    free(id);

    xmpp_stanza_t *carbons_enable = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(carbons_enable, STANZA_NAME_ENABLE);
    xmpp_stanza_set_ns(carbons_enable, STANZA_NS_CARBONS);

    xmpp_stanza_add_child(iq, carbons_enable);
    xmpp_stanza_release(carbons_enable);

    return iq;
}

xmpp_stanza_t*
stanza_disable_carbons(xmpp_ctx_t *ctx)
{
    xmpp_stanza_t *iq = xmpp_stanza_new(ctx);
    char *id = create_unique_id("carbons");

    xmpp_stanza_set_name(iq, STANZA_NAME_IQ);
    xmpp_stanza_set_type(iq, STANZA_TYPE_SET);
    xmpp_stanza_set_id(iq, id);
    free(id);

    xmpp_stanza_t *carbons_disable = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(carbons_disable, STANZA_NAME_DISABLE);
    xmpp_stanza_set_ns(carbons_disable, STANZA_NS_CARBONS);

    xmpp_stanza_add_child(iq, carbons_disable);
    xmpp_stanza_release(carbons_disable);

    return iq;
}

xmpp_stanza_t*
stanza_create_chat_state(xmpp_ctx_t *ctx, const char *const fulljid, const char *const state)
{
    xmpp_stanza_t *msg, *chat_state;

    msg = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(msg, STANZA_NAME_MESSAGE);
    xmpp_stanza_set_type(msg, STANZA_TYPE_CHAT);
    xmpp_stanza_set_attribute(msg, STANZA_ATTR_TO, fulljid);
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

xmpp_stanza_t*
stanza_create_room_subject_message(xmpp_ctx_t *ctx, const char *const room, const char *const subject)
{
    xmpp_stanza_t *msg = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(msg, STANZA_NAME_MESSAGE);
    xmpp_stanza_set_type(msg, STANZA_TYPE_GROUPCHAT);
    xmpp_stanza_set_attribute(msg, STANZA_ATTR_TO, room);

    xmpp_stanza_t *subject_st = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(subject_st, STANZA_NAME_SUBJECT);
    if (subject) {
        xmpp_stanza_t *subject_text = xmpp_stanza_new(ctx);
        xmpp_stanza_set_text(subject_text, subject);
        xmpp_stanza_add_child(subject_st, subject_text);
        xmpp_stanza_release(subject_text);
    }

    xmpp_stanza_add_child(msg, subject_st);
    xmpp_stanza_release(subject_st);

    return msg;
}

xmpp_stanza_t*
stanza_attach_state(xmpp_ctx_t *ctx, xmpp_stanza_t *stanza, const char *const state)
{
    xmpp_stanza_t *chat_state = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(chat_state, state);
    xmpp_stanza_set_ns(chat_state, STANZA_NS_CHATSTATES);
    xmpp_stanza_add_child(stanza, chat_state);
    xmpp_stanza_release(chat_state);

    return stanza;
}

xmpp_stanza_t*
stanza_attach_carbons_private(xmpp_ctx_t *ctx, xmpp_stanza_t *stanza)
{
    xmpp_stanza_t *private_carbon = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(private_carbon, "private");
    xmpp_stanza_set_ns(private_carbon, STANZA_NS_CARBONS);
    xmpp_stanza_add_child(stanza, private_carbon);
    xmpp_stanza_release(private_carbon);

    return stanza;
}

xmpp_stanza_t*
stanza_attach_hints_no_copy(xmpp_ctx_t *ctx, xmpp_stanza_t *stanza)
{
    xmpp_stanza_t *no_copy = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(no_copy, "no-copy");
    xmpp_stanza_set_ns(no_copy, STANZA_NS_HINTS);
    xmpp_stanza_add_child(stanza, no_copy);
    xmpp_stanza_release(no_copy);

    return stanza;
}

xmpp_stanza_t*
stanza_attach_hints_no_store(xmpp_ctx_t *ctx, xmpp_stanza_t *stanza)
{
    xmpp_stanza_t *no_store = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(no_store, "no-store");
    xmpp_stanza_set_ns(no_store, STANZA_NS_HINTS);
    xmpp_stanza_add_child(stanza, no_store);
    xmpp_stanza_release(no_store);

    return stanza;
}

xmpp_stanza_t*
stanza_attach_receipt_request(xmpp_ctx_t *ctx, xmpp_stanza_t *stanza)
{
    xmpp_stanza_t *receipet_request = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(receipet_request, "request");
    xmpp_stanza_set_ns(receipet_request, STANZA_NS_RECEIPTS);
    xmpp_stanza_add_child(stanza, receipet_request);
    xmpp_stanza_release(receipet_request);

    return stanza;
}

xmpp_stanza_t*
stanza_create_message(xmpp_ctx_t *ctx, char *id, const char *const recipient,
    const char *const type, const char *const message)
{
    xmpp_stanza_t *msg, *body, *text;

    msg = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(msg, STANZA_NAME_MESSAGE);
    xmpp_stanza_set_type(msg, type);
    xmpp_stanza_set_attribute(msg, STANZA_ATTR_TO, recipient);
    xmpp_stanza_set_id(msg, id);

    body = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(body, STANZA_NAME_BODY);

    text = xmpp_stanza_new(ctx);
    xmpp_stanza_set_text(text, message);
    xmpp_stanza_add_child(body, text);
    xmpp_stanza_release(text);
    xmpp_stanza_add_child(msg, body);
    xmpp_stanza_release(body);

    return msg;
}

xmpp_stanza_t*
stanza_create_roster_remove_set(xmpp_ctx_t *ctx, const char *const barejid)
{
    xmpp_stanza_t *iq = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(iq, STANZA_NAME_IQ);
    xmpp_stanza_set_type(iq, STANZA_TYPE_SET);

    char *id = create_unique_id("roster");
    xmpp_stanza_set_id(iq, id);
    free(id);

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

xmpp_stanza_t*
stanza_create_roster_set(xmpp_ctx_t *ctx, const char *const id,
    const char *const jid, const char *const handle, GSList *groups)
{
    xmpp_stanza_t *iq = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(iq, STANZA_NAME_IQ);
    xmpp_stanza_set_type(iq, STANZA_TYPE_SET);
    if (id) {
        xmpp_stanza_set_id(iq, id);
    }

    xmpp_stanza_t *query = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(query, STANZA_NAME_QUERY);
    xmpp_stanza_set_ns(query, XMPP_NS_ROSTER);

    xmpp_stanza_t *item = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(item, STANZA_NAME_ITEM);
    xmpp_stanza_set_attribute(item, STANZA_ATTR_JID, jid);

    if (handle) {
        xmpp_stanza_set_attribute(item, STANZA_ATTR_NAME, handle);
    } else {
        xmpp_stanza_set_attribute(item, STANZA_ATTR_NAME, "");
    }

    while (groups) {
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

xmpp_stanza_t*
stanza_create_invite(xmpp_ctx_t *ctx, const char *const room,
    const char *const contact, const char *const reason, const char *const password)
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
    if (reason) {
        xmpp_stanza_set_attribute(x, STANZA_ATTR_REASON, reason);
    }
    if (password) {
        xmpp_stanza_set_attribute(x, STANZA_ATTR_PASSWORD, password);
    }

    xmpp_stanza_add_child(message, x);
    xmpp_stanza_release(x);

    return message;
}

xmpp_stanza_t*
stanza_create_mediated_invite(xmpp_ctx_t *ctx, const char *const room,
    const char *const contact, const char *const reason)
{
    xmpp_stanza_t *message, *x, *invite;

    message = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(message, STANZA_NAME_MESSAGE);
    xmpp_stanza_set_attribute(message, STANZA_ATTR_TO, room);
    char *id = create_unique_id(NULL);
    xmpp_stanza_set_id(message, id);
    free(id);

    x = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(x, STANZA_NAME_X);
    xmpp_stanza_set_ns(x, STANZA_NS_MUC_USER);

    invite = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(invite, STANZA_NAME_INVITE);
    xmpp_stanza_set_attribute(invite, STANZA_ATTR_TO, contact);

    if (reason) {
        xmpp_stanza_t *reason_st = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(reason_st, STANZA_NAME_REASON);
        xmpp_stanza_t *reason_txt = xmpp_stanza_new(ctx);
        xmpp_stanza_set_text(reason_txt, reason);
        xmpp_stanza_add_child(reason_st, reason_txt);
        xmpp_stanza_release(reason_txt);
        xmpp_stanza_add_child(invite, reason_st);
        xmpp_stanza_release(reason_st);
    }

    xmpp_stanza_add_child(x, invite);
    xmpp_stanza_release(invite);
    xmpp_stanza_add_child(message, x);
    xmpp_stanza_release(x);

    return message;
}

xmpp_stanza_t*
stanza_create_room_join_presence(xmpp_ctx_t *const ctx,
    const char *const full_room_jid, const char *const passwd)
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
    if (passwd) {
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

xmpp_stanza_t*
stanza_create_room_newnick_presence(xmpp_ctx_t *ctx,
    const char *const full_room_jid)
{
    xmpp_stanza_t *presence = xmpp_stanza_new(ctx);
    char *id = create_unique_id("sub");
    xmpp_stanza_set_id(presence, id);
    free(id);
    xmpp_stanza_set_name(presence, STANZA_NAME_PRESENCE);
    xmpp_stanza_set_attribute(presence, STANZA_ATTR_TO, full_room_jid);

    return presence;
}

xmpp_stanza_t*
stanza_create_room_leave_presence(xmpp_ctx_t *ctx, const char *const room,
    const char *const nick)
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

xmpp_stanza_t*
stanza_create_instant_room_request_iq(xmpp_ctx_t *ctx, const char *const room_jid)
{
    xmpp_stanza_t *iq = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(iq, STANZA_NAME_IQ);
    xmpp_stanza_set_type(iq, STANZA_TYPE_SET);
    xmpp_stanza_set_attribute(iq, STANZA_ATTR_TO, room_jid);
    char *id = create_unique_id("room");
    xmpp_stanza_set_id(iq, id);
    free(id);

    xmpp_stanza_t *query = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(query, STANZA_NAME_QUERY);
    xmpp_stanza_set_ns(query, STANZA_NS_MUC_OWNER);

    xmpp_stanza_t *x = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(x, STANZA_NAME_X);
    xmpp_stanza_set_type(x, "submit");
    xmpp_stanza_set_ns(x, STANZA_NS_DATA);

    xmpp_stanza_add_child(query, x);
    xmpp_stanza_release(x);

    xmpp_stanza_add_child(iq, query);
    xmpp_stanza_release(query);

    return iq;
}

xmpp_stanza_t*
stanza_create_instant_room_destroy_iq(xmpp_ctx_t *ctx, const char *const room_jid)
{
    xmpp_stanza_t *iq = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(iq, STANZA_NAME_IQ);
    xmpp_stanza_set_type(iq, STANZA_TYPE_SET);
    xmpp_stanza_set_attribute(iq, STANZA_ATTR_TO, room_jid);
    char *id = create_unique_id("room");
    xmpp_stanza_set_id(iq, id);
    free(id);

    xmpp_stanza_t *query = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(query, STANZA_NAME_QUERY);
    xmpp_stanza_set_ns(query, STANZA_NS_MUC_OWNER);

    xmpp_stanza_t *destroy = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(destroy, STANZA_NAME_DESTROY);

    xmpp_stanza_add_child(query, destroy);
    xmpp_stanza_release(destroy);

    xmpp_stanza_add_child(iq, query);
    xmpp_stanza_release(query);

    return iq;
}

xmpp_stanza_t*
stanza_create_room_config_request_iq(xmpp_ctx_t *ctx, const char *const room_jid)
{
    xmpp_stanza_t *iq = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(iq, STANZA_NAME_IQ);
    xmpp_stanza_set_type(iq, STANZA_TYPE_GET);
    xmpp_stanza_set_attribute(iq, STANZA_ATTR_TO, room_jid);
    char *id = create_unique_id("room");
    xmpp_stanza_set_id(iq, id);
    free(id);

    xmpp_stanza_t *query = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(query, STANZA_NAME_QUERY);
    xmpp_stanza_set_ns(query, STANZA_NS_MUC_OWNER);

    xmpp_stanza_add_child(iq, query);
    xmpp_stanza_release(query);

    return iq;
}

xmpp_stanza_t*
stanza_create_room_config_cancel_iq(xmpp_ctx_t *ctx, const char *const room_jid)
{
    xmpp_stanza_t *iq = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(iq, STANZA_NAME_IQ);
    xmpp_stanza_set_type(iq, STANZA_TYPE_SET);
    xmpp_stanza_set_attribute(iq, STANZA_ATTR_TO, room_jid);
    char *id = create_unique_id("room");
    xmpp_stanza_set_id(iq, id);
    free(id);

    xmpp_stanza_t *query = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(query, STANZA_NAME_QUERY);
    xmpp_stanza_set_ns(query, STANZA_NS_MUC_OWNER);

    xmpp_stanza_t *x = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(x, STANZA_NAME_X);
    xmpp_stanza_set_type(x, "cancel");
    xmpp_stanza_set_ns(x, STANZA_NS_DATA);

    xmpp_stanza_add_child(query, x);
    xmpp_stanza_release(x);

    xmpp_stanza_add_child(iq, query);
    xmpp_stanza_release(query);

    return iq;
}

xmpp_stanza_t*
stanza_create_room_affiliation_list_iq(xmpp_ctx_t *ctx, const char *const room, const char *const affiliation)
{
    xmpp_stanza_t *iq = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(iq, STANZA_NAME_IQ);
    xmpp_stanza_set_type(iq, STANZA_TYPE_GET);
    xmpp_stanza_set_attribute(iq, STANZA_ATTR_TO, room);
    char *id = create_unique_id("affiliation_get");
    xmpp_stanza_set_id(iq, id);
    free(id);

    xmpp_stanza_t *query = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(query, STANZA_NAME_QUERY);
    xmpp_stanza_set_ns(query, STANZA_NS_MUC_ADMIN);

    xmpp_stanza_t *item = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(item, STANZA_NAME_ITEM);
    xmpp_stanza_set_attribute(item, "affiliation", affiliation);

    xmpp_stanza_add_child(query, item);
    xmpp_stanza_release(item);
    xmpp_stanza_add_child(iq, query);
    xmpp_stanza_release(query);

    return iq;
}

xmpp_stanza_t*
stanza_create_room_role_list_iq(xmpp_ctx_t *ctx, const char *const room, const char *const role)
{
    xmpp_stanza_t *iq = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(iq, STANZA_NAME_IQ);
    xmpp_stanza_set_type(iq, STANZA_TYPE_GET);
    xmpp_stanza_set_attribute(iq, STANZA_ATTR_TO, room);
    char *id = create_unique_id("role_get");
    xmpp_stanza_set_id(iq, id);
    free(id);

    xmpp_stanza_t *query = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(query, STANZA_NAME_QUERY);
    xmpp_stanza_set_ns(query, STANZA_NS_MUC_ADMIN);

    xmpp_stanza_t *item = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(item, STANZA_NAME_ITEM);
    xmpp_stanza_set_attribute(item, "role", role);

    xmpp_stanza_add_child(query, item);
    xmpp_stanza_release(item);
    xmpp_stanza_add_child(iq, query);
    xmpp_stanza_release(query);

    return iq;
}

xmpp_stanza_t*
stanza_create_room_affiliation_set_iq(xmpp_ctx_t *ctx, const char *const room, const char *const jid,
    const char *const affiliation, const char *const reason)
{
    xmpp_stanza_t *iq = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(iq, STANZA_NAME_IQ);
    xmpp_stanza_set_type(iq, STANZA_TYPE_SET);
    xmpp_stanza_set_attribute(iq, STANZA_ATTR_TO, room);
    char *id = create_unique_id("affiliation_set");
    xmpp_stanza_set_id(iq, id);
    free(id);

    xmpp_stanza_t *query = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(query, STANZA_NAME_QUERY);
    xmpp_stanza_set_ns(query, STANZA_NS_MUC_ADMIN);

    xmpp_stanza_t *item = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(item, STANZA_NAME_ITEM);
    xmpp_stanza_set_attribute(item, "affiliation", affiliation);
    xmpp_stanza_set_attribute(item, STANZA_ATTR_JID, jid);

    if (reason) {
        xmpp_stanza_t *reason_st = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(reason_st, STANZA_NAME_REASON);
        xmpp_stanza_t *reason_text = xmpp_stanza_new(ctx);
        xmpp_stanza_set_text(reason_text, reason);
        xmpp_stanza_add_child(reason_st, reason_text);
        xmpp_stanza_release(reason_text);

        xmpp_stanza_add_child(item, reason_st);
        xmpp_stanza_release(reason_st);
    }

    xmpp_stanza_add_child(query, item);
    xmpp_stanza_release(item);
    xmpp_stanza_add_child(iq, query);
    xmpp_stanza_release(query);

    return iq;
}

xmpp_stanza_t*
stanza_create_room_role_set_iq(xmpp_ctx_t *const ctx, const char *const room, const char *const nick,
    const char *const role, const char *const reason)
{
    xmpp_stanza_t *iq = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(iq, STANZA_NAME_IQ);
    xmpp_stanza_set_type(iq, STANZA_TYPE_SET);
    xmpp_stanza_set_attribute(iq, STANZA_ATTR_TO, room);
    char *id = create_unique_id("role_set");
    xmpp_stanza_set_id(iq, id);
    free(id);

    xmpp_stanza_t *query = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(query, STANZA_NAME_QUERY);
    xmpp_stanza_set_ns(query, STANZA_NS_MUC_ADMIN);

    xmpp_stanza_t *item = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(item, STANZA_NAME_ITEM);
    xmpp_stanza_set_attribute(item, "role", role);
    xmpp_stanza_set_attribute(item, STANZA_ATTR_NICK, nick);

    if (reason) {
        xmpp_stanza_t *reason_st = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(reason_st, STANZA_NAME_REASON);
        xmpp_stanza_t *reason_text = xmpp_stanza_new(ctx);
        xmpp_stanza_set_text(reason_text, reason);
        xmpp_stanza_add_child(reason_st, reason_text);
        xmpp_stanza_release(reason_text);

        xmpp_stanza_add_child(item, reason_st);
        xmpp_stanza_release(reason_st);
    }

    xmpp_stanza_add_child(query, item);
    xmpp_stanza_release(item);
    xmpp_stanza_add_child(iq, query);
    xmpp_stanza_release(query);

    return iq;
}

xmpp_stanza_t*
stanza_create_room_kick_iq(xmpp_ctx_t *const ctx, const char *const room, const char *const nick,
    const char *const reason)
{
    xmpp_stanza_t *iq = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(iq, STANZA_NAME_IQ);
    xmpp_stanza_set_type(iq, STANZA_TYPE_SET);
    xmpp_stanza_set_attribute(iq, STANZA_ATTR_TO, room);
    char *id = create_unique_id("room_kick");
    xmpp_stanza_set_id(iq, id);
    free(id);

    xmpp_stanza_t *query = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(query, STANZA_NAME_QUERY);
    xmpp_stanza_set_ns(query, STANZA_NS_MUC_ADMIN);

    xmpp_stanza_t *item = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(item, STANZA_NAME_ITEM);
    xmpp_stanza_set_attribute(item, STANZA_ATTR_NICK, nick);
    xmpp_stanza_set_attribute(item, "role", "none");

    if (reason) {
        xmpp_stanza_t *reason_st = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(reason_st, STANZA_NAME_REASON);
        xmpp_stanza_t *reason_text = xmpp_stanza_new(ctx);
        xmpp_stanza_set_text(reason_text, reason);
        xmpp_stanza_add_child(reason_st, reason_text);
        xmpp_stanza_release(reason_text);

        xmpp_stanza_add_child(item, reason_st);
        xmpp_stanza_release(reason_st);
    }

    xmpp_stanza_add_child(query, item);
    xmpp_stanza_release(item);
    xmpp_stanza_add_child(iq, query);
    xmpp_stanza_release(query);

    return iq;
}

xmpp_stanza_t*
stanza_create_presence(xmpp_ctx_t *const ctx)
{
    xmpp_stanza_t *presence = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(presence, STANZA_NAME_PRESENCE);

    return presence;
}

xmpp_stanza_t*
stanza_create_software_version_iq(xmpp_ctx_t *ctx, const char *const fulljid)
{
    xmpp_stanza_t *iq = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(iq, STANZA_NAME_IQ);
    xmpp_stanza_set_type(iq, STANZA_TYPE_GET);

    char *id = create_unique_id("sv");
    xmpp_stanza_set_id(iq, id);
    free(id);

    xmpp_stanza_set_attribute(iq, "to", fulljid);

    xmpp_stanza_t *query = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(query, STANZA_NAME_QUERY);
    xmpp_stanza_set_ns(query, STANZA_NS_VERSION);

    xmpp_stanza_add_child(iq, query);
    xmpp_stanza_release(query);

    return iq;
}

xmpp_stanza_t*
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

xmpp_stanza_t*
stanza_create_disco_info_iq(xmpp_ctx_t *ctx, const char *const id, const char *const to,
    const char *const node)
{
    xmpp_stanza_t *iq = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(iq, STANZA_NAME_IQ);
    xmpp_stanza_set_type(iq, STANZA_TYPE_GET);
    xmpp_stanza_set_attribute(iq, STANZA_ATTR_TO, to);
    xmpp_stanza_set_id(iq, id);

    xmpp_stanza_t *query = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(query, STANZA_NAME_QUERY);
    xmpp_stanza_set_ns(query, XMPP_NS_DISCO_INFO);
    if (node) {
        xmpp_stanza_set_attribute(query, STANZA_ATTR_NODE, node);
    }

    xmpp_stanza_add_child(iq, query);
    xmpp_stanza_release(query);

    return iq;
}

xmpp_stanza_t*
stanza_create_disco_items_iq(xmpp_ctx_t *ctx, const char *const id,
    const char *const jid)
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

xmpp_stanza_t*
stanza_create_last_activity_iq(xmpp_ctx_t *ctx, const char *const id, const char *const to)
{
    xmpp_stanza_t *iq = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(iq, STANZA_NAME_IQ);
    xmpp_stanza_set_type(iq, STANZA_TYPE_GET);
    xmpp_stanza_set_attribute(iq, STANZA_ATTR_TO, to);
    xmpp_stanza_set_id(iq, id);

    xmpp_stanza_t *query = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(query, STANZA_NAME_QUERY);
    xmpp_stanza_set_ns(query, STANZA_NS_LASTACTIVITY);

    xmpp_stanza_add_child(iq, query);
    xmpp_stanza_release(query);

    return iq;
}

xmpp_stanza_t*
stanza_create_room_config_submit_iq(xmpp_ctx_t *ctx, const char *const room, DataForm *form)
{
    xmpp_stanza_t *iq = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(iq, STANZA_NAME_IQ);
    xmpp_stanza_set_type(iq, STANZA_TYPE_SET);
    xmpp_stanza_set_attribute(iq, STANZA_ATTR_TO, room);
    char *id = create_unique_id("roomconf_submit");
    xmpp_stanza_set_id(iq, id);
    free(id);

    xmpp_stanza_t *query = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(query, STANZA_NAME_QUERY);
    xmpp_stanza_set_ns(query, STANZA_NS_MUC_OWNER);

    xmpp_stanza_t *x = form_create_submission(form);
    xmpp_stanza_add_child(query, x);
    xmpp_stanza_release(x);

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

xmpp_stanza_t*
stanza_create_ping_iq(xmpp_ctx_t *ctx, const char *const target)
{
    xmpp_stanza_t *iq = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(iq, STANZA_NAME_IQ);
    xmpp_stanza_set_type(iq, STANZA_TYPE_GET);
    if (target) {
        xmpp_stanza_set_attribute(iq, STANZA_ATTR_TO, target);
    }
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

GDateTime*
stanza_get_delay(xmpp_stanza_t *const stanza)
{
    GTimeVal utc_stamp;
    // first check for XEP-0203 delayed delivery
    xmpp_stanza_t *delay = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_DELAY);
    if (delay) {
        char *xmlns = xmpp_stanza_get_attribute(delay, STANZA_ATTR_XMLNS);
        if (xmlns && (strcmp(xmlns, "urn:xmpp:delay") == 0)) {
            char *stamp = xmpp_stanza_get_attribute(delay, STANZA_ATTR_STAMP);
            if (stamp && (g_time_val_from_iso8601(stamp, &utc_stamp))) {
                GDateTime *utc_datetime = g_date_time_new_from_timeval_utc(&utc_stamp);
                GDateTime *local_datetime = g_date_time_to_local(utc_datetime);
                g_date_time_unref(utc_datetime);
                return local_datetime;
            }
        }
    }

    // otherwise check for XEP-0091 legacy delayed delivery
    // stanp format : CCYYMMDDThh:mm:ss
    xmpp_stanza_t *x = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_X);
    if (x) {
        char *xmlns = xmpp_stanza_get_attribute(x, STANZA_ATTR_XMLNS);
        if (xmlns && (strcmp(xmlns, "jabber:x:delay") == 0)) {
            char *stamp = xmpp_stanza_get_attribute(x, STANZA_ATTR_STAMP);
            if (stamp && (g_time_val_from_iso8601(stamp, &utc_stamp))) {
                GDateTime *utc_datetime = g_date_time_new_from_timeval_utc(&utc_stamp);
                GDateTime *local_datetime = g_date_time_to_local(utc_datetime);
                g_date_time_unref(utc_datetime);
                return local_datetime;
            }
        }
    }

    return NULL;
}

char*
stanza_get_status(xmpp_stanza_t *stanza, char *def)
{
    xmpp_ctx_t *ctx = connection_get_ctx();
    xmpp_stanza_t *status =
        xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_STATUS);

    if (status) {
        // xmpp_free and free may be different functions so convert all to
        // libc malloc
        char *s1, *s2 = NULL;
        s1 = xmpp_stanza_get_text(status);
        if (s1) {
            s2 = strdup(s1);
            xmpp_free(ctx, s1);
        }
        return s2;
    } else if (def) {
        return strdup(def);
    } else {
        return NULL;
    }
}

char*
stanza_get_show(xmpp_stanza_t *stanza, char *def)
{
    xmpp_ctx_t *ctx = connection_get_ctx();
    xmpp_stanza_t *show =
        xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_SHOW);

    if (show) {
        // xmpp_free and free may be different functions so convert all to
        // libc malloc
        char *s1, *s2 = NULL;
        s1 = xmpp_stanza_get_text(show);
        if (s1) {
            s2 = strdup(s1);
            xmpp_free(ctx, s1);
        }
        return s2;
    } else if (def) {
        return strdup(def);
    } else {
        return NULL;
    }
}

gboolean
stanza_is_muc_presence(xmpp_stanza_t *const stanza)
{
    if (stanza == NULL) {
        return FALSE;
    }
    if (strcmp(xmpp_stanza_get_name(stanza), STANZA_NAME_PRESENCE) != 0) {
        return FALSE;
    }

    xmpp_stanza_t *x = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_MUC_USER);

    if (x) {
        return TRUE;
    } else {
        return FALSE;
    }
}

gboolean
stanza_muc_requires_config(xmpp_stanza_t *const stanza)
{
    // no stanza, or not presence stanza
    if ((stanza == NULL) || (g_strcmp0(xmpp_stanza_get_name(stanza), STANZA_NAME_PRESENCE) != 0)) {
        return FALSE;
    }

    // muc user namespaced x element
    xmpp_stanza_t *x = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_MUC_USER);
    if (x) {

        // check for item element with owner affiliation
        xmpp_stanza_t *item = xmpp_stanza_get_child_by_name(x, "item");
        if (item == NULL) {
            return FALSE;
        }
        char *affiliation = xmpp_stanza_get_attribute(item, "affiliation");
        if (g_strcmp0(affiliation, "owner") != 0) {
            return FALSE;
        }

        // check for status code 201
        xmpp_stanza_t *x_children = xmpp_stanza_get_children(x);
        while (x_children) {
            if (g_strcmp0(xmpp_stanza_get_name(x_children), STANZA_NAME_STATUS) == 0) {
                char *code = xmpp_stanza_get_attribute(x_children, STANZA_ATTR_CODE);
                if (g_strcmp0(code, "201") == 0) {
                    return TRUE;
                }
            }
            x_children = xmpp_stanza_get_next(x_children);
        }
    }
    return FALSE;
}

gboolean
stanza_is_muc_self_presence(xmpp_stanza_t *const stanza,
    const char *const self_jid)
{
    // no stanza, or not presence stanza
    if ((stanza == NULL) || (g_strcmp0(xmpp_stanza_get_name(stanza), STANZA_NAME_PRESENCE) != 0)) {
        return FALSE;
    }

    // muc user namespaced x element
    xmpp_stanza_t *x = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_MUC_USER);
    if (x) {

        // check for status child element with 110 code
        xmpp_stanza_t *x_children = xmpp_stanza_get_children(x);
        while (x_children) {
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
        if (item) {
            char *jid = xmpp_stanza_get_attribute(item, STANZA_ATTR_JID);
            if (jid) {
                if (g_str_has_prefix(self_jid, jid)) {
                    return TRUE;
                }
            }
        }

        // check if 'from' attribute identifies this user
        char *from = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_FROM);
        if (from) {
            Jid *from_jid = jid_create(from);
            if (muc_active(from_jid->barejid)) {
                char *nick = muc_nick(from_jid->barejid);
                if (g_strcmp0(from_jid->resourcepart, nick) == 0) {
                    jid_destroy(from_jid);
                    return TRUE;
                }
            }

            // check if a new nickname maps to a pending nick change for this user
            if (muc_nick_change_pending(from_jid->barejid)) {
                char *new_nick = from_jid->resourcepart;
                if (new_nick) {
                    char *nick = muc_nick(from_jid->barejid);
                    char *old_nick = muc_old_nick(from_jid->barejid, new_nick);
                    if (g_strcmp0(old_nick, nick) == 0) {
                        jid_destroy(from_jid);
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

GSList*
stanza_get_status_codes_by_ns(xmpp_stanza_t *const stanza, char *ns)
{
    GSList *codes = NULL;
    xmpp_stanza_t *ns_child = xmpp_stanza_get_child_by_ns(stanza, ns);
    if (ns_child) {
        xmpp_stanza_t *child = xmpp_stanza_get_children(ns_child);
        while (child) {
            char *name = xmpp_stanza_get_name(child);
            if (g_strcmp0(name, STANZA_NAME_STATUS) == 0) {
                char *code = xmpp_stanza_get_attribute(child, STANZA_ATTR_CODE);
                if (code) {
                    codes = g_slist_append(codes, strdup(code));
                }
            }
            child = xmpp_stanza_get_next(child);
        }
    }
    return codes;
}

gboolean
stanza_room_destroyed(xmpp_stanza_t *stanza)
{
    char *stanza_name = xmpp_stanza_get_name(stanza);
    if (g_strcmp0(stanza_name, STANZA_NAME_PRESENCE) == 0) {
        xmpp_stanza_t *x = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_MUC_USER);
        if (x) {
            xmpp_stanza_t *destroy = xmpp_stanza_get_child_by_name(x, STANZA_NAME_DESTROY);
            if (destroy) {
                return TRUE;
            }
        }
    }

    return FALSE;
}

char*
stanza_get_muc_destroy_alternative_room(xmpp_stanza_t *stanza)
{
    char *stanza_name = xmpp_stanza_get_name(stanza);
    if (g_strcmp0(stanza_name, STANZA_NAME_PRESENCE) == 0) {
        xmpp_stanza_t *x = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_MUC_USER);
        if (x) {
            xmpp_stanza_t *destroy = xmpp_stanza_get_child_by_name(x, STANZA_NAME_DESTROY);
            if (destroy) {
                char *jid = xmpp_stanza_get_attribute(destroy, STANZA_ATTR_JID);
                if (jid) {
                    return jid;
                }
            }
        }
    }

    return NULL;
}

char*
stanza_get_muc_destroy_alternative_password(xmpp_stanza_t *stanza)
{
    char *stanza_name = xmpp_stanza_get_name(stanza);
    if (g_strcmp0(stanza_name, STANZA_NAME_PRESENCE) == 0) {
        xmpp_stanza_t *x = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_MUC_USER);
        if (x) {
            xmpp_stanza_t *destroy = xmpp_stanza_get_child_by_name(x, STANZA_NAME_DESTROY);
            if (destroy) {
                xmpp_stanza_t *password_st = xmpp_stanza_get_child_by_name(destroy, STANZA_NAME_PASSWORD);
                if (password_st) {
                    char *password = xmpp_stanza_get_text(password_st);
                    if (password) {
                        return password;
                    }
                }
            }
        }
    }
    return NULL;
}

char*
stanza_get_muc_destroy_reason(xmpp_stanza_t *stanza)
{
    char *stanza_name = xmpp_stanza_get_name(stanza);
    if (g_strcmp0(stanza_name, STANZA_NAME_PRESENCE) == 0) {
        xmpp_stanza_t *x = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_MUC_USER);
        if (x) {
            xmpp_stanza_t *destroy = xmpp_stanza_get_child_by_name(x, STANZA_NAME_DESTROY);
            if (destroy) {
                xmpp_stanza_t *reason_st = xmpp_stanza_get_child_by_name(destroy, STANZA_NAME_REASON);
                if (reason_st) {
                    char *reason = xmpp_stanza_get_text(reason_st);
                    if (reason) {
                        return reason;
                    }
                }
            }
        }
    }
    return NULL;
}

char*
stanza_get_actor(xmpp_stanza_t *stanza)
{
    char *stanza_name = xmpp_stanza_get_name(stanza);
    if (g_strcmp0(stanza_name, STANZA_NAME_PRESENCE) == 0) {
        xmpp_stanza_t *x = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_MUC_USER);
        if (x) {
            xmpp_stanza_t *item = xmpp_stanza_get_child_by_name(x, STANZA_NAME_ITEM);
            if (item) {
                xmpp_stanza_t *actor = xmpp_stanza_get_child_by_name(item, STANZA_NAME_ACTOR);
                if (actor) {
                    char *nick = xmpp_stanza_get_attribute(actor, STANZA_ATTR_NICK);
                    if (nick) {
                        return nick;
                    }
                    char *jid = xmpp_stanza_get_attribute(actor, STANZA_ATTR_JID);
                    if (jid) {
                        return jid;
                    }
                }
            }
        }
    }
    return NULL;
}

char*
stanza_get_reason(xmpp_stanza_t *stanza)
{
    char *stanza_name = xmpp_stanza_get_name(stanza);
    if (g_strcmp0(stanza_name, STANZA_NAME_PRESENCE) == 0) {
        xmpp_stanza_t *x = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_MUC_USER);
        if (x) {
            xmpp_stanza_t *item = xmpp_stanza_get_child_by_name(x, STANZA_NAME_ITEM);
            if (item) {
                xmpp_stanza_t *reason_st = xmpp_stanza_get_child_by_name(item, STANZA_NAME_REASON);
                if (reason_st) {
                    char *reason = xmpp_stanza_get_text(reason_st);
                    if (reason) {
                        return reason;
                    }
                }
            }
        }
    }
    return NULL;
}

gboolean
stanza_is_room_nick_change(xmpp_stanza_t *const stanza)
{
    // no stanza, or not presence stanza
    if ((stanza == NULL) || (g_strcmp0(xmpp_stanza_get_name(stanza), STANZA_NAME_PRESENCE) != 0)) {
        return FALSE;
    }

    // muc user namespaced x element
    xmpp_stanza_t *x = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_MUC_USER);
    if (x) {

        // check for status child element with 303 code
        xmpp_stanza_t *x_children = xmpp_stanza_get_children(x);
        while (x_children) {
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

char*
stanza_get_new_nick(xmpp_stanza_t *const stanza)
{
    if (!stanza_is_room_nick_change(stanza)) {
        return NULL;
    }

    xmpp_stanza_t *x = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_X);
    xmpp_stanza_t *x_children = xmpp_stanza_get_children(x);

    while (x_children) {
        if (strcmp(xmpp_stanza_get_name(x_children), STANZA_NAME_ITEM) == 0) {
            char *nick = xmpp_stanza_get_attribute(x_children, STANZA_ATTR_NICK);
            if (nick) {
                return nick;
            }
        }
        x_children = xmpp_stanza_get_next(x_children);
    }

    return NULL;
}

int
stanza_get_idle_time(xmpp_stanza_t *const stanza)
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

XMPPCaps*
stanza_parse_caps(xmpp_stanza_t *const stanza)
{
    xmpp_stanza_t *caps_st = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_C);

    if (!caps_st) {
        return NULL;
    }

    char *ns = xmpp_stanza_get_ns(caps_st);
    if (g_strcmp0(ns, STANZA_NS_CAPS) != 0) {
        return NULL;
    }

    char *hash = xmpp_stanza_get_attribute(caps_st, STANZA_ATTR_HASH);
    char *node = xmpp_stanza_get_attribute(caps_st, STANZA_ATTR_NODE);
    char *ver = xmpp_stanza_get_attribute(caps_st, STANZA_ATTR_VER);

    XMPPCaps *caps = (XMPPCaps *)malloc(sizeof(XMPPCaps));
    caps->hash = hash ? strdup(hash) : NULL;
    caps->node = node ? strdup(node) : NULL;
    caps->ver = ver ? strdup(ver) : NULL;

    return caps;
}

char*
stanza_get_caps_str(xmpp_stanza_t *const stanza)
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

char*
stanza_get_error_message(xmpp_stanza_t *stanza)
{
    xmpp_ctx_t *ctx = connection_get_ctx();
    xmpp_stanza_t *error_stanza = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_ERROR);

    // return nothing if no error stanza
    if (error_stanza == NULL) {
        return strdup("unknown");
    } else {

        // check for text child
        xmpp_stanza_t *text_stanza = xmpp_stanza_get_child_by_name(error_stanza, STANZA_NAME_TEXT);

        // check for text
        if (text_stanza) {
            gchar *err_msg = xmpp_stanza_get_text(text_stanza);
            if (err_msg) {
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
                if (cond_stanza) {
                    char *result = strdup(xmpp_stanza_get_name(cond_stanza));
                    return result;
                }
            }

        }
    }

    // if undefined-condition or no condition, return nothing
    return strdup("unknown");
}

void
stanza_attach_priority(xmpp_ctx_t *const ctx, xmpp_stanza_t *const presence,
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
stanza_attach_show(xmpp_ctx_t *const ctx, xmpp_stanza_t *const presence,
    const char *const show)
{
    if (show) {
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
stanza_attach_status(xmpp_ctx_t *const ctx, xmpp_stanza_t *const presence,
    const char *const status)
{
    if (status) {
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
stanza_attach_last_activity(xmpp_ctx_t *const ctx,
    xmpp_stanza_t *const presence, const int idle)
{
    xmpp_stanza_t *query = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(query, STANZA_NAME_QUERY);
    xmpp_stanza_set_ns(query, STANZA_NS_LASTACTIVITY);
    char idle_str[10];
    snprintf(idle_str, sizeof(idle_str), "%d", idle);
    xmpp_stanza_set_attribute(query, STANZA_ATTR_SECONDS, idle_str);
    xmpp_stanza_add_child(presence, query);
    xmpp_stanza_release(query);
}

void
stanza_attach_caps(xmpp_ctx_t *const ctx, xmpp_stanza_t *const presence)
{
    xmpp_stanza_t *caps = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(caps, STANZA_NAME_C);
    xmpp_stanza_set_ns(caps, STANZA_NS_CAPS);
    xmpp_stanza_t *query = caps_create_query_response_stanza(ctx);

    char *sha1 = caps_get_my_sha1(ctx);
    xmpp_stanza_set_attribute(caps, STANZA_ATTR_HASH, "sha-1");
    xmpp_stanza_set_attribute(caps, STANZA_ATTR_NODE, "http://www.profanity.im");
    xmpp_stanza_set_attribute(caps, STANZA_ATTR_VER, sha1);
    xmpp_stanza_add_child(presence, caps);
    xmpp_stanza_release(caps);
    xmpp_stanza_release(query);
}

const char*
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

Resource*
stanza_resource_from_presence(XMPPPresence *presence)
{
    // create Resource
    Resource *resource = NULL;
    resource_presence_t resource_presence = resource_presence_from_string(presence->show);
    if (presence->jid->resourcepart == NULL) { // hack for servers that do not send full jid
        resource = resource_new("__prof_default", resource_presence, presence->status, presence->priority);
    } else {
        resource = resource_new(presence->jid->resourcepart, resource_presence, presence->status, presence->priority);
    }

    return resource;
}

void
stanza_free_caps(XMPPCaps *caps)
{
    if (caps) {
        if (caps->hash) {
            free(caps->hash);
        }
        if (caps->node) {
            free(caps->node);
        }
        if (caps->ver) {
            free(caps->ver);
        }
        FREE_SET_NULL(caps);
    }
}

void
stanza_free_presence(XMPPPresence *presence)
{
    if (presence) {
        if (presence->jid) {
            jid_destroy(presence->jid);
        }
        if (presence->last_activity) {
            g_date_time_unref(presence->last_activity);
        }
        if (presence->show) {
            free(presence->show);
        }
        if (presence->status) {
            free(presence->status);
        }
        FREE_SET_NULL(presence);
    }
}

XMPPPresence*
stanza_parse_presence(xmpp_stanza_t *stanza, int *err)
{
    char *from = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_FROM);
    if (!from) {
        *err = STANZA_PARSE_ERROR_NO_FROM;
        return NULL;
    }

    Jid *from_jid = jid_create(from);
    if (!from_jid) {
        *err = STANZA_PARSE_ERROR_INVALID_FROM;
        return NULL;
    }

    XMPPPresence *result = (XMPPPresence *)malloc(sizeof(XMPPPresence));
    result->jid = from_jid;

    result->show = stanza_get_show(stanza, "online");
    result->status = stanza_get_status(stanza, NULL);

    int idle_seconds = stanza_get_idle_time(stanza);
    if (idle_seconds > 0) {
        GDateTime *now = g_date_time_new_now_local();
        result->last_activity = g_date_time_add_seconds(now, 0 - idle_seconds);
        g_date_time_unref(now);
    } else {
        result->last_activity = NULL;
    }

    result->priority = 0;
    xmpp_stanza_t *priority_stanza = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_PRIORITY);
    if (priority_stanza) {
        char *priority_str = xmpp_stanza_get_text(priority_stanza);
        if (priority_str) {
            result->priority = atoi(priority_str);
        }
        free(priority_str);
    }

    return result;
}
