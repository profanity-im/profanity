/*
 * tlscerts.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
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

#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <glib/gstdio.h>

#include "log.h"
#include "common.h"
#include "config/files.h"
#include "config/tlscerts.h"
#include "tools/autocomplete.h"

static prof_keyfile_t tlscerts_prof_keyfile;
static GKeyFile* tlscerts;

static void _save_tlscerts(void);

static Autocomplete certs_ac;

static gchar* current_fp;

static void
_tlscerts_close(void)
{
    free_keyfile(&tlscerts_prof_keyfile);
    tlscerts = NULL;

    g_free(current_fp);
    current_fp = NULL;

    autocomplete_free(certs_ac);
}

void
tlscerts_init(void)
{
    log_info("Loading TLS certificates");

    prof_add_shutdown_routine(_tlscerts_close);

    load_data_keyfile(&tlscerts_prof_keyfile, FILE_TLSCERTS);
    tlscerts = tlscerts_prof_keyfile.keyfile;

    certs_ac = autocomplete_new();
    gsize len = 0;
    auto_gcharv gchar** groups = g_key_file_get_groups(tlscerts, &len);

    for (guint i = 0; i < g_strv_length(groups); i++) {
        autocomplete_add(certs_ac, groups[i]);
    }

    current_fp = NULL;
}

void
tlscerts_set_current(const TLSCertificate* cert)
{
    if (current_fp) {
        g_free(current_fp);
    }
    current_fp = g_strdup(cert->fingerprint);
}

gboolean
tlscerts_current_fingerprint_equals(const TLSCertificate* cert)
{
    return g_strcmp0(current_fp, cert->fingerprint) == 0;
}

void
tlscerts_clear_current(void)
{
    if (current_fp) {
        g_free(current_fp);
        current_fp = NULL;
    }
}

gboolean
tlscerts_exists(const TLSCertificate* cert)
{
    return g_key_file_has_group(tlscerts, cert->fingerprint);
}

#define _name_to_tlscert_name(in, cert, which)            \
    do {                                                  \
        _name_to_tlscert_name_(in, &cert->which, #which); \
    } while (0)
static void
_name_to_tlscert_name_(const char* in, tls_cert_name_t* out, const char* name);

static TLSCertificate*
_tlscerts_new(gchar* fingerprint_sha1, int version, gchar* serialnumber, gchar* subjectname,
              gchar* issuername, gchar* notbefore, gchar* notafter,
              gchar* key_alg, gchar* signature_alg, gchar* pem,
              gchar* fingerprint_sha256, gchar* pubkey_fingerprint)
{
    TLSCertificate* cert = g_new0(TLSCertificate, 1);

    if (fingerprint_sha256 && fingerprint_sha1) {
        cert->fingerprint_sha256 = fingerprint_sha256;
        cert->fingerprint_sha1 = fingerprint_sha1;
        cert->fingerprint = cert->fingerprint_sha256;
    } else {
        if (fingerprint_sha256) {
            size_t s = strlen(fingerprint_sha256);
            switch (s) {
            case 40:
                cert->fingerprint_sha1 = fingerprint_sha256;
                cert->fingerprint = cert->fingerprint_sha1;
                break;
            case 64:
                cert->fingerprint_sha256 = fingerprint_sha256;
                cert->fingerprint = cert->fingerprint_sha256;
                break;
            default:
                log_error("fingerprint_sha256 of length %d unexpected: %s", s, fingerprint_sha256);
            }
        } else {
            cert->fingerprint_sha1 = fingerprint_sha1;
            cert->fingerprint = cert->fingerprint_sha1;
        }
    }

    cert->version = version;
    cert->serialnumber = serialnumber;
    cert->subjectname = subjectname;
    cert->issuername = issuername;
    cert->notbefore = notbefore;
    cert->notafter = notafter;
    cert->key_alg = key_alg;
    cert->signature_alg = signature_alg;
    cert->pem = pem;
    cert->pubkey_fingerprint = pubkey_fingerprint;

    /* Do this check after assigning all values, in order to not leak them,
     * even though the situation is most likely already FUBAR */
    if (!fingerprint_sha256 && !fingerprint_sha1) {
        log_error("No TLSCertificate w/o fingerprint");
        tlscerts_free(cert);
        return NULL;
    }

    _name_to_tlscert_name(subjectname, cert, subject);
    _name_to_tlscert_name(issuername, cert, issuer);

    return cert;
}

