/*
 * ui.h
 *
 * Copyright (C) 2012 - 2017 James Booth <boothj5@gmail.com>
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

#ifndef UI_UI_H
#define UI_UI_H

#include "config.h"

#include "config/tlscerts.h"
#include "config/account.h"
#include "command/cmd_funcs.h"
#include "ui/win_types.h"
#include "xmpp/muc.h"

#ifdef HAVE_LIBOTR
#include "otr/otr.h"
#endif

#define NO_ME           1
#define NO_DATE         2
#define NO_EOL          4
#define NO_COLOUR_FROM  8
#define NO_COLOUR_DATE  16

typedef enum {
    PROF_MSG_PLAIN,
    PROF_MSG_OTR,
    PROF_MSG_PGP
} prof_enc_t;

// core UI
void ui_init(void);
void ui_load_colours(void);
void ui_update(void);
void ui_close(void);
void ui_redraw(void);
void ui_resize(void);
void ui_focus_win(ProfWin *window);
void ui_sigwinch_handler(int sig);
void ui_handle_otr_error(const char *const barejid, const char *const message);
unsigned long ui_get_idle_time(void);
void ui_reset_idle_time(void);
void ui_print_system_msg_from_recipient(const char *const barejid, const char *message);
void ui_close_connected_win(int index);
int ui_close_all_wins(void);
int ui_close_read_wins(void);
void ui_close_win(int index);
int ui_win_unread(int index);
char* ui_ask_password(void);
char* ui_get_line(void);
char* ui_ask_pgp_passphrase(const char *hint, int prev_fail);
void ui_contact_online(char *barejid, Resource *resource, GDateTime *last_activity);
void ui_contact_typing(const char *const barejid, const char *const resource);
void ui_disconnected(void);
void ui_room_join(const char *const roomjid, gboolean focus);
void ui_switch_to_room(const char *const roomjid);
void ui_room_destroy(const char *const roomjid);
void ui_room_destroyed(const char *const roomjid, const char *const reason, const char *const new_jid,
    const char *const password);
void ui_room_kicked(const char *const roomjid, const char *const actor, const char *const reason);
void ui_room_banned(const char *const roomjid, const char *const actor, const char *const reason);
void ui_leave_room(const char *const roomjid);
void ui_show_roster(void);
void ui_hide_roster(void);
void ui_roster_add(const char *const barejid, const char *const name);
void ui_roster_remove(const char *const barejid);
void ui_contact_already_in_group(const char *const contact, const char *const group);
void ui_contact_not_in_group(const char *const contact, const char *const group);
void ui_group_added(const char *const contact, const char *const group);
void ui_group_removed(const char *const contact, const char *const group);
void ui_contact_offline(char *barejid, char *resource, char *status);
void ui_handle_recipient_error(const char *const recipient, const char *const err_msg);
void ui_handle_error(const char *const err_msg);
void ui_clear_win_title(void);
void ui_goodbye_title(void);
void ui_handle_room_configuration_form_error(const char *const roomjid, const char *const message);
void ui_handle_room_config_submit_result(const char *const roomjid);
void ui_handle_room_config_submit_result_error(const char *const roomjid, const char *const message);
void ui_show_lines(ProfWin *window, gchar** lines);
void ui_redraw_all_room_rosters(void);
void ui_show_all_room_rosters(void);
void ui_hide_all_room_rosters(void);
void ui_handle_software_version_error(const char *const roomjid, const char *const message);
void ui_show_software_version(const char *const jid, const char *const  presence, const char *const name,
    const char *const version, const char *const os);
void ui_prune_wins(void);
void ui_auto_away(char *message, gint time, resource_presence_t res_presence);
void ui_handle_login_account_success(ProfAccount *account, gboolean secured);
void ui_update_presence(const resource_presence_t resource_presence, const char *const message, const char *const show);
void ui_invalid_command_usage(const char *const cmd, void (*setting_func)(void));
gboolean ui_win_has_unsaved_form(int num);

// Chat window
ProfChatWin* chatwin_new(const char *const barejid);
void chatwin_incoming_msg(ProfChatWin *chatwin, const char *const resource, const char *const message,
    GDateTime *timestamp, gboolean win_created, prof_enc_t enc_mode);
void chatwin_receipt_received(ProfChatWin *chatwin, const char *const id);
void chatwin_recipient_gone(ProfChatWin *chatwin);
void chatwin_outgoing_msg(ProfChatWin *chatwin, const char *const message, char *id, prof_enc_t enc_mode,
    gboolean request_receipt);
void chatwin_outgoing_carbon(ProfChatWin *chatwin, const char *const message, prof_enc_t enc_mode);
void chatwin_contact_online(ProfChatWin *chatwin, Resource *resource, GDateTime *last_activity);
void chatwin_contact_offline(ProfChatWin *chatwin, char *resource, char *status);
char* chatwin_get_string(ProfChatWin *chatwin);
#ifdef HAVE_LIBOTR
void chatwin_otr_secured(ProfChatWin *chatwin, gboolean trusted);
void chatwin_otr_unsecured(ProfChatWin *chatwin);
void chatwin_otr_trust(ProfChatWin *chatwin);
void chatwin_otr_untrust(ProfChatWin *chatwin);
void chatwin_otr_smp_event(ProfChatWin *chatwin, prof_otr_smp_event_t event, void *data);
#endif
void chatwin_set_enctext(ProfChatWin *chatwin, const char *const enctext);
void chatwin_unset_enctext(ProfChatWin *chatwin);
void chatwin_set_incoming_char(ProfChatWin *chatwin, const char *const ch);
void chatwin_unset_incoming_char(ProfChatWin *chatwin);
void chatwin_set_outgoing_char(ProfChatWin *chatwin, const char *const ch);
void chatwin_unset_outgoing_char(ProfChatWin *chatwin);

// MUC window
void mucwin_role_change(ProfMucWin *mucwin, const char *const role, const char *const actor, const char *const reason);
void mucwin_affiliation_change(ProfMucWin *mucwin, const char *const affiliation, const char *const actor,
    const char *const reason);
void mucwin_role_and_affiliation_change(ProfMucWin *mucwin, const char *const role,
    const char *const affiliation, const char *const actor, const char *const reason);
void mucwin_occupant_role_change(ProfMucWin *mucwin, const char *const nick, const char *const role,
    const char *const actor, const char *const reason);
void mucwin_occupant_affiliation_change(ProfMucWin *mucwin, const char *const nick,
    const char *const affiliation, const char *const actor, const char *const reason);
void mucwin_occupant_role_and_affiliation_change(ProfMucWin *mucwin, const char *const nick,
    const char *const role, const char *const affiliation, const char *const actor, const char *const reason);
void mucwin_roster(ProfMucWin *mucwin, GList *occupants, const char *const presence);
void mucwin_history(ProfMucWin *mucwin, const char *const nick, GDateTime *timestamp, const char *const message);
void mucwin_message(ProfMucWin *mucwin, const char *const nick, const char *const message, GSList *mentions, GList *triggers);
void mucwin_subject(ProfMucWin *mucwin, const char *const nick, const char *const subject);
void mucwin_requires_config(ProfMucWin *mucwin);
void mucwin_info(ProfMucWin *mucwin);
void mucwin_show_role_list(ProfMucWin *mucwin, muc_role_t role);
void mucwin_show_affiliation_list(ProfMucWin *mucwin, muc_affiliation_t affiliation);
void mucwin_room_info_error(ProfMucWin *mucwin, const char *const error);
void mucwin_room_disco_info(ProfMucWin *mucwin, GSList *identities, GSList *features);
void mucwin_occupant_kicked(ProfMucWin *mucwin, const char *const nick, const char *const actor,
    const char *const reason);
void mucwin_occupant_banned(ProfMucWin *mucwin, const char *const nick, const char *const actor,
    const char *const reason);
void mucwin_broadcast(ProfMucWin *mucwin, const char *const message);
void mucwin_occupant_offline(ProfMucWin *mucwin, const char *const nick);
void mucwin_occupant_online(ProfMucWin *mucwin, const char *const nick, const char *const roles,
    const char *const affiliation, const char *const show, const char *const status);
void mucwin_occupant_nick_change(ProfMucWin *mucwin, const char *const old_nick, const char *const nick);
void mucwin_nick_change(ProfMucWin *mucwin, const char *const nick);
void mucwin_occupant_presence(ProfMucWin *mucwin, const char *const nick, const char *const show,
    const char *const status);
void mucwin_update_occupants(ProfMucWin *mucwin);
void mucwin_show_occupants(ProfMucWin *mucwin);
void mucwin_hide_occupants(ProfMucWin *mucwin);
void mucwin_affiliation_list_error(ProfMucWin *mucwin, const char *const affiliation, const char *const error);
void mucwin_handle_affiliation_list(ProfMucWin *mucwin, const char *const affiliation, GSList *jids);
void mucwin_affiliation_set_error(ProfMucWin *mucwin, const char *const jid, const char *const affiliation,
    const char *const error);
void mucwin_role_set_error(ProfMucWin *mucwin, const char *const nick, const char *const role, const char *const error);
void mucwin_role_list_error(ProfMucWin *mucwin, const char *const role, const char *const error);
void mucwin_handle_role_list(ProfMucWin *mucwin, const char *const role, GSList *nicks);
void mucwin_kick_error(ProfMucWin *mucwin, const char *const nick, const char *const error);
char* mucwin_get_string(ProfMucWin *mucwin);
void mucwin_set_enctext(ProfMucWin *mucwin, const char *const enctext);
void mucwin_unset_enctext(ProfMucWin *mucwin);
void mucwin_set_message_char(ProfMucWin *mucwin, const char *const ch);
void mucwin_unset_message_char(ProfMucWin *mucwin);

// MUC private chat window
void privwin_incoming_msg(ProfPrivateWin *privatewin, const char *const message, GDateTime *timestamp);
void privwin_outgoing_msg(ProfPrivateWin *privwin, const char *const message);
void privwin_message_occupant_offline(ProfPrivateWin *privwin);

void privwin_message_left_room(ProfPrivateWin *privwin);

char* privwin_get_string(ProfPrivateWin *privwin);
void privwin_occupant_offline(ProfPrivateWin *privwin);
void privwin_occupant_kicked(ProfPrivateWin *privwin, const char *const actor, const char *const reason);
void privwin_occupant_banned(ProfPrivateWin *privwin, const char *const actor, const char *const reason);
void privwin_occupant_online(ProfPrivateWin *privwin);

void privwin_room_destroyed(ProfPrivateWin *privwin);
void privwin_room_left(ProfPrivateWin *privwin);
void privwin_room_kicked(ProfPrivateWin *privwin, const char *const actor, const char *const reason);
void privwin_room_banned(ProfPrivateWin *privwin, const char *const actor, const char *const reason);
void privwin_room_joined(ProfPrivateWin *privwin);

// MUC room config window
void mucconfwin_handle_configuration(ProfMucConfWin *confwin, DataForm *form);
void mucconfwin_show_form(ProfMucConfWin *confwin);
void mucconfwin_show_form_field(ProfMucConfWin *confwin, DataForm *form, char *tag);
void mucconfwin_form_help(ProfMucConfWin *confwin);
void mucconfwin_field_help(ProfMucConfWin *confwin, char *tag);
char* mucconfwin_get_string(ProfMucConfWin *confwin);

// xml console
void xmlwin_show(ProfXMLWin *xmlwin, const char *const msg);
char* xmlwin_get_string(ProfXMLWin *xmlwin);

// Input window
char* inp_readline(void);
void inp_nonblocking(gboolean reset);

// Console window
void cons_show(const char *const msg, ...);
void cons_show_padded(int pad, const char *const msg, ...);
void cons_about(void);
void cons_help(void);
void cons_show_help(const char *const cmd, CommandHelp *help);
void cons_bad_cmd_usage(const char *const cmd);
void cons_navigation_help(void);
void cons_prefs(void);
void cons_show_ui_prefs(void);
void cons_show_desktop_prefs(void);
void cons_show_chat_prefs(void);
void cons_show_log_prefs(void);
void cons_show_presence_prefs(void);
void cons_show_connection_prefs(void);
void cons_show_otr_prefs(void);
void cons_show_pgp_prefs(void);
void cons_show_account(ProfAccount *account);
void cons_debug(const char *const msg, ...);
void cons_show_error(const char *const cmd, ...);
void cons_show_contacts(GSList *list);
void cons_show_roster(GSList *list);
void cons_show_roster_group(const char *const group, GSList *list);
void cons_show_wins(gboolean unread);
char* cons_get_string(ProfConsoleWin *conswin);
void cons_show_status(const char *const barejid);
void cons_show_info(PContact pcontact);
void cons_show_caps(const char *const fulljid, resource_presence_t presence);
void cons_show_themes(GSList *themes);
void cons_show_scripts(GSList *scripts);
void cons_show_script(const char *const script, GSList *commands);
void cons_show_aliases(GList *aliases);
void cons_show_login_success(ProfAccount *account, gboolean secured);
void cons_show_account_list(gchar **accounts);
void cons_show_room_list(GSList *room, const char *const conference_node);
void cons_show_bookmarks(const GList *list);
void cons_show_disco_items(GSList *items, const char *const jid);
void cons_show_disco_info(const char *from, GSList *identities, GSList *features);
void cons_show_room_invite(const char *const invitor, const char *const room, const char *const reason);
void cons_check_version(gboolean not_available_msg);
void cons_show_typing(const char *const barejid);
void cons_show_incoming_room_message(const char *const nick, const char *const room, const int win_index,
    gboolean mention, GList *triggers, int unread);
void cons_show_incoming_message(const char *const short_from, const int win_index, int unread);
void cons_show_incoming_private_message(const char *const nick, const char *const room, const int win_index, int unread);
void cons_show_room_invites(GList *invites);
void cons_show_received_subs(void);
void cons_show_sent_subs(void);
void cons_alert(void);
void cons_theme_setting(void);
void cons_resource_setting(void);
void cons_privileges_setting(void);
void cons_beep_setting(void);
void cons_console_setting(void);
void cons_flash_setting(void);
void cons_tray_setting(void);
void cons_splash_setting(void);
void cons_encwarn_setting(void);
void cons_tlsshow_setting(void);
void cons_vercheck_setting(void);
void cons_occupants_setting(void);
void cons_roster_setting(void);
void cons_presence_setting(void);
void cons_wrap_setting(void);
void cons_winstidy_setting(void);
void cons_time_setting(void);
void cons_wintitle_setting(void);
void cons_notify_setting(void);
void cons_show_desktop_prefs(void);
void cons_states_setting(void);
void cons_outtype_setting(void);
void cons_intype_setting(void);
void cons_gone_setting(void);
void cons_history_setting(void);
void cons_carbons_setting(void);
void cons_receipts_setting(void);
void cons_log_setting(void);
void cons_chlog_setting(void);
void cons_grlog_setting(void);
void cons_autoaway_setting(void);
void cons_reconnect_setting(void);
void cons_autoping_setting(void);
void cons_autoconnect_setting(void);
void cons_inpblock_setting(void);
void cons_winpos_setting(void);
void cons_show_contact_online(PContact contact, Resource *resource, GDateTime *last_activity);
void cons_show_contact_offline(PContact contact, char *resource, char *status);
void cons_theme_properties(void);
void cons_theme_colours(void);
void cons_show_tlscert(TLSCertificate *cert);
void cons_show_tlscert_summary(TLSCertificate *cert);

// title bar
void title_bar_set_presence(contact_presence_t presence);

// status bar
void status_bar_inactive(const int win);
void status_bar_active(const int win);
void status_bar_new(const int win);
void status_bar_set_all_inactive(void);

// roster window
void rosterwin_roster(void);

// occupants window
void occupantswin_occupants(const char *const room);

// window interface
ProfWin* win_create_console(void);
ProfWin* win_create_xmlconsole(void);
ProfWin* win_create_chat(const char *const barejid);
ProfWin* win_create_muc(const char *const roomjid);
ProfWin* win_create_muc_config(const char *const title, DataForm *form);
ProfWin* win_create_private(const char *const fulljid);
ProfWin* win_create_plugin(const char *const plugin_name, const char *const tag);
void win_update_virtual(ProfWin *window);
void win_free(ProfWin *window);
gboolean win_notify_remind(ProfWin *window);
int win_unread(ProfWin *window);
void win_resize(ProfWin *window);
void win_hide_subwin(ProfWin *window);
void win_show_subwin(ProfWin *window);
void win_refresh_without_subwin(ProfWin *window);
void win_refresh_with_subwin(ProfWin *window);

void win_print(ProfWin *window, theme_item_t theme_item, const char ch, const char *const message, ...);
void win_println(ProfWin *window, theme_item_t theme_item, const char ch, const char *const message, ...);
void win_println_indent(ProfWin *window, int pad, const char *const message, ...);

void win_append(ProfWin *window, theme_item_t theme_item, const char *const message, ...);
void win_appendln(ProfWin *window, theme_item_t theme_item, const char *const message, ...);

void win_append_highlight(ProfWin *window, theme_item_t theme_item, const char *const message, ...);
void win_appendln_highlight(ProfWin *window, theme_item_t theme_item, const char *const message, ...);

char* win_get_title(ProfWin *window);
void win_show_occupant(ProfWin *window, Occupant *occupant);
void win_show_occupant_info(ProfWin *window, const char *const room, Occupant *occupant);
void win_show_contact(ProfWin *window, PContact contact);
void win_show_info(ProfWin *window, PContact contact);
void win_clear(ProfWin *window);
char* win_to_string(ProfWin *window);

// desktop notifications
void notifier_initialise(void);
void notifier_uninit(void);
void notify_typing(const char *const name);
void notify_message(const char *const name, int win, const char *const text);
void notify_room_message(const char *const nick, const char *const room, int win, const char *const text);
void notify_remind(void);
void notify_invite(const char *const from, const char *const room, const char *const reason);
void notify(const char *const message, int timeout, const char *const category);
void notify_subscription(const char *const from);

#endif
