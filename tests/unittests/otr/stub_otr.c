/*
 * stub_otr.c
 *
 * Copyright (C) 2015 - 2016 James Booth <boothj5@gmail.com>
 * Copyright (C) 2020 - 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include <libotr/proto.h>
#include <libotr/message.h>
#include <glib.h>

#include "prof_cmocka.h"

#include "config/account.h"

#include "otr/otr.h"

OtrlUserState
otr_userstate(void)
{
    return NULL;
}

OtrlMessageAppOps*
otr_messageops(void)
{
    return NULL;
}

GHashTable*
otr_smpinitators(void)
{
    return NULL;
}

void
otr_init(void)
{
}

char*
otr_libotr_version(void)
{
    return mock_ptr_type(char*);
}

char*
otr_start_query(void)
{
    return mock_ptr_type(char*);
}

void
otr_poll(void)
{
}
void
otr_on_connect(ProfAccount* account)
{
}
gchar*
otr_on_message_recv(const gchar* const barejid, const gchar* const resource, const gchar* const message, gboolean* was_decrypted)
{
    return NULL;
}
gboolean
otr_on_message_send(ProfChatWin* chatwin, const gchar* const message, gboolean request_receipt, const gchar* const replace_id)
{
    return FALSE;
}

void
otr_keygen(ProfAccount* account)
{
    check_expected(account);
}

gboolean
otr_key_loaded(void)
{
    return mock_type(gboolean);
}

gchar*
otr_tag_message(const gchar* const msg)
{
    return NULL;
}

gboolean
otr_is_secure(const gchar* const recipient)
{
    return FALSE;
}

gboolean
otr_is_trusted(const gchar* const recipient)
{
    return FALSE;
}

void
otr_trust(const gchar* const recipient)
{
}
void
otr_untrust(const gchar* const recipient)
{
}

void
otr_smp_secret(const gchar* const recipient, const gchar* secret)
{
}
void
otr_smp_question(const gchar* const recipient, const gchar* question, const gchar* answer)
{
}
void
otr_smp_answer(const gchar* const recipient, const gchar* answer)
{
}

void
otr_end_session(const gchar* const recipient)
{
}

gchar*
otr_get_my_fingerprint(void)
{
    return mock_ptr_type(char*);
}

gchar*
otr_get_their_fingerprint(const gchar* const recipient)
{
    check_expected(recipient);
    return mock_ptr_type(char*);
}

gchar*
otr_encrypt_message(const gchar* const to, const gchar* const message)
{
    return NULL;
}

gchar*
otr_decrypt_message(const gchar* const from, const gchar* const message,
                    gboolean* was_decrypted)
{
    return NULL;
}

void
otr_free_message(gchar* message)
{
}

prof_otrpolicy_t
otr_get_policy(const gchar* const recipient)
{
    return PROF_OTRPOLICY_MANUAL;
}
