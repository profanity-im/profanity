/*
 * jabber.c
 *
 * Copyright (C) 2012, 2013 James Booth <boothj5@gmail.com>
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

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <openssl/evp.h>
#include <strophe.h>

#include "capabilities.h"
#include "chat_session.h"
#include "common.h"
#include "contact_list.h"
#include "jabber.h"
#include "jid.h"
#include "log.h"
#include "preferences.h"
#include "profanity.h"
#include "muc.h"
#include "stanza.h"

static struct _jabber_conn_t {
    xmpp_log_t *log;
    xmpp_ctx_t *ctx;
    xmpp_conn_t *conn;
    jabber_conn_status_t conn_status;
    jabber_presence_t presence;
    char *status;
    int tls_disabled;
    int priority;
} jabber_conn;

static GHashTable *sub_requests;

// for auto reconnect
static struct {
    char *account;
    char *jid;
    char *passwd;
    char *altdomain;
} saved_user;

static GTimer *reconnect_timer;

static log_level_t _get_log_level(xmpp_log_level_t xmpp_level);
static xmpp_log_level_t _get_xmpp_log_level();
static void _xmpp_file_logger(void * const userdata,
    const xmpp_log_level_t level, const char * const area,
    const char * const msg);
static xmpp_log_t * _xmpp_get_file_logger();

static void _jabber_roster_request(void);

// XMPP event handlers
static void _connection_handler(xmpp_conn_t * const conn,
    const xmpp_conn_event_t status, const int error,
    xmpp_stream_error_t * const stream_error, void * const userdata);

static int _message_handler(xmpp_conn_t * const conn,
    xmpp_stanza_t * const stanza, void * const userdata);
static int _groupchat_message_handler(xmpp_stanza_t * const stanza);
static int _error_handler(xmpp_stanza_t * const stanza);
static int _chat_message_handler(xmpp_stanza_t * const stanza);

static int _iq_handler(xmpp_conn_t * const conn,
    xmpp_stanza_t * const stanza, void * const userdata);
static int _roster_handler(xmpp_conn_t * const conn,
    xmpp_stanza_t * const stanza, void * const userdata);
static int _disco_response_handler(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza,
    void * const userdata);
static int _disco_request_handler(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza,
    void * const userdata);
static int _presence_handler(xmpp_conn_t * const conn,
    xmpp_stanza_t * const stanza, void * const userdata);
static int _ping_timed_handler(xmpp_conn_t * const conn, void * const userdata);
static char * _handle_presence_caps(xmpp_stanza_t * const stanza);

void
jabber_init(const int disable_tls)
{
    log_info("Initialising XMPP");
    jabber_conn.conn_status = JABBER_STARTED;
    jabber_conn.presence = PRESENCE_OFFLINE;
    jabber_conn.status = NULL;
    jabber_conn.tls_disabled = disable_tls;
    sub_requests = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
}

void
jabber_restart(void)
{
    jabber_conn.conn_status = JABBER_STARTED;
    jabber_conn.presence = PRESENCE_OFFLINE;
    FREE_SET_NULL(jabber_conn.status);
}

jabber_conn_status_t
jabber_connect_with_account(ProfAccount *account, const char * const passwd)
{
    FREE_SET_NULL(saved_user.account);

    if (account->name == NULL)
        return JABBER_UNDEFINED;

    saved_user.account = strdup(account->name);
    log_info("Connecting with account: %s", account->name);
    return jabber_connect(account->jid, passwd, account->server);
}

jabber_conn_status_t
jabber_connect(const char * const jid,
    const char * const passwd, const char * const altdomain)
{
    FREE_SET_NULL(saved_user.jid);
    FREE_SET_NULL(saved_user.passwd);
    FREE_SET_NULL(saved_user.altdomain);

    if (jid == NULL || passwd == NULL)
        return JABBER_UNDEFINED;

    saved_user.jid = strdup(jid);
    saved_user.passwd = strdup(passwd);
    if (altdomain != NULL)
        saved_user.altdomain = strdup(altdomain);

    log_info("Connecting as %s", jid);
    xmpp_initialize();

    jabber_conn.log = _xmpp_get_file_logger();
    jabber_conn.ctx = xmpp_ctx_new(NULL, jabber_conn.log);
    jabber_conn.conn = xmpp_conn_new(jabber_conn.ctx);

    xmpp_conn_set_jid(jabber_conn.conn, jid);
    xmpp_conn_set_pass(jabber_conn.conn, passwd);

    if (jabber_conn.tls_disabled)
        xmpp_conn_disable_tls(jabber_conn.conn);

    int connect_status = xmpp_connect_client(jabber_conn.conn, altdomain, 0,
        _connection_handler, jabber_conn.ctx);

    if (connect_status == 0)
        jabber_conn.conn_status = JABBER_CONNECTING;
    else
        jabber_conn.conn_status = JABBER_DISCONNECTED;

    return jabber_conn.conn_status;
}

void
jabber_disconnect(void)
{
    // if connected, send end stream and wait for response
    if (jabber_conn.conn_status == JABBER_CONNECTED) {
        log_info("Closing connection");
        jabber_conn.conn_status = JABBER_DISCONNECTING;
        xmpp_disconnect(jabber_conn.conn);

        while (jabber_get_connection_status() == JABBER_DISCONNECTING) {
            jabber_process_events();
        }
        jabber_free_resources();
    }
}

char *
jabber_get_local_sha1_caps_str(void)
{
/*
    GString *str = g_string_new("");
    unsigned char hash[SHA_DIGEST_LENGTH];

    g_string_append(str, "client/pc//Profanity ");
    g_string_append(str, PACKAGE_VERSION);
    if (strcmp(PACKAGE_STATUS, "development") == 0) {
        g_string_append(str, "dev");
    }
    g_string_append(str, "<");

    SHA1((unsigned char *)str->str, strlen(str->str), hash);

    char *result = g_base64_encode(hash, strlen((char *)hash));

    g_string_free(str, TRUE);

    return result;
*/
    return NULL;
}


