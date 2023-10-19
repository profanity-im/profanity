/*
 * database.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2020 - 2023 Michael Vetter <jubalh@idoru.org>
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
GSList* log_database_get_previous_chat(const gchar* const contact_barejid, const char* start_time, char* end_time, gboolean from_start, gboolean flip, gboolean limit_results);
ProfMessage* log_database_get_limits_info(const gchar* const contact_barejid, gboolean is_last, char* from_timestamp);
void log_database_close(void);

#endif // DATABASE_H