static TLSCertificate*
_get_TLSCertificate(const gchar* fingerprint)
{
    int version = g_key_file_get_integer(tlscerts, fingerprint, "version", NULL);
    gchar* serialnumber = g_key_file_get_string(tlscerts, fingerprint, "serialnumber", NULL);
    gchar* subjectname = g_key_file_get_string(tlscerts, fingerprint, "subjectname", NULL);
    gchar* issuername = g_key_file_get_string(tlscerts, fingerprint, "issuername", NULL);
    gchar* notbefore = g_key_file_get_string(tlscerts, fingerprint, "start", NULL);
    gchar* notafter = g_key_file_get_string(tlscerts, fingerprint, "end", NULL);
    gchar* keyalg = g_key_file_get_string(tlscerts, fingerprint, "keyalg", NULL);
    gchar* signaturealg = g_key_file_get_string(tlscerts, fingerprint, "signaturealg", NULL);
    gchar* fingerprint_sha1 = g_key_file_get_string(tlscerts, fingerprint, "fingerprint_sha1", NULL);
    gchar* pubkey_fingerprint = g_key_file_get_string(tlscerts, fingerprint, "pubkey_fingerprint", NULL);

    return _tlscerts_new(fingerprint_sha1, version, serialnumber, subjectname, issuername, notbefore,
                         notafter, keyalg, signaturealg, NULL, g_strdup(fingerprint), pubkey_fingerprint);
}

GList*
tlscerts_list(void)
{
    GList* res = NULL;
    gsize len = 0;
    auto_gcharv gchar** groups = g_key_file_get_groups(tlscerts, &len);

    for (gsize i = 0; i < len; i++) {
        res = g_list_append(res, _get_TLSCertificate(groups[i]));
    }

    return res;
}

