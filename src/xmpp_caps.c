/*
 * xmpp_caps.c
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

#include "config.h"
#include "common.h"
#include "xmpp.h"

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
caps_create_sha1_str(xmpp_stanza_t * const query)
{
    char *category = NULL;
    char *type = NULL;
    char *lang = NULL;
    char *name = NULL;
    char *feature_str = NULL;
    GSList *identities = NULL;
    GSList *features = NULL;
    GSList *form_names = NULL;
    DataForm *form = NULL;
    FormField *field = NULL;
    GHashTable *forms = g_hash_table_new_full(g_str_hash, g_str_equal, free, (GDestroyNotify)stanza_destroy_form);

    GString *s = g_string_new("");

    xmpp_stanza_t *child = xmpp_stanza_get_children(query);
    while (child != NULL) {
        if (g_strcmp0(xmpp_stanza_get_name(child), STANZA_NAME_IDENTITY) == 0) {
            category = xmpp_stanza_get_attribute(child, "category");
            type = xmpp_stanza_get_attribute(child, "type");
            lang = xmpp_stanza_get_attribute(child, "xml:lang");
            name = xmpp_stanza_get_attribute(child, "name");

            GString *identity_str = g_string_new(g_strdup(category));
            g_string_append(identity_str, "/");
            if (type != NULL) {
                g_string_append(identity_str, g_strdup(type));
            }
            g_string_append(identity_str, "/");
            if (lang != NULL) {
                g_string_append(identity_str, g_strdup(lang));
            }
            g_string_append(identity_str, "/");
            if (name != NULL) {
                g_string_append(identity_str, g_strdup(name));
            }
            g_string_append(identity_str, "<");
            identities = g_slist_insert_sorted(identities, g_strdup(identity_str->str), (GCompareFunc)octet_compare);
            g_string_free(identity_str, TRUE);
        } else if (g_strcmp0(xmpp_stanza_get_name(child), STANZA_NAME_FEATURE) == 0) {
            feature_str = xmpp_stanza_get_attribute(child, "var");
            features = g_slist_insert_sorted(features, g_strdup(feature_str), (GCompareFunc)octet_compare);
        } else if (g_strcmp0(xmpp_stanza_get_name(child), STANZA_NAME_X) == 0) {
            if (strcmp(xmpp_stanza_get_ns(child), STANZA_NS_DATA) == 0) {
                form = stanza_create_form(child);
                form_names = g_slist_insert_sorted(form_names, strdup(form->form_type), (GCompareFunc)octet_compare);
                g_hash_table_insert(forms, strdup(form->form_type), form);
            }
        }
        child = xmpp_stanza_get_next(child);
    }

    GSList *curr = identities;
    while (curr != NULL) {
        g_string_append(s, strdup(curr->data));
        curr = g_slist_next(curr);
    }

    curr = features;
    while (curr != NULL) {
        g_string_append(s, strdup(curr->data));
        g_string_append(s, "<");
        curr = g_slist_next(curr);
    }

    curr = form_names;
    while (curr != NULL) {
        form = g_hash_table_lookup(forms, curr->data);
        g_string_append(s, strdup(form->form_type));
        g_string_append(s, "<");

        GSList *curr_field = form->fields;
        while (curr_field != NULL) {
            field = curr_field->data;
            g_string_append(s, strdup(field->var));
            GSList *curr_value = field->values;
            while (curr_value != NULL) {
                g_string_append(s, strdup(curr_value->data));
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
    g_slist_free_full(identities, free);
    g_slist_free_full(features, free);
    g_slist_free_full(form_names, free);
    g_hash_table_destroy(forms);

    return result;
}

xmpp_stanza_t *
caps_create_query_response_stanza(xmpp_ctx_t * const ctx)
{
    xmpp_stanza_t *query = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(query, STANZA_NAME_QUERY);
    xmpp_stanza_set_ns(query, XMPP_NS_DISCO_INFO);

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

    xmpp_stanza_t *feature_discoinfo = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(feature_discoinfo, STANZA_NAME_FEATURE);
    xmpp_stanza_set_attribute(feature_discoinfo, STANZA_ATTR_VAR, XMPP_NS_DISCO_INFO);

    xmpp_stanza_t *feature_muc = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(feature_muc, STANZA_NAME_FEATURE);
    xmpp_stanza_set_attribute(feature_muc, STANZA_ATTR_VAR, STANZA_NS_MUC);

    xmpp_stanza_t *feature_version = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(feature_version, STANZA_NAME_FEATURE);
    xmpp_stanza_set_attribute(feature_version, STANZA_ATTR_VAR, STANZA_NS_VERSION);

    xmpp_stanza_add_child(query, identity);
    xmpp_stanza_add_child(query, feature_muc);
    xmpp_stanza_add_child(query, feature_discoinfo);
    xmpp_stanza_add_child(query, feature_caps);
    xmpp_stanza_add_child(query, feature_version);

    xmpp_stanza_release(identity);
    xmpp_stanza_release(feature_muc);
    xmpp_stanza_release(feature_discoinfo);
    xmpp_stanza_release(feature_caps);
    xmpp_stanza_release(feature_version);

    return query;
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
