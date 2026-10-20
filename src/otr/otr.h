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

gchar* otr_on_message_recv(const gchar* const barejid, const gchar* const resource, const gchar* const message, gboolean* decrypted);
gboolean otr_on_message_send(ProfChatWin* chatwin, const gchar* const message, gboolean request_receipt, const gchar* const replace_id);

void otr_keygen(ProfAccount* account);

gchar* otr_tag_message(const gchar* const msg);

gboolean otr_key_loaded(void);
gboolean otr_is_secure(const gchar* const recipient);

gboolean otr_is_trusted(const gchar* const recipient);
void otr_trust(const gchar* const recipient);
void otr_untrust(const gchar* const recipient);

void otr_smp_secret(const gchar* const recipient, const gchar* secret);
void otr_smp_question(const gchar* const recipient, const gchar* question, const gchar* answer);
void otr_smp_answer(const gchar* const recipient, const gchar* answer);

void otr_end_session(const gchar* const recipient);

gchar* otr_get_my_fingerprint(void);
gchar* otr_get_their_fingerprint(const gchar* const recipient);

gchar* otr_encrypt_message(const gchar* const to, const gchar* const message);
gchar* otr_decrypt_message(const gchar* const from, const gchar* const message,
                           gboolean* decrypted);

void otr_free_message(gchar* message);

prof_otrpolicy_t otr_get_policy(const gchar* const recipient);

#endif
