/*
 * preferences.h
 *
 * Copyright (C) 2012 James Booth <boothj5@gmail.com>
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

#ifdef HAVE_NCURSES_H
#include <ncurses.h>
#endif
#ifdef HAVE_NCURSES_NCURSES_H
#include <ncurses/ncurses.h>
#endif

void prefs_load(void);
void prefs_close(void);

char * prefs_find_login(char *prefix);
void prefs_reset_login_search(void);
char * prefs_autocomplete_boolean_choice(char *prefix);
void prefs_reset_boolean_choice(void);

gboolean prefs_get_beep(void);
void prefs_set_beep(gboolean value);
gboolean prefs_get_notify(void);
void prefs_set_notify(gboolean value);
gboolean prefs_get_typing(void);
void prefs_set_typing(gboolean value);
gboolean prefs_get_flash(void);
void prefs_set_flash(gboolean value);
gboolean prefs_get_chlog(void);
void prefs_set_chlog(gboolean value);
gboolean prefs_get_history(void);
void prefs_set_history(gboolean value);
gboolean prefs_get_showsplash(void);
void prefs_set_showsplash(gboolean value);
gint prefs_get_remind(void);
void prefs_set_remind(gint value);

void prefs_add_login(const char *jid);

NCURSES_COLOR_T prefs_get_bkgnd();
NCURSES_COLOR_T prefs_get_text();
NCURSES_COLOR_T prefs_get_online();
NCURSES_COLOR_T prefs_get_away();
NCURSES_COLOR_T prefs_get_chat();
NCURSES_COLOR_T prefs_get_dnd();
NCURSES_COLOR_T prefs_get_xa();
NCURSES_COLOR_T prefs_get_offline();
NCURSES_COLOR_T prefs_get_err();
NCURSES_COLOR_T prefs_get_inc();
NCURSES_COLOR_T prefs_get_bar();
NCURSES_COLOR_T prefs_get_bar_draw();
NCURSES_COLOR_T prefs_get_bar_text();

#endif
