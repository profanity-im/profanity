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

#include <glib.h>

#ifdef HAVE_NCURSESW_NCURSES_H
#include <ncursesw/ncurses.h>
#elif HAVE_NCURSES_H
#include <ncurses.h>
#endif

#include "config.h"
#include "files.h"
#include "log.h"
#include "preferences.h"
#include "prof_autocomplete.h"

static gchar *prefs_loc;
static GKeyFile *prefs;
gint log_maxsize = 0;

static PAutocomplete boolean_choice_ac;

static void _save_prefs(void);

void
prefs_load(void)
{
    GError *err;

    log_info("Loading preferences");
    prefs_loc = files_get_preferences_file();

    prefs = g_key_file_new();
    g_key_file_load_from_file(prefs, prefs_loc, G_KEY_FILE_KEEP_COMMENTS,
        NULL);

    err = NULL;
    log_maxsize = g_key_file_get_integer(prefs, "logging", "maxsize", &err);
    if (err != NULL) {
        log_maxsize = 0;
        g_error_free(err);
    }

    boolean_choice_ac = p_autocomplete_new();
    p_autocomplete_add(boolean_choice_ac, strdup("on"));
    p_autocomplete_add(boolean_choice_ac, strdup("off"));
}

void
prefs_close(void)
{
    p_autocomplete_free(boolean_choice_ac);
    g_key_file_free(prefs);
}

char *
prefs_autocomplete_boolean_choice(char *prefix)
{
    return p_autocomplete_complete(boolean_choice_ac, prefix);
}

void
prefs_reset_boolean_choice(void)
{
    p_autocomplete_reset(boolean_choice_ac);
}

gboolean
prefs_get_beep(void)
{
    return g_key_file_get_boolean(prefs, "ui", "beep", NULL);
}

void
prefs_set_beep(gboolean value)
{
    g_key_file_set_boolean(prefs, "ui", "beep", value);
    _save_prefs();
}

gchar *
prefs_get_theme(void)
{
    return g_key_file_get_string(prefs, "ui", "theme", NULL);
}

void
prefs_set_theme(gchar *value)
{
    g_key_file_set_string(prefs, "ui", "theme", value);
    _save_prefs();
}

gboolean
prefs_get_states(void)
{
    return g_key_file_get_boolean(prefs, "chatstates", "enabled", NULL);
}

void
prefs_set_states(gboolean value)
{
    g_key_file_set_boolean(prefs, "chatstates", "enabled", value);
    _save_prefs();
}

gboolean
prefs_get_outtype(void)
{
    return g_key_file_get_boolean(prefs, "chatstates", "outtype", NULL);
}

void
prefs_set_outtype(gboolean value)
{
    g_key_file_set_boolean(prefs, "chatstates", "outtype", value);
    _save_prefs();
}

gint
prefs_get_gone(void)
{
    return g_key_file_get_integer(prefs, "chatstates", "gone", NULL);
}

void
prefs_set_gone(gint value)
{
    g_key_file_set_integer(prefs, "chatstates", "gone", value);
    _save_prefs();
}

gboolean
prefs_get_notify_typing(void)
{
    return g_key_file_get_boolean(prefs, "notifications", "typing", NULL);
}

void
prefs_set_notify_typing(gboolean value)
{
    g_key_file_set_boolean(prefs, "notifications", "typing", value);
    _save_prefs();
}

gboolean
prefs_get_notify_message(void)
{
    return g_key_file_get_boolean(prefs, "notifications", "message", NULL);
}

void
prefs_set_notify_message(gboolean value)
{
    g_key_file_set_boolean(prefs, "notifications", "message", value);
    _save_prefs();
}

gint
prefs_get_notify_remind(void)
{
    return g_key_file_get_integer(prefs, "notifications", "remind", NULL);
}

void
prefs_set_notify_remind(gint value)
{
    g_key_file_set_integer(prefs, "notifications", "remind", value);
    _save_prefs();
}

gint
prefs_get_max_log_size(void)
{
    if (log_maxsize < PREFS_MIN_LOG_SIZE)
        return PREFS_MAX_LOG_SIZE;
    else
        return log_maxsize;
}

void
prefs_set_max_log_size(gint value)
{
    log_maxsize = value;
    g_key_file_set_integer(prefs, "logging", "maxsize", value);
    _save_prefs();
}

gint
prefs_get_priority(void)
{
    return g_key_file_get_integer(prefs, "presence", "priority", NULL);
}

void
prefs_set_priority(gint value)
{
    g_key_file_set_integer(prefs, "presence", "priority", value);
    _save_prefs();
}

