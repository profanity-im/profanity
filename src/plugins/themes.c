/*
 * themes.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#include "config.h"

#include <stdlib.h>

#include <glib.h>
#include <glib/gstdio.h>

#include "common.h"
#include "config/theme.h"
#include "config/files.h"

static prof_keyfile_t themes_prof_keyfile;
static GKeyFile* themes;

void
plugin_themes_init(void)
{
    load_data_keyfile(&themes_prof_keyfile, FILE_PLUGIN_THEMES);
    themes = themes_prof_keyfile.keyfile;
}

void
plugin_themes_close(void)
{
    free_keyfile(&themes_prof_keyfile);
    themes = NULL;
}

theme_item_t
plugin_themes_get(const char* const group, const char* const key, const char* const def)
{
    if (group && key && g_key_file_has_key(themes, group, key, NULL)) {
        auto_gchar gchar* result = g_key_file_get_string(themes, group, key, NULL);

        theme_item_t ret;

        if (g_strcmp0(result, "white") == 0)
            ret = THEME_WHITE;
        else if (g_strcmp0(result, "bold_white") == 0)
            ret = THEME_WHITE_BOLD;
        else if (g_strcmp0(result, "red") == 0)
            ret = THEME_RED;
        else if (g_strcmp0(result, "bold_red") == 0)
            ret = THEME_RED_BOLD;
        else if (g_strcmp0(result, "green") == 0)
            ret = THEME_GREEN;
        else if (g_strcmp0(result, "bold_green") == 0)
            ret = THEME_GREEN_BOLD;
        else if (g_strcmp0(result, "blue") == 0)
            ret = THEME_BLUE;
        else if (g_strcmp0(result, "bold_blue") == 0)
            ret = THEME_BLUE_BOLD;
        else if (g_strcmp0(result, "yellow") == 0)
            ret = THEME_YELLOW;
        else if (g_strcmp0(result, "bold_yellow") == 0)
            ret = THEME_YELLOW_BOLD;
        else if (g_strcmp0(result, "cyan") == 0)
            ret = THEME_CYAN;
        else if (g_strcmp0(result, "bold_cyan") == 0)
            ret = THEME_CYAN_BOLD;
        else if (g_strcmp0(result, "magenta") == 0)
            ret = THEME_MAGENTA;
        else if (g_strcmp0(result, "bold_magenta") == 0)
            ret = THEME_MAGENTA_BOLD;
        else if (g_strcmp0(result, "black") == 0)
            ret = THEME_BLACK;
        else if (g_strcmp0(result, "bold_black") == 0)
            ret = THEME_BLACK_BOLD;

        else if (g_strcmp0(def, "white") == 0)
            ret = THEME_WHITE;
        else if (g_strcmp0(def, "bold_white") == 0)
            ret = THEME_WHITE_BOLD;
        else if (g_strcmp0(def, "red") == 0)
            ret = THEME_RED;
        else if (g_strcmp0(def, "bold_red") == 0)
            ret = THEME_RED_BOLD;
        else if (g_strcmp0(def, "green") == 0)
            ret = THEME_GREEN;
        else if (g_strcmp0(def, "bold_green") == 0)
            ret = THEME_GREEN_BOLD;
        else if (g_strcmp0(def, "blue") == 0)
            ret = THEME_BLUE;
        else if (g_strcmp0(def, "bold_blue") == 0)
            ret = THEME_BLUE_BOLD;
        else if (g_strcmp0(def, "yellow") == 0)
            ret = THEME_YELLOW;
        else if (g_strcmp0(def, "bold_yellow") == 0)
            ret = THEME_YELLOW_BOLD;
        else if (g_strcmp0(def, "cyan") == 0)
            ret = THEME_CYAN;
        else if (g_strcmp0(def, "bold_cyan") == 0)
            ret = THEME_CYAN_BOLD;
        else if (g_strcmp0(def, "magenta") == 0)
            ret = THEME_MAGENTA;
        else if (g_strcmp0(def, "bold_magenta") == 0)
            ret = THEME_MAGENTA_BOLD;
        else if (g_strcmp0(def, "black") == 0)
            ret = THEME_BLACK;
        else if (g_strcmp0(def, "bold_black") == 0)
            ret = THEME_BLACK_BOLD;

        else
            ret = THEME_TEXT;

        return ret;

    } else {
        if (g_strcmp0(def, "white") == 0)
            return THEME_WHITE;
        else if (g_strcmp0(def, "bold_white") == 0)
            return THEME_WHITE_BOLD;
        else if (g_strcmp0(def, "red") == 0)
            return THEME_RED;
        else if (g_strcmp0(def, "bold_red") == 0)
            return THEME_RED_BOLD;
        else if (g_strcmp0(def, "green") == 0)
            return THEME_GREEN;
        else if (g_strcmp0(def, "bold_green") == 0)
            return THEME_GREEN_BOLD;
        else if (g_strcmp0(def, "blue") == 0)
            return THEME_BLUE;
        else if (g_strcmp0(def, "bold_blue") == 0)
            return THEME_BLUE_BOLD;
        else if (g_strcmp0(def, "yellow") == 0)
            return THEME_YELLOW;
        else if (g_strcmp0(def, "bold_yellow") == 0)
            return THEME_YELLOW_BOLD;
        else if (g_strcmp0(def, "cyan") == 0)
            return THEME_CYAN;
        else if (g_strcmp0(def, "bold_cyan") == 0)
            return THEME_CYAN_BOLD;
        else if (g_strcmp0(def, "magenta") == 0)
            return THEME_MAGENTA;
        else if (g_strcmp0(def, "bold_magenta") == 0)
            return THEME_MAGENTA_BOLD;
        else if (g_strcmp0(def, "black") == 0)
            return THEME_BLACK;
        else if (g_strcmp0(def, "bold_black") == 0)
            return THEME_BLACK_BOLD;

        else
            return THEME_TEXT;
    }
}
