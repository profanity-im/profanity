/* 
 * preferences.c
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

#include <stdlib.h>
#include <string.h>

#include <ncurses.h>
#include <glib.h>

#include "preferences.h"
#include "prof_autocomplete.h"

static GString *prefs_loc;
static GKeyFile *prefs;

// search logins list
static PAutocomplete ac;

struct colour_string_t {
    char *str;
    NCURSES_COLOR_T colour;
};
/*
static struct colour_string_t colours[] = {
    { "bkgnd", -1 },
    { "white", COLOR_WHITE },
    { "green", COLOR_GREEN },
    { "red", COLOR_RED },
    { "yellow", COLOR_YELLOW },
    { "blue", COLOR_BLUE },
    { "cyan", COLOR_CYAN },
};
*/
// colour preferences
static struct colours_t {
        NCURSES_COLOR_T bkgnd;
        NCURSES_COLOR_T text;
        NCURSES_COLOR_T online;
        NCURSES_COLOR_T err;
        NCURSES_COLOR_T inc;
        NCURSES_COLOR_T bar;
        NCURSES_COLOR_T bar_text;
} colour_prefs;

static void _load_colours(void);
static void _save_prefs(void);

void prefs_load(void)
{
    ac = p_autocomplete_new();
    prefs_loc = g_string_new(getenv("HOME"));
    g_string_append(prefs_loc, "/.profanity");

    prefs = g_key_file_new();
    g_key_file_load_from_file(prefs, prefs_loc->str, G_KEY_FILE_NONE, NULL);

    // create the logins searchable list for autocompletion
    gsize njids;
    gchar **jids =
        g_key_file_get_string_list(prefs, "connections", "logins", &njids, NULL);

    gsize i;
    for (i = 0; i < njids; i++) {
        p_autocomplete_add(ac, jids[i]);
    }

    _load_colours();
}

static void _load_colours(void)
{
    colour_prefs.bkgnd = -1;
    colour_prefs.text = COLOR_WHITE;
    colour_prefs.online = COLOR_GREEN;
    colour_prefs.err = COLOR_RED;
    colour_prefs.inc = COLOR_YELLOW;
    colour_prefs.bar = COLOR_BLUE;
    colour_prefs.bar_text = COLOR_CYAN;
}

char * find_login(char *prefix)
{
    return p_autocomplete_complete(ac, prefix);
}

void reset_login_search(void)
{
    p_autocomplete_reset(ac);
}

gboolean prefs_get_beep(void)
{
    return g_key_file_get_boolean(prefs, "ui", "beep", NULL);
}

void prefs_set_beep(gboolean value)
{
    g_key_file_set_boolean(prefs, "ui", "beep", value);
    _save_prefs();
}

gboolean prefs_get_flash(void)
{
    return g_key_file_get_boolean(prefs, "ui", "flash", NULL);
}

void prefs_set_flash(gboolean value)
{
    g_key_file_set_boolean(prefs, "ui", "flash", value);
    _save_prefs();
}

void prefs_add_login(const char *jid)
{
    gsize njids;
    gchar **jids = 
        g_key_file_get_string_list(prefs, "connections", "logins", &njids, NULL);

    // no logins remembered yet
    if (jids == NULL) {
        njids = 1;
        jids = (gchar**) g_malloc(sizeof(gchar *) * 2);
        jids[0] = g_strdup(jid);
        jids[1] = NULL;
        g_key_file_set_string_list(prefs, "connections", "logins", 
            (const gchar * const *)jids, njids);
        _save_prefs();
        g_strfreev(jids);
        
        return;
    } else {
        gsize i;
        for (i = 0; i < njids; i++) {
            if (strcmp(jid, jids[i]) == 0) {
                g_strfreev(jids);
                return;
            }
        }
    
        // jid not found, add to the list
        jids = (gchar **) g_realloc(jids, (sizeof(gchar *) * (njids+2)));
        jids[njids] = g_strdup(jid);
        njids++;
        jids[njids] = NULL;
        g_key_file_set_string_list(prefs, "connections", "logins",
            (const gchar * const *)jids, njids);
        _save_prefs();
        g_strfreev(jids);

        return;
    }
}

gboolean prefs_get_showsplash(void)
{
    return g_key_file_get_boolean(prefs, "ui", "showsplash", NULL);
}

void prefs_set_showsplash(gboolean value)
{
    g_key_file_set_boolean(prefs, "ui", "showsplash", value);
    _save_prefs();
}

static void _save_prefs(void)
{
    gsize g_data_size;
    char *g_prefs_data = g_key_file_to_data(prefs, &g_data_size, NULL);
    g_file_set_contents(prefs_loc->str, g_prefs_data, g_data_size, NULL);
}

NCURSES_COLOR_T prefs_get_bkgnd() 
{
    return colour_prefs.bkgnd;
}

NCURSES_COLOR_T prefs_get_text() 
{
    return colour_prefs.text;
}

NCURSES_COLOR_T prefs_get_online() 
{
    return colour_prefs.online;
}

NCURSES_COLOR_T prefs_get_err() 
{
    return colour_prefs.err;
}

NCURSES_COLOR_T prefs_get_inc() 
{
    return colour_prefs.inc;
}

NCURSES_COLOR_T prefs_get_bar() 
{
    return colour_prefs.bar;
}

NCURSES_COLOR_T prefs_get_bar_text() 
{
    return colour_prefs.bar_text;
}
