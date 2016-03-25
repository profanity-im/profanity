/*
 * api.c
 *
 * Copyright (C) 2012 - 2016 James Booth <boothj5@gmail.com>
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

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <glib.h>

#include "log.h"
#include "plugins/callbacks.h"
#include "plugins/autocompleters.h"
#include "plugins/themes.h"
#include "plugins/settings.h"
#include "profanity.h"
#include "ui/ui.h"
#include "config/theme.h"
#include "command/command.h"
#include "window_list.h"
#include "common.h"

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
    win_print(console, '-', 0, NULL, 0, themeitem, "", parsed);

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
api_register_command(const char *command_name, int min_args, int max_args,
    const char **synopsis, const char *description, const char *arguments[][2], const char **examples, void *callback,
    void(*callback_func)(PluginCommand *command, gchar **args))
{
    PluginCommand *command = malloc(sizeof(PluginCommand));
    command->command_name = command_name;
    command->min_args = min_args;
    command->max_args = max_args;
    command->callback = callback;
    command->callback_func = callback_func;

    CommandHelp *help = malloc(sizeof(CommandHelp));

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

    callbacks_add_command(command);
}

void
api_register_timed(void *callback, int interval_seconds,
    void (*callback_func)(PluginTimedFunction *timed_function))
{
    PluginTimedFunction *timed_function = malloc(sizeof(PluginTimedFunction));
    timed_function->callback = callback;
    timed_function->callback_func = callback_func;
    timed_function->interval_seconds = interval_seconds;
    timed_function->timer = g_timer_new();

    callbacks_add_timed(timed_function);
}

void
api_register_ac(const char *key, char **items)
{
    autocompleters_add(key, items);
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
    const char *tag,
    void *callback,
    void(*destroy)(void *callback),
    void(*callback_func)(PluginWindowCallback *window_callback, const char *tag, const char * const line))
{
    PluginWindowCallback *window = malloc(sizeof(PluginWindowCallback));
    window->callback = callback;
    window->callback_func = callback_func;
    window->destroy = destroy;
    callbacks_add_window_handler(tag, window);
    wins_new_plugin(tag);

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
    win_print(window, '!', 0, NULL, 0, 0, "", line);

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
    win_print(window, '!', 0, NULL, 0, themeitem, "", line);

    return 1;
}

int
api_send_stanza(const char *const stanza)
{
    return jabber_send_stanza(stanza);
}

gboolean
api_settings_get_boolean(const char *const group, const char *const key, gboolean def)
{
    return plugin_settings_get_boolean(group, key, def);
}

void
api_settings_set_boolean(const char *const group, const char *const key, gboolean value)
{
    plugin_settings_set_boolean(group, key, value);
}
