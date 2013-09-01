/*
 * prof_api.c
 *
 * Copyright (C) 2012, 2013 James Booth <boothj5@gmail.com>
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

#include <stdlib.h>

#include "plugins/callbacks.h"

void (*prof_cons_alert)(void) = NULL;

void (*prof_cons_show)(const char * const message) = NULL;

void (*prof_register_command)(const char *command_name, int min_args, int max_args,
    const char *usage, const char *short_help, const char *long_help, void(*callback)(char **args)) = NULL;

void (*prof_register_timed)(void(*callback)(void), int interval_seconds) = NULL;

void (*prof_notify)(const char *message, int timeout_ms, const char *category) = NULL;

void (*prof_send_line)(char *line) = NULL;

char* (*prof_get_current_recipient)(void) = NULL;
