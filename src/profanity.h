/*
 * profanity.h
 *
 * Copyright (C) 2012 James Booth <boothj5@gmail.com>
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

#ifndef PROFANITY_H
#define PROFANITY_H

typedef struct roster_entry_t {
    char *name;
    char *jid;
} jabber_roster_entry;

void prof_run(const int disable_tls, char *log_level);

void prof_handle_login_success(const char *jid);
void prof_handle_lost_connection(void);
void prof_handle_failed_login(void);
void prof_handle_typing(char *from);
void prof_handle_contact_online(char *contact, char *show, char *status);
void prof_handle_contact_offline(char *contact, char *show, char *status);
void prof_handle_incoming_message(char *from, char *message);
void prof_handle_error_message(const char *from, const char *err_msg);
void prof_handle_roster(GSList *roster);

#endif
