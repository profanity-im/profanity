/*
 * tlscerts.c
 *
 * Copyright (C) 2012 - 2015 James Booth <boothj5@gmail.com>
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

#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <glib/gstdio.h>

#include "config/tlscerts.h"
#include "log.h"
#include "common.h"
#include "tools/autocomplete.h"

static gchar *tlscerts_loc;
static GKeyFile *tlscerts;

static gchar* _get_tlscerts_file(void);
static void _save_tlscerts(void);

static Autocomplete certs_ac;

static char *current_fp;

void
tlscerts_init(void)
{
    log_info("Loading TLS certificates");
    tlscerts_loc = _get_tlscerts_file();

    if (g_file_test(tlscerts_loc, G_FILE_TEST_EXISTS)) {
        g_chmod(tlscerts_loc, S_IRUSR | S_IWUSR);
    }

    tlscerts = g_key_file_new();
    g_key_file_load_from_file(tlscerts, tlscerts_loc, G_KEY_FILE_KEEP_COMMENTS, NULL);

    certs_ac = autocomplete_new();
    gsize len = 0;
    gchar **groups = g_key_file_get_groups(tlscerts, &len);

    int i = 0;
    for (i = 0; i < g_strv_length(groups); i++) {
        autocomplete_add(certs_ac, groups[i]);
    }
    g_strfreev(groups);

    current_fp = NULL;
}

void
tlscerts_set_current(const char *const fp)
{
    if (current_fp) {
        free(current_fp);
    }
    current_fp = strdup(fp);
}

char*
tlscerts_get_current(void)
{
    return current_fp;
}

void
tlscerts_clear_current(void)
{
    if (current_fp) {
        free(current_fp);
        current_fp = NULL;
    }
}

gboolean
tlscerts_exists(const char *const fingerprint)
{
    return g_key_file_has_group(tlscerts, fingerprint);
}

GList*
tlscerts_list(void)
{
    GList *res = NULL;
    gsize len = 0;
    gchar **groups = g_key_file_get_groups(tlscerts, &len);

    int i = 0;
    for (i = 0; i < g_strv_length(groups); i++) {
        char *fingerprint = strdup(groups[i]);
        char *domain = g_key_file_get_string(tlscerts, fingerprint, "domain", NULL);
        char *organisation = g_key_file_get_string(tlscerts, fingerprint, "organisation", NULL);
        char *email = g_key_file_get_string(tlscerts, fingerprint, "email", NULL);
        char *notbefore = g_key_file_get_string(tlscerts, fingerprint, "start", NULL);
        char *notafter = g_key_file_get_string(tlscerts, fingerprint, "end", NULL);

        TLSCertificate *cert = tlscerts_new(fingerprint, domain, organisation, email, notbefore, notafter);

        res = g_list_append(res, cert);
    }

    if (groups) {
        g_strfreev(groups);
    }

    return res;
}

TLSCertificate*
tlscerts_new(const char *const fingerprint, const char *const domain, const char *const organisation,
    const char *const email, const char *const notbefore, const char *const notafter)
{
    TLSCertificate *cert = malloc(sizeof(TLSCertificate));
    if (fingerprint) {
        cert->fingerprint = strdup(fingerprint);
    } else {
        cert->fingerprint = NULL;
    }
    if (domain) {
        cert->domain = strdup(domain);
    } else {
        cert->domain = NULL;
    }
    if (organisation) {
        cert->organisation = strdup(organisation);
    } else {
        cert->organisation = NULL;
    }
    if (email) {
        cert->email = strdup(email);
    } else {
        cert->email= NULL;
    }
    if (notbefore) {
        cert->notbefore = strdup(notbefore);
    } else {
        cert->notbefore = NULL;
    }
    if (notafter) {
        cert->notafter = strdup(notafter);
    } else {
        cert->notafter = NULL;
    }

    return cert;
}

void
tlscerts_add(TLSCertificate *cert)
{
    if (!cert) {
        return;
    }

    if (!cert->fingerprint) {
        return;
    }

    autocomplete_add(certs_ac, cert->fingerprint);

    if (cert->domain) {
        g_key_file_set_string(tlscerts, cert->fingerprint, "domain", cert->domain);
    }
    if (cert->organisation) {
        g_key_file_set_string(tlscerts, cert->fingerprint, "organisation", cert->organisation);
    }
    if (cert->email) {
        g_key_file_set_string(tlscerts, cert->fingerprint, "email", cert->email);
    }
    if (cert->notbefore) {
        g_key_file_set_string(tlscerts, cert->fingerprint, "start", cert->notbefore);
    }
    if (cert->notafter) {
        g_key_file_set_string(tlscerts, cert->fingerprint, "end", cert->notafter);
    }

    _save_tlscerts();
}

gboolean
tlscerts_revoke(const char *const fingerprint)
{
    gboolean result =  g_key_file_remove_group(tlscerts, fingerprint, NULL);
    if (result) {
        autocomplete_remove(certs_ac, fingerprint);
    }

    _save_tlscerts();

    return result;
}

char*
tlscerts_complete(const char *const prefix)
{
    return autocomplete_complete(certs_ac, prefix, TRUE);
}

void
tlscerts_reset_ac(void)
{
    autocomplete_reset(certs_ac);
}

void
tlscerts_free(TLSCertificate *cert)
{
    if (cert) {
        free(cert->fingerprint);
        free(cert->domain);
        free(cert->organisation);
        free(cert->email);
        free(cert->notbefore);
        free(cert->notafter);
    }
}

void
tlscerts_close(void)
{
    g_key_file_free(tlscerts);
    tlscerts = NULL;

    free(current_fp);
    current_fp = NULL;

    autocomplete_free(certs_ac);
}

static gchar*
_get_tlscerts_file(void)
{
    gchar *xdg_data = xdg_get_data_home();
    GString *tlscerts_file = g_string_new(xdg_data);
    g_string_append(tlscerts_file, "/profanity/tlscerts");
    gchar *result = strdup(tlscerts_file->str);
    g_free(xdg_data);
    g_string_free(tlscerts_file, TRUE);

    return result;
}

static void
_save_tlscerts(void)
{
    gsize g_data_size;
    gchar *g_tlscerts_data = g_key_file_to_data(tlscerts, &g_data_size, NULL);
    g_file_set_contents(tlscerts_loc, g_tlscerts_data, g_data_size, NULL);
    g_chmod(tlscerts_loc, S_IRUSR | S_IWUSR);
    g_free(g_tlscerts_data);
}