char *
sha1_caps_str(xmpp_stanza_t *query)
{
    GSList *identities = NULL;
    GSList *features = NULL;
    GSList *form_names = NULL;
    GHashTable *forms = g_hash_table_new(g_str_hash, g_str_equal);

    GString *s = g_string_new("");

    xmpp_stanza_t *child = xmpp_stanza_get_children(query);
    while (child != NULL) {
        if (g_strcmp0(xmpp_stanza_get_name(child), STANZA_NAME_IDENTITY) == 0) {
            char *category = xmpp_stanza_get_attribute(child, "category");
            char *type = xmpp_stanza_get_attribute(child, "type");
            char *lang = xmpp_stanza_get_attribute(child, "xml:lang");
            char *name = xmpp_stanza_get_attribute(child, "name");

            GString *identity_str = g_string_new(category);
            g_string_append(identity_str, "/");
            if (type != NULL) {
                g_string_append(identity_str, type);
            }
            g_string_append(identity_str, "/");
            if (lang != NULL) {
                g_string_append(identity_str, lang);
            }
            g_string_append(identity_str, "/");
            if (name != NULL) {
                g_string_append(identity_str, name);
            }
            g_string_append(identity_str, "<");
            identities = g_slist_insert_sorted(identities, identity_str->str, (GCompareFunc)octet_compare);
        } else if (g_strcmp0(xmpp_stanza_get_name(child), STANZA_NAME_FEATURE) == 0) {
            char *feature_str = xmpp_stanza_get_attribute(child, "var");
            features = g_slist_insert_sorted(features, feature_str, (GCompareFunc)octet_compare);
        } else if (g_strcmp0(xmpp_stanza_get_name(child), STANZA_NAME_X) == 0) {
            if (strcmp(xmpp_stanza_get_ns(child), STANZA_NS_DATA) == 0) {
                DataForm *form = stanza_get_form(child);
                form_names = g_slist_insert_sorted(form_names, form->form_type, (GCompareFunc)octet_compare);
                g_hash_table_insert(forms, form->form_type, form);
            }
        }
        child = xmpp_stanza_get_next(child);
    }

    GSList *curr = identities;
    while (curr != NULL) {
        g_string_append(s, curr->data);
        curr = g_slist_next(curr);
    }

    curr = features;
    while (curr != NULL) {
        g_string_append(s, curr->data);
        g_string_append(s, "<");
        curr = g_slist_next(curr);
    }

    curr = form_names;
    while (curr != NULL) {
        DataForm *form = g_hash_table_lookup(forms, curr->data);
        g_string_append(s, form->form_type);
        g_string_append(s, "<");

        GSList *curr_field = form->fields;
        while (curr_field != NULL) {
            FormField *field = curr_field->data;
            g_string_append(s, field->var);
            GSList *curr_value = field->values;
            while (curr_value != NULL) {
                g_string_append(s, curr_value->data);
                g_string_append(s, "<");
                curr_value = g_slist_next(curr_value);
            }
            curr_field = g_slist_next(curr_value);
        }
    }

    EVP_MD_CTX mdctx;
    const EVP_MD *md;

    unsigned char md_value[EVP_MAX_MD_SIZE];
    unsigned int md_len;
    OpenSSL_add_all_digests();
    md = EVP_get_digestbyname("SHA1");
    EVP_MD_CTX_init(&mdctx);
    EVP_DigestInit_ex(&mdctx, md, NULL);
    EVP_DigestUpdate(&mdctx, s->str, strlen(s->str));
    EVP_DigestFinal_ex(&mdctx, md_value, &md_len);
    EVP_MD_CTX_cleanup(&mdctx);

    char *result = g_base64_encode(md_value, md_len);

    g_string_free(s, TRUE);
    g_slist_free(identities);
    g_slist_free(features);

    return result;
}

void
jabber_process_events(void)
{
    // run xmpp event loop if connected, connecting or disconnecting
    if (jabber_conn.conn_status == JABBER_CONNECTED
            || jabber_conn.conn_status == JABBER_CONNECTING
            || jabber_conn.conn_status == JABBER_DISCONNECTING) {
        xmpp_run_once(jabber_conn.ctx, 10);

    // check timer and reconnect if disconnected and timer set
    } else if (prefs_get_reconnect() != 0) {
        if ((jabber_conn.conn_status == JABBER_DISCONNECTED) &&
            (reconnect_timer != NULL)) {
            if (g_timer_elapsed(reconnect_timer, NULL) > prefs_get_reconnect()) {
                log_debug("Attempting reconnect as %s", saved_user.jid);
                jabber_connect(saved_user.jid, saved_user.passwd, saved_user.altdomain);
            }
        }
    }

}

void
jabber_send(const char * const msg, const char * const recipient)
{
    if (prefs_get_states()) {
        if (!chat_session_exists(recipient)) {
            chat_session_start(recipient, TRUE);
        }
    }

    xmpp_stanza_t *message;
    if (prefs_get_states() && chat_session_get_recipient_supports(recipient)) {
        chat_session_set_active(recipient);
        message = stanza_create_message(jabber_conn.ctx, recipient, STANZA_TYPE_CHAT,
            msg, STANZA_NAME_ACTIVE);
    } else {
        message = stanza_create_message(jabber_conn.ctx, recipient, STANZA_TYPE_CHAT,
            msg, NULL);
    }

    xmpp_send(jabber_conn.conn, message);
    xmpp_stanza_release(message);
}

void
jabber_send_groupchat(const char * const msg, const char * const recipient)
{
    xmpp_stanza_t *message = stanza_create_message(jabber_conn.ctx, recipient,
        STANZA_TYPE_GROUPCHAT, msg, NULL);

    xmpp_send(jabber_conn.conn, message);
    xmpp_stanza_release(message);
}

void
jabber_send_composing(const char * const recipient)
{
    xmpp_stanza_t *stanza = stanza_create_chat_state(jabber_conn.ctx, recipient,
        STANZA_NAME_COMPOSING);

    xmpp_send(jabber_conn.conn, stanza);
    xmpp_stanza_release(stanza);
    chat_session_set_sent(recipient);
}

void
jabber_send_paused(const char * const recipient)
{
    xmpp_stanza_t *stanza = stanza_create_chat_state(jabber_conn.ctx, recipient,
        STANZA_NAME_PAUSED);

    xmpp_send(jabber_conn.conn, stanza);
    xmpp_stanza_release(stanza);
    chat_session_set_sent(recipient);
}

