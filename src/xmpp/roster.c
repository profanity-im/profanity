/*
 * roster.c
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

#include <assert.h>
#include <string.h>

#include <glib.h>
#include <strophe.h>

#include "log.h"
#include "profanity.h"
#include "tools/autocomplete.h"
#include "xmpp/connection.h"
#include "xmpp/roster.h"
#include "xmpp/stanza.h"
#include "xmpp/xmpp.h"

#define HANDLE(type, func) xmpp_handler_add(conn, func, XMPP_NS_ROSTER, \
STANZA_NAME_IQ, type, ctx)

// nicknames
static Autocomplete name_ac;

// barejids
static Autocomplete barejid_ac;

// fulljids
static Autocomplete fulljid_ac;

// groups
static Autocomplete groups_ac;

// contacts, indexed on barejid
static GHashTable *contacts;

// nickname to jid map
static GHashTable *name_to_barejid;

// event handlers
static int _roster_handle_push(xmpp_conn_t * const conn,
    xmpp_stanza_t * const stanza, void * const userdata);
static int _roster_handle_result(xmpp_conn_t * const conn,
    xmpp_stanza_t * const stanza, void * const userdata);

// helper functions
static void _add_name_and_barejid(const char * const name,
    const char * const barejid);
static void _replace_name(const char * const current_name,
    const char * const new_name, const char * const barejid);
GSList * _get_groups_from_item(xmpp_stanza_t *item);
static gboolean _key_equals(void *key1, void *key2);
static gboolean _datetimes_equal(GDateTime *dt1, GDateTime *dt2);
static gint _compare_contacts(PContact a, PContact b);

void
roster_add_handlers(void)
{
    xmpp_conn_t * const conn = connection_get_conn();
    xmpp_ctx_t * const ctx = connection_get_ctx();
    HANDLE(STANZA_TYPE_SET,    _roster_handle_push);
    HANDLE(STANZA_TYPE_RESULT, _roster_handle_result);
}

void
roster_request(void)
{
    xmpp_conn_t * const conn = connection_get_conn();
    xmpp_ctx_t * const ctx = connection_get_ctx();
    xmpp_stanza_t *iq = stanza_create_roster_iq(ctx);
    xmpp_send(conn, iq);
    xmpp_stanza_release(iq);
}

void
roster_init(void)
{
    name_ac = autocomplete_new();
    barejid_ac = autocomplete_new();
    fulljid_ac = autocomplete_new();
    groups_ac = autocomplete_new();
    contacts = g_hash_table_new_full(g_str_hash, (GEqualFunc)_key_equals, g_free,
        (GDestroyNotify)p_contact_free);
    name_to_barejid = g_hash_table_new_full(g_str_hash, g_str_equal, g_free,
        g_free);
}

void
roster_clear(void)
{
    autocomplete_clear(name_ac);
    autocomplete_clear(barejid_ac);
    autocomplete_clear(fulljid_ac);
    autocomplete_clear(groups_ac);
    g_hash_table_destroy(contacts);
    contacts = g_hash_table_new_full(g_str_hash, (GEqualFunc)_key_equals, g_free,
        (GDestroyNotify)p_contact_free);
    g_hash_table_destroy(name_to_barejid);
    name_to_barejid = g_hash_table_new_full(g_str_hash, g_str_equal, g_free,
        g_free);
}

void
roster_free()
{
    autocomplete_free(name_ac);
    autocomplete_free(barejid_ac);
    autocomplete_free(fulljid_ac);
    autocomplete_free(groups_ac);
}

void
roster_reset_search_attempts(void)
{
    autocomplete_reset(name_ac);
    autocomplete_reset(barejid_ac);
    autocomplete_reset(fulljid_ac);
    autocomplete_reset(groups_ac);
}

void
roster_add_new(const char * const barejid, const char * const name)
{
    xmpp_conn_t * const conn = connection_get_conn();
    xmpp_ctx_t * const ctx = connection_get_ctx();
    xmpp_stanza_t *iq = stanza_create_roster_set(ctx, barejid, name, NULL);
    xmpp_send(conn, iq);
    xmpp_stanza_release(iq);
}

void
roster_remove(const char * const barejid)
{
    xmpp_conn_t * const conn = connection_get_conn();
    xmpp_ctx_t * const ctx = connection_get_ctx();
    xmpp_stanza_t *iq = stanza_create_roster_remove_set(ctx, barejid);
    xmpp_send(conn, iq);
    xmpp_stanza_release(iq);
}

gboolean
roster_add(const char * const barejid, const char * const name, GSList *groups,
    const char * const subscription, gboolean pending_out, gboolean from_initial)
{
    gboolean added = FALSE;
    PContact contact = g_hash_table_lookup(contacts, barejid);

    if (contact == NULL) {
        contact = p_contact_new(barejid, name, groups, subscription, NULL,
            pending_out);

        // add groups
        while (groups != NULL) {
            autocomplete_add(groups_ac, strdup(groups->data));
            groups = g_slist_next(groups);
        }

        g_hash_table_insert(contacts, strdup(barejid), contact);
        autocomplete_add(barejid_ac, strdup(barejid));
        _add_name_and_barejid(name, barejid);

        if (!from_initial) {
            prof_handle_roster_add(barejid, name);
        }

        added = TRUE;
    }

    return added;
}

void
roster_update(const char * const barejid, const char * const name,
    GSList *groups, const char * const subscription, gboolean pending_out)
{
    PContact contact = g_hash_table_lookup(contacts, barejid);

    if (contact == NULL) {
        roster_add(barejid, name, groups, subscription, pending_out, FALSE);
    } else {
        p_contact_set_subscription(contact, subscription);
        p_contact_set_pending_out(contact, pending_out);

        const char * const new_name = name;
        const char * current_name = NULL;
        if (p_contact_name(contact) != NULL) {
            current_name = strdup(p_contact_name(contact));
        }

        p_contact_set_name(contact, new_name);
        p_contact_set_groups(contact, groups);
        _replace_name(current_name, new_name, barejid);

        // add groups
        while (groups != NULL) {
            autocomplete_add(groups_ac, strdup(groups->data));
            groups = g_slist_next(groups);
        }
    }
}

gboolean
roster_update_presence(const char * const barejid, Resource *resource,
    GDateTime *last_activity)
{
    assert(barejid != NULL);
    assert(resource != NULL);

    PContact contact = g_hash_table_lookup(contacts, barejid);
    if (contact == NULL) {
        return FALSE;
    }
    if (!_datetimes_equal(p_contact_last_activity(contact), last_activity)) {
        p_contact_set_last_activity(contact, last_activity);
    }
    p_contact_set_presence(contact, resource);
    Jid *jid = jid_create_from_bare_and_resource(barejid, resource->name);
    autocomplete_add(fulljid_ac, strdup(jid->fulljid));
    jid_destroy(jid);

    return TRUE;
}

gboolean
roster_contact_offline(const char * const barejid,
    const char * const resource, const char * const status)
{
    PContact contact = g_hash_table_lookup(contacts, barejid);

    if (contact == NULL) {
        return FALSE;
    }
    if (resource == NULL) {
        return TRUE;
    } else {
        gboolean result = p_contact_remove_resource(contact, resource);
        if (result == TRUE) {
            Jid *jid = jid_create_from_bare_and_resource(barejid, resource);
            autocomplete_remove(fulljid_ac, jid->fulljid);
            jid_destroy(jid);
        }

        return result;
    }
}

void
roster_change_name(const char * const barejid, const char * const new_name)
{
    PContact contact = g_hash_table_lookup(contacts, barejid);
    const char * current_name = NULL;
    if (p_contact_name(contact) != NULL) {
        current_name = strdup(p_contact_name(contact));
    }

    if (contact != NULL) {
        p_contact_set_name(contact, new_name);
        _replace_name(current_name, new_name, barejid);

        GSList *groups = p_contact_groups(contact);

        xmpp_conn_t * const conn = connection_get_conn();
        xmpp_ctx_t * const ctx = connection_get_ctx();
        xmpp_stanza_t *iq = stanza_create_roster_set(ctx, barejid, new_name,
            groups);
        xmpp_send(conn, iq);
        xmpp_stanza_release(iq);
    }
}

gboolean
roster_has_pending_subscriptions(void)
{
    GHashTableIter iter;
    gpointer key;
    gpointer value;

    g_hash_table_iter_init(&iter, contacts);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        PContact contact = (PContact) value;
        if (p_contact_pending_out(contact)) {
            return TRUE;
        }
    }

    return FALSE;
}

GSList *
roster_get_contacts(void)
{
    GSList *result = NULL;
    GHashTableIter iter;
    gpointer key;
    gpointer value;

    g_hash_table_iter_init(&iter, contacts);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        result = g_slist_insert_sorted(result, value, (GCompareFunc)_compare_contacts);
    }

    // resturn all contact structs
    return result;
}

char *
roster_find_contact(char *search_str)
{
    return autocomplete_complete(name_ac, search_str);
}

char *
roster_find_jid(char *search_str)
{
    return autocomplete_complete(barejid_ac, search_str);
}

char *
roster_find_resource(char *search_str)
{
    return autocomplete_complete(fulljid_ac, search_str);
}

char *
roster_barejid_from_name(const char * const name)
{
    return g_hash_table_lookup(name_to_barejid, name);
}

PContact
roster_get_contact(const char const *barejid)
{
    return g_hash_table_lookup(contacts, barejid);
}

static int
_roster_handle_push(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza,
    void * const userdata)
{
    xmpp_stanza_t *query =
        xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_QUERY);
    xmpp_stanza_t *item =
        xmpp_stanza_get_child_by_name(query, STANZA_NAME_ITEM);

    if (item == NULL) {
        return 1;
    }

    // if from attribute exists and it is not current users barejid, ignore push
    Jid *my_jid = jid_create(jabber_get_fulljid());
    const char *from = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_FROM);
    if ((from != NULL) && (strcmp(from, my_jid->barejid) != 0)) {
        jid_destroy(my_jid);
        return 1;
    }
    jid_destroy(my_jid);

    const char *barejid = xmpp_stanza_get_attribute(item, STANZA_ATTR_JID);
    const char *name = xmpp_stanza_get_attribute(item, STANZA_ATTR_NAME);
    const char *sub = xmpp_stanza_get_attribute(item, STANZA_ATTR_SUBSCRIPTION);
    const char *ask = xmpp_stanza_get_attribute(item, STANZA_ATTR_ASK);

    // remove from roster
    if (g_strcmp0(sub, "remove") == 0) {
        // remove barejid and name
        if (name == NULL) {
            name = barejid;
        }
        autocomplete_remove(barejid_ac, barejid);
        autocomplete_remove(name_ac, name);
        g_hash_table_remove(name_to_barejid, name);

        // remove each fulljid
        PContact contact = roster_get_contact(barejid);
        GList *resources = p_contact_get_available_resources(contact);
        while (resources != NULL) {
            GString *fulljid = g_string_new(strdup(barejid));
            g_string_append(fulljid, "/");
            g_string_append(fulljid, strdup(resources->data));
            autocomplete_remove(fulljid_ac, fulljid->str);
            g_string_free(fulljid, TRUE);
            resources = g_list_next(resources);
        }

        // remove the contact
        g_hash_table_remove(contacts, barejid);

    // otherwise update local roster
    } else {

        // check for pending out subscriptions
        gboolean pending_out = FALSE;
        if ((ask != NULL) && (strcmp(ask, "subscribe") == 0)) {
            pending_out = TRUE;
        }

        GSList *groups = _get_groups_from_item(item);

        // update the local roster
        roster_update(barejid, name, groups, sub, pending_out);
    }

    return 1;
}

static int
_roster_handle_result(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza,
    void * const userdata)
{
    const char *id = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_ID);

    // handle initial roster response
    if (g_strcmp0(id, "roster") == 0) {
        xmpp_stanza_t *query = xmpp_stanza_get_child_by_name(stanza,
            STANZA_NAME_QUERY);
        xmpp_stanza_t *item = xmpp_stanza_get_children(query);

        while (item != NULL) {
            const char *barejid =
                xmpp_stanza_get_attribute(item, STANZA_ATTR_JID);
            const char *name =
                xmpp_stanza_get_attribute(item, STANZA_ATTR_NAME);
            const char *sub =
                xmpp_stanza_get_attribute(item, STANZA_ATTR_SUBSCRIPTION);

            gboolean pending_out = FALSE;
            const char *ask = xmpp_stanza_get_attribute(item, STANZA_ATTR_ASK);
            if (g_strcmp0(ask, "subscribe") == 0) {
                pending_out = TRUE;
            }

            GSList *groups = _get_groups_from_item(item);

            gboolean added = roster_add(barejid, name, groups, sub, pending_out, TRUE);

            if (!added) {
                log_warning("Attempt to add contact twice: %s", barejid);
            }

            item = xmpp_stanza_get_next(item);
        }

        contact_presence_t conn_presence =
            accounts_get_login_presence(jabber_get_account_name());
        presence_update(conn_presence, NULL, 0);
    }

    return 1;
}

static void
_add_name_and_barejid(const char * const name, const char * const barejid)
{
    if (name != NULL) {
        autocomplete_add(name_ac, strdup(name));
        g_hash_table_insert(name_to_barejid, strdup(name), strdup(barejid));
    } else {
        autocomplete_add(name_ac, strdup(barejid));
        g_hash_table_insert(name_to_barejid, strdup(barejid), strdup(barejid));
    }
}

static void
_replace_name(const char * const current_name, const char * const new_name,
    const char * const barejid)
{
    // current handle exists already
    if (current_name != NULL) {
        autocomplete_remove(name_ac, current_name);
        g_hash_table_remove(name_to_barejid, current_name);
        _add_name_and_barejid(new_name, barejid);
    // no current handle
    } else if (new_name != NULL) {
        autocomplete_remove(name_ac, barejid);
        g_hash_table_remove(name_to_barejid, barejid);
        _add_name_and_barejid(new_name, barejid);
    }
}

GSList *
_get_groups_from_item(xmpp_stanza_t *item)
{
    GSList *groups = NULL;
    xmpp_stanza_t *group_element = xmpp_stanza_get_children(item);

    while (group_element != NULL) {
        if (strcmp(xmpp_stanza_get_name(group_element), STANZA_NAME_GROUP) == 0) {
            char *groupname = xmpp_stanza_get_text(group_element);
            if (groupname != NULL) {
                groups = g_slist_append(groups, groupname);
            }
        }
        group_element = xmpp_stanza_get_next(group_element);
    }

    return groups;
}

static
gboolean _key_equals(void *key1, void *key2)
{
    gchar *str1 = (gchar *) key1;
    gchar *str2 = (gchar *) key2;

    return (g_strcmp0(str1, str2) == 0);
}

static
gint _compare_contacts(PContact a, PContact b)
{
    const char * utf8_str_a = NULL;
    const char * utf8_str_b = NULL;

    if (p_contact_name(a) != NULL) {
        utf8_str_a = p_contact_name(a);
    } else {
        utf8_str_a = p_contact_barejid(a);
    }
    if (p_contact_name(b) != NULL) {
        utf8_str_b = p_contact_name(b);
    } else {
        utf8_str_b = p_contact_barejid(b);
    }

    gchar *key_a = g_utf8_collate_key(utf8_str_a, -1);
    gchar *key_b = g_utf8_collate_key(utf8_str_b, -1);

    gint result = g_strcmp0(key_a, key_b);

    g_free(key_a);
    g_free(key_b);

    return result;
}

static gboolean
_datetimes_equal(GDateTime *dt1, GDateTime *dt2)
{
    if ((dt1 == NULL) && (dt2 == NULL)) {
        return TRUE;
    } else if ((dt1 == NULL) && (dt2 != NULL)) {
        return FALSE;
    } else if ((dt1 != NULL) && (dt2 == NULL)) {
        return FALSE;
    } else {
        return g_date_time_equal(dt1, dt2);
    }
}
