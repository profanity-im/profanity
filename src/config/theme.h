/*
 * theme.h
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

#ifndef CONFIG_THEME_H
#define CONFIG_THEME_H

#include "config.h"

#include <glib.h>

#define THEME_DEFAULT 0

typedef enum {
    THEME_TEXT,
    THEME_TEXT_ME,
    THEME_TEXT_THEM,
    THEME_SPLASH,
    THEME_HELP_HEADER,
    THEME_ERROR,
    THEME_INCOMING,
    THEME_MENTION,
    THEME_TRIGGER,
    THEME_INPUT_TEXT,
    THEME_TIME,
    THEME_TITLE_TEXT,
    THEME_TITLE_BRACKET,
    THEME_TITLE_UNENCRYPTED,
    THEME_TITLE_ENCRYPTED,
    THEME_TITLE_UNTRUSTED,
    THEME_TITLE_TRUSTED,
    THEME_TITLE_ONLINE,
    THEME_TITLE_OFFLINE,
    THEME_TITLE_AWAY,
    THEME_TITLE_CHAT,
    THEME_TITLE_DND,
    THEME_TITLE_XA,
    THEME_STATUS_TEXT,
    THEME_STATUS_BRACKET,
    THEME_STATUS_ACTIVE,
    THEME_STATUS_NEW,
    THEME_STATUS_TIME,
    THEME_ME,
    THEME_THEM,
    THEME_ROOMINFO,
    THEME_ROOMMENTION,
    THEME_ROOMMENTION_TERM,
    THEME_ROOMTRIGGER,
    THEME_ROOMTRIGGER_TERM,
    THEME_ONLINE,
    THEME_OFFLINE,
    THEME_AWAY,
    THEME_CHAT,
    THEME_DND,
    THEME_XA,
    THEME_TYPING,
    THEME_GONE,
    THEME_SUBSCRIBED,
    THEME_UNSUBSCRIBED,
    THEME_OTR_STARTED_TRUSTED,
    THEME_OTR_STARTED_UNTRUSTED,
    THEME_OTR_ENDED,
    THEME_OTR_TRUSTED,
    THEME_OTR_UNTRUSTED,
    THEME_OCCUPANTS_HEADER,
    THEME_ROSTER_HEADER,
    THEME_ROSTER_ONLINE,
    THEME_ROSTER_OFFLINE,
    THEME_ROSTER_AWAY,
    THEME_ROSTER_CHAT,
    THEME_ROSTER_DND,
    THEME_ROSTER_XA,
    THEME_ROSTER_ONLINE_ACTIVE,
    THEME_ROSTER_OFFLINE_ACTIVE,
    THEME_ROSTER_AWAY_ACTIVE,
    THEME_ROSTER_CHAT_ACTIVE,
    THEME_ROSTER_DND_ACTIVE,
    THEME_ROSTER_XA_ACTIVE,
    THEME_ROSTER_ONLINE_UNREAD,
    THEME_ROSTER_OFFLINE_UNREAD,
    THEME_ROSTER_AWAY_UNREAD,
    THEME_ROSTER_CHAT_UNREAD,
    THEME_ROSTER_DND_UNREAD,
    THEME_ROSTER_XA_UNREAD,
    THEME_ROSTER_ROOM,
    THEME_ROSTER_ROOM_UNREAD,
    THEME_ROSTER_ROOM_TRIGGER,
    THEME_ROSTER_ROOM_MENTION,
    THEME_RECEIPT_SENT,
    THEME_NONE,
    THEME_WHITE,
    THEME_WHITE_BOLD,
    THEME_GREEN,
    THEME_GREEN_BOLD,
    THEME_RED,
    THEME_RED_BOLD,
    THEME_YELLOW,
    THEME_YELLOW_BOLD,
    THEME_BLUE,
    THEME_BLUE_BOLD,
    THEME_CYAN,
    THEME_CYAN_BOLD,
    THEME_BLACK,
    THEME_BLACK_BOLD,
    THEME_MAGENTA,
    THEME_MAGENTA_BOLD
} theme_item_t;

void theme_init(const char *const theme_name);
void theme_init_colours(void);
gboolean theme_load(const char *const theme_name);
gboolean theme_exists(const char *const theme_name);
GSList* theme_list(void);
void theme_close(void);
int theme_attrs(theme_item_t attrs);
char* theme_get_string(char *str);
void theme_free_string(char *str);
theme_item_t theme_main_presence_attrs(const char *const presence);
theme_item_t theme_roster_unread_presence_attrs(const char *const presence);
theme_item_t theme_roster_active_presence_attrs(const char *const presence);
theme_item_t theme_roster_presence_attrs(const char *const presence);

#endif