void
jabber_send_inactive(const char * const recipient)
{
    xmpp_stanza_t *stanza = stanza_create_chat_state(jabber_conn.ctx, recipient,
        STANZA_NAME_INACTIVE);

    xmpp_send(jabber_conn.conn, stanza);
    xmpp_stanza_release(stanza);
    chat_session_set_sent(recipient);
}

void
jabber_send_gone(const char * const recipient)
{
    xmpp_stanza_t *stanza = stanza_create_chat_state(jabber_conn.ctx, recipient,
        STANZA_NAME_GONE);

    xmpp_send(jabber_conn.conn, stanza);
    xmpp_stanza_release(stanza);
    chat_session_set_sent(recipient);
}

void
jabber_subscription(const char * const jid, jabber_subscr_t action)
{
    xmpp_stanza_t *presence;
    char *type, *jid_cpy, *bare_jid;

    // jid must be a bare JID
    jid_cpy = strdup(jid);
    bare_jid = strtok(jid_cpy, "/");
    g_hash_table_remove(sub_requests, bare_jid);

    if (action == PRESENCE_SUBSCRIBE)
        type = STANZA_TYPE_SUBSCRIBE;
    else if (action == PRESENCE_SUBSCRIBED)
        type = STANZA_TYPE_SUBSCRIBED;
    else if (action == PRESENCE_UNSUBSCRIBED)
        type = STANZA_TYPE_UNSUBSCRIBED;
    else { // unknown action
        free(jid_cpy);
        return;
    }

    presence = xmpp_stanza_new(jabber_conn.ctx);
    xmpp_stanza_set_name(presence, STANZA_NAME_PRESENCE);
    xmpp_stanza_set_type(presence, type);
    xmpp_stanza_set_attribute(presence, STANZA_ATTR_TO, bare_jid);
    xmpp_send(jabber_conn.conn, presence);
    xmpp_stanza_release(presence);
    free(jid_cpy);
}

GList *
jabber_get_subscription_requests(void)
{
    return g_hash_table_get_keys(sub_requests);
}

void
jabber_join(Jid *jid)
{
    xmpp_stanza_t *presence = stanza_create_room_join_presence(jabber_conn.ctx,
        jid->fulljid);
    xmpp_send(jabber_conn.conn, presence);
    xmpp_stanza_release(presence);

    muc_join_room(jid->barejid, jid->resourcepart);
}

void
jabber_change_room_nick(const char * const room, const char * const nick)
{
    char *full_room_jid = create_full_room_jid(room, nick);
    xmpp_stanza_t *presence = stanza_create_room_newnick_presence(jabber_conn.ctx,
        full_room_jid);
    xmpp_send(jabber_conn.conn, presence);
    xmpp_stanza_release(presence);

    free(full_room_jid);
}

void
jabber_leave_chat_room(const char * const room_jid)
{
    char *nick = muc_get_room_nick(room_jid);

    xmpp_stanza_t *presence = stanza_create_room_leave_presence(jabber_conn.ctx,
        room_jid, nick);
    xmpp_send(jabber_conn.conn, presence);
    xmpp_stanza_release(presence);
}

void
jabber_update_presence(jabber_presence_t status, const char * const msg,
    int idle)
{
    int pri;
    char *show;

    // don't send presence when disconnected
    if (jabber_conn.conn_status != JABBER_CONNECTED)
        return;

    pri = prefs_get_priority();
    if (pri < JABBER_PRIORITY_MIN || pri > JABBER_PRIORITY_MAX)
        pri = 0;

    jabber_conn.presence = status;
    jabber_conn.priority = pri;

    switch(status)
    {
        case PRESENCE_AWAY:
            show = STANZA_TEXT_AWAY;
            break;
        case PRESENCE_DND:
            show = STANZA_TEXT_DND;
            break;
        case PRESENCE_CHAT:
            show = STANZA_TEXT_CHAT;
            break;
        case PRESENCE_XA:
            show = STANZA_TEXT_XA;
            break;
        default: // PRESENCE_ONLINE
            show = NULL;
            break;
    }

    if (jabber_conn.status != NULL) {
        free(jabber_conn.status);
        jabber_conn.status = NULL;
    }
    if (msg != NULL)
        jabber_conn.status = strdup(msg);

    xmpp_stanza_t *presence = stanza_create_presence(jabber_conn.ctx, show, msg);
    if (pri != 0) {
        xmpp_stanza_t *priority, *value;
        char pri_str[10];

        snprintf(pri_str, sizeof(pri_str), "%d", pri);
        priority = xmpp_stanza_new(jabber_conn.ctx);
        value = xmpp_stanza_new(jabber_conn.ctx);
        xmpp_stanza_set_name(priority, STANZA_NAME_PRIORITY);
        xmpp_stanza_set_text(value, pri_str);
        xmpp_stanza_add_child(priority, value);
        xmpp_stanza_add_child(presence, priority);
    }

    if (idle > 0) {
        xmpp_stanza_t *query = xmpp_stanza_new(jabber_conn.ctx);
        xmpp_stanza_set_name(query, STANZA_NAME_QUERY);
        xmpp_stanza_set_ns(query, STANZA_NS_LASTACTIVITY);
        char idle_str[10];
        snprintf(idle_str, sizeof(idle_str), "%d", idle);
        xmpp_stanza_set_attribute(query, STANZA_ATTR_SECONDS, idle_str);
        xmpp_stanza_add_child(presence, query);
    }

    // add caps
    xmpp_stanza_t *caps = xmpp_stanza_new(jabber_conn.ctx);
    xmpp_stanza_set_name(caps, STANZA_NAME_C);
    xmpp_stanza_set_ns(caps, STANZA_NS_CAPS);
    xmpp_stanza_set_attribute(caps, STANZA_ATTR_HASH, "sha-1");
    xmpp_stanza_set_attribute(caps, STANZA_ATTR_NODE, "http://www.profanity.im");

    xmpp_stanza_t *query = xmpp_stanza_new(jabber_conn.ctx);
    xmpp_stanza_set_name(query, STANZA_NAME_QUERY);
    xmpp_stanza_set_ns(query, XMPP_NS_DISCO_INFO);

    xmpp_stanza_t *identity = xmpp_stanza_new(jabber_conn.ctx);
    xmpp_stanza_set_name(identity, "identity");
    xmpp_stanza_set_attribute(identity, "category", "client");
    xmpp_stanza_set_attribute(identity, "type", "pc");

    GString *name_str = g_string_new("Profanity ");
    g_string_append(name_str, PACKAGE_VERSION);
    if (strcmp(PACKAGE_STATUS, "development") == 0) {
        g_string_append(name_str, "dev");
    }
    xmpp_stanza_set_attribute(identity, "name", name_str->str);

    xmpp_stanza_t *feature_caps = xmpp_stanza_new(jabber_conn.ctx);
    xmpp_stanza_set_name(feature_caps, STANZA_NAME_FEATURE);
    xmpp_stanza_set_attribute(feature_caps, STANZA_ATTR_VAR, STANZA_NS_CAPS);

    xmpp_stanza_t *feature_disoinfo = xmpp_stanza_new(jabber_conn.ctx);
    xmpp_stanza_set_name(feature_disoinfo, STANZA_NAME_FEATURE);
    xmpp_stanza_set_attribute(feature_disoinfo, STANZA_ATTR_VAR, XMPP_NS_DISCO_INFO);

    xmpp_stanza_t *feature_muc = xmpp_stanza_new(jabber_conn.ctx);
    xmpp_stanza_set_name(feature_muc, STANZA_NAME_FEATURE);
    xmpp_stanza_set_attribute(feature_muc, STANZA_ATTR_VAR, STANZA_NS_MUC);

    xmpp_stanza_add_child(query, identity);
    xmpp_stanza_add_child(query, feature_muc);
    xmpp_stanza_add_child(query, feature_disoinfo);
    xmpp_stanza_add_child(query, feature_caps);

    char *sha1 = sha1_caps_str(query);
    xmpp_stanza_set_attribute(caps, STANZA_ATTR_VER, sha1);

    xmpp_stanza_add_child(presence, caps);

    xmpp_send(jabber_conn.conn, presence);

    // send presence for each room
    GList *rooms = muc_get_active_room_list();
    while (rooms != NULL) {
        char *room = rooms->data;
        char *nick = muc_get_room_nick(room);
        char *full_room_jid = create_full_room_jid(room, nick);

        xmpp_stanza_set_attribute(presence, STANZA_ATTR_TO, full_room_jid);
        xmpp_send(jabber_conn.conn, presence);

        rooms = g_list_next(rooms);
    }
    g_list_free(rooms);

    xmpp_stanza_release(presence);
}

