/*
 * commands.h
 *
 * Copyright (C) 2012 - 2014 James Booth <boothj5@gmail.com>
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

#ifndef COMMANDS_H
#define COMMANDS_H

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
    gboolean (*func)(gchar **args, struct cmd_help_t help);
    gchar** (*parser)(const char * const inp, int min, int max);
    int min_args;
    int max_args;
    void (**setting_func)(void);
    CommandHelp help;
} Command;

gboolean cmd_about(gchar **args, struct cmd_help_t help);
gboolean cmd_account(gchar **args, struct cmd_help_t help);
gboolean cmd_autoaway(gchar **args, struct cmd_help_t help);
gboolean cmd_autoconnect(gchar **args, struct cmd_help_t help);
gboolean cmd_autoping(gchar **args, struct cmd_help_t help);
gboolean cmd_away(gchar **args, struct cmd_help_t help);
gboolean cmd_beep(gchar **args, struct cmd_help_t help);
gboolean cmd_caps(gchar **args, struct cmd_help_t help);
gboolean cmd_chat(gchar **args, struct cmd_help_t help);
gboolean cmd_chlog(gchar **args, struct cmd_help_t help);
gboolean cmd_clear(gchar **args, struct cmd_help_t help);
gboolean cmd_close(gchar **args, struct cmd_help_t help);
gboolean cmd_connect(gchar **args, struct cmd_help_t help);
gboolean cmd_decline(gchar **args, struct cmd_help_t help);
gboolean cmd_disco(gchar **args, struct cmd_help_t help);
gboolean cmd_disconnect(gchar **args, struct cmd_help_t help);
gboolean cmd_dnd(gchar **args, struct cmd_help_t help);
gboolean cmd_duck(gchar **args, struct cmd_help_t help);
gboolean cmd_flash(gchar **args, struct cmd_help_t help);
gboolean cmd_gone(gchar **args, struct cmd_help_t help);
gboolean cmd_grlog(gchar **args, struct cmd_help_t help);
gboolean cmd_group(gchar **args, struct cmd_help_t help);
gboolean cmd_help(gchar **args, struct cmd_help_t help);
gboolean cmd_history(gchar **args, struct cmd_help_t help);
gboolean cmd_info(gchar **args, struct cmd_help_t help);
gboolean cmd_intype(gchar **args, struct cmd_help_t help);
gboolean cmd_invite(gchar **args, struct cmd_help_t help);
gboolean cmd_invites(gchar **args, struct cmd_help_t help);
gboolean cmd_join(gchar **args, struct cmd_help_t help);
gboolean cmd_leave(gchar **args, struct cmd_help_t help);
gboolean cmd_log(gchar **args, struct cmd_help_t help);
gboolean cmd_mouse(gchar **args, struct cmd_help_t help);
gboolean cmd_msg(gchar **args, struct cmd_help_t help);
gboolean cmd_nick(gchar **args, struct cmd_help_t help);
gboolean cmd_notify(gchar **args, struct cmd_help_t help);
gboolean cmd_online(gchar **args, struct cmd_help_t help);
gboolean cmd_otr(gchar **args, struct cmd_help_t help);
gboolean cmd_outtype(gchar **args, struct cmd_help_t help);
gboolean cmd_prefs(gchar **args, struct cmd_help_t help);
gboolean cmd_priority(gchar **args, struct cmd_help_t help);
gboolean cmd_quit(gchar **args, struct cmd_help_t help);
gboolean cmd_reconnect(gchar **args, struct cmd_help_t help);
gboolean cmd_rooms(gchar **args, struct cmd_help_t help);
gboolean cmd_bookmark(gchar **args, struct cmd_help_t help);
gboolean cmd_roster(gchar **args, struct cmd_help_t help);
gboolean cmd_software(gchar **args, struct cmd_help_t help);
gboolean cmd_splash(gchar **args, struct cmd_help_t help);
gboolean cmd_states(gchar **args, struct cmd_help_t help);
gboolean cmd_status(gchar **args, struct cmd_help_t help);
gboolean cmd_statuses(gchar **args, struct cmd_help_t help);
gboolean cmd_sub(gchar **args, struct cmd_help_t help);
gboolean cmd_theme(gchar **args, struct cmd_help_t help);
gboolean cmd_tiny(gchar **args, struct cmd_help_t help);
gboolean cmd_titlebar(gchar **args, struct cmd_help_t help);
gboolean cmd_vercheck(gchar **args, struct cmd_help_t help);
gboolean cmd_who(gchar **args, struct cmd_help_t help);
gboolean cmd_win(gchar **args, struct cmd_help_t help);
gboolean cmd_wins(gchar **args, struct cmd_help_t help);
gboolean cmd_xa(gchar **args, struct cmd_help_t help);
gboolean cmd_alias(gchar **args, struct cmd_help_t help);

#endif
