/*
 * themes.c
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

#include <stdlib.h>

#include <glib.h>
#include <glib/gstdio.h>

#include "common.h"
#include "config/theme.h"
#include "config/files.h"

static GKeyFile *themes;

void
plugin_themes_init(void)
{
    char *themes_file = files_get_data_path(FILE_PLUGIN_THEMES);

    if (g_file_test(themes_file, G_FILE_TEST_EXISTS)) {
        g_chmod(themes_file, S_IRUSR | S_IWUSR);
    }

    themes = g_key_file_new();
    g_key_file_load_from_file(themes, themes_file, G_KEY_FILE_KEEP_COMMENTS, NULL);

    gsize g_data_size;
    gchar *g_data = g_key_file_to_data(themes, &g_data_size, NULL);
    g_file_set_contents(themes_file, g_data, g_data_size, NULL);
    g_chmod(themes_file, S_IRUSR | S_IWUSR);
    g_free(g_data);
    free(themes_file);
}

void
plugin_themes_close(void)
{
    g_key_file_free(themes);
    themes = NULL;
}

theme_item_t
plugin_themes_get(const char *const group, const char *const key, const char *const def)
{
    if (group && key && g_key_file_has_key(themes, group, key, NULL)) {
        gchar *result = g_key_file_get_string(themes, group, key, NULL);

        theme_item_t ret;

        if (g_strcmp0(result, "white") == 0)               ret = THEME_WHITE;
        else if (g_strcmp0(result, "bold_white") == 0)     ret = THEME_WHITE_BOLD;
        else if (g_strcmp0(result, "red") == 0)            ret = THEME_RED;
        else if (g_strcmp0(result, "bold_red") == 0)       ret = THEME_RED_BOLD;
        else if (g_strcmp0(result, "green") == 0)          ret = THEME_GREEN;
        else if (g_strcmp0(result, "bold_green") == 0)     ret = THEME_GREEN_BOLD;
        else if (g_strcmp0(result, "blue") == 0)           ret = THEME_BLUE;
        else if (g_strcmp0(result, "bold_blue") == 0)      ret = THEME_BLUE_BOLD;
        else if (g_strcmp0(result, "yellow") == 0)         ret = THEME_YELLOW;
        else if (g_strcmp0(result, "bold_yellow") == 0)    ret = THEME_YELLOW_BOLD;
        else if (g_strcmp0(result, "cyan") == 0)           ret = THEME_CYAN;
        else if (g_strcmp0(result, "bold_cyan") == 0)      ret = THEME_CYAN_BOLD;
        else if (g_strcmp0(result, "magenta") == 0)        ret = THEME_MAGENTA;
        else if (g_strcmp0(result, "bold_magenta") == 0)   ret = THEME_MAGENTA_BOLD;
        else if (g_strcmp0(result, "black") == 0)          ret = THEME_BLACK;
        else if (g_strcmp0(result, "bold_black") == 0)     ret = THEME_BLACK_BOLD;

        else if (g_strcmp0(def, "white") == 0)          ret = THEME_WHITE;
        else if (g_strcmp0(def, "bold_white") == 0)     ret = THEME_WHITE_BOLD;
        else if (g_strcmp0(def, "red") == 0)            ret = THEME_RED;
        else if (g_strcmp0(def, "bold_red") == 0)       ret = THEME_RED_BOLD;
        else if (g_strcmp0(def, "green") == 0)          ret = THEME_GREEN;
        else if (g_strcmp0(def, "bold_green") == 0)     ret = THEME_GREEN_BOLD;
        else if (g_strcmp0(def, "blue") == 0)           ret = THEME_BLUE;
        else if (g_strcmp0(def, "bold_blue") == 0)      ret = THEME_BLUE_BOLD;
        else if (g_strcmp0(def, "yellow") == 0)         ret = THEME_YELLOW;
        else if (g_strcmp0(def, "bold_yellow") == 0)    ret = THEME_YELLOW_BOLD;
        else if (g_strcmp0(def, "cyan") == 0)           ret = THEME_CYAN;
        else if (g_strcmp0(def, "bold_cyan") == 0)      ret = THEME_CYAN_BOLD;
        else if (g_strcmp0(def, "magenta") == 0)        ret = THEME_MAGENTA;
        else if (g_strcmp0(def, "bold_magenta") == 0)   ret = THEME_MAGENTA_BOLD;
        else if (g_strcmp0(def, "black") == 0)          ret = THEME_BLACK;
        else if (g_strcmp0(def, "bold_black") == 0)     ret = THEME_BLACK_BOLD;

        else ret = THEME_TEXT;

        g_free(result);

        return ret;

    } else {
        if (g_strcmp0(def, "white") == 0)               return THEME_WHITE;
        else if (g_strcmp0(def, "bold_white") == 0)     return THEME_WHITE_BOLD;
        else if (g_strcmp0(def, "red") == 0)            return THEME_RED;
        else if (g_strcmp0(def, "bold_red") == 0)       return THEME_RED_BOLD;
        else if (g_strcmp0(def, "green") == 0)          return THEME_GREEN;
        else if (g_strcmp0(def, "bold_green") == 0)     return THEME_GREEN_BOLD;
        else if (g_strcmp0(def, "blue") == 0)           return THEME_BLUE;
        else if (g_strcmp0(def, "bold_blue") == 0)      return THEME_BLUE_BOLD;
        else if (g_strcmp0(def, "yellow") == 0)         return THEME_YELLOW;
        else if (g_strcmp0(def, "bold_yellow") == 0)    return THEME_YELLOW_BOLD;
        else if (g_strcmp0(def, "cyan") == 0)           return THEME_CYAN;
        else if (g_strcmp0(def, "bold_cyan") == 0)      return THEME_CYAN_BOLD;
        else if (g_strcmp0(def, "magenta") == 0)        return THEME_MAGENTA;
        else if (g_strcmp0(def, "bold_magenta") == 0)   return THEME_MAGENTA_BOLD;
        else if (g_strcmp0(def, "black") == 0)          return THEME_BLACK;
        else if (g_strcmp0(def, "bold_black") == 0)     return THEME_BLACK_BOLD;

        else return THEME_TEXT;
    }
}
