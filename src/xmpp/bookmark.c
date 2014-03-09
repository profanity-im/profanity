/*
 * bookmark.c
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <strophe.h>

#include "common.h"
#include "log.h"
#include "muc.h"
#include "ui/ui.h"
#include "xmpp/connection.h"
#include "xmpp/stanza.h"
#include "xmpp/xmpp.h"
#include "xmpp/bookmark.h"

#define BOOKMARK_TIMEOUT 5000
/* TODO: replace with a preference */
#define BOOKMARK_AUTOJOIN_MAX 5

static int autojoin_count;

static Autocomplete bookmark_ac;
static GList *bookmark_list;

static int _bookmark_handle_result(xmpp_conn_t * const conn,
    xmpp_stanza_t * const stanza, void * const userdata);
static int _bookmark_handle_delete(xmpp_conn_t * const conn,
    void * const userdata);
static void _bookmark_item_destroy(gpointer item);

void
bookmark_request(void)
{
    char *id;
    xmpp_conn_t *conn = connection_get_conn();
    xmpp_ctx_t *ctx = connection_get_ctx();
    xmpp_stanza_t *iq;

    id = strdup("bookmark_init_request");

    autojoin_count = 0;
    if (bookmark_ac != NULL) {
        autocomplete_free(bookmark_ac);
    }
    bookmark_ac = autocomplete_new();
    if (bookmark_list != NULL) {
        g_list_free_full(bookmark_list, _bookmark_item_destroy);
        bookmark_list = NULL;
    }

    xmpp_timed_handler_add(conn, _bookmark_handle_delete, BOOKMARK_TIMEOUT, id);
    xmpp_id_handler_add(conn, _bookmark_handle_result, id, id);

    iq = stanza_create_storage_bookmarks(ctx);
    xmpp_stanza_set_id(iq, id);
    xmpp_send(conn, iq);
    xmpp_stanza_release(iq);
}

static gboolean
_bookmark_add(const char *jid, const char *nick, gboolean autojoin)
{
    gboolean added = TRUE;
    if (autocomplete_contains(bookmark_ac, jid)) {
        added = FALSE;
    }

    xmpp_conn_t *conn = connection_get_conn();
    xmpp_ctx_t *ctx = connection_get_ctx();

    /* TODO: send request */
    xmpp_stanza_t *stanza = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(stanza, STANZA_NAME_IQ);
    char *id = generate_unique_id("bookmark_add");
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

    xmpp_stanza_t *storage = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(storage, STANZA_NAME_STORAGE);
    xmpp_stanza_set_ns(storage, "storage;bookmarks");
    xmpp_stanza_add_child(item, storage);

    xmpp_stanza_t *conference = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(conference, STANZA_NAME_CONFERENCE);
    xmpp_stanza_set_attribute(conference, STANZA_ATTR_JID, jid);

    if (autojoin) {
        xmpp_stanza_set_attribute(conference, STANZA_ATTR_AUTOJOIN, "true");
    } else {
        xmpp_stanza_set_attribute(conference, STANZA_ATTR_AUTOJOIN, "false");
    }

    xmpp_stanza_add_child(storage, conference);

    if (nick != NULL) {
        xmpp_stanza_t *nick_st = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(nick_st, STANZA_NAME_NICK);
        xmpp_stanza_set_text(nick_st, nick);
        xmpp_stanza_add_child(conference, nick_st);
    }

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

    xmpp_send(conn, stanza);
    xmpp_stanza_release(stanza);

    /* TODO: manage bookmark_list */

    /* this may be command for modifying */
    autocomplete_remove(bookmark_ac, jid);
    autocomplete_add(bookmark_ac, jid);

    return added;
}

static gboolean
_bookmark_remove(const char *jid, gboolean autojoin)
{
    gboolean removed = FALSE;
    if (autocomplete_contains(bookmark_ac, jid)) {
        removed = TRUE;
    }
    /* TODO: manage bookmark_list */
    if (autojoin) {
        /* TODO: just set autojoin=0 */
    } else {
        /* TODO: send request */
        autocomplete_remove(bookmark_ac, jid);
    }

    return removed;
}

static const GList *
_bookmark_get_list(void)
{
    return bookmark_list;
}

static char *
_bookmark_find(char *search_str)
{
    return autocomplete_complete(bookmark_ac, search_str);
}

