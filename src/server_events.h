/*
 * server_events.h
 *
 * Copyright (C) 2012, 2013 James Booth <boothj5@gmail.com>
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

#ifndef SERVER_EVENTS_H
#define SERVER_EVENTS_H

void handle_error_message(const char *from, const char *err_msg);
void handle_login_account_success(char *account_name);
void handle_lost_connection(void);
void handle_failed_login(void);
void handle_software_version_result(const char * const jid, const char * const  presence,
    const char * const name, const char * const version, const char * const os);
void handle_disco_info(const char *from, GSList *identities, GSList *features);

#endif