gint
prefs_get_reconnect(void)
{
    return g_key_file_get_integer(prefs, "connection", "reconnect", NULL);
}

void
prefs_set_reconnect(gint value)
{
    g_key_file_set_integer(prefs, "connection", "reconnect", value);
    _save_prefs();
}

gint
prefs_get_autoping(void)
{
    return g_key_file_get_integer(prefs, "connection", "autoping", NULL);
}

void
prefs_set_autoping(gint value)
{
    g_key_file_set_integer(prefs, "connection", "autoping", value);
    _save_prefs();
}

gboolean
prefs_get_vercheck(void)
{
    return g_key_file_get_boolean(prefs, "ui", "vercheck", NULL);
}

void
prefs_set_vercheck(gboolean value)
{
    g_key_file_set_boolean(prefs, "ui", "vercheck", value);
    _save_prefs();
}

gboolean
prefs_get_titlebarversion(void)
{
    return g_key_file_get_boolean(prefs, "ui", "titlebar.version", NULL);
}

void
prefs_set_titlebarversion(gboolean value)
{
    g_key_file_set_boolean(prefs, "ui", "titlebar.version", value);
    _save_prefs();
}

gboolean
prefs_get_flash(void)
{
    return g_key_file_get_boolean(prefs, "ui", "flash", NULL);
}

void
prefs_set_flash(gboolean value)
{
    g_key_file_set_boolean(prefs, "ui", "flash", value);
    _save_prefs();
}

gboolean
prefs_get_intype(void)
{
    return g_key_file_get_boolean(prefs, "ui", "intype", NULL);
}

void
prefs_set_intype(gboolean value)
{
    g_key_file_set_boolean(prefs, "ui", "intype", value);
    _save_prefs();
}

gboolean
prefs_get_chlog(void)
{
    return g_key_file_get_boolean(prefs, "logging", "chlog", NULL);
}

void
prefs_set_chlog(gboolean value)
{
    g_key_file_set_boolean(prefs, "logging", "chlog", value);
    _save_prefs();
}

gboolean
prefs_get_history(void)
{
    return g_key_file_get_boolean(prefs, "ui", "history", NULL);
}

void
prefs_set_history(gboolean value)
{
    g_key_file_set_boolean(prefs, "ui", "history", value);
    _save_prefs();
}

gchar *
prefs_get_autoaway_mode(void)
{
    gchar *result = g_key_file_get_string(prefs, "presence", "autoaway.mode", NULL);
    if (result == NULL) {
        return strdup("off");
    } else {
        return result;
    }
}

void
prefs_set_autoaway_mode(gchar *value)
{
    g_key_file_set_string(prefs, "presence", "autoaway.mode", value);
    _save_prefs();
}

gint
prefs_get_autoaway_time(void)
{
    gint result = g_key_file_get_integer(prefs, "presence", "autoaway.time", NULL);

    if (result == 0) {
        return 15;
    } else {
        return result;
    }
}

void
prefs_set_autoaway_time(gint value)
{
    g_key_file_set_integer(prefs, "presence", "autoaway.time", value);
    _save_prefs();
}

gchar *
prefs_get_autoaway_message(void)
{
    return g_key_file_get_string(prefs, "presence", "autoaway.message", NULL);
}

void
prefs_set_autoaway_message(gchar *value)
{
    if (value == NULL) {
        g_key_file_remove_key(prefs, "presence", "autoaway.message", NULL);
    } else {
        g_key_file_set_string(prefs, "presence", "autoaway.message", value);
    }
    _save_prefs();
}

gboolean
prefs_get_autoaway_check(void)
{
    if (g_key_file_has_key(prefs, "presence", "autoaway.check", NULL)) {
        return g_key_file_get_boolean(prefs, "presence", "autoaway.check", NULL);
    } else {
        return TRUE;
    }
}

void
prefs_set_autoaway_check(gboolean value)
{
    g_key_file_set_boolean(prefs, "presence", "autoaway.check", value);
    _save_prefs();
}

gboolean
prefs_get_splash(void)
{
    return g_key_file_get_boolean(prefs, "ui", "splash", NULL);
}

void
prefs_set_splash(gboolean value)
{
    g_key_file_set_boolean(prefs, "ui", "splash", value);
    _save_prefs();
}

static void
_save_prefs(void)
{
    gsize g_data_size;
    char *g_prefs_data = g_key_file_to_data(prefs, &g_data_size, NULL);
    g_file_set_contents(prefs_loc, g_prefs_data, g_data_size, NULL);
}
