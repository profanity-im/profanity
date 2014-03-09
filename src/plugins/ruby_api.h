/*
 * ruby_api.h
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

#ifndef RUBY_API_H
#define RUBY_API_H

void ruby_env_init(void);
void ruby_api_init(void);
void ruby_shutdown(void);

void ruby_command_callback(PluginCommand *command, gchar **args);
void ruby_timed_callback(PluginTimedFunction *timed_function);
void ruby_window_callback(PluginWindowCallback *window_callback, char *tag, char *line);

#endif
