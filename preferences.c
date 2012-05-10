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
#include <glib.h>

#include "windows.h"

void prefs_load(void)
{
    GString *prefs_loc = g_string_new(getenv("HOME"));
    g_string_append(prefs_loc, "/.profanity");

    GKeyFile *g_prefs = g_key_file_new();
    g_key_file_load_from_file(g_prefs, prefs_loc->str,
        G_KEY_FILE_NONE, NULL);

    gboolean beep = g_key_file_get_boolean(g_prefs, "ui", "beep", NULL);
    gboolean flash = g_key_file_get_boolean(g_prefs, "ui", "flash", NULL);

    win_set_beep(beep);
    status_bar_set_flash(flash);

//    g_key_file_set_string(g_prefs, "settings", "somekey2", "someothervalue");
//    gsize g_data_len;
//    char *g_prefs_data = g_key_file_to_data(g_prefs, &g_data_len, NULL);
//    g_file_set_contents("/home/james/.profanity", g_prefs_data, g_data_len, NULL);
}
