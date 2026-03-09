/*
 * otr.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
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
char* otr_libotr_version(void);
char* otr_start_query(void);
void otr_poll(void);
void otr_on_connect(ProfAccount* account);

char* otr_on_message_recv(const char* const barejid, const char* const resource, const char* const message, gboolean* decrypted);
gboolean otr_on_message_send(ProfChatWin* chatwin, const char* const message, gboolean request_receipt, const char* const replace_id);

void otr_keygen(ProfAccount* account);

char* otr_tag_message(const char* const msg);

gboolean otr_key_loaded(void);
gboolean otr_is_secure(const char* const recipient);

gboolean otr_is_trusted(const char* const recipient);
void otr_trust(const char* const recipient);
void otr_untrust(const char* const recipient);

void otr_smp_secret(const char* const recipient, const char* secret);
void otr_smp_question(const char* const recipient, const char* question, const char* answer);
void otr_smp_answer(const char* const recipient, const char* answer);

void otr_end_session(const char* const recipient);

char* otr_get_my_fingerprint(void);
char* otr_get_their_fingerprint(const char* const recipient);

char* otr_encrypt_message(const char* const to, const char* const message);
char* otr_decrypt_message(const char* const from, const char* const message,
                          gboolean* decrypted);

void otr_free_message(char* message);

prof_otrpolicy_t otr_get_policy(const char* const recipient);

#endif
