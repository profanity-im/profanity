/*
 * preferences.c
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

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <glib.h>
#include <glib/gstdio.h>

#include "common.h"
#include "log.h"
#include "preferences.h"
#include "tools/autocomplete.h"
#include "config/files.h"
#include "config/conflists.h"

// preference groups refer to the sections in .profrc, for example [ui]
#define PREF_GROUP_LOGGING "logging"
#define PREF_GROUP_CHATSTATES "chatstates"
#define PREF_GROUP_UI "ui"
#define PREF_GROUP_NOTIFICATIONS "notifications"
#define PREF_GROUP_PRESENCE "presence"
#define PREF_GROUP_CONNECTION "connection"
#define PREF_GROUP_ALIAS "alias"
#define PREF_GROUP_OTR "otr"
#define PREF_GROUP_PGP "pgp"
#define PREF_GROUP_MUC "muc"
#define PREF_GROUP_PLUGINS "plugins"

#define INPBLOCK_DEFAULT 1000

static char *prefs_loc;
static GKeyFile *prefs;
gint log_maxsize = 0;

static Autocomplete boolean_choice_ac;
static Autocomplete room_trigger_ac;

static void _save_prefs(void);
static const char* _get_group(preference_t pref);
static const char* _get_key(preference_t pref);
static gboolean _get_default_boolean(preference_t pref);
static char* _get_default_string(preference_t pref);

void
prefs_load(void)
{
    GError *err;
    prefs_loc = files_get_config_path(FILE_PROFRC);

    if (g_file_test(prefs_loc, G_FILE_TEST_EXISTS)) {
        g_chmod(prefs_loc, S_IRUSR | S_IWUSR);
    }

    prefs = g_key_file_new();
    g_key_file_load_from_file(prefs, prefs_loc, G_KEY_FILE_KEEP_COMMENTS, NULL);

    err = NULL;
    log_maxsize = g_key_file_get_integer(prefs, PREF_GROUP_LOGGING, "maxsize", &err);
    if (err) {
        log_maxsize = 0;
        g_error_free(err);
    }

    // move pre 0.5.0 autoaway.time to autoaway.awaytime
    if (g_key_file_has_key(prefs, PREF_GROUP_PRESENCE, "autoaway.time", NULL)) {
        gint time = g_key_file_get_integer(prefs, PREF_GROUP_PRESENCE, "autoaway.time", NULL);
        g_key_file_set_integer(prefs, PREF_GROUP_PRESENCE, "autoaway.awaytime", time);
        g_key_file_remove_key(prefs, PREF_GROUP_PRESENCE, "autoaway.time", NULL);
    }

    // move pre 0.5.0 autoaway.message to autoaway.awaymessage
    if (g_key_file_has_key(prefs, PREF_GROUP_PRESENCE, "autoaway.message", NULL)) {
        char *message = g_key_file_get_string(prefs, PREF_GROUP_PRESENCE, "autoaway.message", NULL);
        g_key_file_set_string(prefs, PREF_GROUP_PRESENCE, "autoaway.awaymessage", message);
        g_key_file_remove_key(prefs, PREF_GROUP_PRESENCE, "autoaway.message", NULL);
        prefs_free_string(message);
    }

    // migrate pre 0.5.0 time settings
    if (g_key_file_has_key(prefs, PREF_GROUP_UI, "time", NULL)) {
        char *time = g_key_file_get_string(prefs, PREF_GROUP_UI, "time", NULL);
        char *val = NULL;
        if (time) {
            val = time;
        } else {
            val = "off";
        }
        g_key_file_set_string(prefs, PREF_GROUP_UI, "time.console", val);
        g_key_file_set_string(prefs, PREF_GROUP_UI, "time.chat", val);
        g_key_file_set_string(prefs, PREF_GROUP_UI, "time.muc", val);
        g_key_file_set_string(prefs, PREF_GROUP_UI, "time.mucconfig", val);
        g_key_file_set_string(prefs, PREF_GROUP_UI, "time.private", val);
        g_key_file_set_string(prefs, PREF_GROUP_UI, "time.xmlconsole", val);
        g_key_file_remove_key(prefs, PREF_GROUP_UI, "time", NULL);
        prefs_free_string(time);
    }

    // move pre 0.5.0 notify settings
    if (g_key_file_has_key(prefs, PREF_GROUP_NOTIFICATIONS, "room", NULL)) {
        char *value = g_key_file_get_string(prefs, PREF_GROUP_NOTIFICATIONS, "room", NULL);
        if (g_strcmp0(value, "on") == 0) {
            g_key_file_set_boolean(prefs, PREF_GROUP_NOTIFICATIONS, "room", TRUE);
        } else if (g_strcmp0(value, "off") == 0) {
            g_key_file_set_boolean(prefs, PREF_GROUP_NOTIFICATIONS, "room", FALSE);
        } else if (g_strcmp0(value, "mention") == 0) {
            g_key_file_set_boolean(prefs, PREF_GROUP_NOTIFICATIONS, "room", FALSE);
            g_key_file_set_boolean(prefs, PREF_GROUP_NOTIFICATIONS, "room.mention", TRUE);
        }
        prefs_free_string(value);
    }

    // move pre 0.6.0 titlebar settings to wintitle
    if (g_key_file_has_key(prefs, PREF_GROUP_UI, "titlebar.show", NULL)) {
        gboolean show = g_key_file_get_boolean(prefs, PREF_GROUP_UI, "titlebar.show", NULL);
        g_key_file_set_boolean(prefs, PREF_GROUP_UI, "wintitle.show", show);
        g_key_file_remove_key(prefs, PREF_GROUP_UI, "titlebar.show", NULL);
    }
    if (g_key_file_has_key(prefs, PREF_GROUP_UI, "titlebar.goodbye", NULL)) {
        gboolean goodbye = g_key_file_get_boolean(prefs, PREF_GROUP_UI, "titlebar.goodbye", NULL);
        g_key_file_set_boolean(prefs, PREF_GROUP_UI, "wintitle.goodbye", goodbye);
        g_key_file_remove_key(prefs, PREF_GROUP_UI, "titlebar.goodbye", NULL);
    }

    _save_prefs();

    boolean_choice_ac = autocomplete_new();
    autocomplete_add(boolean_choice_ac, "on");
    autocomplete_add(boolean_choice_ac, "off");

    room_trigger_ac = autocomplete_new();
    gsize len = 0;
    gchar **triggers = g_key_file_get_string_list(prefs, PREF_GROUP_NOTIFICATIONS, "room.trigger.list", &len, NULL);

    int i = 0;
    for (i = 0; i < len; i++) {
        autocomplete_add(room_trigger_ac, triggers[i]);
    }
    g_strfreev(triggers);
}

void
prefs_close(void)
{
    autocomplete_free(boolean_choice_ac);
    autocomplete_free(room_trigger_ac);
    g_key_file_free(prefs);
    prefs = NULL;
}

char*
prefs_autocomplete_boolean_choice(const char *const prefix, gboolean previous)
{
    return autocomplete_complete(boolean_choice_ac, prefix, TRUE, previous);
}

void
prefs_reset_boolean_choice(void)
{
    autocomplete_reset(boolean_choice_ac);
}

char*
prefs_autocomplete_room_trigger(const char *const prefix, gboolean previous)
{
    return autocomplete_complete(room_trigger_ac, prefix, TRUE, previous);
}

void
prefs_reset_room_trigger_ac(void)
{
    autocomplete_reset(room_trigger_ac);
}

gboolean
prefs_do_chat_notify(gboolean current_win)
{
    if (prefs_get_boolean(PREF_NOTIFY_CHAT) == FALSE) {
        return FALSE;
    } else {
        if ((current_win == FALSE) || ((current_win == TRUE) && prefs_get_boolean(PREF_NOTIFY_CHAT_CURRENT))) {
            return TRUE;
        } else {
            return FALSE;
        }
    }
}

GList*
prefs_message_get_triggers(const char *const message)
{
    GList *result = NULL;

    char *message_lower = g_utf8_strdown(message, -1);
    gsize len = 0;
    gchar **triggers = g_key_file_get_string_list(prefs, PREF_GROUP_NOTIFICATIONS, "room.trigger.list", &len, NULL);
    int i;
    for (i = 0; i < len; i++) {
        char *trigger_lower = g_utf8_strdown(triggers[i], -1);
        if (g_strrstr(message_lower, trigger_lower)) {
            result = g_list_append(result, strdup(triggers[i]));
        }
        g_free(trigger_lower);
    }
    g_strfreev(triggers);
    g_free(message_lower);

    return result;
}

gboolean
prefs_do_room_notify(gboolean current_win, const char *const roomjid, const char *const mynick,
    const char *const theirnick, const char *const message, gboolean mention, gboolean trigger_found)
{
    if (g_strcmp0(mynick, theirnick) == 0) {
        return FALSE;
    }

    gboolean notify_current = prefs_get_boolean(PREF_NOTIFY_ROOM_CURRENT);
    gboolean notify_window = FALSE;
    if (!current_win || (current_win && notify_current) ) {
        notify_window = TRUE;
    }
    if (!notify_window) {
        return FALSE;
    }

    gboolean notify_room = FALSE;
    if (g_key_file_has_key(prefs, roomjid, "notify", NULL)) {
        notify_room = g_key_file_get_boolean(prefs, roomjid, "notify", NULL);
    } else {
        notify_room = prefs_get_boolean(PREF_NOTIFY_ROOM);
    }
    if (notify_room) {
        return TRUE;
    }

    gboolean notify_mention = FALSE;
    if (g_key_file_has_key(prefs, roomjid, "notify.mention", NULL)) {
        notify_mention = g_key_file_get_boolean(prefs, roomjid, "notify.mention", NULL);
    } else {
        notify_mention = prefs_get_boolean(PREF_NOTIFY_ROOM_MENTION);
    }
    if (notify_mention && mention) {
        return TRUE;
    }

    gboolean notify_trigger = FALSE;
    if (g_key_file_has_key(prefs, roomjid, "notify.trigger", NULL)) {
        notify_trigger = g_key_file_get_boolean(prefs, roomjid, "notify.trigger", NULL);
    } else {
        notify_trigger = prefs_get_boolean(PREF_NOTIFY_ROOM_TRIGGER);
    }
    if (notify_trigger && trigger_found) {
        return TRUE;
    }

    return FALSE;
}

gboolean
prefs_do_room_notify_mention(const char *const roomjid, int unread, gboolean mention, gboolean trigger)
{
    gboolean notify_room = FALSE;
    if (g_key_file_has_key(prefs, roomjid, "notify", NULL)) {
        notify_room = g_key_file_get_boolean(prefs, roomjid, "notify", NULL);
    } else {
        notify_room = prefs_get_boolean(PREF_NOTIFY_ROOM);
    }
    if (notify_room && unread > 0) {
        return TRUE;
    }

    gboolean notify_mention = FALSE;
    if (g_key_file_has_key(prefs, roomjid, "notify.mention", NULL)) {
        notify_mention = g_key_file_get_boolean(prefs, roomjid, "notify.mention", NULL);
    } else {
        notify_mention = prefs_get_boolean(PREF_NOTIFY_ROOM_MENTION);
    }
    if (notify_mention && mention) {
        return TRUE;
    }

    gboolean notify_trigger = FALSE;
    if (g_key_file_has_key(prefs, roomjid, "notify.trigger", NULL)) {
        notify_trigger = g_key_file_get_boolean(prefs, roomjid, "notify.trigger", NULL);
    } else {
        notify_trigger = prefs_get_boolean(PREF_NOTIFY_ROOM_TRIGGER);
    }
    if (notify_trigger && trigger) {
        return TRUE;
    }

    return FALSE;
}

void
prefs_set_room_notify(const char *const roomjid, gboolean value)
{
    g_key_file_set_boolean(prefs, roomjid, "notify", value);
    _save_prefs();
}

void
prefs_set_room_notify_mention(const char *const roomjid, gboolean value)
{
    g_key_file_set_boolean(prefs, roomjid, "notify.mention", value);
    _save_prefs();
}

void
prefs_set_room_notify_trigger(const char *const roomjid, gboolean value)
{
    g_key_file_set_boolean(prefs, roomjid, "notify.trigger", value);
    _save_prefs();
}

gboolean
prefs_has_room_notify(const char *const roomjid)
{
    return g_key_file_has_key(prefs, roomjid, "notify", NULL);
}

gboolean
prefs_has_room_notify_mention(const char *const roomjid)
{
    return g_key_file_has_key(prefs, roomjid, "notify.mention", NULL);
}

gboolean
prefs_has_room_notify_trigger(const char *const roomjid)
{
    return g_key_file_has_key(prefs, roomjid, "notify.trigger", NULL);
}

gboolean
prefs_get_room_notify(const char *const roomjid)
{
    return g_key_file_get_boolean(prefs, roomjid, "notify", NULL);
}

gboolean
prefs_get_room_notify_mention(const char *const roomjid)
{
    return g_key_file_get_boolean(prefs, roomjid, "notify.mention", NULL);
}

gboolean
prefs_get_room_notify_trigger(const char *const roomjid)
{
    return g_key_file_get_boolean(prefs, roomjid, "notify.trigger", NULL);
}

gboolean
prefs_reset_room_notify(const char *const roomjid)
{
    if (g_key_file_has_group(prefs, roomjid)) {
        g_key_file_remove_group(prefs, roomjid, NULL);
        _save_prefs();
        return TRUE;
    }

    return FALSE;
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

char*
prefs_get_string(preference_t pref)
{
    const char *group = _get_group(pref);
    const char *key = _get_key(pref);
    char *def = _get_default_string(pref);

    char *result = g_key_file_get_string(prefs, group, key, NULL);

    if (result == NULL) {
        if (def) {
            return g_strdup(def);
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
    if (pref) {
        g_free(pref);
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

char*
prefs_get_tls_certpath(void)
{
    const char *group = _get_group(PREF_TLS_CERTPATH);
    const char *key = _get_key(PREF_TLS_CERTPATH);

    char *setting = g_key_file_get_string(prefs, group, key, NULL);

    if (g_strcmp0(setting, "none") == 0) {
        prefs_free_string(setting);
        return NULL;
    }

    if (setting == NULL) {
        if (g_file_test("/etc/ssl/certs",  G_FILE_TEST_IS_DIR)) {
            return strdup("/etc/ssl/certs");
        }
        if (g_file_test("/etc/pki/tls/certs",  G_FILE_TEST_IS_DIR)) {
            return strdup("/etc/pki/tls/certs");
        }
        if (g_file_test("/etc/ssl",  G_FILE_TEST_IS_DIR)) {
            return strdup("/etc/ssl");
        }
        if (g_file_test("/etc/pki/tls",  G_FILE_TEST_IS_DIR)) {
            return strdup("/etc/pki/tls");
        }
        if (g_file_test("/system/etc/security/cacerts",  G_FILE_TEST_IS_DIR)) {
            return strdup("/system/etc/security/cacerts");
        }

        return NULL;
    }

    char *result = strdup(setting);
    prefs_free_string(setting);

    return result;
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
prefs_get_inpblock(void)
{
    int val = g_key_file_get_integer(prefs, PREF_GROUP_UI, "inpblock", NULL);
    if (val == 0) {
        return INPBLOCK_DEFAULT;
    } else {
        return val;
    }
}

void
prefs_set_inpblock(gint value)
{
    g_key_file_set_integer(prefs, PREF_GROUP_UI, "inpblock", value);
    _save_prefs();
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
    if (!g_key_file_has_key(prefs, PREF_GROUP_CONNECTION, "autoping", NULL)) {
        return 60;
    } else {
        return g_key_file_get_integer(prefs, PREF_GROUP_CONNECTION, "autoping", NULL);
    }
}

void
prefs_set_autoping(gint value)
{
    g_key_file_set_integer(prefs, PREF_GROUP_CONNECTION, "autoping", value);
    _save_prefs();
}

gint
prefs_get_autoping_timeout(void)
{
    if (!g_key_file_has_key(prefs, PREF_GROUP_CONNECTION, "autoping.timeout", NULL)) {
        return 20;
    } else {
        return g_key_file_get_integer(prefs, PREF_GROUP_CONNECTION, "autoping.timeout", NULL);
    }
}

void
prefs_set_autoping_timeout(gint value)
{
    g_key_file_set_integer(prefs, PREF_GROUP_CONNECTION, "autoping.timeout", value);
    _save_prefs();
}

gint
prefs_get_autoaway_time(void)
{
    gint result = g_key_file_get_integer(prefs, PREF_GROUP_PRESENCE, "autoaway.awaytime", NULL);

    if (result == 0) {
        return 15;
    } else {
        return result;
    }
}

gint
prefs_get_autoxa_time(void)
{
    return g_key_file_get_integer(prefs, PREF_GROUP_PRESENCE, "autoaway.xatime", NULL);
}

void
prefs_set_autoaway_time(gint value)
{
    g_key_file_set_integer(prefs, PREF_GROUP_PRESENCE, "autoaway.awaytime", value);
    _save_prefs();
}

void
prefs_set_autoxa_time(gint value)
{
    g_key_file_set_integer(prefs, PREF_GROUP_PRESENCE, "autoaway.xatime", value);
    _save_prefs();
}

void
prefs_set_tray_timer(gint value)
{
    g_key_file_set_integer(prefs, PREF_GROUP_NOTIFICATIONS, "tray.timer", value);
    _save_prefs();
}

gint
prefs_get_tray_timer(void)
{
    gint result = g_key_file_get_integer(prefs, PREF_GROUP_NOTIFICATIONS, "tray.timer", NULL);

    if (result == 0) {
        return 5;
    } else {
        return result;
    }
}

gchar**
prefs_get_plugins(void)
{
    if (!g_key_file_has_group(prefs, PREF_GROUP_PLUGINS)) {
        return NULL;
    }
    if (!g_key_file_has_key(prefs, PREF_GROUP_PLUGINS, "load", NULL)) {
        return NULL;
    }

    return g_key_file_get_string_list(prefs, PREF_GROUP_PLUGINS, "load", NULL, NULL);
}

void
prefs_add_plugin(const char *const name)
{
    conf_string_list_add(prefs, PREF_GROUP_PLUGINS, "load", name);
    _save_prefs();
}

void
prefs_remove_plugin(const char *const name)
{
    conf_string_list_remove(prefs, PREF_GROUP_PLUGINS, "load", name);
    _save_prefs();
}

void
prefs_free_plugins(gchar **plugins)
{
    g_strfreev(plugins);
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

char
prefs_get_otr_char(void)
{
    char result = '~';

    char *resultstr = g_key_file_get_string(prefs, PREF_GROUP_OTR, "otr.char", NULL);
    if (!resultstr) {
        result =  '~';
    } else {
        result = resultstr[0];
    }
    free(resultstr);

    return result;
}

void
prefs_set_otr_char(char ch)
{
    char str[2];
    str[0] = ch;
    str[1] = '\0';

    g_key_file_set_string(prefs, PREF_GROUP_OTR, "otr.char", str);
    _save_prefs();
}

char
prefs_get_pgp_char(void)
{
    char result = '~';

    char *resultstr = g_key_file_get_string(prefs, PREF_GROUP_PGP, "pgp.char", NULL);
    if (!resultstr) {
        result =  '~';
    } else {
        result = resultstr[0];
    }
    free(resultstr);

    return result;
}

void
prefs_set_pgp_char(char ch)
{
    char str[2];
    str[0] = ch;
    str[1] = '\0';

    g_key_file_set_string(prefs, PREF_GROUP_PGP, "pgp.char", str);
    _save_prefs();
}

char
prefs_get_roster_header_char(void)
{
    char result = 0;

    char *resultstr = g_key_file_get_string(prefs, PREF_GROUP_UI, "roster.header.char", NULL);
    if (!resultstr) {
        result =  0;
    } else {
        result = resultstr[0];
    }
    free(resultstr);

    return result;
}

void
prefs_set_roster_header_char(char ch)
{
    char str[2];
    str[0] = ch;
    str[1] = '\0';

    g_key_file_set_string(prefs, PREF_GROUP_UI, "roster.header.char", str);
    _save_prefs();
}

void
prefs_clear_roster_header_char(void)
{
    g_key_file_remove_key(prefs, PREF_GROUP_UI, "roster.header.char", NULL);
    _save_prefs();
}

char
prefs_get_roster_contact_char(void)
{
    char result = 0;

    char *resultstr = g_key_file_get_string(prefs, PREF_GROUP_UI, "roster.contact.char", NULL);
    if (!resultstr) {
        result =  0;
    } else {
        result = resultstr[0];
    }
    free(resultstr);

    return result;
}

void
prefs_set_roster_contact_char(char ch)
{
    char str[2];
    str[0] = ch;
    str[1] = '\0';

    g_key_file_set_string(prefs, PREF_GROUP_UI, "roster.contact.char", str);
    _save_prefs();
}

void
prefs_clear_roster_contact_char(void)
{
    g_key_file_remove_key(prefs, PREF_GROUP_UI, "roster.contact.char", NULL);
    _save_prefs();
}

char
prefs_get_roster_resource_char(void)
{
    char result = 0;

    char *resultstr = g_key_file_get_string(prefs, PREF_GROUP_UI, "roster.resource.char", NULL);
    if (!resultstr) {
        result =  0;
    } else {
        result = resultstr[0];
    }
    free(resultstr);

    return result;
}

void
prefs_set_roster_resource_char(char ch)
{
    char str[2];
    str[0] = ch;
    str[1] = '\0';

    g_key_file_set_string(prefs, PREF_GROUP_UI, "roster.resource.char", str);
    _save_prefs();
}

void
prefs_clear_roster_resource_char(void)
{
    g_key_file_remove_key(prefs, PREF_GROUP_UI, "roster.resource.char", NULL);
    _save_prefs();
}

char
prefs_get_roster_private_char(void)
{
    char result = 0;

    char *resultstr = g_key_file_get_string(prefs, PREF_GROUP_UI, "roster.private.char", NULL);
    if (!resultstr) {
        result =  0;
    } else {
        result = resultstr[0];
    }
    free(resultstr);

    return result;
}

void
prefs_set_roster_private_char(char ch)
{
    char str[2];
    str[0] = ch;
    str[1] = '\0';

    g_key_file_set_string(prefs, PREF_GROUP_UI, "roster.private.char", str);
    _save_prefs();
}

void
prefs_clear_roster_private_char(void)
{
    g_key_file_remove_key(prefs, PREF_GROUP_UI, "roster.private.char", NULL);
    _save_prefs();
}

char
prefs_get_roster_room_char(void)
{
    char result = 0;

    char *resultstr = g_key_file_get_string(prefs, PREF_GROUP_UI, "roster.rooms.char", NULL);
    if (!resultstr) {
        result =  0;
    } else {
        result = resultstr[0];
    }
    free(resultstr);

    return result;
}

void
prefs_set_roster_room_char(char ch)
{
    char str[2];
    str[0] = ch;
    str[1] = '\0';

    g_key_file_set_string(prefs, PREF_GROUP_UI, "roster.rooms.char", str);
    _save_prefs();
}

void
prefs_clear_roster_room_char(void)
{
    g_key_file_remove_key(prefs, PREF_GROUP_UI, "roster.rooms.char", NULL);
    _save_prefs();
}

char
prefs_get_roster_room_private_char(void)
{
    char result = 0;

    char *resultstr = g_key_file_get_string(prefs, PREF_GROUP_UI, "roster.rooms.private.char", NULL);
    if (!resultstr) {
        result =  0;
    } else {
        result = resultstr[0];
    }
    free(resultstr);

    return result;
}

void
prefs_set_roster_room_private_char(char ch)
{
    char str[2];
    str[0] = ch;
    str[1] = '\0';

    g_key_file_set_string(prefs, PREF_GROUP_UI, "roster.rooms.private.char", str);
    _save_prefs();
}

void
prefs_clear_roster_room_private_char(void)
{
    g_key_file_remove_key(prefs, PREF_GROUP_UI, "roster.rooms.pruvate.char", NULL);
    _save_prefs();
}

gint
prefs_get_roster_contact_indent(void)
{
    if (!g_key_file_has_key(prefs, PREF_GROUP_UI, "roster.contact.indent", NULL)) {
        return 2;
    }

    gint result = g_key_file_get_integer(prefs, PREF_GROUP_UI, "roster.contact.indent", NULL);
    if (result < 0) {
        result = 0;
    }

    return result;
}

void
prefs_set_roster_contact_indent(gint value)
{
    g_key_file_set_integer(prefs, PREF_GROUP_UI, "roster.contact.indent", value);
    _save_prefs();
}

gint
prefs_get_roster_resource_indent(void)
{
    if (!g_key_file_has_key(prefs, PREF_GROUP_UI, "roster.resource.indent", NULL)) {
        return 2;
    }

    gint result = g_key_file_get_integer(prefs, PREF_GROUP_UI, "roster.resource.indent", NULL);
    if (result < 0) {
        result = 0;
    }

    return result;
}

void
prefs_set_roster_resource_indent(gint value)
{
    g_key_file_set_integer(prefs, PREF_GROUP_UI, "roster.resource.indent", value);
    _save_prefs();
}

gint
prefs_get_roster_presence_indent(void)
{
    if (!g_key_file_has_key(prefs, PREF_GROUP_UI, "roster.presence.indent", NULL)) {
        return 2;
    }

    gint result = g_key_file_get_integer(prefs, PREF_GROUP_UI, "roster.presence.indent", NULL);
    if (result < -1) {
        result = 0;
    }

    return result;
}

void
prefs_set_roster_presence_indent(gint value)
{
    g_key_file_set_integer(prefs, PREF_GROUP_UI, "roster.presence.indent", value);
    _save_prefs();
}

gboolean
prefs_add_room_notify_trigger(const char * const text)
{
    gboolean res = conf_string_list_add(prefs, PREF_GROUP_NOTIFICATIONS, "room.trigger.list", text);
    _save_prefs();

    if (res) {
        autocomplete_add(room_trigger_ac, text);
    }

    return res;
}

gboolean
prefs_remove_room_notify_trigger(const char * const text)
{
    gboolean res = conf_string_list_remove(prefs, PREF_GROUP_NOTIFICATIONS, "room.trigger.list", text);
    _save_prefs();

    if (res) {
        autocomplete_remove(room_trigger_ac, text);
    }

    return res;
}

GList*
prefs_get_room_notify_triggers(void)
{
    GList *result = NULL;
    gsize len = 0;
    gchar **triggers = g_key_file_get_string_list(prefs, PREF_GROUP_NOTIFICATIONS, "room.trigger.list", &len, NULL);

    int i;
    for (i = 0; i < len; i++) {
        result = g_list_append(result, strdup(triggers[i]));
    }

    g_strfreev(triggers);

    return result;
}

ProfWinPlacement*
prefs_create_profwin_placement(int titlebar, int mainwin, int statusbar, int inputwin)
{
    ProfWinPlacement *placement = malloc(sizeof(ProfWinPlacement));
    placement->titlebar_pos = titlebar;
    placement->mainwin_pos = mainwin;
    placement->statusbar_pos = statusbar;
    placement->inputwin_pos = inputwin;

    return placement;
}

void
prefs_free_win_placement(ProfWinPlacement *placement)
{
    free(placement);
}

ProfWinPlacement*
prefs_get_win_placement(void)
{
    // read from settings file
    int titlebar_pos = g_key_file_get_integer(prefs, PREF_GROUP_UI, "titlebar.position", NULL);
    int mainwin_pos = g_key_file_get_integer(prefs, PREF_GROUP_UI, "mainwin.position", NULL);
    int statusbar_pos = g_key_file_get_integer(prefs, PREF_GROUP_UI, "statusbar.position", NULL);
    int inputwin_pos = g_key_file_get_integer(prefs, PREF_GROUP_UI, "inputwin.position", NULL);

    // default if setting invalid, or not present
    if (titlebar_pos < 1 || titlebar_pos > 4) {
        titlebar_pos = 1;
    }
    if (mainwin_pos < 1 || mainwin_pos > 4) {
        mainwin_pos = 2;
    }
    if (statusbar_pos < 1 || statusbar_pos > 4) {
        statusbar_pos = 3;
    }
    if (inputwin_pos < 1 || inputwin_pos > 4) {
        inputwin_pos = 4;
    }

    // return default if duplicates found
    if (titlebar_pos == mainwin_pos) {
        return prefs_create_profwin_placement(1, 2, 3, 4);
    }
    if (titlebar_pos == statusbar_pos) {
        return prefs_create_profwin_placement(1, 2, 3, 4);
    }
    if (titlebar_pos == inputwin_pos) {
        return prefs_create_profwin_placement(1, 2, 3, 4);
    }

    if (mainwin_pos == statusbar_pos) {
        return prefs_create_profwin_placement(1, 2, 3, 4);
    }
    if (mainwin_pos == inputwin_pos) {
        return prefs_create_profwin_placement(1, 2, 3, 4);
    }

    if (statusbar_pos == inputwin_pos) {
        return prefs_create_profwin_placement(1, 2, 3, 4);
    }

    // return settings
    return prefs_create_profwin_placement(titlebar_pos, mainwin_pos, statusbar_pos, inputwin_pos);
}

void
prefs_save_win_placement(ProfWinPlacement *placement)
{
    g_key_file_set_integer(prefs, PREF_GROUP_UI, "titlebar.position", placement->titlebar_pos);
    g_key_file_set_integer(prefs, PREF_GROUP_UI, "mainwin.position", placement->mainwin_pos);
    g_key_file_set_integer(prefs, PREF_GROUP_UI, "statusbar.position", placement->statusbar_pos);
    g_key_file_set_integer(prefs, PREF_GROUP_UI, "inputwin.position", placement->inputwin_pos);
    _save_prefs();
}

gboolean
prefs_titlebar_pos_up(void)
{
    ProfWinPlacement *placement = prefs_get_win_placement();

    int pos = 2;
    for (pos = 2; pos<5; pos++) {
        if (placement->titlebar_pos == pos) {
            placement->titlebar_pos = pos-1;

            if (placement->mainwin_pos == pos-1) {
                placement->mainwin_pos = pos;
            } else if (placement->statusbar_pos == pos-1) {
                placement->statusbar_pos = pos;
            } else if (placement->inputwin_pos == pos-1) {
                placement->inputwin_pos = pos;
            }

            prefs_save_win_placement(placement);
            prefs_free_win_placement(placement);
            return TRUE;
        }
    }

    prefs_free_win_placement(placement);
    return FALSE;
}

gboolean
prefs_mainwin_pos_up(void)
{
    ProfWinPlacement *placement = prefs_get_win_placement();

    int pos = 2;
    for (pos = 2; pos<5; pos++) {
        if (placement->mainwin_pos == pos) {
            placement->mainwin_pos = pos-1;

            if (placement->titlebar_pos == pos-1) {
                placement->titlebar_pos = pos;
            } else if (placement->statusbar_pos == pos-1) {
                placement->statusbar_pos = pos;
            } else if (placement->inputwin_pos == pos-1) {
                placement->inputwin_pos = pos;
            }

            prefs_save_win_placement(placement);
            prefs_free_win_placement(placement);
            return TRUE;
        }
    }

    prefs_free_win_placement(placement);
    return FALSE;
}

gboolean
prefs_statusbar_pos_up(void)
{
    ProfWinPlacement *placement = prefs_get_win_placement();

    int pos = 2;
    for (pos = 2; pos<5; pos++) {
        if (placement->statusbar_pos == pos) {
            placement->statusbar_pos = pos-1;

            if (placement->titlebar_pos == pos-1) {
                placement->titlebar_pos = pos;
            } else if (placement->mainwin_pos == pos-1) {
                placement->mainwin_pos = pos;
            } else if (placement->inputwin_pos == pos-1) {
                placement->inputwin_pos = pos;
            }

            prefs_save_win_placement(placement);
            prefs_free_win_placement(placement);
            return TRUE;
        }
    }

    prefs_free_win_placement(placement);
    return FALSE;
}

gboolean
prefs_inputwin_pos_up(void)
{
    ProfWinPlacement *placement = prefs_get_win_placement();

    int pos = 2;
    for (pos = 2; pos<5; pos++) {
        if (placement->inputwin_pos == pos) {
            placement->inputwin_pos = pos-1;

            if (placement->titlebar_pos == pos-1) {
                placement->titlebar_pos = pos;
            } else if (placement->mainwin_pos == pos-1) {
                placement->mainwin_pos = pos;
            } else if (placement->statusbar_pos == pos-1) {
                placement->statusbar_pos = pos;
            }

            prefs_save_win_placement(placement);
            prefs_free_win_placement(placement);
            return TRUE;
        }
    }

    prefs_free_win_placement(placement);
    return FALSE;
}

gboolean
prefs_titlebar_pos_down(void)
{
    ProfWinPlacement *placement = prefs_get_win_placement();

    int pos = 1;
    for (pos = 1; pos<4; pos++) {
        if (placement->titlebar_pos == pos) {
            placement->titlebar_pos = pos+1;

            if (placement->mainwin_pos == pos+1) {
                placement->mainwin_pos = pos;
            } else if (placement->statusbar_pos == pos+1) {
                placement->statusbar_pos = pos;
            } else if (placement->inputwin_pos == pos+1) {
                placement->inputwin_pos = pos;
            }

            prefs_save_win_placement(placement);
            prefs_free_win_placement(placement);
            return TRUE;
        }
    }

    prefs_free_win_placement(placement);
    return FALSE;
}

gboolean
prefs_mainwin_pos_down(void)
{
    ProfWinPlacement *placement = prefs_get_win_placement();

    int pos = 1;
    for (pos = 1; pos<4; pos++) {
        if (placement->mainwin_pos == pos) {
            placement->mainwin_pos = pos+1;

            if (placement->titlebar_pos == pos+1) {
                placement->titlebar_pos = pos;
            } else if (placement->statusbar_pos == pos+1) {
                placement->statusbar_pos = pos;
            } else if (placement->inputwin_pos == pos+1) {
                placement->inputwin_pos = pos;
            }

            prefs_save_win_placement(placement);
            prefs_free_win_placement(placement);
            return TRUE;
        }
    }

    prefs_free_win_placement(placement);
    return FALSE;
}

gboolean
prefs_statusbar_pos_down(void)
{
    ProfWinPlacement *placement = prefs_get_win_placement();

    int pos = 1;
    for (pos = 1; pos<4; pos++) {
        if (placement->statusbar_pos == pos) {
            placement->statusbar_pos = pos+1;

            if (placement->titlebar_pos == pos+1) {
                placement->titlebar_pos = pos;
            } else if (placement->mainwin_pos == pos+1) {
                placement->mainwin_pos = pos;
            } else if (placement->inputwin_pos == pos+1) {
                placement->inputwin_pos = pos;
            }

            prefs_save_win_placement(placement);
            prefs_free_win_placement(placement);
            return TRUE;
        }
    }

    prefs_free_win_placement(placement);
    return FALSE;
}


gboolean
prefs_inputwin_pos_down(void)
{
    ProfWinPlacement *placement = prefs_get_win_placement();

    int pos = 1;
    for (pos = 1; pos<4; pos++) {
        if (placement->inputwin_pos == pos) {
            placement->inputwin_pos = pos+1;

            if (placement->titlebar_pos == pos+1) {
                placement->titlebar_pos = pos;
            } else if (placement->mainwin_pos == pos+1) {
                placement->mainwin_pos = pos;
            } else if (placement->statusbar_pos == pos+1) {
                placement->statusbar_pos = pos;
            }

            prefs_save_win_placement(placement);
            prefs_free_win_placement(placement);
            return TRUE;
        }
    }

    prefs_free_win_placement(placement);
    return FALSE;
}

gboolean
prefs_add_alias(const char *const name, const char *const value)
{
    if (g_key_file_has_key(prefs, PREF_GROUP_ALIAS, name, NULL)) {
        return FALSE;
    } else {
        g_key_file_set_string(prefs, PREF_GROUP_ALIAS, name, value);
        _save_prefs();
        return TRUE;
    }
}

char*
prefs_get_alias(const char *const name)
{
    return g_key_file_get_string(prefs, PREF_GROUP_ALIAS, name, NULL);

}

gboolean
prefs_remove_alias(const char *const name)
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

GList*
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

            if (value) {
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
    gchar *base = g_path_get_basename(prefs_loc);
    gchar *true_loc = get_file_or_linked(prefs_loc, base);

    g_file_set_contents(true_loc, g_prefs_data, g_data_size, NULL);
    g_chmod(prefs_loc, S_IRUSR | S_IWUSR);

    g_free(base);
    free(true_loc);
    g_free(g_prefs_data);
}

// get the preference group for a specific preference
// for example the PREF_BEEP setting ("beep" in .profrc, see _get_key) belongs
// to the [ui] section.
static const char*
_get_group(preference_t pref)
{
    switch (pref)
    {
        case PREF_SPLASH:
        case PREF_BEEP:
        case PREF_THEME:
        case PREF_VERCHECK:
        case PREF_WINTITLE_SHOW:
        case PREF_WINTITLE_GOODBYE:
        case PREF_FLASH:
        case PREF_INTYPE:
        case PREF_HISTORY:
        case PREF_OCCUPANTS:
        case PREF_OCCUPANTS_JID:
        case PREF_STATUSES:
        case PREF_STATUSES_CONSOLE:
        case PREF_STATUSES_CHAT:
        case PREF_STATUSES_MUC:
        case PREF_MUC_PRIVILEGES:
        case PREF_PRESENCE:
        case PREF_WRAP:
        case PREF_WINS_AUTO_TIDY:
        case PREF_TIME_CONSOLE:
        case PREF_TIME_CHAT:
        case PREF_TIME_MUC:
        case PREF_TIME_MUCCONFIG:
        case PREF_TIME_PRIVATE:
        case PREF_TIME_XMLCONSOLE:
        case PREF_TIME_STATUSBAR:
        case PREF_TIME_LASTACTIVITY:
        case PREF_ROSTER:
        case PREF_ROSTER_OFFLINE:
        case PREF_ROSTER_RESOURCE:
        case PREF_ROSTER_PRESENCE:
        case PREF_ROSTER_STATUS:
        case PREF_ROSTER_EMPTY:
        case PREF_ROSTER_BY:
        case PREF_ROSTER_ORDER:
        case PREF_ROSTER_UNREAD:
        case PREF_ROSTER_COUNT:
        case PREF_ROSTER_COUNT_ZERO:
        case PREF_ROSTER_PRIORITY:
        case PREF_ROSTER_WRAP:
        case PREF_ROSTER_RESOURCE_JOIN:
        case PREF_ROSTER_CONTACTS:
        case PREF_ROSTER_UNSUBSCRIBED:
        case PREF_ROSTER_ROOMS:
        case PREF_ROSTER_ROOMS_POS:
        case PREF_ROSTER_ROOMS_BY:
        case PREF_ROSTER_ROOMS_ORDER:
        case PREF_ROSTER_ROOMS_UNREAD:
        case PREF_ROSTER_PRIVATE:
        case PREF_RESOURCE_TITLE:
        case PREF_RESOURCE_MESSAGE:
        case PREF_ENC_WARN:
        case PREF_INPBLOCK_DYNAMIC:
        case PREF_TLS_SHOW:
        case PREF_CONSOLE_MUC:
        case PREF_CONSOLE_PRIVATE:
        case PREF_CONSOLE_CHAT:
            return PREF_GROUP_UI;
        case PREF_STATES:
        case PREF_OUTTYPE:
            return PREF_GROUP_CHATSTATES;
        case PREF_NOTIFY_TYPING:
        case PREF_NOTIFY_TYPING_CURRENT:
        case PREF_NOTIFY_CHAT:
        case PREF_NOTIFY_CHAT_CURRENT:
        case PREF_NOTIFY_CHAT_TEXT:
        case PREF_NOTIFY_ROOM:
        case PREF_NOTIFY_ROOM_MENTION:
        case PREF_NOTIFY_ROOM_TRIGGER:
        case PREF_NOTIFY_ROOM_CURRENT:
        case PREF_NOTIFY_ROOM_TEXT:
        case PREF_NOTIFY_INVITE:
        case PREF_NOTIFY_SUB:
        case PREF_NOTIFY_MENTION_CASE_SENSITIVE:
        case PREF_NOTIFY_MENTION_WHOLE_WORD:
        case PREF_TRAY:
        case PREF_TRAY_READ:
            return PREF_GROUP_NOTIFICATIONS;
        case PREF_CHLOG:
        case PREF_GRLOG:
        case PREF_LOG_ROTATE:
        case PREF_LOG_SHARED:
            return PREF_GROUP_LOGGING;
        case PREF_AUTOAWAY_CHECK:
        case PREF_AUTOAWAY_MODE:
        case PREF_AUTOAWAY_MESSAGE:
        case PREF_AUTOXA_MESSAGE:
        case PREF_LASTACTIVITY:
            return PREF_GROUP_PRESENCE;
        case PREF_CONNECT_ACCOUNT:
        case PREF_DEFAULT_ACCOUNT:
        case PREF_CARBONS:
        case PREF_RECEIPTS_SEND:
        case PREF_RECEIPTS_REQUEST:
        case PREF_TLS_CERTPATH:
            return PREF_GROUP_CONNECTION;
        case PREF_OTR_LOG:
        case PREF_OTR_POLICY:
            return PREF_GROUP_OTR;
        case PREF_PGP_LOG:
            return PREF_GROUP_PGP;
        case PREF_BOOKMARK_INVITE:
            return PREF_GROUP_MUC;
        case PREF_PLUGINS_SOURCEPATH:
            return PREF_GROUP_PLUGINS;
        default:
            return NULL;
    }
}

// get the key used in .profrc for the preference
// for example the PREF_AUTOAWAY_MODE maps to "autoaway.mode" in .profrc
static const char*
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
        case PREF_WINTITLE_SHOW:
            return "wintitle.show";
        case PREF_WINTITLE_GOODBYE:
            return "wintitle.goodbye";
        case PREF_FLASH:
            return "flash";
        case PREF_TRAY:
            return "tray";
        case PREF_TRAY_READ:
            return "tray.read";
        case PREF_INTYPE:
            return "intype";
        case PREF_HISTORY:
            return "history";
        case PREF_CARBONS:
            return "carbons";
        case PREF_RECEIPTS_SEND:
            return "receipts.send";
        case PREF_RECEIPTS_REQUEST:
            return "receipts.request";
        case PREF_OCCUPANTS:
            return "occupants";
        case PREF_OCCUPANTS_JID:
            return "occupants.jid";
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
        case PREF_NOTIFY_CHAT:
            return "message";
        case PREF_NOTIFY_CHAT_CURRENT:
            return "message.current";
        case PREF_NOTIFY_CHAT_TEXT:
            return "message.text";
        case PREF_NOTIFY_ROOM:
            return "room";
        case PREF_NOTIFY_ROOM_TRIGGER:
            return "room.trigger";
        case PREF_NOTIFY_ROOM_MENTION:
            return "room.mention";
        case PREF_NOTIFY_ROOM_CURRENT:
            return "room.current";
        case PREF_NOTIFY_ROOM_TEXT:
            return "room.text";
        case PREF_NOTIFY_INVITE:
            return "invite";
        case PREF_NOTIFY_SUB:
            return "sub";
        case PREF_NOTIFY_MENTION_CASE_SENSITIVE:
            return "room.mention.casesensitive";
        case PREF_NOTIFY_MENTION_WHOLE_WORD:
            return "room.mention.wholeword";
        case PREF_CHLOG:
            return "chlog";
        case PREF_GRLOG:
            return "grlog";
        case PREF_AUTOAWAY_CHECK:
            return "autoaway.check";
        case PREF_AUTOAWAY_MODE:
            return "autoaway.mode";
        case PREF_AUTOAWAY_MESSAGE:
            return "autoaway.awaymessage";
        case PREF_AUTOXA_MESSAGE:
            return "autoaway.xamessage";
        case PREF_CONNECT_ACCOUNT:
            return "account";
        case PREF_DEFAULT_ACCOUNT:
            return "defaccount";
        case PREF_OTR_LOG:
            return "log";
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
        case PREF_WINS_AUTO_TIDY:
            return "wins.autotidy";
        case PREF_TIME_CONSOLE:
            return "time.console";
        case PREF_TIME_CHAT:
            return "time.chat";
        case PREF_TIME_MUC:
            return "time.muc";
        case PREF_TIME_MUCCONFIG:
            return "time.mucconfig";
        case PREF_TIME_PRIVATE:
            return "time.private";
        case PREF_TIME_XMLCONSOLE:
            return "time.xmlconsole";
        case PREF_TIME_STATUSBAR:
            return "time.statusbar";
        case PREF_TIME_LASTACTIVITY:
            return "time.lastactivity";
        case PREF_ROSTER:
            return "roster";
        case PREF_ROSTER_OFFLINE:
            return "roster.offline";
        case PREF_ROSTER_RESOURCE:
            return "roster.resource";
        case PREF_ROSTER_PRESENCE:
            return "roster.presence";
        case PREF_ROSTER_STATUS:
            return "roster.status";
        case PREF_ROSTER_EMPTY:
            return "roster.empty";
        case PREF_ROSTER_BY:
            return "roster.by";
        case PREF_ROSTER_ORDER:
            return "roster.order";
        case PREF_ROSTER_UNREAD:
            return "roster.unread";
        case PREF_ROSTER_COUNT:
            return "roster.count";
        case PREF_ROSTER_COUNT_ZERO:
            return "roster.count.zero";
        case PREF_ROSTER_PRIORITY:
            return "roster.priority";
        case PREF_ROSTER_WRAP:
            return "roster.wrap";
        case PREF_ROSTER_RESOURCE_JOIN:
            return "roster.resource.join";
        case PREF_ROSTER_CONTACTS:
            return "roster.contacts";
        case PREF_ROSTER_UNSUBSCRIBED:
            return "roster.unsubscribed";
        case PREF_ROSTER_ROOMS:
            return "roster.rooms";
        case PREF_ROSTER_ROOMS_POS:
            return "roster.rooms.pos";
        case PREF_ROSTER_ROOMS_BY:
            return "roster.rooms.by";
        case PREF_ROSTER_ROOMS_ORDER:
            return "roster.rooms.order";
        case PREF_ROSTER_ROOMS_UNREAD:
            return "roster.rooms.unread";
        case PREF_ROSTER_PRIVATE:
            return "roster.private";
        case PREF_RESOURCE_TITLE:
            return "resource.title";
        case PREF_RESOURCE_MESSAGE:
            return "resource.message";
        case PREF_INPBLOCK_DYNAMIC:
            return "inpblock.dynamic";
        case PREF_ENC_WARN:
            return "enc.warn";
        case PREF_PGP_LOG:
            return "log";
        case PREF_TLS_CERTPATH:
            return "tls.certpath";
        case PREF_TLS_SHOW:
            return "tls.show";
        case PREF_LASTACTIVITY:
            return "lastactivity";
        case PREF_CONSOLE_MUC:
            return "console.muc";
        case PREF_CONSOLE_PRIVATE:
            return "console.private";
        case PREF_CONSOLE_CHAT:
            return "console.chat";
        case PREF_BOOKMARK_INVITE:
            return "bookmark.invite";
        case PREF_PLUGINS_SOURCEPATH:
            return "sourcepath";
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
        case PREF_ENC_WARN:
        case PREF_AUTOAWAY_CHECK:
        case PREF_LOG_ROTATE:
        case PREF_LOG_SHARED:
        case PREF_NOTIFY_CHAT:
        case PREF_NOTIFY_CHAT_CURRENT:
        case PREF_NOTIFY_ROOM:
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
        case PREF_WINS_AUTO_TIDY:
        case PREF_INPBLOCK_DYNAMIC:
        case PREF_RESOURCE_TITLE:
        case PREF_RESOURCE_MESSAGE:
        case PREF_ROSTER:
        case PREF_ROSTER_OFFLINE:
        case PREF_ROSTER_EMPTY:
        case PREF_ROSTER_COUNT_ZERO:
        case PREF_ROSTER_PRIORITY:
        case PREF_ROSTER_RESOURCE_JOIN:
        case PREF_ROSTER_CONTACTS:
        case PREF_ROSTER_UNSUBSCRIBED:
        case PREF_ROSTER_ROOMS:
        case PREF_TLS_SHOW:
        case PREF_LASTACTIVITY:
        case PREF_NOTIFY_MENTION_WHOLE_WORD:
        case PREF_TRAY_READ:
        case PREF_BOOKMARK_INVITE:
            return TRUE;
        default:
            return FALSE;
    }
}

// the default setting for a string type preference
// if it is not specified in .profrc
static char*
_get_default_string(preference_t pref)
{
    switch (pref)
    {
        case PREF_AUTOAWAY_MODE:
            return "off";
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
        case PREF_ROSTER_COUNT:
            return "unread";
        case PREF_ROSTER_ORDER:
            return "presence";
        case PREF_ROSTER_UNREAD:
            return "after";
        case PREF_ROSTER_ROOMS_POS:
            return "last";
        case PREF_ROSTER_ROOMS_BY:
            return "none";
        case PREF_ROSTER_ROOMS_ORDER:
            return "name";
        case PREF_ROSTER_ROOMS_UNREAD:
            return "after";
        case PREF_ROSTER_PRIVATE:
            return "room";
        case PREF_TIME_CONSOLE:
            return "%H:%M:%S";
        case PREF_TIME_CHAT:
            return "%H:%M:%S";
        case PREF_TIME_MUC:
            return "%H:%M:%S";
        case PREF_TIME_MUCCONFIG:
            return "%H:%M:%S";
        case PREF_TIME_PRIVATE:
            return "%H:%M:%S";
        case PREF_TIME_XMLCONSOLE:
            return "%H:%M:%S";
        case PREF_TIME_STATUSBAR:
            return "%H:%M";
        case PREF_TIME_LASTACTIVITY:
            return "%d/%m/%y %H:%M:%S";
        case PREF_PGP_LOG:
            return "redact";
        case PREF_CONSOLE_MUC:
        case PREF_CONSOLE_PRIVATE:
        case PREF_CONSOLE_CHAT:
            return "all";
        default:
            return NULL;
    }
}
