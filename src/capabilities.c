/*
 * capabilities.c
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

#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <openssl/evp.h>
#include <strophe.h>

#include "common.h"
#include "capabilities.h"
#include "stanza.h"

static GHashTable *capabilities;

static void _caps_destroy(Capabilities *caps);

void
caps_init(void)
{
    capabilities = g_hash_table_new_full(g_str_hash, g_str_equal, g_free,
        (GDestroyNotify)_caps_destroy);
}

void
caps_add(const char * const caps_str, const char * const client)
{
    Capabilities *new_caps = malloc(sizeof(struct capabilities_t));

    if (client != NULL) {
        new_caps->client = strdup(client);
    } else {
        new_caps->client = NULL;
    }

    g_hash_table_insert(capabilities, strdup(caps_str), new_caps);
}

gboolean
caps_contains(const char * const caps_str)
{
    return (g_hash_table_lookup(capabilities, caps_str) != NULL);
}

Capabilities *
caps_get(const char * const caps_str)
{
    return g_hash_table_lookup(capabilities, caps_str);
}

char *
caps_get_sha1_str(xmpp_stanza_t *query)
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
caps_close(void)
{
    g_hash_table_destroy(capabilities);
}

static void
_caps_destroy(Capabilities *caps)
{
    if (caps != NULL) {
        FREE_SET_NULL(caps->client);
        FREE_SET_NULL(caps);
    }
}