void
jabber_set_autoping(int seconds)
{
    if (jabber_conn.conn_status == JABBER_CONNECTED) {
        xmpp_timed_handler_delete(jabber_conn.conn, _ping_timed_handler);

        if (seconds != 0) {
            int millis = seconds * 1000;
            xmpp_timed_handler_add(jabber_conn.conn, _ping_timed_handler, millis,
                jabber_conn.ctx);
        }
    }
}

jabber_conn_status_t
jabber_get_connection_status(void)
{
    return (jabber_conn.conn_status);
}

const char *
jabber_get_jid(void)
{
    return xmpp_conn_get_jid(jabber_conn.conn);
}

int
jabber_get_priority(void)
{
    return jabber_conn.priority;
}

jabber_presence_t
jabber_get_presence(void)
{
    return jabber_conn.presence;
}

char *
jabber_get_status(void)
{
    if (jabber_conn.status == NULL)
        return NULL;
    else
        return strdup(jabber_conn.status);
}

void
jabber_free_resources(void)
{
    FREE_SET_NULL(saved_user.jid);
    FREE_SET_NULL(saved_user.passwd);
    FREE_SET_NULL(saved_user.account);
    FREE_SET_NULL(saved_user.altdomain);
    chat_sessions_clear();
    if (sub_requests != NULL)
        g_hash_table_remove_all(sub_requests);
    xmpp_conn_release(jabber_conn.conn);
    xmpp_ctx_free(jabber_conn.ctx);
    xmpp_shutdown();
}

static void
_jabber_roster_request(void)
{
    xmpp_stanza_t *iq = stanza_create_roster_iq(jabber_conn.ctx);
    xmpp_send(jabber_conn.conn, iq);
    xmpp_stanza_release(iq);
}

static int
_message_handler(xmpp_conn_t * const conn,
    xmpp_stanza_t * const stanza, void * const userdata)
{
    gchar *type = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_TYPE);

    if (type == NULL) {
        log_error("Message stanza received with no type attribute");
        return 1;
    } else if (strcmp(type, STANZA_TYPE_ERROR) == 0) {
        return _error_handler(stanza);
    } else if (strcmp(type, STANZA_TYPE_GROUPCHAT) == 0) {
        return _groupchat_message_handler(stanza);
    } else if (strcmp(type, STANZA_TYPE_CHAT) == 0) {
        return _chat_message_handler(stanza);
    } else {
        log_error("Message stanza received with unknown type: %s", type);
        return 1;
    }
}

static int
_groupchat_message_handler(xmpp_stanza_t * const stanza)
{
    char *message = NULL;
    char *room_jid = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_FROM);
    Jid *jid = jid_create(room_jid);

    // handle room broadcasts
    if (jid->resourcepart == NULL) {
        xmpp_stanza_t *subject = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_SUBJECT);

        // handle subject
        if (subject != NULL) {
            message = xmpp_stanza_get_text(subject);
            if (message != NULL) {
                prof_handle_room_subject(jid->barejid, message);
            }

            return 1;

        // handle other room broadcasts
        } else {
            xmpp_stanza_t *body = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_BODY);
            if (body != NULL) {
                message = xmpp_stanza_get_text(body);
                if (message != NULL) {
                    prof_handle_room_broadcast(room_jid, message);
                }
            }

            return 1;
        }
    }


    if (!jid_is_valid_room_form(jid)) {
        log_error("Invalid room JID: %s", jid->str);
        return 1;
    }

    // room not active in profanity
    if (!muc_room_is_active(jid)) {
        log_error("Message recieved for inactive chat room: %s", jid->str);
        return 1;
    }

    // determine if the notifications happened whilst offline
    GTimeVal tv_stamp;
    gboolean delayed = stanza_get_delay(stanza, &tv_stamp);
    xmpp_stanza_t *body = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_BODY);

    // check for and deal with message
    if (body != NULL) {
        char *message = xmpp_stanza_get_text(body);
        if (delayed) {
            prof_handle_room_history(jid->barejid, jid->resourcepart, tv_stamp, message);
        } else {
            prof_handle_room_message(jid->barejid, jid->resourcepart, message);
        }
    }

    jid_destroy(jid);

    return 1;
}

