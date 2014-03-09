/*
 * theme.h
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

#ifndef THEME_H
#define THEME_H

#include "config.h"

#include <glib.h>
#ifdef HAVE_NCURSESW_NCURSES_H
#include <ncursesw/ncurses.h>
#elif HAVE_NCURSES_H
#include <ncurses.h>
#endif

#define COLOUR_TEXT                     COLOR_PAIR(1)
#define COLOUR_SPLASH                   COLOR_PAIR(2)
#define COLOUR_ERROR                    COLOR_PAIR(3)
#define COLOUR_INCOMING                 COLOR_PAIR(4)
#define COLOUR_INPUT_TEXT               COLOR_PAIR(5)
#define COLOUR_TIME                     COLOR_PAIR(6)
#define COLOUR_TITLE_TEXT               COLOR_PAIR(7)
#define COLOUR_TITLE_BRACKET            COLOR_PAIR(8)
#define COLOUR_TITLE_UNENCRYPTED        COLOR_PAIR(9)
#define COLOUR_TITLE_ENCRYPTED          COLOR_PAIR(10)
#define COLOUR_TITLE_UNTRUSTED          COLOR_PAIR(11)
#define COLOUR_TITLE_TRUSTED            COLOR_PAIR(12)
#define COLOUR_STATUS_TEXT              COLOR_PAIR(13)
#define COLOUR_STATUS_BRACKET           COLOR_PAIR(14)
#define COLOUR_STATUS_ACTIVE            COLOR_PAIR(15)
#define COLOUR_STATUS_NEW               COLOR_PAIR(16)
#define COLOUR_ME                       COLOR_PAIR(17)
#define COLOUR_THEM                     COLOR_PAIR(18)
#define COLOUR_ROOMINFO                 COLOR_PAIR(19)
#define COLOUR_ONLINE                   COLOR_PAIR(20)
#define COLOUR_OFFLINE                  COLOR_PAIR(21)
#define COLOUR_AWAY                     COLOR_PAIR(22)
#define COLOUR_CHAT                     COLOR_PAIR(23)
#define COLOUR_DND                      COLOR_PAIR(24)
#define COLOUR_XA                       COLOR_PAIR(25)
#define COLOUR_TYPING                   COLOR_PAIR(26)
#define COLOUR_GONE                     COLOR_PAIR(27)
#define COLOUR_SUBSCRIBED               COLOR_PAIR(28)
#define COLOUR_UNSUBSCRIBED             COLOR_PAIR(29)
#define COLOUR_OTR_STARTED_TRUSTED      COLOR_PAIR(30)
#define COLOUR_OTR_STARTED_UNTRUSTED    COLOR_PAIR(31)
#define COLOUR_OTR_ENDED                COLOR_PAIR(32)
#define COLOUR_OTR_TRUSTED              COLOR_PAIR(33)
#define COLOUR_OTR_UNTRUSTED            COLOR_PAIR(34)

void theme_init(const char * const theme_name);
void theme_init_colours(void);
gboolean theme_load(const char * const theme_name);
GSList* theme_list(void);
void theme_close(void);

#endif
