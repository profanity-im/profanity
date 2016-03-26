/*
 * bookmark.c
 *
 * Copyright (C) 2012 - 2016 James Booth <boothj5@gmail.com>
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

#include "prof_config.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#ifdef PROF_HAVE_LIBMESODE
#include <mesode.h>
#endif
#ifdef PROF_HAVE_LIBSTROPHE
#include <strophe.h>
#endif

#include "common.h"
#include "log.h"
#include "muc.h"
#include "event/server_events.h"
#include "xmpp/connection.h"
#include "xmpp/stanza.h"
#include "xmpp/xmpp.h"
#include "xmpp/bookmark.h"
#include "ui/ui.h"
#include "plugins/plugins.h"

#define BOOKMARK_TIMEOUT 5000

static Autocomplete bookmark_ac;
static GList *bookmark_list;

static int _bookmark_handle_result(xmpp_conn_t *const conn,
    xmpp_stanza_t *const stanza, void *const userdata);
static int _bookmark_handle_delete(xmpp_conn_t *const conn,
    void *const userdata);
static void _bookmark_item_destroy(gpointer item);
static int _match_bookmark_by_jid(gconstpointer a, gconstpointer b);
static void _send_bookmarks(void);

static void _send_iq_stanza(xmpp_conn_t *const conn, xmpp_stanza_t *const stanza);

void
bookmark_request(void)
{
    char *id;
    xmpp_conn_t *conn = connection_get_conn();
    xmpp_ctx_t *ctx = connection_get_ctx();
    xmpp_stanza_t *iq;

    id = strdup("bookmark_init_request");

    autocomplete_free(bookmark_ac);
    bookmark_ac = autocomplete_new();
    if (bookmark_list) {
        g_list_free_full(bookmark_list, _bookmark_item_destroy);
        bookmark_list = NULL;
    }

    xmpp_timed_handler_add(conn, _bookmark_handle_delete, BOOKMARK_TIMEOUT, id);
    xmpp_id_handler_add(conn, _bookmark_handle_result, id, id);

    iq = stanza_create_bookmarks_storage_request(ctx);
    xmpp_stanza_set_id(iq, id);
    _send_iq_stanza(conn, iq);
    xmpp_stanza_release(iq);
}

gboolean
bookmark_add(const char *jid, const char *nick, const char *password, const char *autojoin_str)
{
    if (autocomplete_contains(bookmark_ac, jid)) {
        return FALSE;
    } else {
        Bookmark *item = malloc(sizeof(*item));
        item->jid = strdup(jid);
        if (nick) {
            item->nick = strdup(nick);
        } else {
            item->nick = NULL;
        }
        if (password) {
            item->password = strdup(password);
        } else {
            item->password = NULL;
        }

        if (g_strcmp0(autojoin_str, "on") == 0) {
            item->autojoin = TRUE;
        } else {
            item->autojoin = FALSE;
        }

        bookmark_list = g_list_append(bookmark_list, item);
        autocomplete_add(bookmark_ac, jid);
        _send_bookmarks();

        return TRUE;
    }
}

gboolean
bookmark_update(const char *jid, const char *nick, const char *password, const char *autojoin_str)
{
    Bookmark *item = malloc(sizeof(*item));
    item->jid = strdup(jid);
    item->nick = NULL;
    item->password = NULL;
    item->autojoin = FALSE;

    GList *found = g_list_find_custom(bookmark_list, item, _match_bookmark_by_jid);
    _bookmark_item_destroy(item);
    if (found == NULL) {
        return FALSE;
    } else {
        Bookmark *bm = found->data;
        if (nick) {
            free(bm->nick);
            bm->nick = strdup(nick);
        }
        if (password) {
            free(bm->password);
            bm->password = strdup(password);
        }
        if (autojoin_str) {
            if (g_strcmp0(autojoin_str, "on") == 0) {
                bm->autojoin = TRUE;
            } else if (g_strcmp0(autojoin_str, "off") == 0) {
                bm->autojoin = FALSE;
            }
        }
        _send_bookmarks();
        return TRUE;
    }
}

gboolean
bookmark_join(const char *jid)
{
    Bookmark *item = malloc(sizeof(*item));
    item->jid = strdup(jid);
    item->nick = NULL;
    item->password = NULL;
    item->autojoin = FALSE;

    GList *found = g_list_find_custom(bookmark_list, item, _match_bookmark_by_jid);
    _bookmark_item_destroy(item);
    if (found == NULL) {
        return FALSE;
    } else {
        char *account_name = jabber_get_account_name();
        ProfAccount *account = accounts_get_account(account_name);
        Bookmark *item = found->data;
        if (!muc_active(item->jid)) {
            char *nick = item->nick;
            if (nick == NULL) {
                nick = account->muc_nick;
            }
            presence_join_room(item->jid, nick, item->password);
            muc_join(item->jid, nick, item->password, FALSE);
            account_free(account);
        } else if (muc_roster_complete(item->jid)) {
            ui_room_join(item->jid, TRUE);
        }
        return TRUE;
    }
}

gboolean
bookmark_remove(const char *jid)
{
    Bookmark *item = malloc(sizeof(*item));
    item->jid = strdup(jid);
    item->nick = NULL;
    item->password = NULL;
    item->autojoin = FALSE;

    GList *found = g_list_find_custom(bookmark_list, item, _match_bookmark_by_jid);
    _bookmark_item_destroy(item);
    gboolean removed = found != NULL;

    if (removed) {
        bookmark_list = g_list_remove_link(bookmark_list, found);
        _bookmark_item_destroy(found->data);
        g_list_free(found);
        autocomplete_remove(bookmark_ac, jid);
        _send_bookmarks();
        return TRUE;
    } else {
        return FALSE;
    }
}

const GList*
bookmark_get_list(void)
{
    return bookmark_list;
}

char*
bookmark_find(const char *const search_str)
{
    return autocomplete_complete(bookmark_ac, search_str, TRUE);
}

void
bookmark_autocomplete_reset(void)
{
    if (bookmark_ac) {
        autocomplete_reset(bookmark_ac);
    }
}

static int
_bookmark_handle_result(xmpp_conn_t *const conn,
    xmpp_stanza_t *const stanza, void *const userdata)
{
    xmpp_ctx_t *ctx = connection_get_ctx();
    char *id = (char *)userdata;
    xmpp_stanza_t *ptr;
    xmpp_stanza_t *nick;
    xmpp_stanza_t *password_st;
    char *name;
    char *jid;
    char *autojoin;
    char *password;
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

        password = NULL;
        password_st = xmpp_stanza_get_child_by_name(ptr, "password");
        if (password_st) {
            char *tmp;
            tmp = xmpp_stanza_get_text(password_st);
            if (tmp) {
                password = strdup(tmp);
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
        item->password = password;
        item->autojoin = autojoin_val;
        bookmark_list = g_list_append(bookmark_list, item);

        if (autojoin_val) {
            Jid *room_jid;

            char *account_name = jabber_get_account_name();
            ProfAccount *account = accounts_get_account(account_name);
            if (name == NULL) {
                name = account->muc_nick;
            }

            log_debug("Autojoin %s with nick=%s", jid, name);
            room_jid = jid_create_from_bare_and_resource(jid, name);
            if (!muc_active(room_jid->barejid)) {
                presence_join_room(jid, name, password);
                muc_join(jid, name, password, TRUE);
            }
            jid_destroy(room_jid);
            account_free(account);
        }

        ptr = xmpp_stanza_get_next(ptr);
    }

    jid_destroy(my_jid);

    return 0;
}

static int
_bookmark_handle_delete(xmpp_conn_t *const conn,
    void *const userdata)
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
    free(p->password);
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
    char *id = create_unique_id("bookmarks_update");
    xmpp_stanza_set_id(iq, id);
    free(id);
    xmpp_stanza_set_type(iq, STANZA_TYPE_SET);

    xmpp_stanza_t *query = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(query, STANZA_NAME_QUERY);
    xmpp_stanza_set_ns(query, "jabber:iq:private");

    xmpp_stanza_t *storage = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(storage, STANZA_NAME_STORAGE);
    xmpp_stanza_set_ns(storage, "storage:bookmarks");

    GList *curr = bookmark_list;
    while (curr) {
        Bookmark *bookmark = curr->data;
        xmpp_stanza_t *conference = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(conference, STANZA_NAME_CONFERENCE);
        xmpp_stanza_set_attribute(conference, STANZA_ATTR_JID, bookmark->jid);

        Jid *jidp = jid_create(bookmark->jid);
        if (jidp->localpart) {
            xmpp_stanza_set_attribute(conference, STANZA_ATTR_NAME, jidp->localpart);
        }
        jid_destroy(jidp);

        if (bookmark->autojoin) {
            xmpp_stanza_set_attribute(conference, STANZA_ATTR_AUTOJOIN, "true");
        } else {
            xmpp_stanza_set_attribute(conference, STANZA_ATTR_AUTOJOIN, "false");
        }

        if (bookmark->nick) {
            xmpp_stanza_t *nick_st = xmpp_stanza_new(ctx);
            xmpp_stanza_set_name(nick_st, STANZA_NAME_NICK);
            xmpp_stanza_t *nick_text = xmpp_stanza_new(ctx);
            xmpp_stanza_set_text(nick_text, bookmark->nick);
            xmpp_stanza_add_child(nick_st, nick_text);
            xmpp_stanza_add_child(conference, nick_st);

            xmpp_stanza_release(nick_text);
            xmpp_stanza_release(nick_st);
        }

        if (bookmark->password) {
            xmpp_stanza_t *password_st = xmpp_stanza_new(ctx);
            xmpp_stanza_set_name(password_st, STANZA_NAME_PASSWORD);
            xmpp_stanza_t *password_text = xmpp_stanza_new(ctx);
            xmpp_stanza_set_text(password_text, bookmark->password);
            xmpp_stanza_add_child(password_st, password_text);
            xmpp_stanza_add_child(conference, password_st);

            xmpp_stanza_release(password_text);
            xmpp_stanza_release(password_st);
        }

        xmpp_stanza_add_child(storage, conference);
        xmpp_stanza_release(conference);

        curr = curr->next;
    }

    xmpp_stanza_add_child(query, storage);
    xmpp_stanza_add_child(iq, query);
    xmpp_stanza_release(storage);
    xmpp_stanza_release(query);

    _send_iq_stanza(conn, iq);
    xmpp_stanza_release(iq);
}

static void
_send_iq_stanza(xmpp_conn_t *const conn, xmpp_stanza_t *const stanza)
{
    char *text;
    size_t text_size;
    xmpp_stanza_to_text(stanza, &text, &text_size);

    char *plugin_text = plugins_on_iq_stanza_send(text);
    if (plugin_text) {
        xmpp_send_raw(conn, plugin_text, strlen(plugin_text));
    } else {
        xmpp_send_raw(conn, text, text_size);
    }
}
