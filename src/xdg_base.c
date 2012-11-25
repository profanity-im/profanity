/*
 * xdg_base.c
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

#include <glib.h>

gchar *
xdg_get_config_home(void)
{
    gchar *xdg_config_home = getenv("XDG_CONFIG_HOME");
    g_strstrip(xdg_config_home);

    if ((xdg_config_home != NULL) && (strcmp(xdg_config_home, "") != 0)) {
        return strdup(xdg_config_home);
    } else {
        GString *default_path = g_string_new(getenv("HOME"));
        g_string_append(default_path, "/.config");
        gchar *result = strdup(default_path->str);
        g_string_free(default_path, TRUE);
        
        return result;
    }
}

gchar *
xdg_get_data_home(void)
{
    gchar *xdg_data_home = getenv("XDG_DATA_HOME");
    g_strstrip(xdg_data_home);

    if ((xdg_data_home != NULL) && (strcmp(xdg_data_home, "") != 0)) {
        return strdup(xdg_data_home);
    } else {
        GString *default_path = g_string_new(getenv("HOME"));
        g_string_append(default_path, "/.local/share");
        gchar *result = strdup(default_path->str);
        g_string_free(default_path, TRUE);
        
        return result;
    }
}
