/*
 * theme.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
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
    THEME_INPUT_MISSPELLED,
    THEME_TIME,
    THEME_TITLE_TEXT,
    THEME_TITLE_BRACKET,
    THEME_TITLE_SCROLLED,
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
    THEME_STATUS_CURRENT,
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
    THEME_UNTRUSTED,
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
    THEME_MAGENTA_BOLD,
    THEME_TEXT_HISTORY,
    THEME_CMD_WINS_UNREAD,
    THEME_TRACKBAR,
} theme_item_t;

void theme_init(const char* const theme_name);
void theme_init_colours(void);
gboolean theme_load(const char* const theme_name, gboolean load_theme_prefs);
gboolean theme_exists(const char* const theme_name);
GSList* theme_list(void);
void theme_close(void);
int theme_hash_attrs(const char* str);
int theme_attrs(theme_item_t attrs);
char* theme_get_string(char* str);
theme_item_t theme_main_presence_attrs(const char* const presence);
theme_item_t theme_roster_unread_presence_attrs(const char* const presence);
theme_item_t theme_roster_active_presence_attrs(const char* const presence);
theme_item_t theme_roster_presence_attrs(const char* const presence);
char* theme_get_bkgnd(void);

#endif
