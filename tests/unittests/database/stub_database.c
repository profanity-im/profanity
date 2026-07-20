/*
 * stub_database.c
 *
 * Copyright (C) 2020 - 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#include <glib.h>
#include "prof_cmocka.h"

#include "database.h"

gboolean
log_database_init(ProfAccount* account)
{
    return TRUE;
}
gboolean
log_database_add_incoming(ProfMessage* message)
{
    return TRUE;
}
void
log_database_add_outgoing_chat(const char* const id, const char* const barejid, const char* const message, const char* const replace_id, prof_enc_t enc)
{
}
void
log_database_add_outgoing_muc(const char* const id, const char* const barejid, const char* const message, const char* const replace_id, prof_enc_t enc)
{
}
void
log_database_add_outgoing_muc_pm(const char* const id, const char* const barejid, const char* const message, const char* const replace_id, prof_enc_t enc)
{
}
void
log_database_close(void)
{
}

gboolean
log_database_update_archive_id(const prof_msg_type_t type, const char* const room_or_contact_jid, const char* const stanza_id, const char* const archive_id)
{
    return FALSE;
}

char*
log_database_get_decrypted_message(const prof_msg_type_t type, const char* const from_jid, const char* const to_jid, const char* const stanza_id, const char* const archive_id)
{
    return NULL;
}
