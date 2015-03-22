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

#include <gpgme.h>

#include "log.h"

static const char *libversion;

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
        log_error("GPG: Could not list keys: %s %s", gpgme_strsource(error), gpgme_strerror(error));
        return NULL;
    }

    error = gpgme_op_keylist_start(ctx, NULL, 1);
    if (error == GPG_ERR_NO_ERROR) {
        while (!error) {
            error = gpgme_op_keylist_next(ctx, &key);
            if (error) {
                break;
            }
            result = g_slist_append(result, strdup(key->uids->uid));
            gpgme_key_release(key);
        }
    } else {
        log_error("GPG: Could not list keys: %s %s", gpgme_strsource(error), gpgme_strerror(error));
    }

    gpgme_release(ctx);

    return result;
}

const char*
p_gpg_libver(void)
{
    return libversion;
}

