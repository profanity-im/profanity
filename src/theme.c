/*
 * theme.c
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

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <glib.h>
#ifdef HAVE_NCURSES_H
#include <ncurses.h>
#endif
#ifdef HAVE_NCURSES_NCURSES_H
#include <ncurses/ncurses.h>
#endif

#include "log.h"

static GString *theme_loc;
static GKeyFile *theme;

struct colour_string_t {
    char *str;
    NCURSES_COLOR_T colour;
};

static int num_colours = 9;
static struct colour_string_t colours[] = {
    { "default", -1 },
    { "white", COLOR_WHITE },
    { "green", COLOR_GREEN },
    { "red", COLOR_RED },
    { "yellow", COLOR_YELLOW },
    { "blue", COLOR_BLUE },
    { "cyan", COLOR_CYAN },
    { "black", COLOR_BLACK },
    { "magenta", COLOR_MAGENTA },
};

// colour preferences
static struct colours_t {
        NCURSES_COLOR_T bkgnd;
        NCURSES_COLOR_T titlebar;
        NCURSES_COLOR_T statusbar;
        NCURSES_COLOR_T titlebartext;
        NCURSES_COLOR_T titlebarbrackets;
        NCURSES_COLOR_T statusbartext;
        NCURSES_COLOR_T statusbarbrackets;
        NCURSES_COLOR_T statusbaractive;
        NCURSES_COLOR_T statusbarnew;
        NCURSES_COLOR_T maintext;
        NCURSES_COLOR_T splashtext;
        NCURSES_COLOR_T online;
        NCURSES_COLOR_T away;
        NCURSES_COLOR_T xa;
        NCURSES_COLOR_T dnd;
        NCURSES_COLOR_T chat;
        NCURSES_COLOR_T offline;
        NCURSES_COLOR_T typing;
        NCURSES_COLOR_T gone;
        NCURSES_COLOR_T error;
        NCURSES_COLOR_T incoming;
        NCURSES_COLOR_T roominfo;
        NCURSES_COLOR_T me;
        NCURSES_COLOR_T them;
} colour_prefs;

static NCURSES_COLOR_T _lookup_colour(const char * const colour);
static void _set_colour(gchar *val, NCURSES_COLOR_T *pref,
    NCURSES_COLOR_T def);
static void _load_colours(void);

void
theme_load(const char * const theme_name)
{
    log_info("Loading theme");
    theme = g_key_file_new();

    if (theme_name != NULL) {
        theme_loc = g_string_new(getenv("HOME"));
        g_string_append(theme_loc, "/.profanity/themes/");
        g_string_append(theme_loc, theme_name);
        g_key_file_load_from_file(theme, theme_loc->str, G_KEY_FILE_KEEP_COMMENTS,
            NULL);
    }

    _load_colours();
}

void
theme_close(void)
{
    g_key_file_free(theme);
}

static NCURSES_COLOR_T
_lookup_colour(const char * const colour)
{
    int i;
    for (i = 0; i < num_colours; i++) {
        if (strcmp(colours[i].str, colour) == 0) {
            return colours[i].colour;
        }
    }

    return -99;
}

static void
_set_colour(gchar *val, NCURSES_COLOR_T *pref,
    NCURSES_COLOR_T def)
{
    if(!val) {
        *pref = def;
    } else {
        NCURSES_COLOR_T col = _lookup_colour(val);
        if (col == -99) {
            *pref = def;
        } else {
            *pref = col;
        }
    }
}

static void
_load_colours(void)
{
    gchar *bkgnd_val = g_key_file_get_string(theme, "colours", "bkgnd", NULL);
    _set_colour(bkgnd_val, &colour_prefs.bkgnd, -1);

    gchar *titlebar_val = g_key_file_get_string(theme, "colours", "titlebar", NULL);
    _set_colour(titlebar_val, &colour_prefs.titlebar, COLOR_BLUE);

    gchar *statusbar_val = g_key_file_get_string(theme, "colours", "statusbar", NULL);
    _set_colour(statusbar_val, &colour_prefs.statusbar, COLOR_BLUE);

    gchar *titlebartext_val = g_key_file_get_string(theme, "colours", "titlebartext", NULL);
    _set_colour(titlebartext_val, &colour_prefs.titlebartext, COLOR_WHITE);

    gchar *titlebarbrackets_val = g_key_file_get_string(theme, "colours", "titlebarbrackets", NULL);
    _set_colour(titlebarbrackets_val, &colour_prefs.titlebarbrackets, COLOR_CYAN);

    gchar *statusbartext_val = g_key_file_get_string(theme, "colours", "statusbartext", NULL);
    _set_colour(statusbartext_val, &colour_prefs.statusbartext, COLOR_WHITE);

    gchar *statusbarbrackets_val = g_key_file_get_string(theme, "colours", "statusbarbrackets", NULL);
    _set_colour(statusbarbrackets_val, &colour_prefs.statusbarbrackets, COLOR_CYAN);

    gchar *statusbaractive_val = g_key_file_get_string(theme, "colours", "statusbaractive", NULL);
    _set_colour(statusbaractive_val, &colour_prefs.statusbaractive, COLOR_CYAN);

    gchar *statusbarnew_val = g_key_file_get_string(theme, "colours", "statusbarnew", NULL);
    _set_colour(statusbarnew_val, &colour_prefs.statusbarnew, COLOR_WHITE);

    gchar *maintext_val = g_key_file_get_string(theme, "colours", "maintext", NULL);
    _set_colour(maintext_val, &colour_prefs.maintext, COLOR_WHITE);

    gchar *splashtext_val = g_key_file_get_string(theme, "colours", "splashtext", NULL);
    _set_colour(splashtext_val, &colour_prefs.splashtext, COLOR_CYAN);

    gchar *online_val = g_key_file_get_string(theme, "colours", "online", NULL);
    _set_colour(online_val, &colour_prefs.online, COLOR_GREEN);

    gchar *away_val = g_key_file_get_string(theme, "colours", "away", NULL);
    _set_colour(away_val, &colour_prefs.away, COLOR_CYAN);

    gchar *chat_val = g_key_file_get_string(theme, "colours", "chat", NULL);
    _set_colour(chat_val, &colour_prefs.chat, COLOR_GREEN);

    gchar *dnd_val = g_key_file_get_string(theme, "colours", "dnd", NULL);
    _set_colour(dnd_val, &colour_prefs.dnd, COLOR_RED);

    gchar *xa_val = g_key_file_get_string(theme, "colours", "xa", NULL);
    _set_colour(xa_val, &colour_prefs.xa, COLOR_CYAN);

    gchar *offline_val = g_key_file_get_string(theme, "colours", "offline", NULL);
    _set_colour(offline_val, &colour_prefs.offline, COLOR_RED);

    gchar *typing_val = g_key_file_get_string(theme, "colours", "typing", NULL);
    _set_colour(typing_val, &colour_prefs.typing, COLOR_YELLOW);

    gchar *gone_val = g_key_file_get_string(theme, "colours", "gone", NULL);
    _set_colour(gone_val, &colour_prefs.gone, COLOR_RED);

    gchar *error_val = g_key_file_get_string(theme, "colours", "error", NULL);
    _set_colour(error_val, &colour_prefs.error, COLOR_RED);

    gchar *incoming_val = g_key_file_get_string(theme, "colours", "incoming", NULL);
    _set_colour(incoming_val, &colour_prefs.incoming, COLOR_YELLOW);

    gchar *roominfo_val = g_key_file_get_string(theme, "colours", "roominfo", NULL);
    _set_colour(roominfo_val, &colour_prefs.roominfo, COLOR_YELLOW);

    gchar *me_val = g_key_file_get_string(theme, "colours", "me", NULL);
    _set_colour(me_val, &colour_prefs.me, COLOR_YELLOW);

    gchar *them_val = g_key_file_get_string(theme, "colours", "them", NULL);
    _set_colour(them_val, &colour_prefs.them, COLOR_GREEN);
}
/*
static void
_save_prefs(void)
{
    gsize g_data_size;
    char *g_prefs_data = g_key_file_to_data(prefs, &g_data_size, NULL);
    g_file_set_contents(prefs_loc->str, g_prefs_data, g_data_size, NULL);
}
*/

