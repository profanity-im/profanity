/*
 * gpg.h
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

#ifndef PGP_GPG_H
#define PGP_GPG_H

typedef struct pgp_key_t {
    char *id;
    char *name;
    char *fp;
    gboolean encrypt;
    gboolean sign;
    gboolean certify;
    gboolean authenticate;
    gboolean secret;
} ProfPGPKey;

typedef struct pgp_pubkeyid_t {
    char *id;
    gboolean received;
} ProfPGPPubKeyId;

void p_gpg_init(void);
void p_gpg_close(void);
void p_gpg_on_connect(const char *const barejid);
void p_gpg_on_disconnect(void);
GHashTable* p_gpg_list_keys(void);
void p_gpg_free_keys(GHashTable *keys);
gboolean p_gpg_addkey(const char *const jid, const char *const keyid);
GHashTable* p_gpg_pubkeys(void);
gboolean p_gpg_valid_key(const char *const keyid, char **err_str);
gboolean p_gpg_available(const char *const barejid);
const char* p_gpg_libver(void);
char* p_gpg_sign(const char *const str, const char *const fp);
void p_gpg_verify(const char *const barejid, const char *const sign);
char* p_gpg_encrypt(const char *const barejid, const char *const message, const char *const fp);
char* p_gpg_decrypt(const char *const cipher);
void p_gpg_free_decrypted(char *decrypted);
char* p_gpg_autocomplete_key(const char *const search_str, gboolean previous);
void p_gpg_autocomplete_key_reset(void);
char* p_gpg_format_fp_str(char *fp);

#endif