static int
_error_handler(xmpp_stanza_t * const stanza)
{
    gchar *err_msg = NULL;
    gchar *from = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_FROM);
    xmpp_stanza_t *error_stanza = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_ERROR);
        xmpp_stanza_t *text_stanza =
            xmpp_stanza_get_child_by_name(error_stanza, STANZA_NAME_TEXT);

    if (error_stanza == NULL) {
        log_debug("error message without <error/> received");
    } else {

        // check for text
        if (text_stanza != NULL) {
            err_msg = xmpp_stanza_get_text(text_stanza);
            prof_handle_error_message(from, err_msg);

            // TODO : process 'type' attribute from <error/> [RFC6120, 8.3.2]

        // otherwise show defined-condition
        } else {
            xmpp_stanza_t *err_cond = xmpp_stanza_get_children(error_stanza);

            if (err_cond == NULL) {
                log_debug("error message without <defined-condition/> or <text/> received");

            } else {
                err_msg = xmpp_stanza_get_name(err_cond);
                prof_handle_error_message(from, err_msg);

                // TODO : process 'type' attribute from <error/> [RFC6120, 8.3.2]
            }
        }
    }

    return 1;
}

static int
_chat_message_handler(xmpp_stanza_t * const stanza)
{
    gchar *from = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_FROM);
    Jid *jid = jid_create(from);

    // private message from chat room use full jid (room/nick)
    if (muc_room_is_active(jid)) {
        // determine if the notifications happened whilst offline
        GTimeVal tv_stamp;
        gboolean delayed = stanza_get_delay(stanza, &tv_stamp);

        // check for and deal with message
        xmpp_stanza_t *body = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_BODY);
        if (body != NULL) {
            char *message = xmpp_stanza_get_text(body);
            if (delayed) {
                prof_handle_delayed_message(jid->str, message, tv_stamp, TRUE);
            } else {
                prof_handle_incoming_message(jid->str, message, TRUE);
            }
        }

        free(jid);
        return 1;

    // standard chat message, use jid without resource
    } else {
        // determine chatstate support of recipient
        gboolean recipient_supports = FALSE;
        if (stanza_contains_chat_state(stanza)) {
            recipient_supports = TRUE;
        }

        // create or update chat session
        if (!chat_session_exists(jid->barejid)) {
            chat_session_start(jid->barejid, recipient_supports);
        } else {
            chat_session_set_recipient_supports(jid->barejid, recipient_supports);
        }

        // determine if the notifications happened whilst offline
        GTimeVal tv_stamp;
        gboolean delayed = stanza_get_delay(stanza, &tv_stamp);

        // deal with chat states if recipient supports them
        if (recipient_supports && (!delayed)) {
            if (xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_COMPOSING) != NULL) {
                if (prefs_get_notify_typing() || prefs_get_intype()) {
                    prof_handle_typing(jid->barejid);
                }
            } else if (xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_GONE) != NULL) {
                prof_handle_gone(jid->barejid);
            } else if (xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_PAUSED) != NULL) {
                // do something
            } else if (xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_INACTIVE) != NULL) {
                // do something
            } else { // handle <active/>
                // do something
            }
        }

        // check for and deal with message
        xmpp_stanza_t *body = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_BODY);
        if (body != NULL) {
            char *message = xmpp_stanza_get_text(body);
            if (delayed) {
                prof_handle_delayed_message(jid->barejid, message, tv_stamp, FALSE);
            } else {
                prof_handle_incoming_message(jid->barejid, message, FALSE);
            }
        }

        free(jid);
        return 1;
    }

}

static void
_connection_handler(xmpp_conn_t * const conn,
    const xmpp_conn_event_t status, const int error,
    xmpp_stream_error_t * const stream_error, void * const userdata)
{
    xmpp_ctx_t *ctx = (xmpp_ctx_t *)userdata;

    // login success
    if (status == XMPP_CONN_CONNECT) {
        if (saved_user.account != NULL) {
            prof_handle_login_account_success(saved_user.account);
        } else {
            const char *jid = xmpp_conn_get_jid(conn);
            prof_handle_login_success(jid, saved_user.altdomain);
        }

        chat_sessions_init();

        xmpp_handler_add(conn, _message_handler, NULL, STANZA_NAME_MESSAGE, NULL, ctx);
        xmpp_handler_add(conn, _presence_handler, NULL, STANZA_NAME_PRESENCE, NULL, ctx);
        xmpp_handler_add(conn, _iq_handler, NULL, STANZA_NAME_IQ, NULL, ctx);

        if (prefs_get_autoping() != 0) {
            int millis = prefs_get_autoping() * 1000;
            xmpp_timed_handler_add(conn, _ping_timed_handler, millis, ctx);
        }

        _jabber_roster_request();
        jabber_conn.conn_status = JABBER_CONNECTED;
        jabber_conn.presence = PRESENCE_ONLINE;

        if (prefs_get_reconnect() != 0) {
            if (reconnect_timer != NULL) {
                g_timer_destroy(reconnect_timer);
                reconnect_timer = NULL;
            }
        }

    } else if (status == XMPP_CONN_DISCONNECT) {

        // lost connection for unkown reason
        if (jabber_conn.conn_status == JABBER_CONNECTED) {
            prof_handle_lost_connection();
            if (prefs_get_reconnect() != 0) {
                assert(reconnect_timer == NULL);
                reconnect_timer = g_timer_new();
                // TODO: free resources but leave saved_user untouched
            } else {
                jabber_free_resources();
            }

        // login attempt failed
        } else if (jabber_conn.conn_status != JABBER_DISCONNECTING) {
            if (reconnect_timer == NULL) {
                prof_handle_failed_login();
                jabber_free_resources();
            } else {
                if (prefs_get_reconnect() != 0) {
                    g_timer_start(reconnect_timer);
                }
                // TODO: free resources but leave saved_user untouched
            }
        }

        // close stream response from server after disconnect is handled too
        jabber_conn.conn_status = JABBER_DISCONNECTED;
        jabber_conn.presence = PRESENCE_OFFLINE;
    }
}

