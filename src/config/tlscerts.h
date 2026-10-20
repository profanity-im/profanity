/*
 * tlscerts.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef CONFIG_TLSCERTS_H
#define CONFIG_TLSCERTS_H

#include <glib.h>

typedef struct tls_cert_name_t
{
    gchar* country;
    gchar* state;
    gchar* distinguishedname;
    gchar* serialnumber;
    gchar* commonname;
    gchar* organisation;
    gchar* organisation_unit;
    gchar* email;
} tls_cert_name_t;

typedef struct tls_cert_t
{
    int version;
    const gchar* fingerprint;
    gchar* serialnumber;
    gchar* subjectname;
    tls_cert_name_t subject;
    gchar* issuername;
    tls_cert_name_t issuer;
    gchar* notbefore;
    gchar* notafter;
    gchar* fingerprint_sha1;
    gchar* fingerprint_sha256;
    gchar* key_alg;
    gchar* signature_alg;
    gchar* pem;
    gchar* pubkey_fingerprint;
} TLSCertificate;

void tlscerts_init(void);

TLSCertificate* tlscerts_new(const gchar* fingerprint_sha1, int version, const gchar* serialnumber, const gchar* subjectname,
                             const gchar* issuername, const gchar* notbefore, const gchar* notafter,
                             const gchar* key_alg, const gchar* signature_alg, const gchar* pem,
                             const gchar* fingerprint_sha256, const gchar* pubkey_fingerprint);

void tlscerts_set_current(const TLSCertificate* cert);

gboolean tlscerts_current_fingerprint_equals(const TLSCertificate* cert);

void tlscerts_clear_current(void);

gboolean tlscerts_exists(const TLSCertificate* cert);

void tlscerts_add(const TLSCertificate* cert);

gboolean tlscerts_revoke(const gchar* fingerprint);

TLSCertificate* tlscerts_get_trusted(const gchar* fingerprint);

void tlscerts_free(TLSCertificate* cert);

GList* tlscerts_list(void);

gchar* tlscerts_complete(const gchar* const prefix, gboolean previous, void* context);

void tlscerts_reset_ac(void);

#endif
