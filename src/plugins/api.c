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
#include <assert.h>

#include <glib.h>

#include "log.h"
#include "plugins/callbacks.h"
#include "plugins/autocompleters.h"
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

void
api_cons_show(const char * const message)
{
    if (message) {
        char *parsed = str_replace(message, "\r\n", "\n");
        cons_show("%s", parsed);
        free(parsed);
    }
}

void
api_register_command(const char *command_name, int min_args, int max_args,
    const char *usage, const char *short_help, const char *long_help, void *callback,
    void(*callback_func)(PluginCommand *command, gchar **args))
{
    PluginCommand *command = malloc(sizeof(PluginCommand));
    command->command_name = command_name;
    command->min_args = min_args;
    command->max_args = max_args;
    command->usage = usage;
    command->short_help = short_help;
    command->long_help = long_help;
    command->callback = callback;
    command->callback_func = callback_func;

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
api_win_create(const char *tag, void *callback,
    void(*callback_func)(PluginWindowCallback *window_callback, const char *tag, const char * const line))
{
    PluginWindowCallback *window = malloc(sizeof(PluginWindowCallback));
    window->callback = callback;
    window->callback_func = callback_func;
    callbacks_add_window_handler(tag, window);
    wins_new_plugin(tag);

    // set status bar active
    ProfPluginWin *pluginwin = wins_get_plugin(tag);
    int num = wins_get_num((ProfWin*)pluginwin);
    status_bar_active(num);
}

void
api_win_focus(const char *tag)
{
    ProfPluginWin *pluginwin = wins_get_plugin(tag);
    ui_focus_win((ProfWin*)pluginwin);
}

void
api_win_show(const char *tag, const char *line)
{
    ProfPluginWin *pluginwin = wins_get_plugin(tag);
    ProfWin *window = (ProfWin*)pluginwin;
    win_print(window, '!', 0, NULL, 0, 0, "", line);
}

void
api_win_show_green(const char *tag, const char *line)
{
    ProfPluginWin *pluginwin = wins_get_plugin(tag);
    ProfWin *window = (ProfWin*)pluginwin;
    win_print(window, '!', 0, NULL, 0, THEME_GREEN, "", line);
}

void
api_win_show_red(const char *tag, const char *line)
{
    ProfPluginWin *pluginwin = wins_get_plugin(tag);
    ProfWin *window = (ProfWin*)pluginwin;
    win_print(window, '!', 0, NULL, 0, THEME_RED, "", line);
}

void
api_win_show_cyan(const char *tag, const char *line)
{
    ProfPluginWin *pluginwin = wins_get_plugin(tag);
    ProfWin *window = (ProfWin*)pluginwin;
    win_print(window, '!', 0, NULL, 0, THEME_CYAN, "", line);
}

void
api_win_show_yellow(const char *tag, const char *line)
{
    ProfPluginWin *pluginwin = wins_get_plugin(tag);
    ProfWin *window = (ProfWin*)pluginwin;
    win_print(window, '!', 0, NULL, 0, THEME_YELLOW, "", line);
}