static void
_name_to_tlscert_name_(const char* in, tls_cert_name_t* out, const char* name)
{
    auto_gcharv gchar** fields = g_strsplit(in, "/", 0);
    const guint fields_num = g_strv_length(fields);
    for (guint i = 0; i < fields_num; i++) {
        auto_gcharv gchar** keyval = g_strsplit(fields[i], "=", 2);
        if (g_strv_length(keyval) == 2) {

#define tls_cert_name_set(which)                                                \
    do {                                                                        \
        if (out->which) {                                                       \
            log_warning("%s." #which " already set, skip %s", name, keyval[1]); \
            continue;                                                           \
        }                                                                       \
        out->which = g_strdup(keyval[1]);                                       \
    } while (0)

            if ((g_strcmp0(keyval[0], "C") == 0) || (g_strcmp0(keyval[0], "countryName") == 0)) {
                tls_cert_name_set(country);
            }
            if ((g_strcmp0(keyval[0], "ST") == 0) || (g_strcmp0(keyval[0], "stateOrProvinceName") == 0)) {
                tls_cert_name_set(state);
            }
            if (g_strcmp0(keyval[0], "dnQualifier") == 0) {
                tls_cert_name_set(distinguishedname);
            }
            if (g_strcmp0(keyval[0], "serialnumber") == 0) {
                tls_cert_name_set(serialnumber);
            }
            if ((g_strcmp0(keyval[0], "CN") == 0) || (g_strcmp0(keyval[0], "commonName") == 0)) {
                tls_cert_name_set(commonname);
            }
            if ((g_strcmp0(keyval[0], "O") == 0) || (g_strcmp0(keyval[0], "organizationName") == 0)) {
                tls_cert_name_set(organisation);
            }
            if ((g_strcmp0(keyval[0], "OU") == 0) || (g_strcmp0(keyval[0], "organizationalUnitName") == 0)) {
                tls_cert_name_set(organisation_unit);
            }
            if (g_strcmp0(keyval[0], "emailAddress") == 0) {
                tls_cert_name_set(email);
            }
#undef tls_cert_name_set
        }
    }
}

static gchar*
_checked_g_strdup(const char* in)
{
    if (in == NULL)
        return NULL;
    return g_strdup(in);
}

TLSCertificate*
tlscerts_new(const char* fingerprint_sha1, int version, const char* serialnumber, const char* subjectname,
             const char* issuername, const char* notbefore, const char* notafter,
             const char* key_alg, const char* signature_alg, const char* pem,
             const char* fingerprint_sha256, const char* pubkey_fingerprint)
{
    return _tlscerts_new(_checked_g_strdup(fingerprint_sha1), version, _checked_g_strdup(serialnumber), _checked_g_strdup(subjectname),
                         _checked_g_strdup(issuername), _checked_g_strdup(notbefore), _checked_g_strdup(notafter),
                         _checked_g_strdup(key_alg), _checked_g_strdup(signature_alg), _checked_g_strdup(pem),
                         _checked_g_strdup(fingerprint_sha256), _checked_g_strdup(pubkey_fingerprint));
}

static void
_checked_g_key_file_set_string(GKeyFile* key_file,
                               const gchar* group_name,
                               const gchar* key,
                               const gchar* string)
{
    if (string == NULL)
        return;
    g_key_file_set_string(key_file, group_name, key, string);
}

void
tlscerts_add(const TLSCertificate* cert)
{
    if (!cert) {
        return;
    }

    if (!cert->fingerprint) {
        return;
    }

    autocomplete_add(certs_ac, cert->fingerprint);

    g_key_file_set_integer(tlscerts, cert->fingerprint, "version", cert->version);
    _checked_g_key_file_set_string(tlscerts, cert->fingerprint, "serialnumber", cert->serialnumber);
    _checked_g_key_file_set_string(tlscerts, cert->fingerprint, "subjectname", cert->subjectname);
    _checked_g_key_file_set_string(tlscerts, cert->fingerprint, "issuername", cert->issuername);
    _checked_g_key_file_set_string(tlscerts, cert->fingerprint, "start", cert->notbefore);
    _checked_g_key_file_set_string(tlscerts, cert->fingerprint, "end", cert->notafter);
    _checked_g_key_file_set_string(tlscerts, cert->fingerprint, "keyalg", cert->key_alg);
    _checked_g_key_file_set_string(tlscerts, cert->fingerprint, "signaturealg", cert->signature_alg);
    _checked_g_key_file_set_string(tlscerts, cert->fingerprint, "fingerprint_sha1", cert->fingerprint_sha1);
    _checked_g_key_file_set_string(tlscerts, cert->fingerprint, "pubkey_fingerprint", cert->pubkey_fingerprint);

    _save_tlscerts();
}

gboolean
tlscerts_revoke(const char* fingerprint)
{
    gboolean result = g_key_file_remove_group(tlscerts, fingerprint, NULL);
    if (result) {
        autocomplete_remove(certs_ac, fingerprint);
    }

    _save_tlscerts();

    return result;
}

TLSCertificate*
tlscerts_get_trusted(const gchar* fingerprint)
{
    if (!g_key_file_has_group(tlscerts, fingerprint)) {
        gsize len = 0;
        auto_gcharv gchar** groups = g_key_file_get_groups(tlscerts, &len);
        for (gsize i = 0; i < len; i++) {
            auto_gchar gchar* fingerprint_sha1 = g_key_file_get_string(tlscerts, groups[i], "fingerprint_sha1", NULL);
            if (g_strcmp0(fingerprint, fingerprint_sha1) == 0) {
                return _get_TLSCertificate(fingerprint_sha1);
            }
        }
        return NULL;
    }
    return _get_TLSCertificate(fingerprint);
}

char*
tlscerts_complete(const char* prefix, gboolean previous, void* context)
{
    return autocomplete_complete(certs_ac, prefix, TRUE, previous);
}

void
tlscerts_reset_ac(void)
{
    autocomplete_reset(certs_ac);
}

static void
_tls_cert_name_free(tls_cert_name_t* name)
{
    g_free(name->country);
    g_free(name->state);
    g_free(name->distinguishedname);
    g_free(name->serialnumber);
    g_free(name->commonname);
    g_free(name->organisation);
    g_free(name->organisation_unit);
    g_free(name->email);
}

void
tlscerts_free(TLSCertificate* cert)
{
    if (cert) {
        _tls_cert_name_free(&cert->subject);
        _tls_cert_name_free(&cert->issuer);

        g_free(cert->pubkey_fingerprint);
        g_free(cert->pem);
        g_free(cert->signature_alg);
        g_free(cert->key_alg);
        g_free(cert->notafter);
        g_free(cert->notbefore);
        g_free(cert->issuername);
        g_free(cert->subjectname);
        g_free(cert->serialnumber);
        g_free(cert->fingerprint_sha1);
        g_free(cert->fingerprint_sha256);

        free(cert);
    }
}

static void
_save_tlscerts(void)
{
    save_keyfile(&tlscerts_prof_keyfile);
}
