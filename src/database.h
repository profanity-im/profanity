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
void log_database_add_incoming(ProfMessage* message);
void log_database_add_outgoing_chat(const char* const id, const char* const barejid, const char* const message, const char* const replace_id, prof_enc_t enc);
void log_database_add_outgoing_muc(const char* const id, const char* const barejid, const char* const message, const char* const replace_id, prof_enc_t enc);
void log_database_add_outgoing_muc_pm(const char* const id, const char* const barejid, const char* const message, const char* const replace_id, prof_enc_t enc);
GSList* log_database_get_previous_chat(const gchar* const contact_barejid, const char* start_time, char* end_time, gboolean from_start, gboolean flip);
ProfMessage* log_database_get_limits_info(const gchar* const contact_barejid, gboolean is_last);
void log_database_close(void);

#endif // DATABASE_H
