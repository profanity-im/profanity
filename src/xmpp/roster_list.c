/*
 * roster_list.c
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


#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <glib.h>
#include <assert.h>

#include "config/preferences.h"
#include "tools/autocomplete.h"
#include "xmpp/roster_list.h"
#include "xmpp/resource.h"
#include "xmpp/contact.h"
#include "xmpp/jid.h"

typedef struct prof_roster_t {
    // contacts, indexed on barejid
    GHashTable *contacts;

    // nicknames
    Autocomplete name_ac;

    // barejids
    Autocomplete barejid_ac;

    // fulljids
    Autocomplete fulljid_ac;

    // nickname to barejid map
    GHashTable *name_to_barejid;

    // groups
    Autocomplete groups_ac;
    GHashTable *group_count;
} ProfRoster;

static ProfRoster *roster = NULL;

static gboolean _key_equals(void *key1, void *key2);
static gboolean _datetimes_equal(GDateTime *dt1, GDateTime *dt2);
static void _replace_name(const char *const current_name, const char *const new_name, const char *const barejid);
static void _add_name_and_barejid(const char *const name, const char *const barejid);
static gint _compare_name(PContact a, PContact b);
static gint _compare_presence(PContact a, PContact b);

void
roster_create(void)
{
    assert(roster == NULL);

    roster = malloc(sizeof(ProfRoster));
    roster->contacts = g_hash_table_new_full(g_str_hash, (GEqualFunc)_key_equals, g_free, (GDestroyNotify)p_contact_free);
    roster->name_ac = autocomplete_new();
    roster->barejid_ac = autocomplete_new();
    roster->fulljid_ac = autocomplete_new();
    roster->name_to_barejid = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    roster->groups_ac = autocomplete_new();
    roster->group_count = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
}

void
roster_destroy(void)
{
    assert(roster != NULL);

    g_hash_table_destroy(roster->contacts);
    autocomplete_free(roster->name_ac);
    autocomplete_free(roster->barejid_ac);
    autocomplete_free(roster->fulljid_ac);
    g_hash_table_destroy(roster->name_to_barejid);
    autocomplete_free(roster->groups_ac);
    g_hash_table_destroy(roster->group_count);

    free(roster);
    roster = NULL;
}

gboolean
roster_update_presence(const char *const barejid, Resource *resource, GDateTime *last_activity)
{
    assert(roster != NULL);

    assert(barejid != NULL);
    assert(resource != NULL);

    PContact contact = roster_get_contact(barejid);
    if (contact == NULL) {
        return FALSE;
    }
    if (!_datetimes_equal(p_contact_last_activity(contact), last_activity)) {
        p_contact_set_last_activity(contact, last_activity);
    }
    p_contact_set_presence(contact, resource);
    Jid *jid = jid_create_from_bare_and_resource(barejid, resource->name);
    autocomplete_add(roster->fulljid_ac, jid->fulljid);
    jid_destroy(jid);

    return TRUE;
}

PContact
roster_get_contact(const char *const barejid)
{
    assert(roster != NULL);

    gchar *barejidlower = g_utf8_strdown(barejid, -1);
    PContact contact = g_hash_table_lookup(roster->contacts, barejidlower);
    g_free(barejidlower);

    return contact;
}

char*
roster_get_msg_display_name(const char *const barejid, const char *const resource)
{
    assert(roster != NULL);

    GString *result = g_string_new("");

    PContact contact = roster_get_contact(barejid);
    if (contact) {
        if (p_contact_name(contact)) {
            g_string_append(result, p_contact_name(contact));
        } else {
            g_string_append(result, barejid);
        }
    } else {
        g_string_append(result, barejid);
    }

    if (resource && prefs_get_boolean(PREF_RESOURCE_MESSAGE)) {
        g_string_append(result, "/");
        g_string_append(result, resource);
    }

    char *result_str = result->str;
    g_string_free(result, FALSE);

    return result_str;
}

gboolean
roster_contact_offline(const char *const barejid, const char *const resource, const char *const status)
{
    assert(roster != NULL);

    PContact contact = roster_get_contact(barejid);

    if (contact == NULL) {
        return FALSE;
    }
    if (resource == NULL) {
        return TRUE;
    } else {
        gboolean result = p_contact_remove_resource(contact, resource);
        if (result == TRUE) {
            Jid *jid = jid_create_from_bare_and_resource(barejid, resource);
            autocomplete_remove(roster->fulljid_ac, jid->fulljid);
            jid_destroy(jid);
        }

        return result;
    }
}

void
roster_reset_search_attempts(void)
{
    assert(roster != NULL);

    autocomplete_reset(roster->name_ac);
    autocomplete_reset(roster->barejid_ac);
    autocomplete_reset(roster->fulljid_ac);
    autocomplete_reset(roster->groups_ac);
}

void
roster_change_name(PContact contact, const char *const new_name)
{
    assert(roster != NULL);

    assert(contact != NULL);

    const char *current_name = NULL;
    const char *barejid = p_contact_barejid(contact);

    if (p_contact_name(contact)) {
        current_name = strdup(p_contact_name(contact));
    }

    p_contact_set_name(contact, new_name);
    _replace_name(current_name, new_name, barejid);
}

void
roster_remove(const char *const name, const char *const barejid)
{
    assert(roster != NULL);

    autocomplete_remove(roster->barejid_ac, barejid);
    autocomplete_remove(roster->name_ac, name);
    g_hash_table_remove(roster->name_to_barejid, name);

    // remove each fulljid
    PContact contact = roster_get_contact(barejid);
    if (contact) {
        GList *resources = p_contact_get_available_resources(contact);
        while (resources) {
            GString *fulljid = g_string_new(strdup(barejid));
            g_string_append(fulljid, "/");
            g_string_append(fulljid, resources->data);
            autocomplete_remove(roster->fulljid_ac, fulljid->str);
            g_string_free(fulljid, TRUE);
            resources = g_list_next(resources);
        }
        g_list_free(resources);

        GSList *groups = p_contact_groups(contact);
        GSList *curr = groups;
        while (curr) {
            gchar *group = curr->data;
            if (g_hash_table_contains(roster->group_count, group)) {
                int count = GPOINTER_TO_INT(g_hash_table_lookup(roster->group_count, group));
                count--;
                if (count < 1) {
                    g_hash_table_remove(roster->group_count, group);
                    autocomplete_remove(roster->groups_ac, group);
                } else {
                    g_hash_table_insert(roster->group_count, strdup(group), GINT_TO_POINTER(count));
                }
            }
            curr = g_slist_next(curr);
        }
    }

    // remove the contact
    g_hash_table_remove(roster->contacts, barejid);
}

void
roster_update(const char *const barejid, const char *const name, GSList *groups, const char *const subscription,
    gboolean pending_out)
{
    assert(roster != NULL);

    PContact contact = roster_get_contact(barejid);
    assert(contact != NULL);

    p_contact_set_subscription(contact, subscription);
    p_contact_set_pending_out(contact, pending_out);

    const char * const new_name = name;
    const char * current_name = NULL;
    if (p_contact_name(contact)) {
        current_name = strdup(p_contact_name(contact));
    }

    p_contact_set_name(contact, new_name);
    _replace_name(current_name, new_name, barejid);

    GSList *curr_new_group = groups;
    while (curr_new_group) {
        char *new_group = curr_new_group->data;

        // contact added to group
        if (!p_contact_in_group(contact, new_group)) {

            // group doesn't yet exist
            if (!g_hash_table_contains(roster->group_count, new_group)) {
                g_hash_table_insert(roster->group_count, strdup(new_group), GINT_TO_POINTER(1));
                autocomplete_add(roster->groups_ac, curr_new_group->data);

            // increment count
            } else {
                int count = GPOINTER_TO_INT(g_hash_table_lookup(roster->group_count, new_group));
                g_hash_table_insert(roster->group_count, strdup(new_group), GINT_TO_POINTER(count + 1));
            }
        }
        curr_new_group = g_slist_next(curr_new_group);
    }

    GSList *old_groups = p_contact_groups(contact);
    GSList *curr_old_group = old_groups;
    while (curr_old_group) {
        char *old_group = curr_old_group->data;
        // removed from group
        if (!g_slist_find_custom(groups, old_group, (GCompareFunc)g_strcmp0)) {
            if (g_hash_table_contains(roster->group_count, old_group)) {
                int count = GPOINTER_TO_INT(g_hash_table_lookup(roster->group_count, old_group));
                count--;
                if (count < 1) {
                    g_hash_table_remove(roster->group_count, old_group);
                    autocomplete_remove(roster->groups_ac, old_group);
                } else {
                    g_hash_table_insert(roster->group_count, strdup(old_group), GINT_TO_POINTER(count));
                }
            }
        }

        curr_old_group = g_slist_next(curr_old_group);
    }

    p_contact_set_groups(contact, groups);
}

gboolean
roster_add(const char *const barejid, const char *const name, GSList *groups, const char *const subscription,
    gboolean pending_out)
{
    assert(roster != NULL);

    PContact contact = roster_get_contact(barejid);
    if (contact) {
        return FALSE;
    }

    contact = p_contact_new(barejid, name, groups, subscription, NULL, pending_out);

    // add groups
    GSList *curr_new_group = groups;
    while (curr_new_group) {
        char *new_group = curr_new_group->data;
        if (g_hash_table_contains(roster->group_count, new_group)) {
            int count = GPOINTER_TO_INT(g_hash_table_lookup(roster->group_count, new_group));
            g_hash_table_insert(roster->group_count, strdup(new_group), GINT_TO_POINTER(count + 1));
        } else {
            g_hash_table_insert(roster->group_count, strdup(new_group), GINT_TO_POINTER(1));
            autocomplete_add(roster->groups_ac, new_group);
        }

        curr_new_group = g_slist_next(curr_new_group);
    }

    g_hash_table_insert(roster->contacts, strdup(barejid), contact);
    autocomplete_add(roster->barejid_ac, barejid);
    _add_name_and_barejid(name, barejid);

    return TRUE;
}

char*
roster_barejid_from_name(const char *const name)
{
    assert(roster != NULL);

    if (name) {
        return g_hash_table_lookup(roster->name_to_barejid, name);
    } else {
        return NULL;
    }
}

GSList*
roster_get_contacts_by_presence(const char *const presence)
{
    assert(roster != NULL);

    GSList *result = NULL;
    GHashTableIter iter;
    gpointer key;
    gpointer value;

    g_hash_table_iter_init(&iter, roster->contacts);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        PContact contact = (PContact)value;
        if (g_strcmp0(p_contact_presence(contact), presence) == 0) {
            result = g_slist_insert_sorted(result, value, (GCompareFunc)_compare_name);
        }
    }

    // return all contact structs
    return result;
}

GSList*
roster_get_contacts(roster_ord_t order)
{
    assert(roster != NULL);

    GSList *result = NULL;
    GHashTableIter iter;
    gpointer key;
    gpointer value;

    GCompareFunc cmp_func;
    if (order == ROSTER_ORD_PRESENCE) {
        cmp_func = (GCompareFunc) _compare_presence;
    } else {
        cmp_func = (GCompareFunc) _compare_name;
    }

    g_hash_table_iter_init(&iter, roster->contacts);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        result = g_slist_insert_sorted(result, value, cmp_func);
    }

    // return all contact structs
    return result;
}

GSList*
roster_get_contacts_online(void)
{
    assert(roster != NULL);

    GSList *result = NULL;
    GHashTableIter iter;
    gpointer key;
    gpointer value;

    g_hash_table_iter_init(&iter, roster->contacts);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        if(strcmp(p_contact_presence(value), "offline"))
            result = g_slist_insert_sorted(result, value, (GCompareFunc)_compare_name);
    }

    // return all contact structs
    return result;
}

gboolean
roster_has_pending_subscriptions(void)
{
    assert(roster != NULL);

    GHashTableIter iter;
    gpointer key;
    gpointer value;

    g_hash_table_iter_init(&iter, roster->contacts);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        PContact contact = (PContact) value;
        if (p_contact_pending_out(contact)) {
            return TRUE;
        }
    }

    return FALSE;
}

char*
roster_contact_autocomplete(const char *const search_str, gboolean previous)
{
    assert(roster != NULL);

    return autocomplete_complete(roster->name_ac, search_str, TRUE, previous);
}

char*
roster_fulljid_autocomplete(const char *const search_str, gboolean previous)
{
    assert(roster != NULL);

    return autocomplete_complete(roster->fulljid_ac, search_str, TRUE, previous);
}

GSList*
roster_get_group(const char *const group, roster_ord_t order)
{
    assert(roster != NULL);

    GSList *result = NULL;
    GHashTableIter iter;
    gpointer key;
    gpointer value;

    GCompareFunc cmp_func;
    if (order == ROSTER_ORD_PRESENCE) {
        cmp_func = (GCompareFunc) _compare_presence;
    } else {
        cmp_func = (GCompareFunc) _compare_name;
    }

    g_hash_table_iter_init(&iter, roster->contacts);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        GSList *groups = p_contact_groups(value);
        if (group == NULL) {
            if (groups == NULL) {
                result = g_slist_insert_sorted(result, value, cmp_func);
            }
        } else {
            while (groups) {
                if (strcmp(groups->data, group) == 0) {
                    result = g_slist_insert_sorted(result, value, cmp_func);
                    break;
                }
                groups = g_slist_next(groups);
            }
        }
    }

    // return all contact structs
    return result;
}

GList*
roster_get_groups(void)
{
    assert(roster != NULL);

    return autocomplete_create_list(roster->groups_ac);
}

char*
roster_group_autocomplete(const char *const search_str, gboolean previous)
{
    assert(roster != NULL);

    return autocomplete_complete(roster->groups_ac, search_str, TRUE, previous);
}

char*
roster_barejid_autocomplete(const char *const search_str, gboolean previous)
{
    assert(roster != NULL);

    return autocomplete_complete(roster->barejid_ac, search_str, TRUE, previous);
}

static gboolean
_key_equals(void *key1, void *key2)
{
    gchar *str1 = (gchar *) key1;
    gchar *str2 = (gchar *) key2;

    return (g_strcmp0(str1, str2) == 0);
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

static void
_replace_name(const char *const current_name, const char *const new_name, const char *const barejid)
{
    assert(roster != NULL);

    // current handle exists already
    if (current_name) {
        autocomplete_remove(roster->name_ac, current_name);
        g_hash_table_remove(roster->name_to_barejid, current_name);
        _add_name_and_barejid(new_name, barejid);
    // no current handle
    } else if (new_name) {
        autocomplete_remove(roster->name_ac, barejid);
        g_hash_table_remove(roster->name_to_barejid, barejid);
        _add_name_and_barejid(new_name, barejid);
    }
}

static void
_add_name_and_barejid(const char *const name, const char *const barejid)
{
    assert(roster != NULL);

    if (name) {
        autocomplete_add(roster->name_ac, name);
        g_hash_table_insert(roster->name_to_barejid, strdup(name), strdup(barejid));
    } else {
        autocomplete_add(roster->name_ac, barejid);
        g_hash_table_insert(roster->name_to_barejid, strdup(barejid), strdup(barejid));
    }
}

static gint
_compare_name(PContact a, PContact b)
{
    const char * utf8_str_a = NULL;
    const char * utf8_str_b = NULL;

    if (p_contact_name_collate_key(a)) {
        utf8_str_a = p_contact_name_collate_key(a);
    } else {
        utf8_str_a = p_contact_barejid_collate_key(a);
    }
    if (p_contact_name_collate_key(b)) {
        utf8_str_b = p_contact_name_collate_key(b);
    } else {
        utf8_str_b = p_contact_barejid_collate_key(b);
    }

    gint result = g_strcmp0(utf8_str_a, utf8_str_b);

    return result;
}

static gint
_get_presence_weight(const char *presence)
{
    if (g_strcmp0(presence, "chat") == 0) {
        return 0;
    } else if (g_strcmp0(presence, "online") == 0) {
        return 1;
    } else if (g_strcmp0(presence, "away") == 0) {
        return 2;
    } else if (g_strcmp0(presence, "xa") == 0) {
        return 3;
    } else if (g_strcmp0(presence, "dnd") == 0) {
        return 4;
    } else { // offline
        return 5;
    }
}

static gint
_compare_presence(PContact a, PContact b)
{
    const char *presence_a = p_contact_presence(a);
    const char *presence_b = p_contact_presence(b);

    // if presence different, order by presence
    if (g_strcmp0(presence_a, presence_b) != 0) {
        int weight_a = _get_presence_weight(presence_a);
        int weight_b = _get_presence_weight(presence_b);
        if (weight_a < weight_b) {
            return -1;
        } else {
            return 1;
        }

    // otherwise order by name
    } else {
        return _compare_name(a, b);
    }
}