NCURSES_COLOR_T
theme_get_bkgnd()
{
    return colour_prefs.bkgnd;
}

NCURSES_COLOR_T
theme_get_titlebar()
{
    return colour_prefs.titlebar;
}

NCURSES_COLOR_T
theme_get_statusbar()
{
    return colour_prefs.statusbar;
}

NCURSES_COLOR_T
theme_get_titlebartext()
{
    return colour_prefs.titlebartext;
}

NCURSES_COLOR_T
theme_get_titlebarbrackets()
{
    return colour_prefs.titlebarbrackets;
}

NCURSES_COLOR_T
theme_get_statusbartext()
{
    return colour_prefs.statusbartext;
}

NCURSES_COLOR_T
theme_get_statusbarbrackets()
{
    return colour_prefs.statusbarbrackets;
}

NCURSES_COLOR_T
theme_get_statusbaractive()
{
    return colour_prefs.statusbaractive;
}

NCURSES_COLOR_T
theme_get_statusbarnew()
{
    return colour_prefs.statusbarnew;
}

NCURSES_COLOR_T
theme_get_maintext()
{
    return colour_prefs.maintext;
}

NCURSES_COLOR_T
theme_get_splashtext()
{
    return colour_prefs.splashtext;
}

NCURSES_COLOR_T
theme_get_online()
{
    return colour_prefs.online;
}

NCURSES_COLOR_T
theme_get_away()
{
    return colour_prefs.away;
}

NCURSES_COLOR_T
theme_get_chat()
{
    return colour_prefs.chat;
}

NCURSES_COLOR_T
theme_get_dnd()
{
    return colour_prefs.dnd;
}

NCURSES_COLOR_T
theme_get_xa()
{
    return colour_prefs.xa;
}

NCURSES_COLOR_T
theme_get_offline()
{
    return colour_prefs.offline;
}

NCURSES_COLOR_T
theme_get_typing()
{
    return colour_prefs.typing;
}

NCURSES_COLOR_T
theme_get_gone()
{
    return colour_prefs.gone;
}

NCURSES_COLOR_T
theme_get_error()
{
    return colour_prefs.error;
}

NCURSES_COLOR_T
theme_get_incoming()
{
    return colour_prefs.incoming;
}

NCURSES_COLOR_T
theme_get_roominfo()
{
    return colour_prefs.roominfo;
}

NCURSES_COLOR_T
theme_get_me()
{
    return colour_prefs.me;
}

NCURSES_COLOR_T
theme_get_them()
{
    return colour_prefs.them;
}
