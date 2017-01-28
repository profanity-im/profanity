/*
 * callbacks.c
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

#include <string.h>
#include <stdlib.h>

#include "command/cmd_defs.h"
#include "command/cmd_ac.h"
#include "plugins/callbacks.h"
#include "plugins/plugins.h"
#include "tools/autocomplete.h"
#include "tools/parser.h"
#include "ui/ui.h"
#include "ui/window_list.h"

static GHashTable *p_commands = NULL;
static GHashTable *p_timed_functions = NULL;
static GHashTable *p_window_callbacks = NULL;

static void
_free_window_callback(PluginWindowCallback *window_callback)
{
    if (window_callback->callback_destroy) {
        window_callback->callback_destroy(window_callback->callback);
    }
    free(window_callback);
}

static void
_free_window_callbacks(GHashTable *window_callbacks)
{
    g_hash_table_destroy(window_callbacks);
}

static void
_free_command_help(CommandHelp *help)
{
    int i = 0;
    while (help->tags[i] != NULL) {
        free(help->tags[i++]);
    }

    i = 0;
    while (help->synopsis[i] != NULL) {
        free(help->synopsis[i++]);
    }

    free(help->desc);

    i = 0;
    while (help->args[i] != NULL && help->args[i][0] != NULL) {
        free(help->args[i][0]);
        free(help->args[i][1]);
        i++;
    }

    i = 0;
    while (help->examples[i] != NULL) {
        free(help->examples[i++]);
    }

    free(help);
}

static void
_free_command(PluginCommand *command)
{
    if (command->callback_destroy) {
        command->callback_destroy(command->callback);
    }
    free(command->command_name);

    _free_command_help(command->help);

    free(command);
}

static void
_free_command_hash(GHashTable *command_hash)
{
    g_hash_table_destroy(command_hash);
}

static void
_free_timed_function(PluginTimedFunction *timed_function)
{
    if (timed_function->callback_destroy) {
        timed_function->callback_destroy(timed_function->callback);
    }

    g_timer_destroy(timed_function->timer);

    free(timed_function);
}

static void
_free_timed_function_list(GList *timed_functions)
{
    g_list_free_full(timed_functions, (GDestroyNotify)_free_timed_function);
}

void
callbacks_init(void)
{
    p_commands = g_hash_table_new_full(g_str_hash, g_str_equal, free, (GDestroyNotify)_free_command_hash);
    p_timed_functions = g_hash_table_new_full(g_str_hash, g_str_equal, free, (GDestroyNotify)_free_timed_function_list);
    p_window_callbacks = g_hash_table_new_full(g_str_hash, g_str_equal, free, (GDestroyNotify)_free_window_callbacks);
}

void
callbacks_remove(const char *const plugin_name)
{
    GHashTable *command_hash = g_hash_table_lookup(p_commands, plugin_name);
    if (command_hash) {
        GList *commands = g_hash_table_get_keys(command_hash);
        GList *curr = commands;
        while (curr) {
            char *command = curr->data;
            cmd_ac_remove(command);
            cmd_ac_remove_help(&command[1]);
            curr = g_list_next(curr);
        }
        g_list_free(commands);
    }

    g_hash_table_remove(p_commands, plugin_name);
    g_hash_table_remove(p_timed_functions, plugin_name);

    GHashTable *tag_to_win_cb_hash = g_hash_table_lookup(p_window_callbacks, plugin_name);
    if (tag_to_win_cb_hash) {
        GList *tags = g_hash_table_get_keys(tag_to_win_cb_hash);
        GList *curr = tags;
        while (curr) {
            wins_close_plugin(curr->data);
            curr = g_list_next(curr);
        }
        g_list_free(tags);
    }

    g_hash_table_remove(p_window_callbacks, plugin_name);
}

void
callbacks_close(void)
{
    g_hash_table_destroy(p_commands);
    g_hash_table_destroy(p_timed_functions);
    g_hash_table_destroy(p_window_callbacks);
}

void
callbacks_add_command(const char *const plugin_name, PluginCommand *command)
{
    GHashTable *command_hash = g_hash_table_lookup(p_commands, plugin_name);
    if (command_hash) {
        g_hash_table_insert(command_hash, strdup(command->command_name), command);
    } else {
        command_hash = g_hash_table_new_full(g_str_hash, g_str_equal, free, (GDestroyNotify)_free_command);
        g_hash_table_insert(command_hash, strdup(command->command_name), command);
        g_hash_table_insert(p_commands, strdup(plugin_name), command_hash);
    }
    cmd_ac_add(command->command_name);
    cmd_ac_add_help(&command->command_name[1]);
}

void
callbacks_add_timed(const char *const plugin_name, PluginTimedFunction *timed_function)
{
    GList *timed_function_list = g_hash_table_lookup(p_timed_functions, plugin_name);
    if (timed_function_list) {
        timed_function_list = g_list_append(timed_function_list, timed_function);
    } else {
        timed_function_list = g_list_append(timed_function_list, timed_function);
        g_hash_table_insert(p_timed_functions, strdup(plugin_name), timed_function_list);
    }
}

gboolean
callbacks_win_exists(const char *const plugin_name, const char *tag)
{
    GHashTable *window_callbacks = g_hash_table_lookup(p_window_callbacks, plugin_name);
    if (window_callbacks) {
        PluginWindowCallback *cb = g_hash_table_lookup(window_callbacks, tag);
        if (cb) {
            return TRUE;
        }
    }

    return FALSE;
}

void
callbacks_remove_win(const char *const plugin_name, const char *const tag)
{
    GHashTable *window_callbacks = g_hash_table_lookup(p_window_callbacks, plugin_name);
    if (window_callbacks) {
        g_hash_table_remove(window_callbacks, tag);
    }
}

void
callbacks_add_window_handler(const char *const plugin_name, const char *tag, PluginWindowCallback *window_callback)
{
    GHashTable *window_callbacks = g_hash_table_lookup(p_window_callbacks, plugin_name);
    if (window_callbacks) {
        g_hash_table_insert(window_callbacks, strdup(tag), window_callback);
    } else {
        window_callbacks = g_hash_table_new_full(g_str_hash, g_str_equal, free, (GDestroyNotify)_free_window_callback);
        g_hash_table_insert(window_callbacks, strdup(tag), window_callback);
        g_hash_table_insert(p_window_callbacks, strdup(plugin_name), window_callbacks);
    }
}

void *
callbacks_get_window_handler(const char *tag)
{
    if (p_window_callbacks) {
        GList *window_callback_hashes = g_hash_table_get_values(p_window_callbacks);
        GList *curr_hash = window_callback_hashes;
        while (curr_hash) {
            GHashTable *window_callback_hash = curr_hash->data;
            PluginWindowCallback *callback = g_hash_table_lookup(window_callback_hash, tag);
            if (callback) {
                g_list_free(window_callback_hashes);
                return callback;
            }

            curr_hash = g_list_next(curr_hash);
        }

        g_list_free(window_callback_hashes);
        return NULL;
    } else {
        return NULL;
    }
}

gboolean
plugins_run_command(const char * const input)
{
    gchar **split = g_strsplit(input, " ", -1);

    GList *command_hashes = g_hash_table_get_values(p_commands);
    GList *curr_hash = command_hashes;
    while (curr_hash) {
        GHashTable *command_hash = curr_hash->data;

        PluginCommand *command = g_hash_table_lookup(command_hash, split[0]);
        if (command) {
            gboolean result;
            gchar **args = parse_args_with_freetext(input, command->min_args, command->max_args, &result);
            if (result == FALSE) {
                ui_invalid_command_usage(command->command_name, NULL);
                g_strfreev(split);
                g_list_free(command_hashes);
                return TRUE;
            } else {
                command->callback_exec(command, args);
                g_strfreev(split);
                g_strfreev(args);
                g_list_free(command_hashes);
                return TRUE;
            }
        }

        curr_hash = g_list_next(curr_hash);
    }

    g_list_free(command_hashes);
    g_strfreev(split);
    return FALSE;
}

CommandHelp*
plugins_get_help(const char *const cmd)
{
    GList *command_hashes = g_hash_table_get_values(p_commands);
    GList *curr_hash = command_hashes;
    while (curr_hash) {
        GHashTable *command_hash = curr_hash->data;

        PluginCommand *command = g_hash_table_lookup(command_hash, cmd);
        if (command) {
            g_list_free(command_hashes);
            return command->help;
        }

        curr_hash = g_list_next(curr_hash);
    }

    g_list_free(command_hashes);

    return NULL;
}

void
plugins_run_timed(void)
{
    GList *timed_functions_lists = g_hash_table_get_values(p_timed_functions);

    GList *curr_list = timed_functions_lists;
    while (curr_list) {
        GList *timed_function_list = curr_list->data;
        GList *curr = timed_function_list;
        while (curr) {
            PluginTimedFunction *timed_function = curr->data;

            gdouble elapsed = g_timer_elapsed(timed_function->timer, NULL);

            if (timed_function->interval_seconds > 0 && elapsed >= timed_function->interval_seconds) {
                timed_function->callback_exec(timed_function);
                g_timer_start(timed_function->timer);
            }

            curr = g_list_next(curr);
        }
        curr_list = g_list_next(curr_list);
    }

    g_list_free(timed_functions_lists);
}

GList*
plugins_get_command_names(void)
{
    GList *result = NULL;

    GList *command_hashes = g_hash_table_get_values(p_commands);
    GList *curr_hash = command_hashes;
    while (curr_hash) {
        GHashTable *command_hash = curr_hash->data;
        GList *commands = g_hash_table_get_keys(command_hash);
        GList *curr = commands;
        while (curr) {
            char *command = curr->data;
            result = g_list_append(result, command);
            curr = g_list_next(curr);
        }
        g_list_free(commands);
        curr_hash = g_list_next(curr_hash);
    }

    g_list_free(command_hashes);

    return result;
}
