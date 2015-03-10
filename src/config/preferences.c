/*
 * preferences.c
 *
 * Copyright (C) 2012 - 2015 James Booth <boothj5@gmail.com>
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

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <glib.h>
#include <glib/gstdio.h>

#ifdef HAVE_NCURSESW_NCURSES_H
#include <ncursesw/ncurses.h>
#elif HAVE_NCURSES_H
#include <ncurses.h>
#endif

#include "common.h"
#include "log.h"
#include "preferences.h"
#include "tools/autocomplete.h"

// preference groups refer to the sections in .profrc, for example [ui]
#define PREF_GROUP_LOGGING "logging"
#define PREF_GROUP_CHATSTATES "chatstates"
#define PREF_GROUP_UI "ui"
#define PREF_GROUP_NOTIFICATIONS "notifications"
#define PREF_GROUP_PRESENCE "presence"
#define PREF_GROUP_CONNECTION "connection"
#define PREF_GROUP_ALIAS "alias"
#define PREF_GROUP_OTR "otr"

#define INPBLOCK_DEFAULT 1000

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

    if (g_file_test(prefs_loc, G_FILE_TEST_EXISTS)) {
        g_chmod(prefs_loc, S_IRUSR | S_IWUSR);
    }

    prefs = g_key_file_new();
    g_key_file_load_from_file(prefs, prefs_loc, G_KEY_FILE_KEEP_COMMENTS,
        NULL);

    err = NULL;
    log_maxsize = g_key_file_get_integer(prefs, PREF_GROUP_LOGGING, "maxsize", &err);
    if (err != NULL) {
        log_maxsize = 0;
        g_error_free(err);
    }

    // move pre 0.4.6 OTR warn preferences to [ui] group
    err = NULL;
    gboolean otr_warn = g_key_file_get_boolean(prefs, PREF_GROUP_OTR, "warn", &err);
    if (err == NULL) {
        g_key_file_set_boolean(prefs, PREF_GROUP_UI, _get_key(PREF_OTR_WARN), otr_warn);
        g_key_file_remove_key(prefs, PREF_GROUP_OTR, "warn", NULL);
    } else {
        g_error_free(err);
    }

    // move pre 0.4.6 titlebar preference
    err = NULL;
    gchar *old_titlebar = g_key_file_get_string(prefs, PREF_GROUP_UI, "titlebar", &err);
    if (err == NULL) {
        g_key_file_set_string(prefs, PREF_GROUP_UI, _get_key(PREF_TITLEBAR_SHOW), old_titlebar);
        g_key_file_remove_key(prefs, PREF_GROUP_UI, "titlebar", NULL);
    } else {
        g_error_free(err);
    }

    _save_prefs();

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
prefs_autocomplete_boolean_choice(const char * const prefix)
{
    return autocomplete_complete(boolean_choice_ac, prefix, TRUE);
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

    char *result = g_key_file_get_string(prefs, group, key, NULL);

    if (result == NULL) {
        if (def != NULL) {
            return strdup(def);
        } else {
            return NULL;
        }
    } else {
        return result;
    }
}

void
prefs_free_string(char *pref)
{
    if (pref != NULL) {
        free(pref);
    }
    pref = NULL;
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

gint prefs_get_inpblock(void)
{
    int val = g_key_file_get_integer(prefs, PREF_GROUP_UI, "inpblock", NULL);
    if (val == 0) {
        return INPBLOCK_DEFAULT;
    } else {
        return val;
    }
}

void prefs_set_inpblock(gint value)
{
    g_key_file_set_integer(prefs, PREF_GROUP_UI, "inpblock", value);
    _save_prefs();
}

gint
prefs_get_priority(void)
{
    return g_key_file_get_integer(prefs, PREF_GROUP_PRESENCE, "priority", NULL);
}

gint
prefs_get_reconnect(void)
{
    if (!g_key_file_has_key(prefs, PREF_GROUP_CONNECTION, "reconnect", NULL)) {
        return 30;
    } else {
        return g_key_file_get_integer(prefs, PREF_GROUP_CONNECTION, "reconnect", NULL);
    }
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

void
prefs_set_occupants_size(gint value)
{
    g_key_file_set_integer(prefs, PREF_GROUP_UI, "occupants.size", value);
    _save_prefs();
}

gint
prefs_get_occupants_size(void)
{
    gint result = g_key_file_get_integer(prefs, PREF_GROUP_UI, "occupants.size", NULL);

    if (result > 99 || result < 1) {
        return 15;
    } else {
        return result;
    }
}

void
prefs_set_roster_size(gint value)
{
    g_key_file_set_integer(prefs, PREF_GROUP_UI, "roster.size", value);
    _save_prefs();
}

gint
prefs_get_roster_size(void)
{
    gint result = g_key_file_get_integer(prefs, PREF_GROUP_UI, "roster.size", NULL);

    if (result > 99 || result < 1) {
        return 25;
    } else {
        return result;
    }
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

            if (value != NULL) {
                ProfAlias *alias = malloc(sizeof(struct prof_alias_t));
                alias->name = strdup(name);
                alias->value = strdup(value);

                free(value);

                result = g_list_insert_sorted(result, alias, (GCompareFunc)_alias_cmp);
            }
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
    gchar *g_prefs_data = g_key_file_to_data(prefs, &g_data_size, NULL);
    gchar *xdg_config = xdg_get_config_home();
    GString *base_str = g_string_new(xdg_config);
    g_string_append(base_str, "/profanity/");
    gchar *true_loc = get_file_or_linked(prefs_loc, base_str->str);
    g_file_set_contents(true_loc, g_prefs_data, g_data_size, NULL);
    g_chmod(prefs_loc, S_IRUSR | S_IWUSR);
    g_free(xdg_config);
    free(true_loc);
    g_free(g_prefs_data);
    g_string_free(base_str, TRUE);
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

// get the preference group for a specific preference
// for example the PREF_BEEP setting ("beep" in .profrc, see _get_key) belongs
// to the [ui] section.
static const char *
_get_group(preference_t pref)
{
    switch (pref)
    {
        case PREF_SPLASH:
        case PREF_BEEP:
        case PREF_THEME:
        case PREF_VERCHECK:
        case PREF_TITLEBAR_SHOW:
        case PREF_TITLEBAR_GOODBYE:
        case PREF_FLASH:
        case PREF_INTYPE:
        case PREF_HISTORY:
        case PREF_MOUSE:
        case PREF_OCCUPANTS:
        case PREF_STATUSES:
        case PREF_STATUSES_CONSOLE:
        case PREF_STATUSES_CHAT:
        case PREF_STATUSES_MUC:
        case PREF_MUC_PRIVILEGES:
        case PREF_PRESENCE:
        case PREF_WRAP:
        case PREF_TIME:
        case PREF_TIME_STATUSBAR:
        case PREF_ROSTER:
        case PREF_ROSTER_OFFLINE:
        case PREF_ROSTER_RESOURCE:
        case PREF_ROSTER_BY:
        case PREF_RESOURCE_TITLE:
        case PREF_RESOURCE_MESSAGE:
        case PREF_OTR_WARN:
        case PREF_INPBLOCK_DYNAMIC:
            return PREF_GROUP_UI;
        case PREF_STATES:
        case PREF_OUTTYPE:
            return PREF_GROUP_CHATSTATES;
        case PREF_NOTIFY_TYPING:
        case PREF_NOTIFY_TYPING_CURRENT:
        case PREF_NOTIFY_MESSAGE:
        case PREF_NOTIFY_MESSAGE_CURRENT:
        case PREF_NOTIFY_MESSAGE_TEXT:
        case PREF_NOTIFY_ROOM:
        case PREF_NOTIFY_ROOM_CURRENT:
        case PREF_NOTIFY_ROOM_TEXT:
        case PREF_NOTIFY_INVITE:
        case PREF_NOTIFY_SUB:
            return PREF_GROUP_NOTIFICATIONS;
        case PREF_CHLOG:
        case PREF_GRLOG:
        case PREF_LOG_ROTATE:
        case PREF_LOG_SHARED:
            return PREF_GROUP_LOGGING;
        case PREF_AUTOAWAY_CHECK:
        case PREF_AUTOAWAY_MODE:
        case PREF_AUTOAWAY_MESSAGE:
            return PREF_GROUP_PRESENCE;
        case PREF_CONNECT_ACCOUNT:
        case PREF_DEFAULT_ACCOUNT:
        case PREF_CARBONS:
            return PREF_GROUP_CONNECTION;
        case PREF_OTR_LOG:
        case PREF_OTR_POLICY:
            return PREF_GROUP_OTR;
        default:
            return NULL;
    }
}

// get the key used in .profrc for the preference
// for example the PREF_AUTOAWAY_MODE maps to "autoaway.mode" in .profrc
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
        case PREF_TITLEBAR_SHOW:
            return "titlebar.show";
        case PREF_TITLEBAR_GOODBYE:
            return "titlebar.goodbye";
        case PREF_FLASH:
            return "flash";
        case PREF_INTYPE:
            return "intype";
        case PREF_HISTORY:
            return "history";
        case PREF_CARBONS:
            return "carbons";
        case PREF_MOUSE:
            return "mouse";
        case PREF_OCCUPANTS:
            return "occupants";
        case PREF_MUC_PRIVILEGES:
            return "privileges";
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
        case PREF_NOTIFY_TYPING_CURRENT:
            return "typing.current";
        case PREF_NOTIFY_MESSAGE:
            return "message";
        case PREF_NOTIFY_MESSAGE_CURRENT:
            return "message.current";
        case PREF_NOTIFY_MESSAGE_TEXT:
            return "message.text";
        case PREF_NOTIFY_ROOM:
            return "room";
        case PREF_NOTIFY_ROOM_CURRENT:
            return "room.current";
        case PREF_NOTIFY_ROOM_TEXT:
            return "room.text";
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
        case PREF_DEFAULT_ACCOUNT:
            return "defaccount";
        case PREF_OTR_LOG:
            return "log";
        case PREF_OTR_WARN:
            return "otr.warn";
        case PREF_OTR_POLICY:
            return "policy";
        case PREF_LOG_ROTATE:
            return "rotate";
        case PREF_LOG_SHARED:
            return "shared";
        case PREF_PRESENCE:
            return "presence";
        case PREF_WRAP:
            return "wrap";
        case PREF_TIME:
            return "time";
        case PREF_TIME_STATUSBAR:
            return "time.statusbar";
        case PREF_ROSTER:
            return "roster";
        case PREF_ROSTER_OFFLINE:
            return "roster.offline";
        case PREF_ROSTER_RESOURCE:
            return "roster.resource";
        case PREF_ROSTER_BY:
            return "roster.by";
        case PREF_RESOURCE_TITLE:
            return "resource.title";
        case PREF_RESOURCE_MESSAGE:
            return "resource.message";
        case PREF_INPBLOCK_DYNAMIC:
            return "inpblock.dynamic";
        default:
            return NULL;
    }
}

// the default setting for a boolean type preference
// if it is not specified in .profrc
static gboolean
_get_default_boolean(preference_t pref)
{
    switch (pref)
    {
        case PREF_OTR_WARN:
        case PREF_AUTOAWAY_CHECK:
        case PREF_LOG_ROTATE:
        case PREF_LOG_SHARED:
        case PREF_NOTIFY_MESSAGE:
        case PREF_NOTIFY_MESSAGE_CURRENT:
        case PREF_NOTIFY_ROOM_CURRENT:
        case PREF_NOTIFY_TYPING:
        case PREF_NOTIFY_TYPING_CURRENT:
        case PREF_NOTIFY_SUB:
        case PREF_NOTIFY_INVITE:
        case PREF_SPLASH:
        case PREF_OCCUPANTS:
        case PREF_MUC_PRIVILEGES:
        case PREF_PRESENCE:
        case PREF_WRAP:
        case PREF_INPBLOCK_DYNAMIC:
        case PREF_RESOURCE_TITLE:
        case PREF_RESOURCE_MESSAGE:
        case PREF_ROSTER:
        case PREF_ROSTER_OFFLINE:
        case PREF_ROSTER_RESOURCE:
            return TRUE;
        default:
            return FALSE;
    }
}

// the default setting for a string type preference
// if it is not specified in .profrc
static char *
_get_default_string(preference_t pref)
{
    switch (pref)
    {
        case PREF_AUTOAWAY_MODE:
        case PREF_NOTIFY_ROOM:
            return "on";
        case PREF_OTR_LOG:
            return "redact";
        case PREF_OTR_POLICY:
            return "manual";
        case PREF_STATUSES_CONSOLE:
        case PREF_STATUSES_CHAT:
        case PREF_STATUSES_MUC:
            return "all";
        case PREF_ROSTER_BY:
            return "presence";
        case PREF_TIME:
            return "seconds";
        case PREF_TIME_STATUSBAR:
            return "minutes";
        default:
            return NULL;
    }
}
