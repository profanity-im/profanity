/*
 * database.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2020 - 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef DATABASE_H
#define DATABASE_H

#include <glib.h>
#include "config/account.h"
#include "xmpp/xmpp.h"

#define MESSAGES_TO_RETRIEVE 10

gboolean log_database_init(ProfAccount* account);
gboolean log_database_add_incoming(ProfMessage* message);
void log_database_add_outgoing_chat(const gchar* const id, const gchar* const barejid, const gchar* const message, const gchar* const replace_id, prof_enc_t enc);
void log_database_add_outgoing_muc(const gchar* const id, const gchar* const barejid, const gchar* const message, const gchar* const replace_id, prof_enc_t enc);
void log_database_add_outgoing_muc_pm(const gchar* const id, const gchar* const barejid, const gchar* const message, const gchar* const replace_id, prof_enc_t enc);
GSList* log_database_get_previous_chat(const gchar* const contact_barejid, const gchar* const start_time, const gchar* const end_time, gboolean from_start, gboolean flip);
GSList* log_database_get_previous_muc(const gchar* const room_jid, const gchar* const start_time, const gchar* const end_time, gboolean from_start, gboolean flip);
int log_database_get_chat_count(const gchar* const contact_barejid, const gchar* const start_time, const gchar* const end_time);
int log_database_get_muc_count(const gchar* const room_jid, const gchar* const start_time, const gchar* const end_time);
ProfMessage* log_database_get_limits_info(const gchar* const contact_barejid, gboolean is_last);
ProfMessage* log_database_get_limits_info_muc(const gchar* const room_jid, gboolean is_last);
gboolean log_database_update_archive_id(const prof_msg_type_t type, const gchar* const room_or_contact_jid, const gchar* const stanza_id, const gchar* const archive_id);
char* log_database_get_decrypted_message(const prof_msg_type_t type, const char* const from_jid, const char* const to_jid, const gchar* const stanza_id, const gchar* const archive_id);
void log_database_close(void);

#endif // DATABASE_H
