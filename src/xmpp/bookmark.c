
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <strophe.h>

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
    /* TODO: send request */
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
                if (!muc_room_is_active(room_jid)) {
                    presence_join_room(room_jid);
                    /* TODO: this should be removed after fixing #195 */
                    ui_room_join(room_jid);
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
