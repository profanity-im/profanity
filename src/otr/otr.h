/*
 * otr.h
 *
 * Copyright (C) 2012 - 2014 James Booth <boothj5@gmail.com>
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

#ifndef OTR_H
#define OTR_H

#include "config/accounts.h"

void otr_init_module(void);

void (*otr_init)(void);
char* (*otr_libotr_version)(void);
char* (*otr_start_query)(void);
void (*otr_on_connect)(ProfAccount *account);
void (*otr_keygen)(ProfAccount *account);

gboolean (*otr_key_loaded)(void);
gboolean (*otr_is_secure)(const char * const recipient);

gboolean (*otr_is_trusted)(const char * const recipient);
void (*otr_trust)(const char * const recipient);
void (*otr_untrust)(const char * const recipient);

void (*otr_end_session)(const char * const recipient);

char * (*otr_get_my_fingerprint)(void);
char * (*otr_get_their_fingerprint)(const char * const recipient);

char * (*otr_encrypt_message)(const char * const to, const char * const message);
char * (*otr_decrypt_message)(const char * const from, const char * const message,
    gboolean *was_decrypted);

void (*otr_free_message)(char *message);

#endif
