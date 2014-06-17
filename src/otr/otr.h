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

#include <libotr/proto.h>
#include <libotr/message.h>

#include "config/accounts.h"

typedef enum {
    PROF_OTRPOLICY_MANUAL,
    PROF_OTRPOLICY_OPPORTUNISTIC,
    PROF_OTRPOLICY_ALWAYS
} prof_otrpolicy_t;

void otr_init_module(void);

OtrlUserState otr_userstate(void);
OtrlMessageAppOps* otr_messageops(void);
GHashTable* otr_smpinitators(void);

void (*otr_init)(void);
char* (*otr_libotr_version)(void);
char* (*otr_start_query)(void);
void (*otr_poll)(void);
void (*otr_on_connect)(ProfAccount *account);
void (*otr_keygen)(ProfAccount *account);

gboolean (*otr_key_loaded)(void);
gboolean (*otr_is_secure)(const char * const recipient);

gboolean (*otr_is_trusted)(const char * const recipient);
void (*otr_trust)(const char * const recipient);
void (*otr_untrust)(const char * const recipient);

void (*otr_smp_secret)(const char * const recipient, const char *secret);
void (*otr_smp_question)(const char * const recipient, const char *question, const char *answer);
void (*otr_smp_answer)(const char * const recipient, const char *answer);

void (*otr_end_session)(const char * const recipient);

char * (*otr_get_my_fingerprint)(void);
char * (*otr_get_their_fingerprint)(const char * const recipient);

char * (*otr_encrypt_message)(const char * const to, const char * const message);
char * (*otr_decrypt_message)(const char * const from, const char * const message,
    gboolean *was_decrypted);

void (*otr_free_message)(char *message);

prof_otrpolicy_t (*otr_get_policy)(const char * const recipient);

#endif
