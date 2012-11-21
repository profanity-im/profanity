/*
 * theme.h
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

#ifndef THEME_H
#define THEME_H

#include "config.h"

#include <glib.h>

#ifdef HAVE_NCURSES_H
#include <ncurses.h>
#endif
#ifdef HAVE_NCURSES_NCURSES_H
#include <ncurses/ncurses.h>
#endif

void theme_load(const char * const theme_name);
gboolean theme_change(const char * const theme_name);
void theme_close(void);

NCURSES_COLOR_T theme_get_bkgnd();
NCURSES_COLOR_T theme_get_titlebar();
NCURSES_COLOR_T theme_get_statusbar();
NCURSES_COLOR_T theme_get_titlebartext();
NCURSES_COLOR_T theme_get_titlebarbrackets();
NCURSES_COLOR_T theme_get_statusbartext();
NCURSES_COLOR_T theme_get_statusbarbrackets();
NCURSES_COLOR_T theme_get_statusbaractive();
NCURSES_COLOR_T theme_get_statusbarnew();
NCURSES_COLOR_T theme_get_maintext();
NCURSES_COLOR_T theme_get_splashtext();
NCURSES_COLOR_T theme_get_online();
NCURSES_COLOR_T theme_get_away();
NCURSES_COLOR_T theme_get_chat();
NCURSES_COLOR_T theme_get_dnd();
NCURSES_COLOR_T theme_get_xa();
NCURSES_COLOR_T theme_get_offline();
NCURSES_COLOR_T theme_get_typing();
NCURSES_COLOR_T theme_get_gone();
NCURSES_COLOR_T theme_get_error();
NCURSES_COLOR_T theme_get_incoming();
NCURSES_COLOR_T theme_get_roominfo();
NCURSES_COLOR_T theme_get_me();
NCURSES_COLOR_T theme_get_them();

#endif