static int
_iq_handler(xmpp_conn_t * const conn,
    xmpp_stanza_t * const stanza, void * const userdata)
{
    char *id = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_ID);
    char *type = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_TYPE);

    // handle the initial roster request
    if (g_strcmp0(id, "roster") == 0) {
        return _roster_handler(conn, stanza, userdata);

    // handle disco responses
    } else if ((g_str_has_prefix(id, "disco")) && (g_strcmp0(type, "result") == 0)) {
        return _disco_response_handler(conn, stanza, userdata);

    // handle disco requests
    } else if (stanza_is_caps_request(stanza)) {
        return _disco_request_handler(conn, stanza, userdata);

    // handle iq
    } else {
        char *type = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_TYPE);
        if (type == NULL) {
            return TRUE;
        }

        // handle roster update
        if (strcmp(type, STANZA_TYPE_SET) == 0) {

            xmpp_stanza_t *query =
                xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_QUERY);
            if (query == NULL) {
                return TRUE;
            }

            char *xmlns = xmpp_stanza_get_attribute(query, STANZA_ATTR_XMLNS);
            if (xmlns == NULL) {
                return TRUE;
            }
            if (strcmp(xmlns, XMPP_NS_ROSTER) != 0) {
                return TRUE;
            }

            xmpp_stanza_t *item =
                xmpp_stanza_get_child_by_name(query, STANZA_NAME_ITEM);
            if (item == NULL) {
                return TRUE;
            }

            const char *jid = xmpp_stanza_get_attribute(item, STANZA_ATTR_JID);
            const char *sub = xmpp_stanza_get_attribute(item, STANZA_ATTR_SUBSCRIPTION);
            if (g_strcmp0(sub, "remove") == 0) {
                contact_list_remove(jid);
                return TRUE;
            }

            gboolean pending_out = FALSE;
            const char *ask = xmpp_stanza_get_attribute(item, STANZA_ATTR_ASK);
            if ((ask != NULL) && (strcmp(ask, "subscribe") == 0)) {
                pending_out = TRUE;
            }

            contact_list_update_subscription(jid, sub, pending_out);

            return TRUE;

        // handle server ping
        } else if (strcmp(type, STANZA_TYPE_GET) == 0) {
            xmpp_stanza_t *ping = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_PING);
            if (ping == NULL) {
                return TRUE;
            }

            char *xmlns = xmpp_stanza_get_attribute(ping, STANZA_ATTR_XMLNS);
            if (xmlns == NULL) {
                return TRUE;
            }

            if (strcmp(xmlns, STANZA_NS_PING) != 0) {
                return TRUE;
            }

            char *to = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_TO);
            char *from = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_FROM);
            if ((from == NULL) || (to == NULL)) {
                return TRUE;
            }

            xmpp_stanza_t *pong = xmpp_stanza_new(jabber_conn.ctx);
            xmpp_stanza_set_name(pong, STANZA_NAME_IQ);
            xmpp_stanza_set_attribute(pong, STANZA_ATTR_TO, from);
            xmpp_stanza_set_attribute(pong, STANZA_ATTR_FROM, to);
            xmpp_stanza_set_attribute(pong, STANZA_ATTR_TYPE, STANZA_TYPE_RESULT);
            if (id != NULL) {
                xmpp_stanza_set_attribute(pong, STANZA_ATTR_ID, id);
            }

            xmpp_send(jabber_conn.conn, pong);
            xmpp_stanza_release(pong);

            return TRUE;
        } else {
            return TRUE;
        }
    }
}

static int
_roster_handler(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza,
    void * const userdata)
{
    xmpp_stanza_t *query, *item;
    char *type = xmpp_stanza_get_type(stanza);

    if (strcmp(type, STANZA_TYPE_ERROR) == 0)
        log_error("Roster query failed");
    else {
        query = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_QUERY);
        item = xmpp_stanza_get_children(query);

        while (item != NULL) {
            const char *jid = xmpp_stanza_get_attribute(item, STANZA_ATTR_JID);
            const char *name = xmpp_stanza_get_attribute(item, STANZA_ATTR_NAME);
            const char *sub = xmpp_stanza_get_attribute(item, STANZA_ATTR_SUBSCRIPTION);

            gboolean pending_out = FALSE;
            const char *ask = xmpp_stanza_get_attribute(item, STANZA_ATTR_ASK);
            if ((ask != NULL) && (strcmp(ask, "subscribe") == 0)) {
                pending_out = TRUE;
            }

            gboolean added = contact_list_add(jid, name, "offline", NULL, sub,
                pending_out);

            if (!added) {
                log_warning("Attempt to add contact twice: %s", jid);
            }

            item = xmpp_stanza_get_next(item);
        }

        /* TODO: Save somehow last presence show and use it for initial
         *       presence rather than PRESENCE_ONLINE. It will be helpful
         *       when I set dnd status and reconnect for some reason */
        // send initial presence
        jabber_update_presence(PRESENCE_ONLINE, NULL, 0);
    }

    return 1;
}

static int
_disco_request_handler(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza,
    void * const userdata)
{
    xmpp_ctx_t *ctx = (xmpp_ctx_t *)userdata;

    xmpp_stanza_t *incoming_query = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_QUERY);
    char *node_str = xmpp_stanza_get_attribute(incoming_query, STANZA_ATTR_NODE);
    char *from  = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_FROM);

    if (from != NULL) {
        xmpp_stanza_t *response = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(response, STANZA_NAME_IQ);
        xmpp_stanza_set_id(response, xmpp_stanza_get_id(stanza));
        xmpp_stanza_set_attribute(response, STANZA_ATTR_TO, from);
        xmpp_stanza_set_type(response, STANZA_TYPE_RESULT);

        xmpp_stanza_t *query = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(query, STANZA_NAME_QUERY);
        xmpp_stanza_set_ns(query, XMPP_NS_DISCO_INFO);
        xmpp_stanza_set_attribute(query, STANZA_ATTR_NODE, node_str);

        xmpp_stanza_t *identity = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(identity, "identity");
        xmpp_stanza_set_attribute(identity, "category", "client");
        xmpp_stanza_set_attribute(identity, "type", "pc");

        GString *name_str = g_string_new("Profanity ");
        g_string_append(name_str, PACKAGE_VERSION);
        if (strcmp(PACKAGE_STATUS, "development") == 0) {
            g_string_append(name_str, "dev");
        }
        xmpp_stanza_set_attribute(identity, "name", name_str->str);

        xmpp_stanza_t *feature_caps = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(feature_caps, STANZA_NAME_FEATURE);
        xmpp_stanza_set_attribute(feature_caps, STANZA_ATTR_VAR, STANZA_NS_CAPS);

        xmpp_stanza_t *feature_disoinfo = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(feature_disoinfo, STANZA_NAME_FEATURE);
        xmpp_stanza_set_attribute(feature_disoinfo, STANZA_ATTR_VAR, XMPP_NS_DISCO_INFO);

        xmpp_stanza_t *feature_muc = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(feature_muc, STANZA_NAME_FEATURE);
        xmpp_stanza_set_attribute(feature_muc, STANZA_ATTR_VAR, STANZA_NS_MUC);

        xmpp_stanza_add_child(query, identity);

        xmpp_stanza_add_child(query, feature_muc);
        xmpp_stanza_add_child(query, feature_disoinfo);
        xmpp_stanza_add_child(query, feature_caps);

        xmpp_stanza_add_child(response, query);

        xmpp_send(conn, response);
    }

    return 1;
}

