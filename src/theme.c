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

#include "files.h"
#include "log.h"
#include "theme.h"

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
        NCURSES_COLOR_T inputtext;
        NCURSES_COLOR_T timetext;
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
        gchar *themes_dir = files_get_themes_dir();
        theme_loc = g_string_new(themes_dir);
        g_free(themes_dir);
        g_string_append(theme_loc, "/");
        g_string_append(theme_loc, theme_name);
        g_key_file_load_from_file(theme, theme_loc->str, G_KEY_FILE_KEEP_COMMENTS,
            NULL);
    }

    _load_colours();
}

gboolean
theme_change(const char * const theme_name)
{
    // use default theme
    if (strcmp(theme_name, "default") == 0) {
        g_key_file_free(theme);
        theme = g_key_file_new();
        _load_colours();
        return TRUE;
    } else {
        gchar *themes_dir = files_get_themes_dir();
        GString *new_theme_file = g_string_new(themes_dir);
        g_free(themes_dir);
        g_string_append(new_theme_file, "/");
        g_string_append(new_theme_file, theme_name);

        // no theme file found
        if (!g_file_test(new_theme_file->str, G_FILE_TEST_EXISTS)) {
            log_info("Theme does not exist \"%s\"", theme_name);
            g_string_free(new_theme_file, TRUE);
            return FALSE;

        // load from theme file
        } else {
            g_string_free(theme_loc, TRUE);
            theme_loc = new_theme_file;
            log_info("Changing theme to \"%s\"", theme_name);
            g_key_file_free(theme);
            theme = g_key_file_new();
            g_key_file_load_from_file(theme, theme_loc->str, G_KEY_FILE_KEEP_COMMENTS,
                NULL);
            _load_colours();
            return TRUE;
        }
    }
}

void
theme_close(void)
{
    g_key_file_free(theme);
    g_string_free(theme_loc, TRUE);
}

