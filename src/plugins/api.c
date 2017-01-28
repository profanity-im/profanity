/*
 * api.c
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

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <glib.h>

#include "profanity.h"
#include "log.h"
#include "common.h"
#include "config/theme.h"
#include "command/cmd_defs.h"
#include "event/server_events.h"
#include "event/client_events.h"
#include "plugins/callbacks.h"
#include "plugins/autocompleters.h"
#include "plugins/themes.h"
#include "plugins/settings.h"
#include "plugins/disco.h"
#include "ui/ui.h"
#include "ui/window_list.h"

void
api_cons_alert(void)
{
    cons_alert();
}

int
api_cons_show(const char * const message)
{
    if (message == NULL) {
        log_warning("%s", "prof_cons_show failed, message is NULL");
        return 0;
    }

    char *parsed = str_replace(message, "\r\n", "\n");
    cons_show("%s", parsed);
    free(parsed);

    return 1;
}

int
api_cons_show_themed(const char *const group, const char *const key, const char *const def, const char *const message)
{
    if (message == NULL) {
        log_warning("%s", "prof_cons_show_themed failed, message is NULL");
        return 0;
    }

    char *parsed = str_replace(message, "\r\n", "\n");
    theme_item_t themeitem = plugin_themes_get(group, key, def);
    ProfWin *console = wins_get_console();
    win_println(console, themeitem, '-', "%s", parsed);

    free(parsed);

    return 1;
}

int
api_cons_bad_cmd_usage(const char *const cmd)
{
    if (cmd == NULL) {
        log_warning("%s", "prof_cons_bad_cmd_usage failed, cmd is NULL");
        return 0;
    }

    cons_bad_cmd_usage(cmd);

    return 1;
}

void
api_register_command(const char *const plugin_name, const char *command_name, int min_args, int max_args,
    char **synopsis, const char *description, char *arguments[][2], char **examples,
    void *callback, void(*callback_exec)(PluginCommand *command, gchar **args), void(*callback_destroy)(void *callback))
{
    PluginCommand *command = malloc(sizeof(PluginCommand));
    command->command_name = strdup(command_name);
    command->min_args = min_args;
    command->max_args = max_args;
    command->callback = callback;
    command->callback_exec = callback_exec;
    command->callback_destroy = callback_destroy;

    CommandHelp *help = malloc(sizeof(CommandHelp));

    help->tags[0] = NULL;

    int i = 0;
    for (i = 0; synopsis[i] != NULL; i++) {
        help->synopsis[i] = strdup(synopsis[i]);
    }
    help->synopsis[i] = NULL;

    help->desc = strdup(description);
    for (i = 0; arguments[i][0] != NULL; i++) {
        help->args[i][0] = strdup(arguments[i][0]);
        help->args[i][1] = strdup(arguments[i][1]);
    }
    help->args[i][0] = NULL;
    help->args[i][1] = NULL;

    for (i = 0; examples[i] != NULL; i++) {
        help->examples[i] = strdup(examples[i]);
    }
    help->examples[i] = NULL;

    command->help = help;

    callbacks_add_command(plugin_name, command);
}

void
api_register_timed(const char *const plugin_name, void *callback, int interval_seconds,
    void (*callback_exec)(PluginTimedFunction *timed_function), void(*callback_destroy)(void *callback))
{
    PluginTimedFunction *timed_function = malloc(sizeof(PluginTimedFunction));
    timed_function->callback = callback;
    timed_function->callback_exec = callback_exec;
    timed_function->callback_destroy = callback_destroy;
    timed_function->interval_seconds = interval_seconds;
    timed_function->timer = g_timer_new();

    callbacks_add_timed(plugin_name, timed_function);
}

void
api_completer_add(const char *const plugin_name, const char *key, char **items)
{
    autocompleters_add(plugin_name, key, items);
}

void
api_completer_remove(const char *const plugin_name, const char *key, char **items)
{
    autocompleters_remove(plugin_name, key, items);
}

void
api_completer_clear(const char *const plugin_name, const char *key)
{
    autocompleters_clear(plugin_name, key);
}

void
api_filepath_completer_add(const char *const plugin_name, const char *prefix)
{
    autocompleters_filepath_add(plugin_name, prefix);
}

void
api_notify(const char *message, const char *category, int timeout_ms)
{
    notify(message, timeout_ms, category);
}

void
api_send_line(char *line)
{
    ProfWin *current = wins_get_current();
    cmd_process_input(current, line);
}

char *
api_get_current_recipient(void)
{
    ProfWin *current = wins_get_current();
    if (current->type == WIN_CHAT) {
        ProfChatWin *chatwin = (ProfChatWin*)current;
        assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
        return chatwin->barejid;
    } else {
        return NULL;
    }
}

char *
api_get_current_muc(void)
{
    ProfWin *current = wins_get_current();
    if (current->type == WIN_MUC) {
        ProfMucWin *mucwin = (ProfMucWin*)current;
        assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
        return mucwin->roomjid;
    } else {
        return NULL;
    }
}

char *
api_get_current_nick(void)
{
    ProfWin *current = wins_get_current();
    if (current->type == WIN_MUC) {
        ProfMucWin *mucwin = (ProfMucWin*)current;
        assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
        return muc_nick(mucwin->roomjid);
    } else {
        return NULL;
    }
}

char**
api_get_current_occupants(void)
{
    ProfWin *current = wins_get_current();
    if (current->type == WIN_MUC) {
        ProfMucWin *mucwin = (ProfMucWin*)current;
        assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
        GList *occupants_list = muc_roster(mucwin->roomjid);
        char **result = malloc((g_list_length(occupants_list) + 1) * sizeof(char*));
        GList *curr = occupants_list;
        int i = 0;
        while (curr) {
            Occupant *occupant = curr->data;
            result[i++] = strdup(occupant->nick);
            curr = g_list_next(curr);
        }
        result[i] = NULL;
        return result;
    } else {
        return NULL;
    }
}

int
api_current_win_is_console(void)
{
    ProfWin *current = wins_get_current();
    if (current && current->type == WIN_CONSOLE) {
        return 1;
    } else {
        return 0;
    }
}

char*
api_get_room_nick(const char *barejid)
{
    return muc_nick(barejid);
}

void
api_log_debug(const char *message)
{
    log_debug("%s", message);
}

void
api_log_info(const char *message)
{
    log_info("%s", message);
}

void
api_log_warning(const char *message)
{
    log_warning("%s", message);
}

void
api_log_error(const char *message)
{
    log_error("%s", message);
}

int
api_win_exists(const char *tag)
{
    return (wins_get_plugin(tag) != NULL);
}

void
api_win_create(
    const char *const plugin_name,
    const char *tag,
    void *callback,
    void(*callback_exec)(PluginWindowCallback *window_callback, const char *tag, const char * const line),
    void(*callback_destroy)(void *callback))
{
    if (callbacks_win_exists(plugin_name, tag)) {
        if (callback_destroy) {
            callback_destroy(callback);
        }
        return;
    }

    PluginWindowCallback *window = malloc(sizeof(PluginWindowCallback));
    window->callback = callback;
    window->callback_exec = callback_exec;
    window->callback_destroy = callback_destroy;

    callbacks_add_window_handler(plugin_name, tag, window);

    wins_new_plugin(plugin_name, tag);

    // set status bar active
    ProfPluginWin *pluginwin = wins_get_plugin(tag);
    int num = wins_get_num((ProfWin*)pluginwin);
    status_bar_active(num);
}

int
api_win_focus(const char *tag)
{
    if (tag == NULL) {
        log_warning("%s", "prof_win_focus failed, tag is NULL");
        return 0;
    }

    ProfPluginWin *pluginwin = wins_get_plugin(tag);
    if (pluginwin == NULL) {
        log_warning("prof_win_focus failed, no window with tag: %s", tag);
        return 0;
    }

    ui_focus_win((ProfWin*)pluginwin);

    return 1;
}

int
api_win_show(const char *tag, const char *line)
{
    if (tag == NULL) {
        log_warning("%s", "prof_win_show failed, tag is NULL");
        return 0;
    }
    if (line == NULL) {
        log_warning("%s", "prof_win_show failed, line is NULL");
        return 0;
    }

    ProfPluginWin *pluginwin = wins_get_plugin(tag);
    if (pluginwin == NULL) {
        log_warning("prof_win_show failed, no window with tag: %s", tag);
        return 0;
    }

    ProfWin *window = (ProfWin*)pluginwin;
    win_println(window, THEME_DEFAULT, '!', "%s", line);

    return 1;
}

int
api_win_show_themed(const char *tag, const char *const group, const char *const key, const char *const def, const char *line)
{
    if (tag == NULL) {
        log_warning("%s", "prof_win_show_themed failed, tag is NULL");
        return 0;
    }
    if (line == NULL) {
        log_warning("%s", "prof_win_show_themed failed, line is NULL");
        return 0;
    }

    ProfPluginWin *pluginwin = wins_get_plugin(tag);
    if (pluginwin == NULL) {
        log_warning("prof_win_show_themed failed, no window with tag: %s", tag);
        return 0;
    }

    theme_item_t themeitem = plugin_themes_get(group, key, def);
    ProfWin *window = (ProfWin*)pluginwin;
    win_println(window, themeitem, '!', "%s", line);

    return 1;
}

int
api_send_stanza(const char *const stanza)
{
    return connection_send_stanza(stanza);
}

gboolean
api_settings_boolean_get(const char *const group, const char *const key, gboolean def)
{
    return plugin_settings_boolean_get(group, key, def);
}

void
api_settings_boolean_set(const char *const group, const char *const key, gboolean value)
{
    plugin_settings_boolean_set(group, key, value);
}

char*
api_settings_string_get(const char *const group, const char *const key, const char *const def)
{
    return plugin_settings_string_get(group, key, def);
}

void
api_settings_string_set(const char *const group, const char *const key, const char *const value)
{
    plugin_settings_string_set(group, key, value);
}

char**
api_settings_string_list_get(const char *const group, const char *const key)
{
    return plugin_settings_string_list_get(group, key);
}

void
api_settings_string_list_add(const char *const group, const char *const key, const char *const value)
{
    plugin_settings_string_list_add(group, key, value);
}

int
api_settings_string_list_remove(const char *const group, const char *const key, const char *const value)
{
    return plugin_settings_string_list_remove(group, key, value);
}

int
api_settings_string_list_clear(const char *const group, const char *const key)
{
    return plugin_settings_string_list_clear(group, key);
}

int
api_settings_int_get(const char *const group, const char *const key, int def)
{
    return plugin_settings_int_get(group, key, def);
}

void
api_settings_int_set(const char *const group, const char *const key, int value)
{
    plugin_settings_int_set(group, key, value);
}

void
api_incoming_message(const char *const barejid, const char *const resource, const char *const message)
{
    sv_ev_incoming_message((char*)barejid, (char*)resource, (char*)message, NULL, NULL);

    // TODO handle all states
    sv_ev_activity((char*)barejid, (char*)resource, FALSE);
}

void
api_disco_add_feature(char *plugin_name, char *feature)
{
    if (feature == NULL) {
        log_warning("%s", "api_disco_add_feature failed, feature is NULL");
        return;
    }

    disco_add_feature(plugin_name, feature);

    caps_reset_ver();

    // resend presence to update server's disco info data for this client
    if (connection_get_status() == JABBER_CONNECTED) {
        resource_presence_t last_presence = accounts_get_last_presence(session_get_account_name());
        cl_ev_presence_send(last_presence, 0);
    }
}

void
api_encryption_reset(const char *const barejid)
{
    if (barejid == NULL) {
        log_warning("%s", "api_encryption_reset failed, barejid is NULL");
        return;
    }

    ProfChatWin *chatwin = wins_get_chat(barejid);
    if (chatwin == NULL) {
        log_warning("%s", "api_encryption_reset failed, could not find chat window for %s", barejid);
        return;
    }

#ifdef HAVE_LIBGPGME
    if (chatwin->pgp_send) {
        chatwin->pgp_send = FALSE;
        win_println((ProfWin*)chatwin, THEME_DEFAULT, '!', "PGP encryption disabled.");
    }
#endif

#ifdef HAVE_LIBOTR
    if (chatwin->is_otr) {
        chatwin_otr_unsecured(chatwin);
        otr_end_session(chatwin->barejid);
    }
#endif
}

int
api_chat_set_titlebar_enctext(const char *const barejid, const char *const enctext)
{
    if (enctext == NULL) {
        log_warning("%s", "api_chat_set_titlebar_enctext failed, enctext is NULL");
        return 0;
    }

    if (barejid == NULL) {
        log_warning("%s", "api_chat_set_titlebar_enctext failed, barejid is NULL");
        return 0;
    }

    ProfChatWin *chatwin = wins_get_chat(barejid);
    if (chatwin == NULL) {
        log_warning("%s", "api_chat_set_titlebar_enctext failed, could not find chat window for %s", barejid);
        return 0;
    }

    chatwin_set_enctext(chatwin, enctext);

    return 1;
}

int
api_chat_unset_titlebar_enctext(const char *const barejid)
{
    if (barejid == NULL) {
        log_warning("%s", "api_chat_unset_titlebar_enctext failed, barejid is NULL");
        return 0;
    }

    ProfChatWin *chatwin = wins_get_chat(barejid);
    if (chatwin == NULL) {
        log_warning("%s", "api_chat_unset_titlebar_enctext failed, could not find chat window for %s", barejid);
        return 0;
    }

    chatwin_unset_enctext(chatwin);

    return 1;
}

int
api_chat_set_incoming_char(const char *const barejid, const char *const ch)
{
    if (ch == NULL) {
        log_warning("%s", "api_chat_set_incoming_char failed, ch is NULL");
        return 0;
    }

    if (strlen(ch) != 1) {
        log_warning("%s", "api_chat_set_incoming_char failed, ch must be a string of length 1");
        return 0;
    }

    if (barejid == NULL) {
        log_warning("%s", "api_chat_set_incoming_char failed, barejid is NULL");
        return 0;
    }

    ProfChatWin *chatwin = wins_get_chat(barejid);
    if (chatwin == NULL) {
        log_warning("%s", "api_chat_set_incoming_char failed, could not find chat window for %s", barejid);
        return 0;
    }

    chatwin_set_incoming_char(chatwin, ch);

    return 1;
}

int
api_chat_unset_incoming_char(const char *const barejid)
{
    if (barejid == NULL) {
        log_warning("%s", "api_chat_unset_incoming_char failed, barejid is NULL");
        return 0;
    }

    ProfChatWin *chatwin = wins_get_chat(barejid);
    if (chatwin == NULL) {
        log_warning("%s", "api_chat_unset_incoming_char failed, could not find chat window for %s", barejid);
        return 0;
    }

    chatwin_unset_incoming_char(chatwin);

    return 1;
}

int
api_chat_set_outgoing_char(const char *const barejid, const char *const ch)
{
    if (ch == NULL) {
        log_warning("%s", "api_chat_set_outgoing_char failed, ch is NULL");
        return 0;
    }

    if (strlen(ch) != 1) {
        log_warning("%s", "api_chat_set_outgoing_char failed, ch must be a string of length 1");
        return 0;
    }

    if (barejid == NULL) {
        log_warning("%s", "api_chat_set_outgoing_char failed, barejid is NULL");
        return 0;
    }

    ProfChatWin *chatwin = wins_get_chat(barejid);
    if (chatwin == NULL) {
        log_warning("%s", "api_chat_set_outgoing_char failed, could not find chat window for %s", barejid);
        return 0;
    }

    chatwin_set_outgoing_char(chatwin, ch);

    return 1;
}

int
api_chat_unset_outgoing_char(const char *const barejid)
{
    if (barejid == NULL) {
        log_warning("%s", "api_chat_unset_outgoing_char failed, barejid is NULL");
        return 0;
    }

    ProfChatWin *chatwin = wins_get_chat(barejid);
    if (chatwin == NULL) {
        log_warning("%s", "api_chat_unset_outgoing_char failed, could not find chat window for %s", barejid);
        return 0;
    }

    chatwin_unset_outgoing_char(chatwin);

    return 1;
}

int
api_room_set_titlebar_enctext(const char *const roomjid, const char *const enctext)
{
    if (enctext == NULL) {
        log_warning("%s", "api_room_set_titlebar_enctext failed, enctext is NULL");
        return 0;
    }

    if (roomjid == NULL) {
        log_warning("%s", "api_room_set_titlebar_enctext failed, roomjid is NULL");
        return 0;
    }

    ProfMucWin *mucwin = wins_get_muc(roomjid);
    if (mucwin == NULL) {
        log_warning("%s", "api_room_set_titlebar_enctext failed, coudl not find room window for %s", roomjid);
        return 0;
    }

    mucwin_set_enctext(mucwin, enctext);

    return 1;
}

int
api_room_unset_titlebar_enctext(const char *const roomjid)
{
    if (roomjid == NULL) {
        log_warning("%s", "api_room_unset_titlebar_enctext failed, roomjid is NULL");
        return 0;
    }

    ProfMucWin *mucwin = wins_get_muc(roomjid);
    if (mucwin == NULL) {
        log_warning("%s", "api_room_unset_titlebar_enctext failed, coudl not find room window for %s", roomjid);
        return 0;
    }

    mucwin_unset_enctext(mucwin);

    return 1;
}

int
api_room_set_message_char(const char *const roomjid, const char *const ch)
{
    if (ch == NULL) {
        log_warning("%s", "api_room_set_message_char failed, ch is NULL");
        return 0;
    }

    if (strlen(ch) != 1) {
        log_warning("%s", "api_room_set_message_char failed, ch must be a string of length 1");
        return 0;
    }

    if (roomjid == NULL) {
        log_warning("%s", "api_room_set_message_char failed, roomjid is NULL");
        return 0;
    }

    ProfMucWin *mucwin = wins_get_muc(roomjid);
    if (mucwin == NULL) {
        log_warning("%s", "api_room_set_message_char failed, could not find room window for %s", roomjid);
        return 0;
    }

    mucwin_set_message_char(mucwin, ch);

    return 1;
}

int
api_room_unset_message_char(const char *const roomjid)
{
    if (roomjid == NULL) {
        log_warning("%s", "api_room_unset_message_char failed, roomjid is NULL");
        return 0;
    }

    ProfMucWin *mucwin = wins_get_muc(roomjid);
    if (mucwin == NULL) {
        log_warning("%s", "api_room_unset_message_char failed, could not find room window for %s", roomjid);
        return 0;
    }

    mucwin_unset_message_char(mucwin);

    return 1;
}

int
api_chat_show(const char *const barejid, const char *message)
{
    if (message == NULL) {
        log_warning("%s", "api_chat_show failed, message is NULL");
        return 0;
    }

    if (barejid == NULL) {
        log_warning("%s", "api_chat_show failed, barejid is NULL");
        return 0;
    }

    ProfChatWin *chatwin = wins_get_chat(barejid);
    if (chatwin == NULL) {
        log_warning("%s", "api_chat_show failed, could not find chat window for %s", barejid);
        return 0;
    }

    char *parsed = str_replace(message, "\r\n", "\n");
    win_println((ProfWin*)chatwin, THEME_TEXT, '-', "%s", parsed);
    free(parsed);

    return 1;
}

int
api_chat_show_themed(const char *const barejid, const char *const group, const char *const key, const char *const def,
    const char *const ch, const char *const message)
{
    if (message == NULL) {
        log_warning("%s", "api_chat_show_themed failed, message is NULL");
        return 0;
    }

    if (barejid == NULL) {
        log_warning("%s", "api_chat_show_themed failed, barejid is NULL");
        return 0;
    }

    char show_ch = '-';
    if (ch) {
        if (strlen(ch) != 1) {
            log_warning("%s", "api_chat_show_themed failed, ch must be a string of length 1");
            return 0;
        } else {
            show_ch = ch[0];
        }
    }

    ProfChatWin *chatwin = wins_get_chat(barejid);
    if (chatwin == NULL) {
        log_warning("%s", "api_chat_show_themed failed, could not find chat window for %s", barejid);
        return 0;
    }

    char *parsed = str_replace(message, "\r\n", "\n");
    theme_item_t themeitem = plugin_themes_get(group, key, def);

    win_println((ProfWin*)chatwin, themeitem, show_ch, "%s", parsed);
    free(parsed);

    return 1;
}

int
api_room_show(const char *const roomjid, const char *message)
{
    if (message == NULL) {
        log_warning("%s", "api_room_show failed, message is NULL");
        return 0;
    }

    if (roomjid == NULL) {
        log_warning("%s", "api_room_show failed, roomjid is NULL");
        return 0;
    }

    ProfMucWin *mucwin = wins_get_muc(roomjid);
    if (mucwin == NULL) {
        log_warning("%s", "api_room_show failed, could not find room window for %s", roomjid);
        return 0;
    }

    char *parsed = str_replace(message, "\r\n", "\n");
    win_println((ProfWin*)mucwin, THEME_TEXT, '-', "%s", parsed);
    free(parsed);

    return 1;
}

int
api_room_show_themed(const char *const roomjid, const char *const group, const char *const key, const char *const def,
    const char *const ch, const char *const message)
{
    if (message == NULL) {
        log_warning("%s", "api_room_show_themed failed, message is NULL");
        return 0;
    }

    if (roomjid == NULL) {
        log_warning("%s", "api_room_show_themed failed, roomjid is NULL");
        return 0;
    }

    char show_ch = '-';
    if (ch) {
        if (strlen(ch) != 1) {
            log_warning("%s", "api_room_show_themed failed, ch must be a string of length 1");
            return 0;
        } else {
            show_ch = ch[0];
        }
    }

    ProfMucWin *mucwin = wins_get_muc(roomjid);
    if (mucwin == NULL) {
        log_warning("%s", "api_room_show_themed failed, could not find room window for %s", roomjid);
        return 0;
    }

    char *parsed = str_replace(message, "\r\n", "\n");
    theme_item_t themeitem = plugin_themes_get(group, key, def);

    win_println((ProfWin*)mucwin, themeitem, show_ch, "%s", parsed);
    free(parsed);

    return 1;
}
