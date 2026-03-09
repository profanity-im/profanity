/*
 * settings.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#include "config.h"

#include <string.h>
#include <stdlib.h>

#include <glib.h>
#include <glib/gstdio.h>

#include "common.h"
#include "config/theme.h"
#include "config/files.h"
#include "config/conflists.h"

static prof_keyfile_t settings_prof_keyfile;
static GKeyFile* settings;

static void _save_settings(void);

void
plugin_settings_init(void)
{
    load_data_keyfile(&settings_prof_keyfile, FILE_PLUGIN_SETTINGS);
    settings = settings_prof_keyfile.keyfile;
}

void
plugin_settings_close(void)
{
    free_keyfile(&settings_prof_keyfile);
    settings = NULL;
}

gboolean
plugin_settings_boolean_get(const char* const group, const char* const key, gboolean def)
{
    if (group && key && g_key_file_has_key(settings, group, key, NULL)) {
        return g_key_file_get_boolean(settings, group, key, NULL);
    } else {
        return def;
    }
}

void
plugin_settings_boolean_set(const char* const group, const char* const key, gboolean value)
{
    g_key_file_set_boolean(settings, group, key, value);
    _save_settings();
}

char*
plugin_settings_string_get(const char* const group, const char* const key, const char* const def)
{
    if (group && key && g_key_file_has_key(settings, group, key, NULL)) {
        return g_key_file_get_string(settings, group, key, NULL);
    } else if (def) {
        return g_strdup(def);
    } else {
        return NULL;
    }
}

void
plugin_settings_string_set(const char* const group, const char* const key, const char* const value)
{
    g_key_file_set_string(settings, group, key, value);
    _save_settings();
}

int
plugin_settings_int_get(const char* const group, const char* const key, int def)
{
    if (group && key && g_key_file_has_key(settings, group, key, NULL)) {
        return g_key_file_get_integer(settings, group, key, NULL);
    } else {
        return def;
    }
}

void
plugin_settings_int_set(const char* const group, const char* const key, int value)
{
    g_key_file_set_integer(settings, group, key, value);
    _save_settings();
}

gchar**
plugin_settings_string_list_get(const char* const group, const char* const key)
{
    if (!g_key_file_has_key(settings, group, key, NULL)) {
        return NULL;
    }

    return g_key_file_get_string_list(settings, group, key, NULL, NULL);
}

int
plugin_settings_string_list_add(const char* const group, const char* const key, const char* const value)
{
    int res = conf_string_list_add(settings, group, key, value);
    _save_settings();

    return res;
}

int
plugin_settings_string_list_remove(const char* const group, const char* const key, const char* const value)
{
    int res = conf_string_list_remove(settings, group, key, value);
    _save_settings();

    return res;
}

int
plugin_settings_string_list_clear(const char* const group, const char* const key)
{
    if (!g_key_file_has_key(settings, group, key, NULL)) {
        return 0;
    }

    g_key_file_remove_key(settings, group, key, NULL);
    _save_settings();

    return 1;
}

static void
_save_settings(void)
{
    save_keyfile(&settings_prof_keyfile);
}
