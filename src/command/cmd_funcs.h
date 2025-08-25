/*
 * cmd_funcs.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 * Copyright (C) 2019 - 2025 Michael Vetter <jubalh@iodoru.org>
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

#ifndef COMMAND_CMD_FUNCS_H
#define COMMAND_CMD_FUNCS_H

#include "ui/win_types.h"

// Command help strings
typedef struct cmd_help_t
{
    gchar* tags[20];
    gchar* synopsis[50];
    gchar* desc;
    gchar* args[128][2];
    gchar* examples[20];
} CommandHelp;

/*
 * Command structure
 *
 * cmd - The command string including leading '/'
 * parser - The function used to parse arguments
 * min_args - Minimum number of arguments
 * max_args - Maximum number of arguments, -1 for infinite
 * setting_func - Function to display current settings to the console
 * sub_funcs - Optional list of functions mapped to the first argument
 * func - Main function to call when no arguments, or sub_funcs not implemented
 * help - A help struct containing usage info etc
 */
typedef struct cmd_t
{
    gchar* cmd;
    gchar** (*parser)(const char* const inp, int min, int max, gboolean* result);
    int min_args;
    int max_args;
    void (*setting_func)(void);
    struct
    {
        const char* cmd;
        gboolean (*func)(ProfWin* window, const char* const command, gchar** args);
    } sub_funcs[50];
    gboolean (*func)(ProfWin* window, const char* const command, gchar** args);
    CommandHelp help;
} Command;

gboolean cmd_process_input(ProfWin* window, char* inp);
void cmd_execute_connect(ProfWin* window, const char* const account);

gboolean cmd_about(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_autoaway(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_autoconnect(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_autoping(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_beep(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_caps(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_logging(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_clear(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_close(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_connect(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_disco(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_sendfile(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_lastactivity(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_disconnect(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_flash(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_tray(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_gone(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_group(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_help(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_history(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_carbons(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_receipts(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_info(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_intype(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_invite(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_join(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_leave(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_log(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_msg(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_nick(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_notify(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_pgp(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_privacy(ProfWin* window, const char* const command, gchar** args);
#ifdef HAVE_LIBGPGME
gboolean cmd_ox(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_ox_log(ProfWin* window, const char* const command, gchar** args);
#endif // HAVE_LIBGPGME
gboolean cmd_outtype(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_prefs(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_priority(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_quit(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_reconnect(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_room(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_rooms(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_bookmark(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_bookmark_ignore(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_roster(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_software(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_splash(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_states(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_status_get(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_status_set(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_sub(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_theme(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_wintitle(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_vercheck(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_who(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_win(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_alias(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_xmlconsole(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_ping(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_form(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_occupants(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_kick(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_ban(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_subject(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_affiliation(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_role(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_privileges(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_presence(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_wrap(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_time(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_resource(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_inpblock(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_titlebar(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_titlebar_show_hide(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_mainwin(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_statusbar(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_inputwin(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_script(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_export(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_charset(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_console(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_command_list(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_command_exec(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_change_password(ProfWin* window, const char* const command, gchar** args);

gboolean cmd_plugins(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_plugins_sourcepath(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_plugins_install(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_plugins_update(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_plugins_uninstall(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_plugins_load(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_plugins_unload(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_plugins_reload(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_plugins_python_version(ProfWin* window, const char* const command, gchar** args);

gboolean cmd_blocked(ProfWin* window, const char* const command, gchar** args);

gboolean cmd_account(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_account_list(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_account_show(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_account_add(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_account_remove(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_account_enable(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_account_disable(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_account_rename(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_account_default(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_account_set(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_account_clear(ProfWin* window, const char* const command, gchar** args);

gboolean cmd_tls_certpath(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_tls_trust(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_tls_trusted(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_tls_revoke(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_tls_cert(ProfWin* window, const char* const command, gchar** args);

gboolean cmd_otr_char(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_otr_log(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_otr_libver(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_otr_policy(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_otr_gen(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_otr_myfp(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_otr_theirfp(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_otr_start(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_otr_end(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_otr_trust(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_otr_untrust(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_otr_secret(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_otr_question(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_otr_answer(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_otr_sendfile(ProfWin* window, const char* const command, gchar** args);

gboolean cmd_wins(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_wins_unread(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_wins_attention(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_wins_prune(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_wins_swap(ProfWin* window, const char* const command, gchar** args);

gboolean cmd_form_field(ProfWin* window, char* tag, gchar** args);

gboolean cmd_omemo_gen(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_omemo_char(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_omemo_log(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_omemo_start(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_omemo_end(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_omemo_fingerprint(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_omemo_trust(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_omemo_untrust(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_omemo_trust_mode(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_omemo_policy(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_omemo_clear_device_list(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_omemo_qrcode(ProfWin* window, const char* const command, gchar** args);

gboolean cmd_save(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_reload(ProfWin* window, const char* const command, gchar** args);

gboolean cmd_paste(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_color(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_avatar(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_os(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_correction(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_correct(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_slashguard(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_serversoftware(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_url_open(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_url_save(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_executable_avatar(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_executable_urlopen(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_executable_urlsave(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_executable_editor(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_executable_vcard_photo(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_mam(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_editor(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_correct_editor(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_redraw(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_silence(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_register(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_mood(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_strophe(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_stamp(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_vcard(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_vcard_add(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_vcard_remove(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_vcard_get(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_vcard_photo(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_vcard_refresh(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_vcard_set(ProfWin* window, const char* const command, gchar** args);
gboolean cmd_vcard_save(ProfWin* window, const char* const command, gchar** args);

#endif
