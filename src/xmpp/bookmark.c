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
#include "server_events.h"
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
static int _match_bookmark_by_jid(gconstpointer a, gconstpointer b);
static void _send_bookmarks(void);

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

    iq = stanza_create_bookmarks_storage_request(ctx);
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

    /* this may be command for modifying */
    Bookmark *item = malloc(sizeof(*item));
    item->jid = strdup(jid);
    if (nick != NULL) {
        item->nick = strdup(nick);
    } else {
        item->nick = NULL;
    }
    item->autojoin = autojoin;

    GList *found = g_list_find_custom(bookmark_list, item, _match_bookmark_by_jid);
    if (found != NULL) {
        bookmark_list = g_list_remove_link(bookmark_list, found);
        _bookmark_item_destroy(found->data);
        g_list_free(found);
    }
    bookmark_list = g_list_append(bookmark_list, item);

    autocomplete_remove(bookmark_ac, jid);
    autocomplete_add(bookmark_ac, jid);

    _send_bookmarks();

    return added;
}

static gboolean
_bookmark_remove(const char *jid, gboolean autojoin)
{
    Bookmark *item = malloc(sizeof(*item));
    item->jid = strdup(jid);
    item->nick = NULL;
    item->autojoin = autojoin;

    GList *found = g_list_find_custom(bookmark_list, item, _match_bookmark_by_jid);
    _bookmark_item_destroy(item);
    gboolean removed = found != NULL;

    // set autojoin FALSE
    if (autojoin) {
        if (found != NULL) {
            Bookmark *bookmark = found->data;
            bookmark->autojoin = FALSE;
            g_list_free(found);
        }

    // remove bookmark
    } else {
        if (found != NULL) {
            bookmark_list = g_list_remove_link(bookmark_list, found);
            _bookmark_item_destroy(found->data);
            g_list_free(found);
        }
        autocomplete_remove(bookmark_ac, jid);
    }

    _send_bookmarks();

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
                    handle_bookmark_autojoin(jid);
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

static int
_match_bookmark_by_jid(gconstpointer a, gconstpointer b)
{
    Bookmark *bookmark_a = (Bookmark *) a;
    Bookmark *bookmark_b = (Bookmark *) b;

    return strcmp(bookmark_a->jid, bookmark_b->jid);
}

static void
_send_bookmarks(void)
{
    xmpp_conn_t *conn = connection_get_conn();
    xmpp_ctx_t *ctx = connection_get_ctx();

    xmpp_stanza_t *iq = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(iq, STANZA_NAME_IQ);
    char *id = generate_unique_id("bookmarks_update");
    xmpp_stanza_set_id(iq, id);
    xmpp_stanza_set_type(iq, STANZA_TYPE_SET);

    xmpp_stanza_t *query = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(query, STANZA_NAME_QUERY);
    xmpp_stanza_set_ns(query, "jabber:iq:private");

    xmpp_stanza_t *storage = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(storage, STANZA_NAME_STORAGE);
    xmpp_stanza_set_ns(storage, "storage:bookmarks");

    GList *curr = bookmark_list;
    while (curr != NULL) {
        Bookmark *bookmark = curr->data;
        xmpp_stanza_t *conference = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(conference, STANZA_NAME_CONFERENCE);
        xmpp_stanza_set_attribute(conference, STANZA_ATTR_JID, bookmark->jid);

        Jid *jidp = jid_create(bookmark->jid);
        xmpp_stanza_set_attribute(conference, STANZA_ATTR_NAME, jidp->localpart);
        jid_destroy(jidp);

        if (bookmark->autojoin) {
            xmpp_stanza_set_attribute(conference, STANZA_ATTR_AUTOJOIN, "true");
        } else {
            xmpp_stanza_set_attribute(conference, STANZA_ATTR_AUTOJOIN, "false");
        }

        if (bookmark->nick != NULL) {
            xmpp_stanza_t *nick_st = xmpp_stanza_new(ctx);
            xmpp_stanza_set_name(nick_st, STANZA_NAME_NICK);
            xmpp_stanza_t *nick_text = xmpp_stanza_new(ctx);
            xmpp_stanza_set_text(nick_text, bookmark->nick);
            xmpp_stanza_add_child(nick_st, nick_text);
            xmpp_stanza_add_child(conference, nick_st);

            xmpp_stanza_release(nick_text);
            xmpp_stanza_release(nick_st);
        }

        xmpp_stanza_add_child(storage, conference);
        xmpp_stanza_release(conference);

        curr = curr->next;
    }

    xmpp_stanza_add_child(query, storage);
    xmpp_stanza_add_child(iq, query);
    xmpp_stanza_release(storage);
    xmpp_stanza_release(query);

    xmpp_send(conn, iq);
    xmpp_stanza_release(iq);
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