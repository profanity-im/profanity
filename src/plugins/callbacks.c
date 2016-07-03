/*
 * callbacks.c
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

#include <string.h>
#include <stdlib.h>

#include "command/cmd_defs.h"
#include "command/cmd_ac.h"
#include "plugins/callbacks.h"
#include "plugins/plugins.h"
#include "tools/autocomplete.h"
#include "tools/parser.h"

#include "ui/ui.h"

static GHashTable *p_commands = NULL;
static GSList *p_timed_functions = NULL;
static GHashTable *p_window_callbacks = NULL;

static void
_free_window_callback(PluginWindowCallback *window_callback)
{
    if (window_callback->callback_destroy) {
        window_callback->callback_destroy(window_callback->callback);
    }
    free(window_callback);
}

//typedef struct cmd_help_t {
//    const gchar *tags[20];
//    const gchar *synopsis[50];
//    const gchar *desc;
//    const gchar *args[128][2];
//    const gchar *examples[20];
//} CommandHelp;

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

void
callbacks_init(void)
{
    p_commands = g_hash_table_new_full(g_str_hash, g_str_equal, free, (GDestroyNotify)_free_command_hash);
    p_window_callbacks = g_hash_table_new_full(g_str_hash, g_str_equal, free, (GDestroyNotify)_free_window_callback);
}

// TODO move to plugin destroy functions
void
callbacks_close(void)
{
    g_hash_table_destroy(p_commands);
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
callbacks_add_timed(PluginTimedFunction *timed_function)
{
    p_timed_functions = g_slist_append(p_timed_functions, timed_function);
}

void
callbacks_add_window_handler(const char *tag, PluginWindowCallback *window_callback)
{
    g_hash_table_insert(p_window_callbacks, strdup(tag), window_callback);
}

void *
callbacks_get_window_handler(const char *tag)
{
    if (p_window_callbacks) {
        return g_hash_table_lookup(p_window_callbacks, tag);
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
    GSList *p_timed_function = p_timed_functions;

    while (p_timed_function) {
        PluginTimedFunction *timed_function = p_timed_function->data;
        gdouble elapsed = g_timer_elapsed(timed_function->timer, NULL);

        if (timed_function->interval_seconds > 0 && elapsed >= timed_function->interval_seconds) {
            timed_function->callback_exec(timed_function);
            g_timer_start(timed_function->timer);
        }

        p_timed_function = g_slist_next(p_timed_function);
    }
    return;
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
