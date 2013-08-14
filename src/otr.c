/*
 * otr.c
 *
 * Copyright (C) 2012, 2013 James Booth <boothj5@gmail.com>
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
 */

#include <libotr/proto.h>
#include <libotr/privkey.h>
#include <glib.h>

#include "otr.h"
#include "ui/ui.h"

static OtrlUserState user_state;

void
otr_init(void)
{
    cons_debug("otr_init()");
    OTRL_INIT;
}

void
otr_account_load(ProfAccount *account)
{
    gcry_error_t err = 0;
    cons_debug("otr_account_load()");
    GString *keys_filename = g_string_new("./");
    g_string_append(keys_filename, account->jid);
    g_string_append(keys_filename, "_keys.txt");

    GString *fp_filename = g_string_new("./");
    g_string_append(fp_filename, account->jid);
    g_string_append(fp_filename, "_fingerprints.txt");

    user_state = otrl_userstate_create();

    err = otrl_privkey_read(user_state, keys_filename->str);
    if (err != 0) {
        cons_debug("Failed to load private keys");
        g_string_free(keys_filename, TRUE);
        g_string_free(fp_filename, TRUE);
        return;
    }

    err = otrl_privkey_read_fingerprints(user_state, fp_filename->str, NULL, NULL);
    if (err != 0) {
        cons_debug("Failed to load fingerprints");
        g_string_free(keys_filename, TRUE);
        g_string_free(fp_filename, TRUE);
        return;
    }

    g_string_free(keys_filename, TRUE);
    g_string_free(fp_filename, TRUE);
}
