/*
 * gpg.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef PGP_GPG_H
#define PGP_GPG_H

typedef struct pgp_key_t
{
    gchar* id;
    gchar* name;
    gchar* fp;
    gboolean encrypt;
    gboolean sign;
    gboolean certify;
    gboolean authenticate;
    gboolean secret;
} ProfPGPKey;

typedef struct pgp_pubkeyid_t
{
    gchar* id;
    gboolean received;
} ProfPGPPubKeyId;

void p_gpg_init(void);
void p_gpg_on_connect(const gchar* const barejid);
void p_gpg_on_disconnect(void);
GHashTable* p_gpg_list_keys(void);
void p_gpg_free_keys(GHashTable* keys);
gboolean p_gpg_addkey(const gchar* const jid, const gchar* const keyid);
GHashTable* p_gpg_pubkeys(void);
gboolean p_gpg_valid_key(const gchar* const keyid, gchar** err_str);
gboolean p_gpg_available(const gchar* const barejid);
const gchar* p_gpg_libver(void);
gchar* p_gpg_sign(const gchar* const str, const gchar* const fp);
void p_gpg_verify(const gchar* const barejid, const gchar* const sign);
gchar* p_gpg_encrypt(const gchar* const barejid, const gchar* const message, const gchar* const fp);
gchar* p_gpg_decrypt(const gchar* const cipher);
void p_gpg_free_decrypted(gchar* decrypted);
gchar* p_gpg_autocomplete_key(const gchar* const search_str, gboolean previous, void* context);
void p_gpg_autocomplete_key_reset(void);
gchar* p_gpg_format_fp_str(gchar* fp);
gchar* p_gpg_get_pubkey(const gchar* const keyid);
gboolean p_gpg_is_public_key_format(const gchar* buffer);
ProfPGPKey* p_gpg_import_pubkey(const gchar* buffer);

ProfPGPKey* p_gpg_key_new(void);
void p_gpg_free_key(ProfPGPKey* key);

#endif
