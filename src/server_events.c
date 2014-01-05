/*
 * server_events.c
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

#include "log.h"
#include "muc.h"
#include "ui/ui.h"

void
handle_error_message(const char *from, const char *err_msg)
{
    ui_handle_error_message(from, err_msg);

    if (g_strcmp0(err_msg, "conflict") == 0) {
        // remove the room from muc
        Jid *room_jid = jid_create(from);
        if (!muc_get_roster_received(room_jid->barejid)) {
            muc_leave_room(room_jid->barejid);
        }
        jid_destroy(room_jid);
    }
}

void
handle_login_account_success(char *account_name)
{
    ProfAccount *account = accounts_get_account(account_name);
    resource_presence_t resource_presence = accounts_get_login_presence(account->name);
    contact_presence_t contact_presence = contact_presence_from_resource_presence(resource_presence);
    cons_show_login_success(account);
    title_bar_set_status(contact_presence);
    log_info("%s logged in successfully", account->jid);
    ui_current_page_off();
    status_bar_print_message(account->jid);
    status_bar_refresh();

    accounts_free_account(account);
}