static void
_bookmark_autocomplete_reset(void)
{
    if (bookmark_ac != NULL) {
        autocomplete_reset(bookmark_ac);
    }
}

static int
_bookmark_handle_result(xmpp_conn_t * const conn,
    xmpp_stanza_t * const stanza, void * const userdata)
{
    xmpp_ctx_t *ctx = connection_get_ctx();
    char *id = (char *)userdata;
    xmpp_stanza_t *ptr;
    xmpp_stanza_t *nick;
    char *name;
    char *jid;
    char *autojoin;
    gboolean autojoin_val;
    Jid *my_jid;
    Bookmark *item;

    xmpp_timed_handler_delete(conn, _bookmark_handle_delete);
    g_free(id);

    name = xmpp_stanza_get_name(stanza);
    if (!name || strcmp(name, STANZA_NAME_IQ) != 0) {
        return 0;
    }

    ptr = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_QUERY);
    if (!ptr) {
        return 0;
    }
    ptr = xmpp_stanza_get_child_by_name(ptr, STANZA_NAME_STORAGE);
    if (!ptr) {
        return 0;
    }

    if (bookmark_ac == NULL) {
        bookmark_ac = autocomplete_new();
    }
    my_jid = jid_create(jabber_get_fulljid());

    ptr = xmpp_stanza_get_children(ptr);
    while (ptr) {
        name = xmpp_stanza_get_name(ptr);
        if (!name || strcmp(name, STANZA_NAME_CONFERENCE) != 0) {
            ptr = xmpp_stanza_get_next(ptr);
            continue;
        }
        jid = xmpp_stanza_get_attribute(ptr, STANZA_ATTR_JID);
        if (!jid) {
            ptr = xmpp_stanza_get_next(ptr);
            continue;
        }

        log_debug("Handle bookmark for %s", jid);

        name = NULL;
        nick = xmpp_stanza_get_child_by_name(ptr, "nick");
        if (nick) {
            char *tmp;
            tmp = xmpp_stanza_get_text(nick);
            if (tmp) {
                name = strdup(tmp);
                xmpp_free(ctx, tmp);
            }
        }

        autojoin = xmpp_stanza_get_attribute(ptr, "autojoin");
        if (autojoin && (strcmp(autojoin, "1") == 0 || strcmp(autojoin, "true") == 0)) {
            autojoin_val = TRUE;
        } else {
            autojoin_val = FALSE;
        }

        autocomplete_add(bookmark_ac, jid);
        item = malloc(sizeof(*item));
        item->jid = strdup(jid);
        item->nick = name;
        item->autojoin = autojoin_val;
        bookmark_list = g_list_append(bookmark_list, item);


        /* TODO: preference whether autojoin */
        if (autojoin_val) {
            if (autojoin_count < BOOKMARK_AUTOJOIN_MAX) {
                Jid *room_jid;

                ++autojoin_count;

                if (name == NULL) {
                    name = my_jid->localpart;
                }

                log_debug("Autojoin %s with nick=%s", jid, name);
                room_jid = jid_create_from_bare_and_resource(jid, name);
                if (!muc_room_is_active(room_jid->barejid)) {
                    presence_join_room(jid, name, NULL);
                    /* TODO: this should be removed after fixing #195 */
                    ui_room_join(jid);
                }
                jid_destroy(room_jid);
            } else {
                log_debug("Rejected autojoin %s (maximum has been reached)", jid);
            }
        }

        ptr = xmpp_stanza_get_next(ptr);
    }

    jid_destroy(my_jid);

    return 0;
}

static int
_bookmark_handle_delete(xmpp_conn_t * const conn,
    void * const userdata)
{
    char *id = (char *)userdata;

    assert(id != NULL);

    log_debug("Timeout for handler with id=%s", id);

    xmpp_id_handler_delete(conn, _bookmark_handle_result, id);
    g_free(id);

    return 0;
}

static void
_bookmark_item_destroy(gpointer item)
{
    Bookmark *p = (Bookmark *)item;

    if (p == NULL) {
        return;
    }

    free(p->jid);
    free(p->nick);
    free(p);
}

void
bookmark_init_module(void)
{
    bookmark_add = _bookmark_add;
    bookmark_remove = _bookmark_remove;
    bookmark_get_list = _bookmark_get_list;
    bookmark_find = _bookmark_find;
    bookmark_autocomplete_reset = _bookmark_autocomplete_reset;
}