static int
_disco_response_handler(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza,
    void * const userdata)
{
    char *type = xmpp_stanza_get_type(stanza);
    char *id = xmpp_stanza_get_id(stanza);

    if (g_strcmp0(type, STANZA_TYPE_ERROR) == 0) {
        log_error("Roster query failed");
        return 1;
    } else {
        xmpp_stanza_t *query = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_QUERY);
        char *node = xmpp_stanza_get_attribute(query, STANZA_ATTR_NODE);
        if (node == NULL) {
            return 1;
        }

        char *caps_key = NULL;

        // xep-0115
        if (g_strcmp0(id, "disco") == 0) {
            caps_key = node;

            // validate sha1
            gchar **split = g_strsplit(node, "#", -1);
            char *given_sha1 = split[1];
            char *generated_sha1 = sha1_caps_str(query);

            if (g_strcmp0(given_sha1, generated_sha1) != 0) {
                log_info("Invalid SHA1 recieved for caps.");
                return 1;
            }
        // non supported hash, or legacy caps
        } else {
            caps_key = id + 6;
        }

        // already cached
        if (caps_contains(caps_key)) {
            log_info("Client info already cached.");
            return 1;
        }

        xmpp_stanza_t *identity = xmpp_stanza_get_child_by_name(query, "identity");

        if (identity == NULL) {
            return 1;
        }

        const char *category = xmpp_stanza_get_attribute(identity, "category");
        if (category == NULL) {
            return 1;
        }

        if (strcmp(category, "client") != 0) {
            return 1;
        }

        const char *name = xmpp_stanza_get_attribute(identity, "name");
        if (name == 0) {
            return 1;
        }

        caps_add(caps_key, name);

        return 1;
    }
}

static int
_ping_timed_handler(xmpp_conn_t * const conn, void * const userdata)
{
    if (jabber_conn.conn_status == JABBER_CONNECTED) {
        xmpp_ctx_t *ctx = (xmpp_ctx_t *)userdata;

        xmpp_stanza_t *iq = stanza_create_ping_iq(ctx);
        xmpp_send(conn, iq);
        xmpp_stanza_release(iq);
    }

    return 1;
}

static int
_room_presence_handler(const char * const jid, xmpp_stanza_t * const stanza)
{
    char *room = NULL;
    char *nick = NULL;

    if (!parse_room_jid(jid, &room, &nick)) {
        log_error("Could not parse room jid: %s", room);
        return 1;
    }

    // handle self presence
    if (stanza_is_muc_self_presence(stanza, jabber_get_jid())) {
        char *type = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_TYPE);
        gboolean nick_change = stanza_is_room_nick_change(stanza);

        if ((type != NULL) && (strcmp(type, STANZA_TYPE_UNAVAILABLE) == 0)) {

            // leave room if not self nick change
            if (nick_change) {
                muc_set_room_pending_nick_change(room);
            } else {
                prof_handle_leave_room(room);
            }

        // handle self nick change
        } else if (muc_is_room_pending_nick_change(room)) {
            muc_complete_room_nick_change(room, nick);
            prof_handle_room_nick_change(room, nick);

        // handle roster complete
        } else if (!muc_get_roster_received(room)) {
            prof_handle_room_roster_complete(room);

        }

    // handle presence from room members
    } else {
        char *type = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_TYPE);
        char *show_str, *status_str;
        char *caps_key = _handle_presence_caps(stanza);

        xmpp_stanza_t *status = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_STATUS);
        if (status != NULL) {
            status_str = xmpp_stanza_get_text(status);
        } else {
            status_str = NULL;
        }

        if ((type != NULL) && (strcmp(type, STANZA_TYPE_UNAVAILABLE) == 0)) {

            // handle nickname change
            if (stanza_is_room_nick_change(stanza)) {
                char *new_nick = stanza_get_new_nick(stanza);
                muc_set_roster_pending_nick_change(room, new_nick, nick);
            } else {
                prof_handle_room_member_offline(room, nick, "offline", status_str);
            }
        } else {
            xmpp_stanza_t *show = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_SHOW);
            if (show != NULL) {
                show_str = xmpp_stanza_get_text(show);
            } else {
                show_str = "online";
            }
            if (!muc_get_roster_received(room)) {
                muc_add_to_roster(room, nick, show_str, status_str, caps_key);
            } else {
                char *old_nick = muc_complete_roster_nick_change(room, nick);

                if (old_nick != NULL) {
                    muc_add_to_roster(room, nick, show_str, status_str, caps_key);
                    prof_handle_room_member_nick_change(room, old_nick, nick);
                } else {
                    if (!muc_nick_in_roster(room, nick)) {
                        prof_handle_room_member_online(room, nick, show_str, status_str, caps_key);
                    } else {
                        prof_handle_room_member_presence(room, nick, show_str, status_str, caps_key);
                    }
                }
            }
        }
    }

    free(room);
    free(nick);

    return 1;
}

