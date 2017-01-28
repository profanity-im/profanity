/*
 * otr.h
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

#ifndef OTR_OTR_H
#define OTR_OTR_H

#include <libotr/proto.h>
#include <libotr/message.h>

#include "config/accounts.h"
#include "ui/win_types.h"

typedef enum {
    PROF_OTRPOLICY_MANUAL,
    PROF_OTRPOLICY_OPPORTUNISTIC,
    PROF_OTRPOLICY_ALWAYS
} prof_otrpolicy_t;

typedef enum {
    PROF_OTR_SMP_INIT,
    PROF_OTR_SMP_INIT_Q,
    PROF_OTR_SMP_SENDER_FAIL,
    PROF_OTR_SMP_RECEIVER_FAIL,
    PROF_OTR_SMP_ABORT,
    PROF_OTR_SMP_SUCCESS,
    PROF_OTR_SMP_SUCCESS_Q,
    PROF_OTR_SMP_FAIL_Q,
    PROF_OTR_SMP_AUTH,
    PROF_OTR_SMP_AUTH_WAIT
} prof_otr_smp_event_t;

OtrlUserState otr_userstate(void);
OtrlMessageAppOps* otr_messageops(void);
GHashTable* otr_smpinitators(void);

void otr_init(void);
void otr_shutdown(void);
char* otr_libotr_version(void);
char* otr_start_query(void);
void otr_poll(void);
void otr_on_connect(ProfAccount *account);

char* otr_on_message_recv(const char *const barejid, const char *const resource, const char *const message, gboolean *decrypted);
gboolean otr_on_message_send(ProfChatWin *chatwin, const char *const message, gboolean request_receipt);

void otr_keygen(ProfAccount *account);

char* otr_tag_message(const char *const msg);

gboolean otr_key_loaded(void);
gboolean otr_is_secure(const char *const recipient);

gboolean otr_is_trusted(const char *const recipient);
void otr_trust(const char *const recipient);
void otr_untrust(const char *const recipient);

void otr_smp_secret(const char *const recipient, const char *secret);
void otr_smp_question(const char *const recipient, const char *question, const char *answer);
void otr_smp_answer(const char *const recipient, const char *answer);

void otr_end_session(const char *const recipient);

char* otr_get_my_fingerprint(void);
char* otr_get_their_fingerprint(const char *const recipient);

char* otr_encrypt_message(const char *const to, const char *const message);
char* otr_decrypt_message(const char *const from, const char *const message,
    gboolean *decrypted);

void otr_free_message(char *message);

prof_otrpolicy_t otr_get_policy(const char *const recipient);

#endif
