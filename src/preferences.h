/*
 * preferences.h
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

#ifndef PREFERENCES_H
#define PREFERENCES_H

#include "config.h"

#include <glib.h>
#ifdef HAVE_NCURSESW_NCURSES_H
#include <ncursesw/ncurses.h>
#elif HAVE_NCURSES_H
#include <ncurses.h>
#endif

#define PREFS_MIN_LOG_SIZE 64
#define PREFS_MAX_LOG_SIZE 1048580

void prefs_load(void);
void prefs_close(void);

char * prefs_find_login(char *prefix);
void prefs_reset_login_search(void);
char * prefs_autocomplete_boolean_choice(char *prefix);
void prefs_reset_boolean_choice(void);

gboolean prefs_get_beep(void);
void prefs_set_beep(gboolean value);
gboolean prefs_get_flash(void);
void prefs_set_flash(gboolean value);
gboolean prefs_get_chlog(void);
void prefs_set_chlog(gboolean value);
gboolean prefs_get_history(void);
void prefs_set_history(gboolean value);
gboolean prefs_get_splash(void);
void prefs_set_splash(gboolean value);
gboolean prefs_get_vercheck(void);
void prefs_set_vercheck(gboolean value);
gboolean prefs_get_titlebarversion(void);
void prefs_set_titlebarversion(gboolean value);
gboolean prefs_get_intype(void);
void prefs_set_intype(gboolean value);
gboolean prefs_get_states(void);
void prefs_set_states(gboolean value);
gboolean prefs_get_outtype(void);
void prefs_set_outtype(gboolean value);
gint prefs_get_gone(void);
void prefs_set_gone(gint value);
gchar * prefs_get_theme(void);
void prefs_set_theme(gchar *value);
gboolean prefs_get_mouse(void);
void prefs_set_mouse(gboolean value);
void prefs_set_statuses(gboolean value);
gboolean prefs_get_statuses(void);

void prefs_set_notify_message(gboolean value);
gboolean prefs_get_notify_message(void);
void prefs_set_notify_typing(gboolean value);
gboolean prefs_get_notify_typing(void);
void prefs_set_notify_remind(gint period);
gint prefs_get_notify_remind(void);
void prefs_set_max_log_size(gint value);
gint prefs_get_max_log_size(void);
void prefs_set_priority(gint value);
gint prefs_get_priority(void);
void prefs_set_reconnect(gint value);
gint prefs_get_reconnect(void);
void prefs_set_autoping(gint value);
gint prefs_get_autoping(void);

gchar* prefs_get_autoaway_mode(void);
void prefs_set_autoaway_mode(gchar *value);
gint prefs_get_autoaway_time(void);
void prefs_set_autoaway_time(gint value);
gchar* prefs_get_autoaway_message(void);
void prefs_set_autoaway_message(gchar *value);
gboolean prefs_get_autoaway_check(void);
void prefs_set_autoaway_check(gboolean value);

void prefs_add_login(const char *jid);

#endif