static char *
_handle_presence_caps(xmpp_stanza_t * const stanza)
{
    char *caps_key = NULL;
    char *node = NULL;
    char *from = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_FROM);
    if (stanza_contains_caps(stanza)) {
        char *hash_type = stanza_caps_get_hash(stanza);

        // xep-0115
        if (hash_type != NULL) {

            // supported hash
            if (strcmp(hash_type, "sha-1") == 0) {
                node = stanza_get_caps_str(stanza);
                caps_key = node;

                if (node != NULL) {
                    if (!caps_contains(caps_key)) {
                        xmpp_stanza_t *iq = stanza_create_disco_iq(jabber_conn.ctx, "disco", from, node);
                        xmpp_send(jabber_conn.conn, iq);
                        xmpp_stanza_release(iq);
                    }
                }

            // unsupported hash
            } else {
                node = stanza_get_caps_str(stanza);
                caps_key = from;

                if (node != NULL) {
                    if (!caps_contains(caps_key)) {
                        GString *id = g_string_new("disco_");
                        g_string_append(id, from);
                        xmpp_stanza_t *iq = stanza_create_disco_iq(jabber_conn.ctx, id->str, from, node);
                        xmpp_send(jabber_conn.conn, iq);
                        xmpp_stanza_release(iq);
                        g_string_free(id, TRUE);
                    }
                }
            }

            return strdup(caps_key);

        //ignore or handle legacy caps
        } else {
            node = stanza_get_caps_str(stanza);
            caps_key = from;

            if (node != NULL) {
                if (!caps_contains(caps_key)) {
                    GString *id = g_string_new("disco_");
                    g_string_append(id, from);
                    xmpp_stanza_t *iq = stanza_create_disco_iq(jabber_conn.ctx, id->str, from, node);
                    xmpp_send(jabber_conn.conn, iq);
                    xmpp_stanza_release(iq);
                    g_string_free(id, TRUE);
                }
            }

            return caps_key;
        }
    }
    return NULL;
}

static int
_presence_handler(xmpp_conn_t * const conn,
    xmpp_stanza_t * const stanza, void * const userdata)
{
    const char *jid = xmpp_conn_get_jid(jabber_conn.conn);
    char *from = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_FROM);
    char *type = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_TYPE);

    Jid *my_jid = jid_create(jid);
    Jid *from_jid = jid_create(from);

    if ((type != NULL) && (strcmp(type, STANZA_TYPE_ERROR) == 0)) {
        return _error_handler(stanza);
    }

    // handle chat room presence
    if (muc_room_is_active(from_jid)) {
        return _room_presence_handler(from_jid->str, stanza);

    // handle regular presence
    } else {
        char *type = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_TYPE);
        char *show_str, *status_str;
        int idle_seconds = stanza_get_idle_time(stanza);
        GDateTime *last_activity = NULL;

        if (idle_seconds > 0) {
            GDateTime *now = g_date_time_new_now_local();
            last_activity = g_date_time_add_seconds(now, 0 - idle_seconds);
            g_date_time_unref(now);
        }

        char *caps_key = _handle_presence_caps(stanza);

        xmpp_stanza_t *status = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_STATUS);
        if (status != NULL)
            status_str = xmpp_stanza_get_text(status);
        else
            status_str = NULL;

        if (type == NULL) { // available
            xmpp_stanza_t *show = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_SHOW);
            if (show != NULL)
                show_str = xmpp_stanza_get_text(show);
            else
                show_str = "online";

            if (strcmp(my_jid->barejid, from_jid->barejid) !=0) {
                prof_handle_contact_online(from_jid->barejid, show_str, status_str, last_activity, caps_key);
            }
        } else if (strcmp(type, STANZA_TYPE_UNAVAILABLE) == 0) {
            if (strcmp(my_jid->barejid, from_jid->barejid) !=0) {
                prof_handle_contact_offline(from_jid->barejid, "offline", status_str);
            }

        if (last_activity != NULL) {
            g_date_time_unref(last_activity);
        }

        // subscriptions
        } else if (strcmp(type, STANZA_TYPE_SUBSCRIBE) == 0) {
            prof_handle_subscription(from_jid->barejid, PRESENCE_SUBSCRIBE);
            g_hash_table_insert(sub_requests, strdup(from_jid->barejid), strdup(from_jid->barejid));
        } else if (strcmp(type, STANZA_TYPE_SUBSCRIBED) == 0) {
            prof_handle_subscription(from_jid->barejid, PRESENCE_SUBSCRIBED);
            g_hash_table_remove(sub_requests, from_jid->barejid);
        } else if (strcmp(type, STANZA_TYPE_UNSUBSCRIBED) == 0) {
            prof_handle_subscription(from_jid->barejid, PRESENCE_UNSUBSCRIBED);
            g_hash_table_remove(sub_requests, from_jid->barejid);
        } else { /* unknown type */
            log_debug("Received presence with unknown type '%s'", type);
        }
    }

    return 1;
}

static log_level_t
_get_log_level(xmpp_log_level_t xmpp_level)
{
    if (xmpp_level == XMPP_LEVEL_DEBUG) {
        return PROF_LEVEL_DEBUG;
    } else if (xmpp_level == XMPP_LEVEL_INFO) {
        return PROF_LEVEL_INFO;
    } else if (xmpp_level == XMPP_LEVEL_WARN) {
        return PROF_LEVEL_WARN;
    } else {
        return PROF_LEVEL_ERROR;
    }
}

static xmpp_log_level_t
_get_xmpp_log_level()
{
    log_level_t prof_level = log_get_filter();

    if (prof_level == PROF_LEVEL_DEBUG) {
        return XMPP_LEVEL_DEBUG;
    } else if (prof_level == PROF_LEVEL_INFO) {
        return XMPP_LEVEL_INFO;
    } else if (prof_level == PROF_LEVEL_WARN) {
        return XMPP_LEVEL_WARN;
    } else {
        return XMPP_LEVEL_ERROR;
    }
}

static void
_xmpp_file_logger(void * const userdata, const xmpp_log_level_t level,
    const char * const area, const char * const msg)
{
    log_level_t prof_level = _get_log_level(level);
    log_msg(prof_level, area, msg);
}

static xmpp_log_t *
_xmpp_get_file_logger()
{
    xmpp_log_level_t level = _get_xmpp_log_level();
    xmpp_log_t *file_log = malloc(sizeof(xmpp_log_t));

    file_log->handler = _xmpp_file_logger;
    file_log->userdata = &level;

    return file_log;
}

