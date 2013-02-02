/*
 * contact_list.c
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

#include <glib.h>

#include "contact.h"
#include "tools/autocomplete.h"

static Autocomplete ac;
static GHashTable *contacts;

static gboolean _key_equals(void *key1, void *key2);
static gboolean _datetimes_equal(GDateTime *dt1, GDateTime *dt2);

void
contact_list_init(void)
{
    ac = autocomplete_new();
    contacts = g_hash_table_new_full(g_str_hash, (GEqualFunc)_key_equals, g_free,
        (GDestroyNotify)p_contact_free);
}

void
contact_list_clear(void)
{
    autocomplete_clear(ac);
    g_hash_table_remove_all(contacts);
}

void
contact_list_free()
{
    autocomplete_free(ac);
}

void
contact_list_reset_search_attempts(void)
{
    autocomplete_reset(ac);
}

gboolean
contact_list_add(const char * const jid, const char * const name,
    const char * const presence, const char * const status,
    const char * const subscription, gboolean pending_out)
{
    gboolean added = FALSE;
    PContact contact = g_hash_table_lookup(contacts, jid);

    if (contact == NULL) {
        contact = p_contact_new(jid, name, presence, status, subscription,
            pending_out, NULL);
        g_hash_table_insert(contacts, strdup(jid), contact);
        autocomplete_add(ac, strdup(jid));
        added = TRUE;
    }

    return added;
}

void
contact_list_remove(const char * const jid)
{
    g_hash_table_remove(contacts, jid);
}

gboolean
contact_list_update_contact(const char * const jid, const char * const presence,
    const char * const status, GDateTime *last_activity, const char * const caps_str)
{
    gboolean presence_changed = FALSE;
    PContact contact = g_hash_table_lookup(contacts, jid);

    if (contact == NULL) {
        return FALSE;
    }

    if (g_strcmp0(p_contact_presence(contact), presence) != 0) {
        p_contact_set_presence(contact, presence);
        presence_changed = TRUE;
    }

    if (g_strcmp0(p_contact_status(contact), status) != 0) {
        p_contact_set_status(contact, status);
        presence_changed = TRUE;
    }

    if (!_datetimes_equal(p_contact_last_activity(contact), last_activity)) {
        p_contact_set_last_activity(contact, last_activity);
        presence_changed = TRUE;
    }

    if (g_strcmp0(p_contact_caps_str(contact), caps_str) != 0) {
        p_contact_set_caps_str(contact, caps_str);
    }

    return presence_changed;
}

void
contact_list_update_subscription(const char * const jid,
    const char * const subscription, gboolean pending_out)
{
    PContact contact = g_hash_table_lookup(contacts, jid);

    if (contact == NULL) {
        contact = p_contact_new(jid, NULL, "offline", NULL, subscription,
            pending_out, NULL);
        g_hash_table_insert(contacts, strdup(jid), contact);
    } else {
        p_contact_set_subscription(contact, subscription);
        p_contact_set_pending_out(contact, pending_out);
    }
}

gboolean
contact_list_has_pending_subscriptions(void)
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
get_contact_list(void)
{
    GSList *result = NULL;
    GHashTableIter iter;
    gpointer key;
    gpointer value;

    g_hash_table_iter_init(&iter, contacts);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        result = g_slist_append(result, value);
    }

    // resturn all contact structs
    return result;
}

char *
contact_list_find_contact(char *search_str)
{
    return autocomplete_complete(ac, search_str);
}

PContact
contact_list_get_contact(const char const *jid)
{
    return g_hash_table_lookup(contacts, jid);
}

static
gboolean _key_equals(void *key1, void *key2)
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

