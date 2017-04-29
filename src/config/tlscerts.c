/*
 * tlscerts.c
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

#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <glib/gstdio.h>

#include "log.h"
#include "common.h"
#include "config/files.h"
#include "config/tlscerts.h"
#include "tools/autocomplete.h"

static char *tlscerts_loc;
static GKeyFile *tlscerts;

static void _save_tlscerts(void);

static Autocomplete certs_ac;

static char *current_fp;

void
tlscerts_init(void)
{
    log_info("Loading TLS certificates");
    tlscerts_loc = files_get_data_path(FILE_TLSCERTS);

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
        int version = g_key_file_get_integer(tlscerts, fingerprint, "version", NULL);
        char *serialnumber = g_key_file_get_string(tlscerts, fingerprint, "serialnumber", NULL);
        char *subjectname = g_key_file_get_string(tlscerts, fingerprint, "subjectname", NULL);
        char *issuername = g_key_file_get_string(tlscerts, fingerprint, "issuername", NULL);
        char *notbefore = g_key_file_get_string(tlscerts, fingerprint, "start", NULL);
        char *notafter = g_key_file_get_string(tlscerts, fingerprint, "end", NULL);
        char *keyalg = g_key_file_get_string(tlscerts, fingerprint, "keyalg", NULL);
        char *signaturealg = g_key_file_get_string(tlscerts, fingerprint, "signaturealg", NULL);

        TLSCertificate *cert = tlscerts_new(fingerprint, version, serialnumber, subjectname, issuername, notbefore,
            notafter, keyalg, signaturealg);

        free(fingerprint);
        free(serialnumber);
        free(subjectname);
        free(issuername);
        free(notbefore);
        free(notafter);
        free(keyalg);
        free(signaturealg);

        res = g_list_append(res, cert);
    }

    if (groups) {
        g_strfreev(groups);
    }

    return res;
}

TLSCertificate*
tlscerts_new(const char *const fingerprint, int version, const char *const serialnumber, const char *const subjectname,
    const char *const issuername, const char *const notbefore, const char *const notafter,
    const char *const key_alg, const char *const signature_alg)
{
    TLSCertificate *cert = malloc(sizeof(TLSCertificate));

    if (fingerprint) {
        cert->fingerprint = strdup(fingerprint);
    } else {
        cert->fingerprint = NULL;
    }
    cert->version = version;
    if (serialnumber) {
        cert->serialnumber = strdup(serialnumber);
    } else {
        cert->serialnumber = NULL;
    }
    if (subjectname) {
        cert->subjectname = strdup(subjectname);
    } else {
        cert->subjectname = NULL;
    }
    if (issuername) {
        cert->issuername = strdup(issuername);
    } else {
        cert->issuername = NULL;
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
    if (key_alg) {
        cert->key_alg = strdup(key_alg);
    } else {
        cert->key_alg = NULL;
    }
    if (signature_alg) {
        cert->signature_alg = strdup(signature_alg);
    } else {
        cert->signature_alg = NULL;
    }

    cert->subject_country = NULL;
    cert->subject_state = NULL;
    cert->subject_distinguishedname = NULL;
    cert->subject_serialnumber = NULL;
    cert->subject_commonname = NULL;
    cert->subject_organisation = NULL;
    cert->subject_organisation_unit = NULL;
    cert->subject_email = NULL;
    gchar** fields = g_strsplit(subjectname, "/", 0);
    int i = 0;
    for (i = 0; i < g_strv_length(fields); i++) {
        gchar** keyval = g_strsplit(fields[i], "=", 2);
        if (g_strv_length(keyval) == 2) {
            if ((g_strcmp0(keyval[0], "C") == 0) || (g_strcmp0(keyval[0], "countryName") == 0)) {
                cert->subject_country = strdup(keyval[1]);
            }
            if ((g_strcmp0(keyval[0], "ST") == 0) || (g_strcmp0(keyval[0], "stateOrProvinceName") == 0)) {
                cert->subject_state = strdup(keyval[1]);
            }
            if (g_strcmp0(keyval[0], "dnQualifier") == 0) {
                cert->subject_distinguishedname = strdup(keyval[1]);
            }
            if (g_strcmp0(keyval[0], "serialnumber") == 0) {
                cert->subject_serialnumber = strdup(keyval[1]);
            }
            if ((g_strcmp0(keyval[0], "CN") == 0) || (g_strcmp0(keyval[0], "commonName") == 0)) {
                cert->subject_commonname = strdup(keyval[1]);
            }
            if ((g_strcmp0(keyval[0], "O") == 0) || (g_strcmp0(keyval[0], "organizationName") == 0)) {
                cert->subject_organisation = strdup(keyval[1]);
            }
            if ((g_strcmp0(keyval[0], "OU") == 0) || (g_strcmp0(keyval[0], "organizationalUnitName") == 0)) {
                cert->subject_organisation_unit = strdup(keyval[1]);
            }
            if (g_strcmp0(keyval[0], "emailAddress") == 0) {
                cert->subject_email = strdup(keyval[1]);
            }
        }
        g_strfreev(keyval);
    }
    g_strfreev(fields);

    cert->issuer_country = NULL;
    cert->issuer_state = NULL;
    cert->issuer_distinguishedname = NULL;
    cert->issuer_serialnumber = NULL;
    cert->issuer_commonname = NULL;
    cert->issuer_organisation = NULL;
    cert->issuer_organisation_unit = NULL;
    cert->issuer_email = NULL;
    fields = g_strsplit(issuername, "/", 0);
    for (i = 0; i < g_strv_length(fields); i++) {
        gchar** keyval = g_strsplit(fields[i], "=", 2);
        if (g_strv_length(keyval) == 2) {
            if ((g_strcmp0(keyval[0], "C") == 0) || (g_strcmp0(keyval[0], "countryName") == 0)) {
                cert->issuer_country = strdup(keyval[1]);
            }
            if ((g_strcmp0(keyval[0], "ST") == 0) || (g_strcmp0(keyval[0], "stateOrProvinceName") == 0)) {
                cert->issuer_state = strdup(keyval[1]);
            }
            if (g_strcmp0(keyval[0], "dnQualifier") == 0) {
                cert->issuer_distinguishedname = strdup(keyval[1]);
            }
            if (g_strcmp0(keyval[0], "serialnumber") == 0) {
                cert->issuer_serialnumber = strdup(keyval[1]);
            }
            if ((g_strcmp0(keyval[0], "CN") == 0) || (g_strcmp0(keyval[0], "commonName") == 0)) {
                cert->issuer_commonname = strdup(keyval[1]);
            }
            if ((g_strcmp0(keyval[0], "O") == 0) || (g_strcmp0(keyval[0], "organizationName") == 0)) {
                cert->issuer_organisation = strdup(keyval[1]);
            }
            if ((g_strcmp0(keyval[0], "OU") == 0) || (g_strcmp0(keyval[0], "organizationalUnitName") == 0)) {
                cert->issuer_organisation_unit = strdup(keyval[1]);
            }
            if (g_strcmp0(keyval[0], "emailAddress") == 0) {
                cert->issuer_email = strdup(keyval[1]);
            }
        }
        g_strfreev(keyval);
    }
    g_strfreev(fields);

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

    g_key_file_set_integer(tlscerts, cert->fingerprint, "version", cert->version);
    if (cert->serialnumber) {
        g_key_file_set_string(tlscerts, cert->fingerprint, "serialnumber", cert->serialnumber);
    }
    if (cert->subjectname) {
        g_key_file_set_string(tlscerts, cert->fingerprint, "subjectname", cert->subjectname);
    }
    if (cert->issuername) {
        g_key_file_set_string(tlscerts, cert->fingerprint, "issuername", cert->issuername);
    }
    if (cert->notbefore) {
        g_key_file_set_string(tlscerts, cert->fingerprint, "start", cert->notbefore);
    }
    if (cert->notafter) {
        g_key_file_set_string(tlscerts, cert->fingerprint, "end", cert->notafter);
    }
    if (cert->key_alg) {
        g_key_file_set_string(tlscerts, cert->fingerprint, "keyalg", cert->key_alg);
    }
    if (cert->signature_alg) {
        g_key_file_set_string(tlscerts, cert->fingerprint, "signaturealg", cert->signature_alg);
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

TLSCertificate*
tlscerts_get_trusted(const char * const fingerprint)
{
    if (!g_key_file_has_group(tlscerts, fingerprint)) {
        return NULL;
    }

    int version = g_key_file_get_integer(tlscerts, fingerprint, "version", NULL);
    char *serialnumber = g_key_file_get_string(tlscerts, fingerprint, "serialnumber", NULL);
    char *subjectname = g_key_file_get_string(tlscerts, fingerprint, "subjectname", NULL);
    char *issuername = g_key_file_get_string(tlscerts, fingerprint, "issuername", NULL);
    char *notbefore = g_key_file_get_string(tlscerts, fingerprint, "start", NULL);
    char *notafter = g_key_file_get_string(tlscerts, fingerprint, "end", NULL);
    char *keyalg = g_key_file_get_string(tlscerts, fingerprint, "keyalg", NULL);
    char *signaturealg = g_key_file_get_string(tlscerts, fingerprint, "signaturealg", NULL);

    TLSCertificate *cert = tlscerts_new(fingerprint, version, serialnumber, subjectname, issuername, notbefore,
        notafter, keyalg, signaturealg);

    free(serialnumber);
    free(subjectname);
    free(issuername);
    free(notbefore);
    free(notafter);
    free(keyalg);
    free(signaturealg);

    return cert;
}

char*
tlscerts_complete(const char *const prefix, gboolean previous)
{
    return autocomplete_complete(certs_ac, prefix, TRUE, previous);
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
        free(cert->serialnumber);

        free(cert->subjectname);
        free(cert->subject_country);
        free(cert->subject_state);
        free(cert->subject_distinguishedname);
        free(cert->subject_serialnumber);
        free(cert->subject_commonname);
        free(cert->subject_organisation);
        free(cert->subject_organisation_unit);
        free(cert->subject_email);

        free(cert->issuername);
        free(cert->issuer_country);
        free(cert->issuer_state);
        free(cert->issuer_distinguishedname);
        free(cert->issuer_serialnumber);
        free(cert->issuer_commonname);
        free(cert->issuer_organisation);
        free(cert->issuer_organisation_unit);
        free(cert->issuer_email);

        free(cert->notbefore);
        free(cert->notafter);
        free(cert->fingerprint);

        free(cert->key_alg);
        free(cert->signature_alg);

        free(cert);
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

static void
_save_tlscerts(void)
{
    gsize g_data_size;
    gchar *g_tlscerts_data = g_key_file_to_data(tlscerts, &g_data_size, NULL);
    g_file_set_contents(tlscerts_loc, g_tlscerts_data, g_data_size, NULL);
    g_chmod(tlscerts_loc, S_IRUSR | S_IWUSR);
    g_free(g_tlscerts_data);
}
