/*
 * prof_api.c
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

#include "plugins/profapi.h"
#include "plugins/callbacks.h"

void (*prof_cons_alert)(void) = NULL;
int (*prof_cons_show)(const char * const message) = NULL;
int (*prof_cons_show_themed)(const char *const group, const char *const item, const char *const def, const char *const message) = NULL;
int (*prof_cons_bad_cmd_usage)(const char *const cmd) = NULL;

void (*_prof_register_command)(const char *filename, const char *command_name, int min_args, int max_args,
    char **synopsis, const char *description, char *arguments[][2], char **examples,
    CMD_CB callback) = NULL;

void (*_prof_register_timed)(const char *filename, TIMED_CB callback, int interval_seconds) = NULL;

void (*_prof_completer_add)(const char *filename, const char *key, char **items) = NULL;
void (*_prof_completer_remove)(const char *filename, const char *key, char **items) = NULL;
void (*_prof_completer_clear)(const char *filename, const char *key) = NULL;

void (*prof_notify)(const char *message, int timeout_ms, const char *category) = NULL;

void (*prof_send_line)(char *line) = NULL;

char* (*prof_get_current_recipient)(void) = NULL;
char* (*prof_get_current_muc)(void) = NULL;
int (*prof_current_win_is_console)(void) = NULL;
char* (*prof_get_current_nick)(void) = NULL;
char** (*prof_get_current_occupants)(void) = NULL;

void (*prof_log_debug)(const char *message) = NULL;
void (*prof_log_info)(const char *message) = NULL;
void (*prof_log_warning)(const char *message) = NULL;
void (*prof_log_error)(const char *message) = NULL;

void (*_prof_win_create)(const char *filename, PROF_WIN_TAG win, WINDOW_CB input_handler) = NULL;
int (*prof_win_exists)(PROF_WIN_TAG win) = NULL;
int (*prof_win_focus)(PROF_WIN_TAG win) = NULL;
int (*prof_win_show)(PROF_WIN_TAG win, char *line) = NULL;
int (*prof_win_show_themed)(PROF_WIN_TAG tag, char *group, char *key, char *def, char *line) = NULL;

int (*prof_send_stanza)(char *stanza) = NULL;

int (*prof_settings_get_boolean)(char *group, char *key, int def) = NULL;
void (*prof_settings_set_boolean)(char *group, char *key, int value) = NULL;
char* (*prof_settings_get_string)(char *group, char *key, char *def) = NULL;
void (*prof_settings_set_string)(char *group, char *key, char *value) = NULL;
int (*prof_settings_get_int)(char *group, char *key, int def) = NULL;
void (*prof_settings_set_int)(char *group, char *key, int value) = NULL;

void (*prof_incoming_message)(char *barejid, char *resource, char *message) = NULL;

void (*prof_disco_add_feature)(char *feature) = NULL;
