/*
 * preferences.c
 *
 * Copyright (C) 2012 - 2014 James Booth <boothj5@gmail.com>
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
#include <stdio.h>
#include <string.h>

#include <glib.h>
#ifdef HAVE_NCURSESW_NCURSES_H
#include <ncursesw/ncurses.h>
#elif HAVE_NCURSES_H
#include <ncurses.h>
#endif

#include "common.h"
#include "log.h"
#include "preferences.h"
#include "tools/autocomplete.h"

#define PREF_GROUP_LOGGING "logging"
#define PREF_GROUP_CHATSTATES "chatstates"
#define PREF_GROUP_UI "ui"
#define PREF_GROUP_NOTIFICATIONS "notifications"
#define PREF_GROUP_PRESENCE "presence"
#define PREF_GROUP_CONNECTION "connection"
#define PREF_GROUP_ALIAS "alias"

static gchar *prefs_loc;
static GKeyFile *prefs;
gint log_maxsize = 0;

static Autocomplete boolean_choice_ac;

static void _save_prefs(void);
static gchar * _get_preferences_file(void);
static const char * _get_group(preference_t pref);
static const char * _get_key(preference_t pref);
static gboolean _get_default_boolean(preference_t pref);
static char * _get_default_string(preference_t pref);

void
prefs_load(void)
{
    GError *err;

    log_info("Loading preferences");
    prefs_loc = _get_preferences_file();

    prefs = g_key_file_new();
    g_key_file_load_from_file(prefs, prefs_loc, G_KEY_FILE_KEEP_COMMENTS,
        NULL);

    err = NULL;
    log_maxsize = g_key_file_get_integer(prefs, PREF_GROUP_LOGGING, "maxsize", &err);
    if (err != NULL) {
        log_maxsize = 0;
        g_error_free(err);
    }

    boolean_choice_ac = autocomplete_new();
    autocomplete_add(boolean_choice_ac, "on");
    autocomplete_add(boolean_choice_ac, "off");
}

void
prefs_close(void)
{
    autocomplete_free(boolean_choice_ac);
    g_key_file_free(prefs);
    prefs = NULL;
}

char *
prefs_autocomplete_boolean_choice(char *prefix)
{
    return autocomplete_complete(boolean_choice_ac, prefix);
}

void
prefs_reset_boolean_choice(void)
{
    autocomplete_reset(boolean_choice_ac);
}

gboolean
prefs_get_boolean(preference_t pref)
{
    const char *group = _get_group(pref);
    const char *key = _get_key(pref);
    gboolean def = _get_default_boolean(pref);

    if (!g_key_file_has_key(prefs, group, key, NULL)) {
        return def;
    }

    return g_key_file_get_boolean(prefs, group, key, NULL);
}

void
prefs_set_boolean(preference_t pref, gboolean value)
{
    const char *group = _get_group(pref);
    const char *key = _get_key(pref);
    g_key_file_set_boolean(prefs, group, key, value);
    _save_prefs();
}

char *
prefs_get_string(preference_t pref)
{
    const char *group = _get_group(pref);
    const char *key = _get_key(pref);
    char *def = _get_default_string(pref);

    if (!g_key_file_has_key(prefs, group, key, NULL)) {
        return def;
    }

    char *result = g_key_file_get_string(prefs, group, key, NULL);

    if (result == NULL) {
        return def;
    } else {
        return result;
    }
}

void
prefs_set_string(preference_t pref, char *value)
{
    const char *group = _get_group(pref);
    const char *key = _get_key(pref);
    if (value == NULL) {
        g_key_file_remove_key(prefs, group, key, NULL);
    } else {
        g_key_file_set_string(prefs, group, key, value);
    }
    _save_prefs();
}

gint
prefs_get_gone(void)
{
    return g_key_file_get_integer(prefs, PREF_GROUP_CHATSTATES, "gone", NULL);
}

void
prefs_set_gone(gint value)
{
    g_key_file_set_integer(prefs, PREF_GROUP_CHATSTATES, "gone", value);
    _save_prefs();
}

gint
prefs_get_notify_remind(void)
{
    return g_key_file_get_integer(prefs, PREF_GROUP_NOTIFICATIONS, "remind", NULL);
}

void
prefs_set_notify_remind(gint value)
{
    g_key_file_set_integer(prefs, PREF_GROUP_NOTIFICATIONS, "remind", value);
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
    g_key_file_set_integer(prefs, PREF_GROUP_LOGGING, "maxsize", value);
    _save_prefs();
}

gint
prefs_get_priority(void)
{
    return g_key_file_get_integer(prefs, PREF_GROUP_PRESENCE, "priority", NULL);
}

void
prefs_set_priority(gint value)
{
    g_key_file_set_integer(prefs, PREF_GROUP_PRESENCE, "priority", value);
    _save_prefs();
}

gint
prefs_get_reconnect(void)
{
    return g_key_file_get_integer(prefs, PREF_GROUP_CONNECTION, "reconnect", NULL);
}

void
prefs_set_reconnect(gint value)
{
    g_key_file_set_integer(prefs, PREF_GROUP_CONNECTION, "reconnect", value);
    _save_prefs();
}

gint
prefs_get_autoping(void)
{
    return g_key_file_get_integer(prefs, PREF_GROUP_CONNECTION, "autoping", NULL);
}

void
prefs_set_autoping(gint value)
{
    g_key_file_set_integer(prefs, PREF_GROUP_CONNECTION, "autoping", value);
    _save_prefs();
}

gint
prefs_get_autoaway_time(void)
{
    gint result = g_key_file_get_integer(prefs, PREF_GROUP_PRESENCE, "autoaway.time", NULL);

    if (result == 0) {
        return 15;
    } else {
        return result;
    }
}

void
prefs_set_autoaway_time(gint value)
{
    g_key_file_set_integer(prefs, PREF_GROUP_PRESENCE, "autoaway.time", value);
    _save_prefs();
}

gboolean
prefs_add_alias(const char * const name, const char * const value)
{
    if (g_key_file_has_key(prefs, PREF_GROUP_ALIAS, name, NULL)) {
        return FALSE;
    } else {
        g_key_file_set_string(prefs, PREF_GROUP_ALIAS, name, value);
        _save_prefs();
        return TRUE;
    }
}

char *
prefs_get_alias(const char * const name)
{
    return g_key_file_get_string(prefs, PREF_GROUP_ALIAS, name, NULL);

}

gboolean
prefs_remove_alias(const char * const name)
{
    if (!g_key_file_has_key(prefs, PREF_GROUP_ALIAS, name, NULL)) {
        return FALSE;
    } else {
        g_key_file_remove_key(prefs, PREF_GROUP_ALIAS, name, NULL);
        _save_prefs();
        return TRUE;
    }
}

static gint
_alias_cmp(gconstpointer *p1, gconstpointer *p2)
{
    ProfAlias *alias1 = (ProfAlias*)p1;
    ProfAlias *alias2 = (ProfAlias*)p2;

    return strcmp(alias1->name, alias2->name);
}

GList *
prefs_get_aliases(void)
{
    if (!g_key_file_has_group(prefs, PREF_GROUP_ALIAS)) {
        return NULL;
    } else {
        GList *result = NULL;
        gsize len;
        gchar **keys = g_key_file_get_keys(prefs, PREF_GROUP_ALIAS, &len, NULL);
        int i;
        for (i = 0; i < len; i++) {
            char *name = keys[i];
            char *value = g_key_file_get_string(prefs, PREF_GROUP_ALIAS, name, NULL);

            ProfAlias *alias = malloc(sizeof(struct prof_alias_t));
            alias->name = strdup(name);
            alias->value = strdup(value);

            result = g_list_insert_sorted(result, alias, (GCompareFunc)_alias_cmp);
        }

        g_strfreev(keys);

        return result;
    }
}

void
_free_alias(ProfAlias *alias)
{
    FREE_SET_NULL(alias->name);
    FREE_SET_NULL(alias->value);
    FREE_SET_NULL(alias);
}

void
prefs_free_aliases(GList *aliases)
{
    g_list_free_full(aliases, (GDestroyNotify)_free_alias);
}

static void
_save_prefs(void)
{
    gsize g_data_size;
    char *g_prefs_data = g_key_file_to_data(prefs, &g_data_size, NULL);
    g_file_set_contents(prefs_loc, g_prefs_data, g_data_size, NULL);
}

static gchar *
_get_preferences_file(void)
{
    gchar *xdg_config = xdg_get_config_home();
    GString *prefs_file = g_string_new(xdg_config);
    g_string_append(prefs_file, "/profanity/profrc");
    gchar *result = strdup(prefs_file->str);
    g_free(xdg_config);
    g_string_free(prefs_file, TRUE);

    return result;
}

static const char *
_get_group(preference_t pref)
{
    switch (pref)
    {
        case PREF_SPLASH:
        case PREF_BEEP:
        case PREF_THEME:
        case PREF_VERCHECK:
        case PREF_TITLEBAR:
        case PREF_FLASH:
        case PREF_INTYPE:
        case PREF_HISTORY:
        case PREF_MOUSE:
        case PREF_STATUSES:
        case PREF_STATUSES_CONSOLE:
        case PREF_STATUSES_CHAT:
        case PREF_STATUSES_MUC:
        case PREF_OTR_WARN:
            return PREF_GROUP_UI;
        case PREF_STATES:
        case PREF_OUTTYPE:
            return PREF_GROUP_CHATSTATES;
        case PREF_NOTIFY_TYPING:
        case PREF_NOTIFY_MESSAGE:
        case PREF_NOTIFY_INVITE:
        case PREF_NOTIFY_SUB:
            return PREF_GROUP_NOTIFICATIONS;
        case PREF_CHLOG:
        case PREF_GRLOG:
        case PREF_OTR_LOG:
            return PREF_GROUP_LOGGING;
        case PREF_AUTOAWAY_CHECK:
        case PREF_AUTOAWAY_MODE:
        case PREF_AUTOAWAY_MESSAGE:
            return PREF_GROUP_PRESENCE;
        case PREF_CONNECT_ACCOUNT:
            return PREF_GROUP_CONNECTION;
        default:
            return NULL;
    }
}

static const char *
_get_key(preference_t pref)
{
    switch (pref)
    {
        case PREF_SPLASH:
            return "splash";
        case PREF_BEEP:
            return "beep";
        case PREF_THEME:
            return "theme";
        case PREF_VERCHECK:
            return "vercheck";
        case PREF_TITLEBAR:
            return "titlebar";
        case PREF_FLASH:
            return "flash";
        case PREF_INTYPE:
            return "intype";
        case PREF_HISTORY:
            return "history";
        case PREF_MOUSE:
            return "mouse";
        case PREF_STATUSES:
            return "statuses";
        case PREF_STATUSES_CONSOLE:
            return "statuses.console";
        case PREF_STATUSES_CHAT:
            return "statuses.chat";
        case PREF_STATUSES_MUC:
            return "statuses.muc";
        case PREF_STATES:
            return "enabled";
        case PREF_OUTTYPE:
            return "outtype";
        case PREF_NOTIFY_TYPING:
            return "typing";
        case PREF_NOTIFY_MESSAGE:
            return "message";
        case PREF_NOTIFY_INVITE:
            return "invite";
        case PREF_NOTIFY_SUB:
            return "sub";
        case PREF_CHLOG:
            return "chlog";
        case PREF_GRLOG:
            return "grlog";
        case PREF_AUTOAWAY_CHECK:
            return "autoaway.check";
        case PREF_AUTOAWAY_MODE:
            return "autoaway.mode";
        case PREF_AUTOAWAY_MESSAGE:
            return "autoaway.message";
        case PREF_CONNECT_ACCOUNT:
            return "account";
        case PREF_OTR_LOG:
            return "otr";
        case PREF_OTR_WARN:
            return "otr.warn";
        default:
            return NULL;
    }
}

static gboolean
_get_default_boolean(preference_t pref)
{
    switch (pref)
    {
        case PREF_TITLEBAR:
        case PREF_OTR_WARN:
        case PREF_AUTOAWAY_CHECK:
            return TRUE;
        default:
            return FALSE;
    }
}

static char *
_get_default_string(preference_t pref)
{
    switch (pref)
    {
        case PREF_AUTOAWAY_MODE:
            return "off";
        case PREF_OTR_LOG:
            return "redact";
        case PREF_STATUSES_CONSOLE:
        case PREF_STATUSES_CHAT:
        case PREF_STATUSES_MUC:
            return "all";
        default:
            return NULL;
    }
}
