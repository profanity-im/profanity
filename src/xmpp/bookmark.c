
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

static int _bookmark_handle_result(xmpp_conn_t * const conn,
    xmpp_stanza_t * const stanza, void * const userdata);

void
bookmark_request(void)
{
    int id;
    char id_str[10];
    xmpp_conn_t * const conn = connection_get_conn();
    xmpp_ctx_t * const ctx = connection_get_ctx();
    xmpp_stanza_t *iq = stanza_create_storage_bookmarks(ctx);

    id = jabber_get_id();
    snprintf(id_str, sizeof(id_str), "%u", id);

    /* TODO: timed handler to remove this id_handler */
    xmpp_id_handler_add(conn, _bookmark_handle_result, id_str, ctx);

    xmpp_stanza_set_id(iq, id_str);
    xmpp_send(conn, iq);
    xmpp_stanza_release(iq);
}

static int
_bookmark_handle_result(xmpp_conn_t * const conn,
    xmpp_stanza_t * const stanza, void * const userdata)
{
    xmpp_ctx_t *ctx = (xmpp_ctx_t *)userdata;
    xmpp_stanza_t *ptr;
    xmpp_stanza_t *nick;
    char *name;
    char *jid;
    char *autojoin;
    Jid *my_jid;

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

    my_jid = jid_create(jabber_get_fulljid());

    ptr = xmpp_stanza_get_children(ptr);
    while (ptr) {
        name = xmpp_stanza_get_name(ptr);
        if (strcmp(name, STANZA_NAME_CONFERENCE) != 0) {
            continue;
        }
        jid = xmpp_stanza_get_attribute(ptr, STANZA_ATTR_JID);
        if (!jid) {
            continue;
        }

        log_debug("Handle bookmark for %s", jid);

        autojoin = xmpp_stanza_get_attribute(ptr, "autojoin");
        if (autojoin && strcmp(autojoin, "1") == 0) {
            name = NULL;
            nick = xmpp_stanza_get_child_by_name(ptr, "nick");
            if (nick) {
                char *tmp;
                tmp = xmpp_stanza_get_text(nick);
                if (tmp) {
                    name = strdup(tmp);
                    xmpp_free(ctx, tmp);
                }
            } else {
                name = strdup(my_jid->localpart);
            }

            if (name) {
                /* TODO: autojoin maximum (e.g. 5) rooms */
                log_debug("Autojoin %s with nick=%s", jid, name);
                Jid *room_jid = jid_create_from_bare_and_resource(jid, name);
                if (!muc_room_is_active(room_jid)) {
                    presence_join_room(room_jid);
                    /* XXX: this should be removed after fixing #195 */
                    ui_room_join(room_jid);
                }
                jid_destroy(room_jid);
                free(name);
            }
        }

        /* TODO: add to autocompleter */

        ptr = xmpp_stanza_get_next(ptr);
    }

    jid_destroy(my_jid);

    return 0;
}