void
theme_init_colours(void)
{
    // main text
    init_pair(1, colour_prefs.maintext, colour_prefs.bkgnd);
    init_pair(2, colour_prefs.splashtext, colour_prefs.bkgnd);
    init_pair(3, colour_prefs.error, colour_prefs.bkgnd);
    init_pair(4, colour_prefs.incoming, colour_prefs.bkgnd);
    init_pair(5, colour_prefs.inputtext, colour_prefs.bkgnd);
    init_pair(6, colour_prefs.timetext, colour_prefs.bkgnd);

    // title bar
    init_pair(10, colour_prefs.titlebartext, colour_prefs.titlebar);
    init_pair(11, colour_prefs.titlebarbrackets, colour_prefs.titlebar);

    // status bar
    init_pair(20, colour_prefs.statusbartext, colour_prefs.statusbar);
    init_pair(21, colour_prefs.statusbarbrackets, colour_prefs.statusbar);
    init_pair(22, colour_prefs.statusbaractive, colour_prefs.statusbar);
    init_pair(23, colour_prefs.statusbarnew, colour_prefs.statusbar);

    // chat
    init_pair(30, colour_prefs.me, colour_prefs.bkgnd);
    init_pair(31, colour_prefs.them, colour_prefs.bkgnd);

    // room chat
    init_pair(40, colour_prefs.roominfo, colour_prefs.bkgnd);

    // statuses
    init_pair(50, colour_prefs.online, colour_prefs.bkgnd);
    init_pair(51, colour_prefs.offline, colour_prefs.bkgnd);
    init_pair(52, colour_prefs.away, colour_prefs.bkgnd);
    init_pair(53, colour_prefs.chat, colour_prefs.bkgnd);
    init_pair(54, colour_prefs.dnd, colour_prefs.bkgnd);
    init_pair(55, colour_prefs.xa, colour_prefs.bkgnd);

    // states
    init_pair(60, colour_prefs.typing, colour_prefs.bkgnd);
    init_pair(61, colour_prefs.gone, colour_prefs.bkgnd);
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
    g_free(bkgnd_val);

    gchar *titlebar_val = g_key_file_get_string(theme, "colours", "titlebar", NULL);
    _set_colour(titlebar_val, &colour_prefs.titlebar, COLOR_BLUE);
    g_free(titlebar_val);

    gchar *statusbar_val = g_key_file_get_string(theme, "colours", "statusbar", NULL);
    _set_colour(statusbar_val, &colour_prefs.statusbar, COLOR_BLUE);
    g_free(statusbar_val);

    gchar *titlebartext_val = g_key_file_get_string(theme, "colours", "titlebartext", NULL);
    _set_colour(titlebartext_val, &colour_prefs.titlebartext, COLOR_WHITE);
    g_free(titlebartext_val);

    gchar *titlebarbrackets_val = g_key_file_get_string(theme, "colours", "titlebarbrackets", NULL);
    _set_colour(titlebarbrackets_val, &colour_prefs.titlebarbrackets, COLOR_CYAN);
    g_free(titlebarbrackets_val);

    gchar *statusbartext_val = g_key_file_get_string(theme, "colours", "statusbartext", NULL);
    _set_colour(statusbartext_val, &colour_prefs.statusbartext, COLOR_WHITE);
    g_free(statusbartext_val);

    gchar *statusbarbrackets_val = g_key_file_get_string(theme, "colours", "statusbarbrackets", NULL);
    _set_colour(statusbarbrackets_val, &colour_prefs.statusbarbrackets, COLOR_CYAN);
    g_free(statusbarbrackets_val);

    gchar *statusbaractive_val = g_key_file_get_string(theme, "colours", "statusbaractive", NULL);
    _set_colour(statusbaractive_val, &colour_prefs.statusbaractive, COLOR_CYAN);
    g_free(statusbaractive_val);

    gchar *statusbarnew_val = g_key_file_get_string(theme, "colours", "statusbarnew", NULL);
    _set_colour(statusbarnew_val, &colour_prefs.statusbarnew, COLOR_WHITE);
    g_free(statusbarnew_val);

    gchar *maintext_val = g_key_file_get_string(theme, "colours", "maintext", NULL);
    _set_colour(maintext_val, &colour_prefs.maintext, COLOR_WHITE);
    g_free(maintext_val);

    gchar *splashtext_val = g_key_file_get_string(theme, "colours", "splashtext", NULL);
    _set_colour(splashtext_val, &colour_prefs.splashtext, COLOR_CYAN);
    g_free(splashtext_val);

    gchar *inputtext_val = g_key_file_get_string(theme, "colours", "inputtext", NULL);
    _set_colour(inputtext_val, &colour_prefs.inputtext, COLOR_WHITE);
    g_free(inputtext_val);

    gchar *timetext_val = g_key_file_get_string(theme, "colours", "timetext", NULL);
    _set_colour(timetext_val, &colour_prefs.timetext, COLOR_WHITE);
    g_free(timetext_val);

    gchar *online_val = g_key_file_get_string(theme, "colours", "online", NULL);
    _set_colour(online_val, &colour_prefs.online, COLOR_GREEN);
    g_free(online_val);

    gchar *away_val = g_key_file_get_string(theme, "colours", "away", NULL);
    _set_colour(away_val, &colour_prefs.away, COLOR_CYAN);
    g_free(away_val);

    gchar *chat_val = g_key_file_get_string(theme, "colours", "chat", NULL);
    _set_colour(chat_val, &colour_prefs.chat, COLOR_GREEN);
    g_free(chat_val);

    gchar *dnd_val = g_key_file_get_string(theme, "colours", "dnd", NULL);
    _set_colour(dnd_val, &colour_prefs.dnd, COLOR_RED);
    g_free(dnd_val);

    gchar *xa_val = g_key_file_get_string(theme, "colours", "xa", NULL);
    _set_colour(xa_val, &colour_prefs.xa, COLOR_CYAN);
    g_free(xa_val);

    gchar *offline_val = g_key_file_get_string(theme, "colours", "offline", NULL);
    _set_colour(offline_val, &colour_prefs.offline, COLOR_RED);
    g_free(offline_val);

    gchar *typing_val = g_key_file_get_string(theme, "colours", "typing", NULL);
    _set_colour(typing_val, &colour_prefs.typing, COLOR_YELLOW);
    g_free(typing_val);

    gchar *gone_val = g_key_file_get_string(theme, "colours", "gone", NULL);
    _set_colour(gone_val, &colour_prefs.gone, COLOR_RED);
    g_free(gone_val);

    gchar *error_val = g_key_file_get_string(theme, "colours", "error", NULL);
    _set_colour(error_val, &colour_prefs.error, COLOR_RED);
    g_free(error_val);

    gchar *incoming_val = g_key_file_get_string(theme, "colours", "incoming", NULL);
    _set_colour(incoming_val, &colour_prefs.incoming, COLOR_YELLOW);
    g_free(incoming_val);

    gchar *roominfo_val = g_key_file_get_string(theme, "colours", "roominfo", NULL);
    _set_colour(roominfo_val, &colour_prefs.roominfo, COLOR_YELLOW);
    g_free(roominfo_val);

    gchar *me_val = g_key_file_get_string(theme, "colours", "me", NULL);
    _set_colour(me_val, &colour_prefs.me, COLOR_YELLOW);
    g_free(me_val);

    gchar *them_val = g_key_file_get_string(theme, "colours", "them", NULL);
    _set_colour(them_val, &colour_prefs.them, COLOR_GREEN);
    g_free(them_val);
}
