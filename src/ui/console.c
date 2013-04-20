/*
 * console.c
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

#include <string.h>
#include <stdlib.h>

#ifdef HAVE_NCURSESW_NCURSES_H
#include <ncursesw/ncurses.h>
#elif HAVE_NCURSES_H
#include <ncurses.h>
#endif

#include "common.h"
#include "config/preferences.h"
#include "config/theme.h"
#include "ui/window.h"

#define CONS_WIN_TITLE "_cons"

static ProfWin* console;
static int dirty;

static void _cons_splash_logo(void);

ProfWin *
cons_create(void)
{
    int cols = getmaxx(stdscr);
    console = window_create(CONS_WIN_TITLE, cols, WIN_CONSOLE);
    dirty = FALSE;
    return console;
}

void
cons_refresh(void)
{
    int rows, cols;
    if (dirty == TRUE) {
        getmaxyx(stdscr, rows, cols);
        prefresh(console->win, console->y_pos, 0, 1, 0, rows-3, cols-1);
        dirty = FALSE;
    }
}

void
cons_show(const char * const msg, ...)
{
    va_list arg;
    va_start(arg, msg);
    GString *fmt_msg = g_string_new(NULL);
    g_string_vprintf(fmt_msg, msg, arg);
    window_show_time(console, '-');
    wprintw(console->win, "%s\n", fmt_msg->str);
    g_string_free(fmt_msg, TRUE);
    va_end(arg);

    dirty = TRUE;
}

void
cons_about(void)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    if (prefs_get_boolean(PREF_SPLASH)) {
        _cons_splash_logo();
    } else {
        window_show_time(console, '-');

        if (strcmp(PACKAGE_STATUS, "development") == 0) {
            wprintw(console->win, "Welcome to Profanity, version %sdev\n", PACKAGE_VERSION);
        } else {
            wprintw(console->win, "Welcome to Profanity, version %s\n", PACKAGE_VERSION);
        }
    }

    window_show_time(console, '-');
    wprintw(console->win, "Copyright (C) 2012, 2013 James Booth <%s>.\n", PACKAGE_BUGREPORT);
    window_show_time(console, '-');
    wprintw(console->win, "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n");
    window_show_time(console, '-');
    wprintw(console->win, "\n");
    window_show_time(console, '-');
    wprintw(console->win, "This is free software; you are free to change and redistribute it.\n");
    window_show_time(console, '-');
    wprintw(console->win, "There is NO WARRANTY, to the extent permitted by law.\n");
    window_show_time(console, '-');
    wprintw(console->win, "\n");
    window_show_time(console, '-');
    wprintw(console->win, "Type '/help' to show complete help.\n");
    window_show_time(console, '-');
    wprintw(console->win, "\n");

    if (prefs_get_boolean(PREF_VERCHECK)) {
        cons_check_version(FALSE);
    }

    prefresh(console->win, 0, 0, 1, 0, rows-3, cols-1);

    dirty = TRUE;
}

void
cons_check_version(gboolean not_available_msg)
{
    char *latest_release = release_get_latest();

    if (latest_release != NULL) {
        gboolean relase_valid = g_regex_match_simple("^\\d+\\.\\d+\\.\\d+$", latest_release, 0, 0);

        if (relase_valid) {
            if (release_is_new(latest_release)) {
                window_show_time(console, '-');
                wprintw(console->win, "A new version of Profanity is available: %s", latest_release);
                window_show_time(console, '-');
                wprintw(console->win, "Check <http://www.profanity.im> for details.\n");
                free(latest_release);
                window_show_time(console, '-');
                wprintw(console->win, "\n");
            } else {
                if (not_available_msg) {
                    cons_show("No new version available.");
                    cons_show("");
                }
            }

            dirty = TRUE;
        }
    }
}

static void
_cons_splash_logo(void)
{
    window_show_time(console, '-');
    wprintw(console->win, "Welcome to\n");

    window_show_time(console, '-');
    wattron(console->win, COLOUR_SPLASH);
    wprintw(console->win, "                   ___            _           \n");
    wattroff(console->win, COLOUR_SPLASH);

    window_show_time(console, '-');
    wattron(console->win, COLOUR_SPLASH);
    wprintw(console->win, "                  / __)          (_)_         \n");
    wattroff(console->win, COLOUR_SPLASH);

    window_show_time(console, '-');
    wattron(console->win, COLOUR_SPLASH);
    wprintw(console->win, " ____   ____ ___ | |__ ____ ____  _| |_ _   _ \n");
    wattroff(console->win, COLOUR_SPLASH);

    window_show_time(console, '-');
    wattron(console->win, COLOUR_SPLASH);
    wprintw(console->win, "|  _ \\ / ___) _ \\|  __) _  |  _ \\| |  _) | | |\n");
    wattroff(console->win, COLOUR_SPLASH);

    window_show_time(console, '-');
    wattron(console->win, COLOUR_SPLASH);
    wprintw(console->win, "| | | | |  | |_| | | ( ( | | | | | | |_| |_| |\n");
    wattroff(console->win, COLOUR_SPLASH);

    window_show_time(console, '-');
    wattron(console->win, COLOUR_SPLASH);
    wprintw(console->win, "| ||_/|_|   \\___/|_|  \\_||_|_| |_|_|\\___)__  |\n");
    wattroff(console->win, COLOUR_SPLASH);

    window_show_time(console, '-');
    wattron(console->win, COLOUR_SPLASH);
    wprintw(console->win, "|_|                                    (____/ \n");
    wattroff(console->win, COLOUR_SPLASH);

    window_show_time(console, '-');
    wprintw(console->win, "\n");
    window_show_time(console, '-');
    if (strcmp(PACKAGE_STATUS, "development") == 0) {
        wprintw(console->win, "Version %sdev\n", PACKAGE_VERSION);
    } else {
        wprintw(console->win, "Version %s\n", PACKAGE_VERSION);
    }
}


