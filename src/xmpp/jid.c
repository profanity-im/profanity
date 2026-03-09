/*
 * jid.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "common.h"
#include "xmpp/jid.h"

#define JID_MAX_PART_LEN  1023
#define JID_MAX_TOTAL_LEN 3071

static gboolean
_is_invalid_local_char(gunichar c)
{
    // RFC 6122: " & ' / : < > @
    // Also space is forbidden.
    return (c == ' ' || c == '"' || c == '&' || c == '\'' || c == '/' || c == ':' || c == '<' || c == '>' || c == '@');
}

Jid*
jid_create(const gchar* const str)
{
    if (!jid_is_valid(str)) {
        return NULL;
    }

    gchar* trimmed = g_strdup(str);
    if (trimmed == NULL) {
        return NULL;
    }

    Jid* result = g_new0(Jid, 1);
    result->refcnt = 1;
    result->str = trimmed;

    gchar* slashp = g_utf8_strchr(trimmed, -1, '/');
    auto_gchar gchar* bare = NULL;

    if (slashp) {
        if (strlen(slashp + 1) > 0) {
            result->resourcepart = g_strdup(slashp + 1);
            result->fulljid = g_strdup(trimmed);
        }
        bare = g_utf8_substring(trimmed, 0, g_utf8_pointer_to_offset(trimmed, slashp));
    } else {
        bare = g_strdup(trimmed);
    }

    gchar* atp = g_utf8_strchr(bare, -1, '@');
    gchar* domain_start = bare;

    if (atp) {
        if (g_utf8_pointer_to_offset(bare, atp) > 0) {
            result->localpart = g_utf8_substring(bare, 0, g_utf8_pointer_to_offset(bare, atp));
        }
        domain_start = atp + 1;
    }

    if (strlen(domain_start) > 0) {
        result->domainpart = g_strdup(domain_start);
    }

    result->barejid = g_utf8_strdown(bare, -1);

    return result;
}

gboolean
jid_is_valid(const gchar* const str)
{
    if (str == NULL) {
        return FALSE;
    }

    size_t total_len = strlen(str);
    if (total_len == 0 || total_len > JID_MAX_TOTAL_LEN) {
        return FALSE;
    }

    if (!g_utf8_validate(str, -1, NULL)) {
        return FALSE;
    }

    const gchar* slash = g_utf8_strchr(str, -1, '/');
    const gchar* at = NULL;

    // Find the first '@' that is NOT in the resourcepart
    // and ensure there is at most one '@' in the bare JID
    const gchar* p = str;
    while (*p && (!slash || p < slash)) {
        if (g_utf8_get_char(p) == '@') {
            if (at == NULL) {
                at = p;
            } else {
                // More than one '@' in bare JID
                return FALSE;
            }
        }
        p = g_utf8_next_char(p);
    }

    const gchar* domain_start = str;
    size_t domain_len = 0;

    // Localpart validation
    if (at) {
        size_t local_len = at - str;
        if (local_len == 0 || local_len > JID_MAX_PART_LEN) {
            return FALSE;
        }

        const gchar* lp = str;
        while (lp < at) {
            gunichar c = g_utf8_get_char(lp);
            if (_is_invalid_local_char(c)) {
                return FALSE;
            }
            lp = g_utf8_next_char(lp);
        }
        domain_start = at + 1;
    }

    // Resourcepart validation if present
    if (slash) {
        domain_len = slash - domain_start;
        size_t resource_len = strlen(slash + 1);
        if (resource_len > JID_MAX_PART_LEN) {
            return FALSE;
        }
    } else {
        domain_len = strlen(domain_start);
    }

    // Domainpart validation
    if (domain_len == 0 || domain_len > JID_MAX_PART_LEN) {
        return FALSE;
    }

    return TRUE;
}

gboolean
jid_is_valid_user_jid(const gchar* const str)
{
    if (!jid_is_valid(str)) {
        return FALSE;
    }

    const gchar* slash = g_utf8_strchr(str, -1, '/');
    const gchar* at = g_utf8_strchr(str, -1, '@');

    // A user JID must have an '@' before any '/'
    if (at && (!slash || at < slash)) {
        return TRUE;
    }

    return FALSE;
}

Jid*
jid_create_from_bare_and_resource(const gchar* const barejid, const gchar* const resource)
{
    auto_gchar gchar* jid = create_fulljid(barejid, resource);
    return jid_create(jid);
}

void
jid_auto_destroy(Jid** jid)
{
    if (jid == NULL)
        return;
    jid_destroy(*jid);
}

void
jid_ref(Jid* jid)
{
    jid->refcnt++;
}

void
jid_destroy(Jid* jid)
{
    if (jid == NULL) {
        return;
    }
    if (jid->refcnt > 1) {
        jid->refcnt--;
        return;
    }

    g_free(jid->str);
    g_free(jid->localpart);
    g_free(jid->domainpart);
    g_free(jid->resourcepart);
    g_free(jid->barejid);
    g_free(jid->fulljid);
    free(jid);
}

gboolean
jid_is_valid_room_form(Jid* jid)
{
    return (jid->fulljid != NULL);
}

/*
 * Given a barejid, and resourcepart, create and return a full JID of the form
 * barejid/resourcepart
 * Will return a newly created string that must be freed by the caller
 */
gchar*
create_fulljid(const gchar* const barejid, const gchar* const resource)
{
    auto_gchar gchar* barejidlower = g_utf8_strdown(barejid, -1);
    if (resource && strlen(resource) > 0) {
        return g_strdup_printf("%s/%s", barejidlower, resource);
    } else {
        return g_strdup(barejidlower);
    }
}

/*
 * Get the nickname part of the full JID, e.g.
 * Full JID = "test@conference.server/person"
 * returns "person"
 */
gchar*
get_nick_from_full_jid(const gchar* const full_room_jid)
{
    auto_gcharv gchar** tokens = g_strsplit(full_room_jid, "/", 0);
    gchar* nick_part = NULL;

    if (tokens) {
        if (tokens[0] && tokens[1]) {
            nick_part = strdup(tokens[1]);
        }
    }

    return nick_part;
}

/*
 * get the fulljid, fall back to the barejid
 */
const gchar*
jid_fulljid_or_barejid(Jid* jid)
{
    if (jid->fulljid) {
        return jid->fulljid;
    } else {
        return jid->barejid;
    }
}

gchar*
jid_random_resource(void)
{
    auto_gchar gchar* rand = get_random_string(4);

    gchar* result = g_strdup_printf("profanity.%s", rand);

    return result;
}
