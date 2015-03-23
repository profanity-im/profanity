/*
 * gpg.c
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

#include <locale.h>
#include <string.h>
#include <stdlib.h>

#include <glib.h>
#include <gpgme.h>

#include "pgp/gpg.h"
#include "log.h"

#define PGP_FOOTER "-----END PGP SIGNATURE-----"

static const char *libversion;

static char* _remove_header_footer(char *str);

void
p_gpg_init(void)
{
    libversion = gpgme_check_version(NULL);
    log_debug("GPG: Found gpgme version: %s", libversion);
    gpgme_set_locale(NULL, LC_CTYPE, setlocale(LC_CTYPE, NULL));
}

GSList *
p_gpg_list_keys(void)
{
    gpgme_error_t error;
    gpgme_ctx_t ctx;
    gpgme_key_t key;
    GSList *result = NULL;

    error = gpgme_new(&ctx);
    if (error) {
        log_error("GPG: Could not list keys. %s %s", gpgme_strsource(error), gpgme_strerror(error));
        return NULL;
    }

    error = gpgme_op_keylist_start(ctx, NULL, 1);
    if (error == GPG_ERR_NO_ERROR) {
        while (!error) {
            error = gpgme_op_keylist_next(ctx, &key);
            if (error) {
                break;
            }

            ProfPGPKey *p_pgpkey = malloc(sizeof(ProfPGPKey));
            p_pgpkey->id = strdup(key->subkeys->keyid);
            p_pgpkey->name = strdup(key->uids->uid);
            p_pgpkey->fp = strdup(key->subkeys->fpr);

            result = g_slist_append(result, p_pgpkey);

            gpgme_key_release(key);
        }
    } else {
        log_error("GPG: Could not list keys. %s %s", gpgme_strsource(error), gpgme_strerror(error));
    }

    gpgme_release(ctx);

    return result;
}

const char*
p_gpg_libver(void)
{
    return libversion;
}

void
p_gpg_free_key(ProfPGPKey *key)
{
    if (key) {
        free(key->id);
        free(key->name);
        free(key->fp);
        free(key);
    }
}

char*
p_gpg_sign_str(const char * const str, const char * const fp)
{
	gpgme_ctx_t ctx;
	gpgme_error_t error = gpgme_new(&ctx);
	if (error) {
        log_error("GPG: Failed to create gpgme context. %s %s", gpgme_strsource(error), gpgme_strerror(error));
		return NULL;
	}

	gpgme_key_t key = NULL;
	error = gpgme_get_key(ctx, fp, &key, 1);
	if (error || key == NULL) {
        log_error("GPG: Failed to get key. %s %s", gpgme_strsource(error), gpgme_strerror(error));
		gpgme_release (ctx);
		return NULL;
	}

	gpgme_signers_clear(ctx);
	error = gpgme_signers_add(ctx, key);
	if (error) {
        log_error("GPG: Failed to load signer. %s %s", gpgme_strsource(error), gpgme_strerror(error));
		gpgme_release(ctx);
		return NULL;
	}

	gpgme_data_t str_data;
	gpgme_data_t signed_data;
    char *str_or_empty = NULL;
    if (str) {
        str_or_empty = strdup(str);
    } else {
        str_or_empty = strdup("");
    }
	gpgme_data_new_from_mem(&str_data, str_or_empty, strlen(str_or_empty), 1);
	gpgme_data_new(&signed_data);

	gpgme_set_armor(ctx,1);
	error = gpgme_op_sign(ctx,str_data,signed_data,GPGME_SIG_MODE_DETACH);
	if (error) {
        log_error("GPG: Failed to sign string. %s %s", gpgme_strsource(error), gpgme_strerror(error));
		gpgme_release(ctx);
		return NULL;
	}

    char *result = NULL;
	gpgme_data_release(str_data);

    size_t len = 0;
	char *signed_str = gpgme_data_release_and_get_mem(signed_data, &len);
	if (signed_str != NULL) {
		signed_str[len] = 0;
		result = _remove_header_footer(signed_str);
	}
	gpgme_free(signed_str);
	gpgme_release(ctx);
    free(str_or_empty);

	return result;
}

static char*
_remove_header_footer(char *str)
{
	char *pointer = str;

	int newlines = 0;
	while (newlines < 3) {
		if (pointer[0] == '\n') {
			newlines++;
        }
		pointer++;

		if (strlen(pointer) == 0) {
			return NULL;
        }
	}

	char *stripped = malloc(strlen(pointer)+1-strlen(PGP_FOOTER));
	strncpy(stripped,pointer,strlen(pointer)-strlen(PGP_FOOTER));
	stripped[strlen(pointer)-strlen(PGP_FOOTER)] = '\0';

	return stripped;
}