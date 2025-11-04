/*
 * tlscerts.h
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

#ifndef CONFIG_TLSCERTS_H
#define CONFIG_TLSCERTS_H

#include <glib.h>

typedef struct tls_cert_t
{
    int version;
    char* serialnumber;
    char* subjectname;
    char* subject_country;
    char* subject_state;
    char* subject_distinguishedname;
    char* subject_serialnumber;
    char* subject_commonname;
    char* subject_organisation;
    char* subject_organisation_unit;
    char* subject_email;
    char* issuername;
    char* issuer_country;
    char* issuer_state;
    char* issuer_distinguishedname;
    char* issuer_serialnumber;
    char* issuer_commonname;
    char* issuer_organisation;
    char* issuer_organisation_unit;
    char* issuer_email;
    char* notbefore;
    char* notafter;
    char* fingerprint_sha1;
    char* key_alg;
    char* signature_alg;
    char* pem;
} TLSCertificate;

void tlscerts_init(void);

TLSCertificate* tlscerts_new(const char* fingerprint_sha1, int version, const char* serialnumber, const char* subjectname,
                             const char* issuername, const char* notbefore, const char* notafter,
                             const char* key_alg, const char* signature_alg, const char* pem);

void tlscerts_set_current(const TLSCertificate* cert);

gboolean tlscerts_current_fingerprint_equals(const TLSCertificate* cert);

void tlscerts_clear_current(void);

gboolean tlscerts_exists(const TLSCertificate* cert);

void tlscerts_add(const TLSCertificate* cert);

gboolean tlscerts_revoke(const char* fingerprint);

TLSCertificate* tlscerts_get_trusted(const char* fingerprint);

void tlscerts_free(TLSCertificate* cert);

GList* tlscerts_list(void);

char* tlscerts_complete(const char* prefix, gboolean previous, void* context);

void tlscerts_reset_ac(void);

#endif
