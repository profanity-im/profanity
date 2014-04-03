/*
 * ui.h
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

#ifndef UI_UI_H
#define UI_UI_H

#include "config.h"

#include <wchar.h>

#include <glib.h>
#ifdef HAVE_NCURSESW_NCURSES_H
#include <ncursesw/ncurses.h>
#elif HAVE_NCURSES_H
#include <ncurses.h>
#endif

#include "contact.h"
#include "jid.h"
#include "ui/window.h"
#include "xmpp/xmpp.h"

#define INP_WIN_MAX 1000

void ui_init_module(void);
void console_init_module(void);
void inputwin_init_module(void);
void notifier_init_module(void);
void statusbar_init_module(void);
void titlebar_init_module(void);

// ui startup and control
void (*ui_init)(void);
void (*ui_load_colours)(void);
void (*ui_update_screen)(void);
void (*ui_close)(void);
void (*ui_resize)(const int ch, const char * const input,
    const int size);
GSList* (*ui_get_recipients)(void);
void (*ui_handle_special_keys)(const wint_t * const ch, const char * const inp,
    const int size);
gboolean (*ui_switch_win)(const int i);
void (*ui_next_win)(void);
void (*ui_previous_win)(void);
void (*ui_gone_secure)(const char * const recipient, gboolean trusted);
void (*ui_gone_insecure)(const char * const recipient);
void (*ui_trust)(const char * const recipient);
void (*ui_untrust)(const char * const recipient);
unsigned long (*ui_get_idle_time)(void);
void (*ui_reset_idle_time)(void);
void (*ui_new_chat_win)(const char * const to);
void (*ui_print_system_msg_from_recipient)(const char * const from, const char *message);
gint (*ui_unread)(void);
void (*ui_close_connected_win)(int index);
int (*ui_close_all_wins)(void);
int (*ui_close_read_wins)(void);

// current window actions
void (*ui_close_current)(void);
void (*ui_clear_current)(void);
win_type_t (*ui_current_win_type)(void);
int (*ui_current_win_index)(void);
gboolean (*ui_current_win_is_otr)(void);
void (*ui_current_set_otr)(gboolean value);
char* (*ui_current_recipient)(void);
void (*ui_current_print_line)(const char * const msg, ...);
void (*ui_current_print_formatted_line)(const char show_char, int attrs, const char * const msg, ...);
void (*ui_current_error_line)(const char * const msg);
void (*ui_current_page_off)(void);
void (*ui_current_update_virtual)(void);

win_type_t (*ui_win_type)(int index);
char * (*ui_recipient)(int index);
void (*ui_close_win)(int index);
gboolean (*ui_win_exists)(int index);
int (*ui_win_unread)(int index);
char * (*ui_ask_password)(void);

// ui events
void (*ui_contact_typing)(const char * const from);
void (*ui_incoming_msg)(const char * const from, const char * const message,
    GTimeVal *tv_stamp, gboolean priv);
void (*ui_disconnected)(void);
void (*ui_recipient_gone)(const char * const barejid);
void (*ui_outgoing_msg)(const char * const from, const char * const to,
    const char * const message);
void (*ui_room_join)(char *room);
void (*ui_room_roster)(const char * const room, GList *roster, const char * const presence);
void (*ui_room_history)(const char * const room_jid, const char * const nick,
    GTimeVal tv_stamp, const char * const message);
void (*ui_room_message)(const char * const room_jid, const char * const nick,
    const char * const message);
void (*ui_room_subject)(const char * const room_jid,
    const char * const subject);
void (*ui_room_broadcast)(const char * const room_jid,
    const char * const message);
void (*ui_room_member_offline)(const char * const room, const char * const nick);
void (*ui_room_member_online)(const char * const room,
    const char * const nick, const char * const show, const char * const status);
void (*ui_room_member_nick_change)(const char * const room,
    const char * const old_nick, const char * const nick);
void (*ui_room_nick_change)(const char * const room, const char * const nick);
void (*ui_room_member_presence)(const char * const room,
    const char * const nick, const char * const show, const char * const status);
void (*ui_roster_add)(const char * const barejid, const char * const name);
void (*ui_roster_remove)(const char * const barejid);
void (*ui_contact_already_in_group)(const char * const contact, const char * const group);
void (*ui_contact_not_in_group)(const char * const contact, const char * const group);
void (*ui_group_added)(const char * const contact, const char * const group);
void (*ui_group_removed)(const char * const contact, const char * const group);
void (*ui_chat_win_contact_online)(PContact contact, Resource *resource, GDateTime *last_activity);
void (*ui_chat_win_contact_offline)(PContact contact, char *resource, char *status);
void (*ui_handle_recipient_not_found)(const char * const recipient, const char * const err_msg);
void (*ui_handle_recipient_error)(const char * const recipient, const char * const err_msg);
void (*ui_handle_error)(const char * const err_msg);
void (*ui_clear_win_title)(void);

// contact status functions
void (*ui_status_room)(const char * const contact);
void (*ui_status)(void);
void (*ui_status_private)(void);

void (*ui_create_duck_win)(void);
void (*ui_open_duck_win)(void);
void (*ui_duck)(const char * const query);
void (*ui_duck_result)(const char * const result);
gboolean (*ui_duck_exists)(void);

void (*ui_tidy_wins)(void);
void (*ui_prune_wins)(void);

// create windows
void (*create_title_bar)(void);
void (*create_status_bar)(void);
void (*create_input_window)(void);

// title bar actions
void (*title_bar_update_virtual)(void);
void (*title_bar_resize)(void);
void (*title_bar_console)(void);
void (*title_bar_set_presence)(contact_presence_t presence);
void (*title_bar_set_recipient)(const char * const from);
void (*title_bar_set_typing)(gboolean is_typing);

// console window actions
void (*cons_show)(const char * const msg, ...);
void (*cons_about)(void);
void (*cons_help)(void);
void (*cons_navigation_help)(void);
void (*cons_prefs)(void);
void (*cons_show_ui_prefs)(void);
void (*cons_show_desktop_prefs)(void);
void (*cons_show_chat_prefs)(void);
void (*cons_show_log_prefs)(void);
void (*cons_show_presence_prefs)(void);
void (*cons_show_connection_prefs)(void);
void (*cons_show_account)(ProfAccount *account);
void (*cons_debug)(const char * const msg, ...);
void (*cons_show_time)(void);
void (*cons_show_word)(const char * const word);
void (*cons_show_error)(const char * const cmd, ...);
void (*cons_show_contacts)(GSList * list);
void (*cons_show_roster)(GSList * list);
void (*cons_show_roster_group)(const char * const group, GSList * list);
void (*cons_show_wins)(void);
void (*cons_show_status)(const char * const barejid);
void (*cons_show_info)(PContact pcontact);
void (*cons_show_caps)(const char * const contact, Resource *resource);
void (*cons_show_themes)(GSList *themes);
void (*cons_show_aliases)(GList *aliases);
void (*cons_show_login_success)(ProfAccount *account);
void (*cons_show_software_version)(const char * const jid,
    const char * const presence, const char * const name,
    const char * const version, const char * const os);
void (*cons_show_account_list)(gchar **accounts);
void (*cons_show_room_list)(GSList *room, const char * const conference_node);
void (*cons_show_bookmarks)(const GList *list);
void (*cons_show_disco_items)(GSList *items, const char * const jid);
void (*cons_show_disco_info)(const char *from, GSList *identities, GSList *features);
void (*cons_show_room_invite)(const char * const invitor, const char * const room,
    const char * const reason);
void (*cons_check_version)(gboolean not_available_msg);
void (*cons_show_typing)(const char * const barejid);
void (*cons_show_incoming_message)(const char * const short_from, const int win_index);
void (*cons_show_room_invites)(GSList *invites);
void (*cons_show_received_subs)(void);
void (*cons_show_sent_subs)(void);
void (*cons_alert)(void);
void (*cons_theme_setting)(void);
void (*cons_beep_setting)(void);
void (*cons_flash_setting)(void);
void (*cons_splash_setting)(void);
void (*cons_vercheck_setting)(void);
void (*cons_mouse_setting)(void);
void (*cons_statuses_setting)(void);
void (*cons_titlebar_setting)(void);
void (*cons_notify_setting)(void);
void (*cons_show_desktop_prefs)(void);
void (*cons_states_setting)(void);
void (*cons_outtype_setting)(void);
void (*cons_intype_setting)(void);
void (*cons_gone_setting)(void);
void (*cons_history_setting)(void);
void (*cons_log_setting)(void);
void (*cons_chlog_setting)(void);
void (*cons_grlog_setting)(void);
void (*cons_otr_log_setting)(void);
void (*cons_otrwarn_setting)(void);
void (*cons_autoaway_setting)(void);
void (*cons_reconnect_setting)(void);
void (*cons_autoping_setting)(void);
void (*cons_priority_setting)(void);
void (*cons_autoconnect_setting)(void);
void (*cons_show_contact_online)(PContact contact, Resource *resource, GDateTime *last_activity);
void (*cons_show_contact_offline)(PContact contact, char *resource, char *status);

// status bar actions
void (*status_bar_update_virtual)(void);
void (*status_bar_resize)(void);
void (*status_bar_clear)(void);
void (*status_bar_clear_message)(void);
void (*status_bar_get_password)(void);
void (*status_bar_print_message)(const char * const msg);
void (*status_bar_inactive)(const int win);
void (*status_bar_active)(const int win);
void (*status_bar_new)(const int win);
void (*status_bar_set_all_inactive)(void);
void (*status_bar_current)(int i);

// input window actions
wint_t (*inp_get_char)(char *input, int *size);
void (*inp_win_reset)(void);
void (*inp_win_resize)(const char * input, const int size);
void (*inp_put_back)(void);
void (*inp_non_block)(void);
void (*inp_block)(void);
void (*inp_get_password)(char *passwd);
void (*inp_replace_input)(char *input, const char * const new_input, int *size);

// desktop notifier actions
void (*notifier_init)(void);
void (*notifier_uninit)(void);

void (*notify_typing)(const char * const handle);
void (*notify_message)(const char * const handle, int win);
void (*notify_room_message)(const char * const handle, const char * const room,
    int win);
void (*notify_remind)(void);
void (*notify_invite)(const char * const from, const char * const room,
    const char * const reason);
void (*notify_subscription)(const char * const from);

#endif
