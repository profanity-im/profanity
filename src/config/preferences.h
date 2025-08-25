/*
 * preferences.h
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

#ifndef CONFIG_PREFERENCES_H
#define CONFIG_PREFERENCES_H

#include "config.h"

#include <glib.h>

#define PREFS_MIN_LOG_SIZE 64
#define PREFS_MAX_LOG_SIZE (10 * 1024 * 1024)

// represents all settings in .profrc
// each enum value is mapped to a group and key in .profrc (see preferences.c)
typedef enum {
    PREF_CLEAR_PERSIST_HISTORY,
    PREF_SPLASH,
    PREF_BEEP,
    PREF_VERCHECK,
    PREF_THEME,
    PREF_WINTITLE_SHOW,
    PREF_WINTITLE_GOODBYE,
    PREF_FLASH,
    PREF_TRAY,
    PREF_TRAY_READ,
    PREF_ADV_NOTIFY_DISCO_OR_VERSION,
    PREF_INTYPE,
    PREF_INTYPE_CONSOLE,
    PREF_HISTORY,
    PREF_CARBONS,
    PREF_RECEIPTS_SEND,
    PREF_RECEIPTS_REQUEST,
    PREF_REVEAL_OS,
    PREF_OCCUPANTS,
    PREF_OCCUPANTS_SIZE,
    PREF_OCCUPANTS_JID,
    PREF_OCCUPANTS_OFFLINE,
    PREF_ROSTER,
    PREF_ROSTER_SIZE,
    PREF_ROSTER_OFFLINE,
    PREF_ROSTER_RESOURCE,
    PREF_ROSTER_PRESENCE,
    PREF_ROSTER_STATUS,
    PREF_ROSTER_EMPTY,
    PREF_ROSTER_BY,
    PREF_ROSTER_ORDER,
    PREF_ROSTER_UNREAD,
    PREF_ROSTER_COUNT,
    PREF_ROSTER_COUNT_ZERO,
    PREF_ROSTER_PRIORITY,
    PREF_ROSTER_WRAP,
    PREF_ROSTER_RESOURCE_JOIN,
    PREF_ROSTER_CONTACTS,
    PREF_ROSTER_UNSUBSCRIBED,
    PREF_ROSTER_ROOMS,
    PREF_ROSTER_ROOMS_POS,
    PREF_ROSTER_ROOMS_BY,
    PREF_ROSTER_ROOMS_ORDER,
    PREF_ROSTER_ROOMS_UNREAD,
    PREF_ROSTER_ROOMS_TITLE,
    PREF_ROSTER_PRIVATE,
    PREF_MUC_PRIVILEGES,
    PREF_PRESENCE,
    PREF_WRAP,
    PREF_TIME_CONSOLE,
    PREF_TIME_CHAT,
    PREF_TIME_MUC,
    PREF_TIME_CONFIG,
    PREF_TIME_PRIVATE,
    PREF_TIME_XMLCONSOLE,
    PREF_TIME_STATUSBAR,
    PREF_TIME_LASTACTIVITY,
    PREF_TIME_VCARD,
    PREF_STATUSES,
    PREF_STATUSES_CONSOLE,
    PREF_STATUSES_CHAT,
    PREF_STATUSES_MUC,
    PREF_STATES,
    PREF_OUTTYPE,
    PREF_NOTIFY_TYPING,
    PREF_NOTIFY_TYPING_CURRENT,
    PREF_NOTIFY_CHAT,
    PREF_NOTIFY_CHAT_CURRENT,
    PREF_NOTIFY_CHAT_TEXT,
    PREF_NOTIFY_ROOM,
    PREF_NOTIFY_ROOM_MENTION,
    PREF_NOTIFY_ROOM_TRIGGER,
    PREF_NOTIFY_ROOM_CURRENT,
    PREF_NOTIFY_ROOM_TEXT,
    PREF_NOTIFY_INVITE,
    PREF_NOTIFY_SUB,
    PREF_NOTIFY_MENTION_CASE_SENSITIVE,
    PREF_NOTIFY_MENTION_WHOLE_WORD,
    PREF_CHLOG,
    PREF_DBLOG,
    PREF_GRLOG,
    PREF_AUTOAWAY_CHECK,
    PREF_AUTOAWAY_MODE,
    PREF_AUTOAWAY_MESSAGE,
    PREF_AUTOXA_MESSAGE,
    PREF_CONNECT_ACCOUNT,
    PREF_DEFAULT_ACCOUNT,
    PREF_LOG_ROTATE,
    PREF_LOG_SHARED,
    PREF_OTR_LOG,
    PREF_OTR_POLICY,
    PREF_OTR_SENDFILE,
    PREF_RESOURCE_TITLE,
    PREF_RESOURCE_MESSAGE,
    PREF_INPBLOCK_DYNAMIC,
    PREF_ENC_WARN,
    PREF_TITLEBAR_MUC_TITLE,
    PREF_PGP_LOG,
    PREF_PGP_SENDFILE,
    PREF_PGP_PUBKEY_AUTOIMPORT,
    PREF_TLS_CERTPATH,
    PREF_TLS_SHOW,
    PREF_LASTACTIVITY,
    PREF_CONSOLE_MUC,
    PREF_CONSOLE_PRIVATE,
    PREF_CONSOLE_CHAT,
    PREF_COLOR_NICK,
    PREF_COLOR_NICK_OWN,
    PREF_ROSTER_COLOR_NICK,
    PREF_OCCUPANTS_COLOR_NICK,
    PREF_BOOKMARK_INVITE,
    PREF_ROOM_LIST_CACHE,
    PREF_STATUSBAR_SHOW_NAME,
    PREF_STATUSBAR_SHOW_NUMBER,
    PREF_STATUSBAR_SHOW_READ,
    PREF_STATUSBAR_SELF,
    PREF_STATUSBAR_CHAT,
    PREF_STATUSBAR_ROOM_TITLE,
    PREF_OMEMO_LOG,
    PREF_OMEMO_POLICY,
    PREF_OMEMO_TRUST_MODE,
    PREF_OCCUPANTS_WRAP,
    PREF_CORRECTION_ALLOW,
    PREF_AVATAR_CMD,
    PREF_SLASH_GUARD,
    PREF_MAM,
    PREF_URL_OPEN_CMD,
    PREF_URL_SAVE_CMD,
    PREF_COMPOSE_EDITOR,
    PREF_SILENCE_NON_ROSTER,
    PREF_OUTGOING_STAMP,
    PREF_INCOMING_STAMP,
    PREF_NOTIFY_ROOM_OFFLINE,
    PREF_OX_LOG,
    PREF_MOOD,
    PREF_STROPHE_VERBOSITY,
    PREF_STROPHE_SM_ENABLED,
    PREF_STROPHE_SM_RESEND,
    PREF_VCARD_PHOTO_CMD,
    PREF_STATUSBAR_TABMODE,
} preference_t;

typedef struct prof_alias_t
{
    gchar* name;
    gchar* value;
} ProfAlias;

typedef struct prof_winplacement_t
{
    int titlebar_pos;
    int mainwin_pos;
    int statusbar_pos;
    int inputwin_pos;
} ProfWinPlacement;

void prefs_load(const char* config_file);
void prefs_save(void);
void prefs_close(void);
void prefs_reload(void);

gchar* prefs_autocomplete_boolean_choice(const char* const prefix, gboolean previous, void* context);
void prefs_reset_boolean_choice(void);

gchar* prefs_autocomplete_room_trigger(const char* const prefix, gboolean previous, void* context);
void prefs_reset_room_trigger_ac(void);

gint prefs_get_gone(void);
void prefs_set_gone(gint value);

void prefs_set_notify_remind(gint period);
gint prefs_get_notify_remind(void);

void prefs_set_max_log_size(gint value);
gint prefs_get_max_log_size(void);
gint prefs_get_priority(void);
void prefs_set_reconnect(gint value);
gint prefs_get_reconnect(void);
void prefs_set_autoping(gint value);
gint prefs_get_autoping(void);
void prefs_set_autoping_timeout(gint value);
gint prefs_get_autoping_timeout(void);
gint prefs_get_inpblock(void);
void prefs_set_inpblock(gint value);

void prefs_set_statusbartabs(gint value);
gint prefs_get_statusbartabs(void);
void prefs_set_statusbartablen(gint value);
gint prefs_get_statusbartablen(void);

void prefs_set_occupants_size(gint value);
gint prefs_get_occupants_size(void);
void prefs_set_roster_size(gint value);
gint prefs_get_roster_size(void);

gint prefs_get_autoaway_time(void);
void prefs_set_autoaway_time(gint value);
gint prefs_get_autoxa_time(void);
void prefs_set_autoxa_time(gint value);

gchar** prefs_get_plugins(void);
void prefs_add_plugin(const char* const name);
void prefs_remove_plugin(const char* const name);

gchar* prefs_get_otr_char(void);
gboolean prefs_set_otr_char(char* ch);
gchar* prefs_get_pgp_char(void);
gboolean prefs_set_pgp_char(char* ch);
gchar* prefs_get_omemo_char(void);
gboolean prefs_set_omemo_char(char* ch);
// XEP-0373: OpenPGP for XMPP
char* prefs_get_ox_char(void);
gboolean prefs_set_ox_char(char* ch);

gchar* prefs_get_roster_header_char(void);
void prefs_set_roster_header_char(char* ch);
void prefs_clear_roster_header_char(void);
gchar* prefs_get_roster_contact_char(void);
void prefs_set_roster_contact_char(char* ch);
void prefs_clear_roster_contact_char(void);
gchar* prefs_get_roster_resource_char(void);
void prefs_set_roster_resource_char(char* ch);
void prefs_clear_roster_resource_char(void);
gchar* prefs_get_roster_private_char(void);
void prefs_set_roster_private_char(char* ch);
void prefs_clear_roster_private_char(void);
gchar* prefs_get_roster_room_char(void);
void prefs_set_roster_room_char(char* ch);
void prefs_clear_roster_room_char(void);
gchar* prefs_get_roster_room_private_char(void);
void prefs_set_roster_room_private_char(char* ch);
void prefs_clear_roster_room_private_char(void);
gchar* prefs_get_occupants_char(void);
void prefs_set_occupants_char(char* ch);
void prefs_clear_occupants_char(void);
gchar* prefs_get_occupants_header_char(void);
void prefs_set_occupants_header_char(char* ch);
void prefs_clear_occupants_header_char(void);

gint prefs_get_roster_contact_indent(void);
void prefs_set_roster_contact_indent(gint value);
gint prefs_get_roster_resource_indent(void);
void prefs_set_roster_resource_indent(gint value);
gint prefs_get_roster_presence_indent(void);
void prefs_set_roster_presence_indent(gint value);
gint prefs_get_occupants_indent(void);
void prefs_set_occupants_indent(gint value);

gchar* prefs_get_correction_char(void);
void prefs_set_correction_char(char* ch);

void prefs_add_login(const char* jid);

void prefs_set_tray_timer(gint value);
gint prefs_get_tray_timer(void);

gboolean prefs_add_alias(const char* const name, const char* const value);
gboolean prefs_remove_alias(const char* const name);
gchar* prefs_get_alias(const char* const name);
GList* prefs_get_aliases(void);
void prefs_free_aliases(GList* aliases);

gboolean prefs_add_room_notify_trigger(const char* const text);
gboolean prefs_remove_room_notify_trigger(const char* const text);
GList* prefs_get_room_notify_triggers(void);

ProfWinPlacement* prefs_get_win_placement(void);
void prefs_free_win_placement(ProfWinPlacement* placement);

gboolean prefs_titlebar_pos_up(void);
gboolean prefs_titlebar_pos_down(void);
gboolean prefs_mainwin_pos_up(void);
gboolean prefs_mainwin_pos_down(void);
gboolean prefs_statusbar_pos_up(void);
gboolean prefs_statusbar_pos_down(void);
gboolean prefs_inputwin_pos_up(void);
gboolean prefs_inputwin_pos_down(void);
ProfWinPlacement* prefs_create_profwin_placement(int titlebar, int mainwin, int statusbar, int inputwin);
void prefs_save_win_placement(ProfWinPlacement* placement);

gboolean prefs_get_boolean(preference_t pref);
void prefs_set_boolean(preference_t pref, gboolean value);
gchar* prefs_get_string(preference_t pref);
gchar* prefs_get_string_with_locale(preference_t pref, gchar* locale);
void prefs_set_string(preference_t pref, const gchar* new_value);
void prefs_set_string_with_option(preference_t pref, char* option, char* value);
void prefs_set_string_list_with_option(preference_t pref, char* option, const gchar* const* values);

char* prefs_get_tls_certpath(void);

gboolean prefs_do_chat_notify(gboolean current_win);
gboolean prefs_do_room_notify(gboolean current_win, const char* const roomjid, const char* const mynick,
                              const char* const theirnick, const char* const message, gboolean mention, gboolean trigger_found);
gboolean prefs_do_room_notify_mention(const char* const roomjid, int unread, gboolean mention, gboolean trigger);
GList* prefs_message_get_triggers(const char* const message);

void prefs_set_room_notify(const char* const roomjid, gboolean value);
void prefs_set_room_notify_mention(const char* const roomjid, gboolean value);
void prefs_set_room_notify_trigger(const char* const roomjid, gboolean value);
gboolean prefs_reset_room_notify(const char* const roomjid);
gboolean prefs_has_room_notify(const char* const roomjid);
gboolean prefs_has_room_notify_mention(const char* const roomjid);
gboolean prefs_has_room_notify_trigger(const char* const roomjid);
gboolean prefs_get_room_notify(const char* const roomjid);
gboolean prefs_get_room_notify_mention(const char* const roomjid);
gboolean prefs_get_room_notify_trigger(const char* const roomjid);

#endif
