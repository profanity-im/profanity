/*
 * commands.h
 *
 * Copyright (C) 2012 - 2015 James Booth <boothj5@gmail.com>
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

#ifndef COMMANDS_H
#define COMMANDS_H

#include "ui/ui.h"

// Command help strings
typedef struct cmd_help_t {
    const gchar *usage;
    const gchar *short_help;
    const gchar *long_help[50];
} CommandHelp;

/*
 * Command structure
 *
 * cmd - The command string including leading '/'
 * func - The function to execute for the command
 * parser - The function used to parse arguments
 * min_args - Minimum number of arguments
 * max_args - Maximum number of arguments
 * help - A help struct containing usage info etc
 */
typedef struct cmd_t {
    gchar *cmd;
    gboolean (*func)(ProfWin *window, gchar **args, struct cmd_help_t help);
    gchar** (*parser)(const char * const inp, int min, int max, gboolean *result);
    int min_args;
    int max_args;
    void (*setting_func)(void);
    CommandHelp help;
} Command;

gboolean cmd_execute_alias(ProfWin *window, const char * const inp, gboolean *ran);
gboolean cmd_execute_default(ProfWin *window, const char * inp);

gboolean cmd_about(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_account(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_autoaway(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_autoconnect(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_autoping(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_away(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_beep(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_caps(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_chat(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_chlog(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_clear(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_close(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_connect(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_decline(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_disco(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_disconnect(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_dnd(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_flash(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_gone(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_grlog(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_group(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_help(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_history(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_carbons(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_receipts(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_info(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_intype(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_invite(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_invites(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_join(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_leave(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_log(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_msg(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_nick(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_notify(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_online(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_otr(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_pgp(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_outtype(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_prefs(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_priority(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_quit(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_reconnect(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_room(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_rooms(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_bookmark(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_roster(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_software(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_splash(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_states(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_status(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_statuses(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_sub(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_theme(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_tiny(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_titlebar(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_vercheck(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_who(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_win(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_wins(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_winstidy(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_xa(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_alias(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_xmlconsole(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_ping(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_form(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_occupants(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_kick(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_ban(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_subject(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_affiliation(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_role(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_privileges(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_presence(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_wrap(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_time(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_resource(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_inpblock(ProfWin *window, gchar **args, struct cmd_help_t help);
gboolean cmd_encwarn(ProfWin *window, gchar **args, struct cmd_help_t help);

gboolean cmd_form_field(ProfWin *window, char *tag, gchar **args);

#endif
