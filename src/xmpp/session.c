/*
 * session.c
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
#include <string.h>
#include <stdlib.h>

#include "profanity.h"
#include "log.h"
#include "common.h"
#include "config/preferences.h"
#include "plugins/plugins.h"
#include "event/server_events.h"
#include "event/client_events.h"
#include "xmpp/bookmark.h"
#include "xmpp/blocking.h"
#include "xmpp/connection.h"
#include "xmpp/capabilities.h"
#include "xmpp/session.h"
#include "xmpp/iq.h"
#include "xmpp/message.h"
#include "xmpp/presence.h"
#include "xmpp/roster.h"
#include "xmpp/stanza.h"
#include "xmpp/xmpp.h"
#include "xmpp/muc.h"
#include "xmpp/chat_session.h"
#include "xmpp/jid.h"

// for auto reconnect
static struct {
    char *name;
    char *passwd;
} saved_account;

static struct {
    char *name;
    char *jid;
    char *passwd;
    char *altdomain;
    int port;
    char *tls_policy;
} saved_details;

typedef enum {
    ACTIVITY_ST_ACTIVE,
    ACTIVITY_ST_IDLE,
    ACTIVITY_ST_AWAY,
    ACTIVITY_ST_XA,
} activity_state_t;

static GTimer *reconnect_timer;
static activity_state_t activity_state;
static resource_presence_t saved_presence;
static char *saved_status;

static void _session_reconnect(void);

static void _session_free_saved_account(void);
static void _session_free_saved_details(void);

void
session_init(void)
{
    log_info("Initialising XMPP");
    connection_init();
    presence_sub_requests_init();
    caps_init();
}

jabber_conn_status_t
session_connect_with_account(const ProfAccount *const account)
{
    assert(account != NULL);

    log_info("Connecting using account: %s", account->name);

    // save account name and password for reconnect
    if (saved_account.name) {
        free(saved_account.name);
    }
    saved_account.name = strdup(account->name);
    if (saved_account.passwd) {
        free(saved_account.passwd);
    }
    saved_account.passwd = strdup(account->password);

    char *jid = NULL;
    if (account->resource) {
        Jid *jidp = jid_create_from_bare_and_resource(account->jid, account->resource);
        jid = strdup(jidp->fulljid);
        jid_destroy(jidp);
    } else {
        jid = strdup(account->jid);
    }

    jabber_conn_status_t result = connection_connect(
        jid,
        account->password,
        account->server,
        account->port,
        account->tls_policy);
    free(jid);

    return result;
}

jabber_conn_status_t
session_connect_with_details(const char *const jid, const char *const passwd, const char *const altdomain,
    const int port, const char *const tls_policy)
{
    assert(jid != NULL);
    assert(passwd != NULL);

    // save details for reconnect, remember name for account creating on success
    saved_details.name = strdup(jid);
    saved_details.passwd = strdup(passwd);
    if (altdomain) {
        saved_details.altdomain = strdup(altdomain);
    } else {
        saved_details.altdomain = NULL;
    }
    if (port != 0) {
        saved_details.port = port;
    } else {
        saved_details.port = 0;
    }
    if (tls_policy) {
        saved_details.tls_policy = strdup(tls_policy);
    } else {
        saved_details.tls_policy = NULL;
    }

    // use 'profanity' when no resourcepart in provided jid
    Jid *jidp = jid_create(jid);
    if (jidp->resourcepart == NULL) {
        jid_destroy(jidp);
        jidp = jid_create_from_bare_and_resource(jid, "profanity");
        saved_details.jid = strdup(jidp->fulljid);
    } else {
        saved_details.jid = strdup(jid);
    }
    jid_destroy(jidp);

    // connect with fulljid
    log_info("Connecting without account, JID: %s", saved_details.jid);

    return connection_connect(
        saved_details.jid,
        passwd,
        saved_details.altdomain,
        saved_details.port,
        saved_details.tls_policy);
}

void
session_autoping_fail(void)
{
    if (connection_get_status() == JABBER_CONNECTED) {
        log_info("Closing connection");

        char *account_name = session_get_account_name();
        const char *fulljid = connection_get_fulljid();
        plugins_on_disconnect(account_name, fulljid);

        accounts_set_last_activity(session_get_account_name());

        connection_disconnect();
    }

    connection_set_disconnected();

    session_lost_connection();
}

void
session_disconnect(void)
{
    // if connected, send end stream and wait for response
    if (connection_get_status() == JABBER_CONNECTED) {
        log_info("Closing connection");

        char *account_name = session_get_account_name();
        const char *fulljid = connection_get_fulljid();
        plugins_on_disconnect(account_name, fulljid);

        accounts_set_last_activity(session_get_account_name());

        connection_disconnect();

        _session_free_saved_account();
        _session_free_saved_details();

        connection_clear_data();

        chat_sessions_clear();
        presence_clear_sub_requests();
    }

    connection_set_disconnected();
}

void
session_shutdown(void)
{
    _session_free_saved_account();
    _session_free_saved_details();

    chat_sessions_clear();
    presence_clear_sub_requests();

    connection_shutdown();
    if (saved_status) {
        free(saved_status);
    }
}

void
session_process_events(void)
{
    int reconnect_sec;

    jabber_conn_status_t conn_status = connection_get_status();
    switch (conn_status)
    {
    case JABBER_CONNECTED:
    case JABBER_CONNECTING:
    case JABBER_DISCONNECTING:
        connection_check_events();
        break;
    case JABBER_DISCONNECTED:
        reconnect_sec = prefs_get_reconnect();
        if ((reconnect_sec != 0) && reconnect_timer) {
            int elapsed_sec = g_timer_elapsed(reconnect_timer, NULL);
            if (elapsed_sec > reconnect_sec) {
                _session_reconnect();
            }
        }
        break;
    default:
        break;
    }
}

char*
session_get_account_name(void)
{
    return saved_account.name;
}

void
session_login_success(gboolean secured)
{
    // logged in with account
    if (saved_account.name) {
        log_debug("Connection handler: logged in with account name: %s", saved_account.name);
        sv_ev_login_account_success(saved_account.name, secured);

    // logged in without account, use details to create new account
    } else {
        log_debug("Connection handler: logged in with jid: %s", saved_details.name);
        accounts_add(saved_details.name, saved_details.altdomain, saved_details.port, saved_details.tls_policy);
        accounts_set_jid(saved_details.name, saved_details.jid);

        sv_ev_login_account_success(saved_details.name, secured);
        saved_account.name = strdup(saved_details.name);
        saved_account.passwd = strdup(saved_details.passwd);

        _session_free_saved_details();
    }

    chat_sessions_init();

    message_handlers_init();
    presence_handlers_init();
    iq_handlers_init();

    roster_request();
    bookmark_request();
    blocking_request();

    // items discovery
    char *domain = connection_get_domain();
    iq_disco_info_request_onconnect(domain);
    iq_disco_items_request_onconnect(domain);

    if (prefs_get_boolean(PREF_CARBONS)){
        iq_enable_carbons();
    }

    if ((prefs_get_reconnect() != 0) && reconnect_timer) {
        g_timer_destroy(reconnect_timer);
        reconnect_timer = NULL;
    }
}

void
session_login_failed(void)
{
    if (reconnect_timer == NULL) {
        log_debug("Connection handler: No reconnect timer");
        sv_ev_failed_login();
        _session_free_saved_account();
        _session_free_saved_details();
    } else {
        log_debug("Connection handler: Restarting reconnect timer");
        if (prefs_get_reconnect() != 0) {
            g_timer_start(reconnect_timer);
        }
    }

    connection_clear_data();
    chat_sessions_clear();
    presence_clear_sub_requests();
}

void
session_lost_connection(void)
{
    sv_ev_lost_connection();
    if (prefs_get_reconnect() != 0) {
        assert(reconnect_timer == NULL);
        reconnect_timer = g_timer_new();
    } else {
        _session_free_saved_account();
        _session_free_saved_details();
    }

    connection_clear_data();
    chat_sessions_clear();
    presence_clear_sub_requests();
}

void
session_init_activity(void)
{
    activity_state = ACTIVITY_ST_ACTIVE;
    saved_status = NULL;
}

void
session_check_autoaway(void)
{
    jabber_conn_status_t conn_status = connection_get_status();
    if (conn_status != JABBER_CONNECTED) {
        return;
    }

    char *mode = prefs_get_string(PREF_AUTOAWAY_MODE);
    gboolean check = prefs_get_boolean(PREF_AUTOAWAY_CHECK);
    gint away_time = prefs_get_autoaway_time();
    gint xa_time = prefs_get_autoxa_time();
    int away_time_ms = away_time * 60000;
    int xa_time_ms = xa_time * 60000;

    char *account = session_get_account_name();
    resource_presence_t curr_presence = accounts_get_last_presence(account);
    char *curr_status = accounts_get_last_status(account);

    unsigned long idle_ms = ui_get_idle_time();

    switch (activity_state) {
    case ACTIVITY_ST_ACTIVE:
        if (idle_ms >= away_time_ms) {
            if (g_strcmp0(mode, "away") == 0) {
                if ((curr_presence == RESOURCE_ONLINE) || (curr_presence == RESOURCE_CHAT) || (curr_presence == RESOURCE_DND)) {
                    activity_state = ACTIVITY_ST_AWAY;

                    // save current presence
                    saved_presence = curr_presence;
                    if (saved_status) {
                        free(saved_status);
                    }
                    if (curr_status) {
                        saved_status = strdup(curr_status);
                    } else {
                        saved_status = NULL;
                    }

                    // send away presence with last activity
                    char *message = prefs_get_string(PREF_AUTOAWAY_MESSAGE);
                    connection_set_presence_msg(message);
                    if (prefs_get_boolean(PREF_LASTACTIVITY)) {
                        cl_ev_presence_send(RESOURCE_AWAY, idle_ms / 1000);
                    } else {
                        cl_ev_presence_send(RESOURCE_AWAY, 0);
                    }

                    int pri = accounts_get_priority_for_presence_type(account, RESOURCE_AWAY);
                    if (message) {
                        cons_show("Idle for %d minutes, status set to away (priority %d), \"%s\".", away_time, pri, message);
                    } else {
                        cons_show("Idle for %d minutes, status set to away (priority %d).", away_time, pri);
                    }
                    prefs_free_string(message);

                    title_bar_set_presence(CONTACT_AWAY);
                }
            } else if (g_strcmp0(mode, "idle") == 0) {
                activity_state = ACTIVITY_ST_IDLE;

                // send current presence with last activity
                connection_set_presence_msg(curr_status);
                cl_ev_presence_send(curr_presence, idle_ms / 1000);
            }
        }
        break;
    case ACTIVITY_ST_IDLE:
        if (check && (idle_ms < away_time_ms)) {
            activity_state = ACTIVITY_ST_ACTIVE;

            cons_show("No longer idle.");

            // send current presence without last activity
            connection_set_presence_msg(curr_status);
            cl_ev_presence_send(curr_presence, 0);
        }
        break;
    case ACTIVITY_ST_AWAY:
        if (xa_time_ms > 0 && (idle_ms >= xa_time_ms)) {
            activity_state = ACTIVITY_ST_XA;

            // send extended away presence with last activity
            char *message = prefs_get_string(PREF_AUTOXA_MESSAGE);
            connection_set_presence_msg(message);
            if (prefs_get_boolean(PREF_LASTACTIVITY)) {
                cl_ev_presence_send(RESOURCE_XA, idle_ms / 1000);
            } else {
                cl_ev_presence_send(RESOURCE_XA, 0);
            }

            int pri = accounts_get_priority_for_presence_type(account, RESOURCE_XA);
            if (message) {
                cons_show("Idle for %d minutes, status set to xa (priority %d), \"%s\".", xa_time, pri, message);
            } else {
                cons_show("Idle for %d minutes, status set to xa (priority %d).", xa_time, pri);
            }
            prefs_free_string(message);

            title_bar_set_presence(CONTACT_XA);
        } else if (check && (idle_ms < away_time_ms)) {
            activity_state = ACTIVITY_ST_ACTIVE;

            cons_show("No longer idle.");

            // send saved presence without last activity
            connection_set_presence_msg(saved_status);
            cl_ev_presence_send(saved_presence, 0);
            contact_presence_t contact_pres = contact_presence_from_resource_presence(saved_presence);
            title_bar_set_presence(contact_pres);
        }
        break;
    case ACTIVITY_ST_XA:
        if (check && (idle_ms < away_time_ms)) {
            activity_state = ACTIVITY_ST_ACTIVE;

            cons_show("No longer idle.");

            // send saved presence without last activity
            connection_set_presence_msg(saved_status);
            cl_ev_presence_send(saved_presence, 0);
            contact_presence_t contact_pres = contact_presence_from_resource_presence(saved_presence);
            title_bar_set_presence(contact_pres);
        }
        break;
    }

    free(curr_status);
    prefs_free_string(mode);
}

static void
_session_reconnect(void)
{
    // reconnect with account.
    ProfAccount *account = accounts_get_account(saved_account.name);
    if (account == NULL) {
        log_error("Unable to reconnect, account no longer exists: %s", saved_account.name);
        return;
    }

    char *jid = NULL;
    if (account->resource) {
        jid = create_fulljid(account->jid, account->resource);
    } else {
        jid = strdup(account->jid);
    }

    log_debug("Attempting reconnect with account %s", account->name);
    connection_connect(jid, saved_account.passwd, account->server, account->port, account->tls_policy);
    free(jid);
    account_free(account);
    g_timer_start(reconnect_timer);
}

static void
_session_free_saved_account(void)
{
    FREE_SET_NULL(saved_account.name);
    FREE_SET_NULL(saved_account.passwd);
}

static void
_session_free_saved_details(void)
{
    FREE_SET_NULL(saved_details.name);
    FREE_SET_NULL(saved_details.jid);
    FREE_SET_NULL(saved_details.passwd);
    FREE_SET_NULL(saved_details.altdomain);
    FREE_SET_NULL(saved_details.tls_policy);
}

