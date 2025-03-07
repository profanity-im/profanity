/*
 * theme.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 * Copyright (C) 2019 - 2024 Michael Vetter <jubalh@iodoru.org>
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
#include <string.h>

#include <glib.h>

#ifdef HAVE_NCURSESW_NCURSES_H
#include <ncursesw/ncurses.h>
#elif HAVE_NCURSES_H
#include <ncurses.h>
#elif HAVE_CURSES_H
#include <curses.h>
#endif

#include "common.h"
#include "log.h"
#include "config/files.h"
#include "config/theme.h"
#include "config/preferences.h"
#include "config/color.h"

static GString* theme_loc;
static GKeyFile* theme;
static GHashTable* bold_items;
static GHashTable* defaults;

static void _load_preferences(void);
static void _theme_list_dir(const gchar* const dir, GSList** result);
static GString* _theme_find(const char* const theme_name);
static gboolean _theme_load_file(const char* const theme_name);

static void
_theme_close(void)
{
    color_pair_cache_free();
    if (theme) {
        g_key_file_free(theme);
        theme = NULL;
    }
    if (theme_loc) {
        g_string_free(theme_loc, TRUE);
        theme_loc = NULL;
    }
    if (bold_items) {
        g_hash_table_destroy(bold_items);
        bold_items = NULL;
    }
    if (defaults) {
        g_hash_table_destroy(defaults);
        defaults = NULL;
    }
}

void
theme_init(const char* const theme_name)
{
    if (!_theme_load_file(theme_name)) {
        log_error("Loading theme %s failed.", theme_name);

        if (!_theme_load_file("default")) {
            log_error("Theme initialisation failed.");
        }
    }

    prof_add_shutdown_routine(_theme_close);

    defaults = g_hash_table_new_full(g_str_hash, g_str_equal, free, free);

    // Set default colors
    g_hash_table_insert(defaults, strdup("main.text"), strdup("default"));
    g_hash_table_insert(defaults, strdup("main.text.history"), strdup("default"));
    g_hash_table_insert(defaults, strdup("main.text.me"), strdup("default"));
    g_hash_table_insert(defaults, strdup("main.text.them"), strdup("default"));
    g_hash_table_insert(defaults, strdup("main.splash"), strdup("cyan"));
    g_hash_table_insert(defaults, strdup("main.help.header"), strdup("default"));
    g_hash_table_insert(defaults, strdup("main.trackbar"), strdup("default"));
    g_hash_table_insert(defaults, strdup("error"), strdup("red"));
    g_hash_table_insert(defaults, strdup("incoming"), strdup("yellow"));
    g_hash_table_insert(defaults, strdup("mention"), strdup("yellow"));
    g_hash_table_insert(defaults, strdup("trigger"), strdup("yellow"));
    g_hash_table_insert(defaults, strdup("input.text"), strdup("default"));
    g_hash_table_insert(defaults, strdup("main.time"), strdup("default"));
    g_hash_table_insert(defaults, strdup("titlebar.text"), strdup("white"));
    g_hash_table_insert(defaults, strdup("titlebar.brackets"), strdup("cyan"));
    g_hash_table_insert(defaults, strdup("titlebar.unencrypted"), strdup("red"));
    g_hash_table_insert(defaults, strdup("titlebar.encrypted"), strdup("white"));
    g_hash_table_insert(defaults, strdup("titlebar.untrusted"), strdup("yellow"));
    g_hash_table_insert(defaults, strdup("titlebar.trusted"), strdup("white"));
    g_hash_table_insert(defaults, strdup("titlebar.online"), strdup("white"));
    g_hash_table_insert(defaults, strdup("titlebar.offline"), strdup("white"));
    g_hash_table_insert(defaults, strdup("titlebar.away"), strdup("white"));
    g_hash_table_insert(defaults, strdup("titlebar.chat"), strdup("white"));
    g_hash_table_insert(defaults, strdup("titlebar.dnd"), strdup("white"));
    g_hash_table_insert(defaults, strdup("titlebar.xa"), strdup("white"));
    g_hash_table_insert(defaults, strdup("titlebar.scrolled"), strdup("default"));
    g_hash_table_insert(defaults, strdup("statusbar.text"), strdup("white"));
    g_hash_table_insert(defaults, strdup("statusbar.brackets"), strdup("cyan"));
    g_hash_table_insert(defaults, strdup("statusbar.active"), strdup("cyan"));
    g_hash_table_insert(defaults, strdup("statusbar.current"), strdup("cyan"));
    g_hash_table_insert(defaults, strdup("statusbar.new"), strdup("white"));
    g_hash_table_insert(defaults, strdup("statusbar.time"), strdup("white"));
    g_hash_table_insert(defaults, strdup("me"), strdup("yellow"));
    g_hash_table_insert(defaults, strdup("them"), strdup("green"));
    g_hash_table_insert(defaults, strdup("receipt.sent"), strdup("red"));
    g_hash_table_insert(defaults, strdup("roominfo"), strdup("yellow"));
    g_hash_table_insert(defaults, strdup("roommention"), strdup("yellow"));
    g_hash_table_insert(defaults, strdup("roommention.term"), strdup("yellow"));
    g_hash_table_insert(defaults, strdup("roomtrigger"), strdup("yellow"));
    g_hash_table_insert(defaults, strdup("roomtrigger.term"), strdup("yellow"));
    g_hash_table_insert(defaults, strdup("online"), strdup("green"));
    g_hash_table_insert(defaults, strdup("offline"), strdup("red"));
    g_hash_table_insert(defaults, strdup("away"), strdup("cyan"));
    g_hash_table_insert(defaults, strdup("chat"), strdup("green"));
    g_hash_table_insert(defaults, strdup("dnd"), strdup("red"));
    g_hash_table_insert(defaults, strdup("xa"), strdup("cyan"));
    g_hash_table_insert(defaults, strdup("typing"), strdup("yellow"));
    g_hash_table_insert(defaults, strdup("gone"), strdup("red"));
    g_hash_table_insert(defaults, strdup("subscribed"), strdup("green"));
    g_hash_table_insert(defaults, strdup("unsubscribed"), strdup("red"));
    g_hash_table_insert(defaults, strdup("otr.started.trusted"), strdup("green"));
    g_hash_table_insert(defaults, strdup("otr.started.untrusted"), strdup("yellow"));
    g_hash_table_insert(defaults, strdup("otr.ended"), strdup("red"));
    g_hash_table_insert(defaults, strdup("otr.trusted"), strdup("green"));
    g_hash_table_insert(defaults, strdup("otr.untrusted"), strdup("yellow"));
    g_hash_table_insert(defaults, strdup("roster.header"), strdup("yellow"));
    g_hash_table_insert(defaults, strdup("roster.online"), strdup("green"));
    g_hash_table_insert(defaults, strdup("roster.offline"), strdup("red"));
    g_hash_table_insert(defaults, strdup("roster.chat"), strdup("green"));
    g_hash_table_insert(defaults, strdup("roster.away"), strdup("cyan"));
    g_hash_table_insert(defaults, strdup("roster.dnd"), strdup("red"));
    g_hash_table_insert(defaults, strdup("roster.xa"), strdup("cyan"));
    g_hash_table_insert(defaults, strdup("roster.online.active"), strdup("green"));
    g_hash_table_insert(defaults, strdup("roster.offline.active"), strdup("red"));
    g_hash_table_insert(defaults, strdup("roster.chat.active"), strdup("green"));
    g_hash_table_insert(defaults, strdup("roster.away.active"), strdup("cyan"));
    g_hash_table_insert(defaults, strdup("roster.dnd.active"), strdup("red"));
    g_hash_table_insert(defaults, strdup("roster.xa.active"), strdup("cyan"));
    g_hash_table_insert(defaults, strdup("roster.online.unread"), strdup("green"));
    g_hash_table_insert(defaults, strdup("roster.offline.unread"), strdup("red"));
    g_hash_table_insert(defaults, strdup("roster.chat.unread"), strdup("green"));
    g_hash_table_insert(defaults, strdup("roster.away.unread"), strdup("cyan"));
    g_hash_table_insert(defaults, strdup("roster.dnd.unread"), strdup("red"));
    g_hash_table_insert(defaults, strdup("roster.xa.unread"), strdup("cyan"));
    g_hash_table_insert(defaults, strdup("roster.room"), strdup("green"));
    g_hash_table_insert(defaults, strdup("roster.room.unread"), strdup("green"));
    g_hash_table_insert(defaults, strdup("roster.room.trigger"), strdup("green"));
    g_hash_table_insert(defaults, strdup("roster.room.mention"), strdup("green"));
    g_hash_table_insert(defaults, strdup("occupants.header"), strdup("yellow"));
    g_hash_table_insert(defaults, strdup("untrusted"), strdup("red"));
    g_hash_table_insert(defaults, strdup("cmd.wins.unread"), strdup("default"));

    //_load_preferences();
}

gboolean
theme_exists(const char* const theme_name)
{
    if (g_strcmp0(theme_name, "default") == 0) {
        return TRUE;
    }

    GString* new_theme_file = _theme_find(theme_name);
    if (new_theme_file == NULL) {
        return FALSE;
    }

    g_string_free(new_theme_file, TRUE);
    return TRUE;
}

gboolean
theme_load(const char* const theme_name, gboolean load_theme_prefs)
{
    if (!theme_exists(theme_name))
        return FALSE;

    color_pair_cache_reset();

    if (_theme_load_file(theme_name)) {
        if (load_theme_prefs) {
            _load_preferences();
        }
        return TRUE;
    } else {
        return FALSE;
    }
}

static gboolean
_theme_load_file(const char* const theme_name)
{
    // use default theme
    if (theme_name == NULL || strcmp(theme_name, "default") == 0) {
        if (theme) {
            g_key_file_free(theme);
        }
        theme = g_key_file_new();

        // load theme from file
    } else {
        GString* new_theme_file = _theme_find(theme_name);
        if (new_theme_file == NULL) {
            log_info("Theme does not exist \"%s\"", theme_name);
            return FALSE;
        }

        if (theme_loc) {
            g_string_free(theme_loc, TRUE);
        }
        theme_loc = new_theme_file;
        log_info("Loading theme \"%s\"", theme_name);
        if (theme) {
            g_key_file_free(theme);
        }
        theme = g_key_file_new();
        g_key_file_load_from_file(theme, theme_loc->str, G_KEY_FILE_KEEP_COMMENTS,
                                  NULL);
    }

    return TRUE;
}

GSList*
theme_list(void)
{
    GSList* result = NULL;
    auto_gchar gchar* themes_dir = files_get_config_path(DIR_THEMES);

    _theme_list_dir(themes_dir, &result);

#ifdef THEMES_PATH
    _theme_list_dir(THEMES_PATH, &result);
#endif

    return result;
}

void
theme_init_colours(void)
{
    assume_default_colors(-1, -1);
    color_pair_cache_reset();
}

static void
_set_string_preference(char* prefstr, preference_t pref)
{
    if (g_key_file_has_key(theme, "ui", prefstr, NULL)) {
        auto_gchar gchar* val = g_key_file_get_string(theme, "ui", prefstr, NULL);
        prefs_set_string(pref, val);
    }
}

static void
_set_boolean_preference(char* prefstr, preference_t pref)
{
    if (g_key_file_has_key(theme, "ui", prefstr, NULL)) {
        gboolean val = g_key_file_get_boolean(theme, "ui", prefstr, NULL);
        prefs_set_boolean(pref, val);
    }
}

static void
_load_preferences(void)
{
    // load booleans from theme and set them to prefs
    _set_boolean_preference("beep", PREF_BEEP);
    _set_boolean_preference("flash", PREF_FLASH);
    _set_boolean_preference("splash", PREF_SPLASH);
    _set_boolean_preference("wrap", PREF_WRAP);
    _set_boolean_preference("resource.title", PREF_RESOURCE_TITLE);
    _set_boolean_preference("resource.message", PREF_RESOURCE_MESSAGE);
    _set_boolean_preference("occupants", PREF_OCCUPANTS);
    _set_boolean_preference("occupants.jid", PREF_OCCUPANTS_JID);
    _set_boolean_preference("occupants.offline", PREF_OCCUPANTS_OFFLINE);
    _set_boolean_preference("occupants.wrap", PREF_OCCUPANTS_WRAP);
    _set_boolean_preference("roster", PREF_ROSTER);
    _set_boolean_preference("roster.offline", PREF_ROSTER_OFFLINE);
    _set_boolean_preference("roster.resource", PREF_ROSTER_RESOURCE);
    _set_boolean_preference("roster.resource.join", PREF_ROSTER_RESOURCE_JOIN);
    _set_boolean_preference("roster.presence", PREF_ROSTER_PRESENCE);
    _set_boolean_preference("roster.status", PREF_ROSTER_STATUS);
    _set_boolean_preference("roster.empty", PREF_ROSTER_EMPTY);
    _set_boolean_preference("roster.wrap", PREF_ROSTER_WRAP);
    _set_boolean_preference("roster.count.zero", PREF_ROSTER_COUNT_ZERO);
    _set_boolean_preference("roster.priority", PREF_ROSTER_PRIORITY);
    _set_boolean_preference("roster.contacts", PREF_ROSTER_CONTACTS);
    _set_boolean_preference("roster.unsubscribed", PREF_ROSTER_UNSUBSCRIBED);
    _set_boolean_preference("roster.rooms", PREF_ROSTER_ROOMS);
    _set_boolean_preference("privileges", PREF_MUC_PRIVILEGES);
    _set_boolean_preference("presence", PREF_PRESENCE);
    _set_boolean_preference("intype", PREF_INTYPE);
    _set_boolean_preference("enc.warn", PREF_ENC_WARN);
    _set_boolean_preference("tls.show", PREF_TLS_SHOW);
    _set_boolean_preference("statusbar.show.name", PREF_STATUSBAR_SHOW_NAME);
    _set_boolean_preference("statusbar.show.number", PREF_STATUSBAR_SHOW_NUMBER);

    // load strings from theme and set them to prefs
    _set_string_preference("time.console", PREF_TIME_CONSOLE);
    _set_string_preference("time.chat", PREF_TIME_CHAT);
    _set_string_preference("time.muc", PREF_TIME_MUC);
    _set_string_preference("time.config", PREF_TIME_CONFIG);
    _set_string_preference("time.private", PREF_TIME_PRIVATE);
    _set_string_preference("time.xmlconsole", PREF_TIME_XMLCONSOLE);
    _set_string_preference("time.statusbar", PREF_TIME_STATUSBAR);
    _set_string_preference("time.lastactivity", PREF_TIME_LASTACTIVITY);
    _set_string_preference("statuses.console", PREF_STATUSES_CONSOLE);
    _set_string_preference("statuses.chat", PREF_STATUSES_CHAT);
    _set_string_preference("statuses.muc", PREF_STATUSES_MUC);
    _set_string_preference("console.muc", PREF_CONSOLE_MUC);
    _set_string_preference("console.private", PREF_CONSOLE_PRIVATE);
    _set_string_preference("console.chat", PREF_CONSOLE_CHAT);
    _set_string_preference("roster.by", PREF_ROSTER_BY);
    _set_string_preference("roster.order", PREF_ROSTER_ORDER);
    _set_string_preference("roster.unread", PREF_ROSTER_UNREAD);
    _set_string_preference("roster.rooms.order", PREF_ROSTER_ROOMS_ORDER);
    _set_string_preference("roster.rooms.unread", PREF_ROSTER_ROOMS_UNREAD);
    _set_string_preference("roster.rooms.pos", PREF_ROSTER_ROOMS_POS);
    _set_string_preference("roster.rooms.by", PREF_ROSTER_ROOMS_BY);
    _set_string_preference("roster.private", PREF_ROSTER_PRIVATE);
    _set_string_preference("roster.count", PREF_ROSTER_COUNT);
    _set_string_preference("roster.rooms.title", PREF_ROSTER_ROOMS_TITLE);
    _set_string_preference("statusbar.self", PREF_STATUSBAR_SELF);
    _set_string_preference("statusbar.chat", PREF_STATUSBAR_CHAT);
    _set_string_preference("statusbar.room.title", PREF_STATUSBAR_ROOM_TITLE);
    _set_string_preference("titlebar.muc.title", PREF_TITLEBAR_MUC_TITLE);

    // load ints from theme and set them to prefs
    // with custom set functions
    if (g_key_file_has_key(theme, "ui", "statusbar.tabs", NULL)) {
        gint tabs_size = g_key_file_get_integer(theme, "ui", "statusbar.tabs", NULL);
        prefs_set_statusbartabs(tabs_size);
    }

    if (g_key_file_has_key(theme, "ui", "statusbar.tablen", NULL)) {
        gint tab_len = g_key_file_get_integer(theme, "ui", "statusbar.tablen", NULL);
        prefs_set_statusbartablen(tab_len);
    }

    if (g_key_file_has_key(theme, "ui", "occupants.size", NULL)) {
        gint occupants_size = g_key_file_get_integer(theme, "ui", "occupants.size", NULL);
        prefs_set_occupants_size(occupants_size);
    }

    if (g_key_file_has_key(theme, "ui", "occupants.indent", NULL)) {
        gint occupants_indent = g_key_file_get_integer(theme, "ui", "occupants.indent", NULL);
        prefs_set_occupants_indent(occupants_indent);
    }

    if (g_key_file_has_key(theme, "ui", "roster.size", NULL)) {
        gint roster_size = g_key_file_get_integer(theme, "ui", "roster.size", NULL);
        prefs_set_roster_size(roster_size);
    }

    if (g_key_file_has_key(theme, "ui", "roster.contact.indent", NULL)) {
        gint contact_indent = g_key_file_get_integer(theme, "ui", "roster.contact.indent", NULL);
        prefs_set_roster_contact_indent(contact_indent);
    }

    if (g_key_file_has_key(theme, "ui", "roster.resource.indent", NULL)) {
        gint resource_indent = g_key_file_get_integer(theme, "ui", "roster.resource.indent", NULL);
        prefs_set_roster_resource_indent(resource_indent);
    }

    if (g_key_file_has_key(theme, "ui", "roster.presence.indent", NULL)) {
        gint presence_indent = g_key_file_get_integer(theme, "ui", "roster.presence.indent", NULL);
        prefs_set_roster_presence_indent(presence_indent);
    }

    // load chars from theme and set them to prefs
    // with custom set functions
    if (g_key_file_has_key(theme, "ui", "occupants.char", NULL)) {
        auto_gchar gchar* ch = g_key_file_get_string(theme, "ui", "occupants.char", NULL);
        if (ch && g_utf8_strlen(ch, 4) == 1) {
            prefs_set_occupants_char(ch);
        }
    }

    if (g_key_file_has_key(theme, "ui", "occupants.header.char", NULL)) {
        auto_gchar gchar* ch = g_key_file_get_string(theme, "ui", "occupants.header.char", NULL);
        if (ch && g_utf8_strlen(ch, 4) == 1) {
            prefs_set_occupants_header_char(ch);
        }
    }

    if (g_key_file_has_key(theme, "ui", "roster.header.char", NULL)) {
        auto_gchar gchar* ch = g_key_file_get_string(theme, "ui", "roster.header.char", NULL);
        if (ch && g_utf8_strlen(ch, 4) == 1) {
            prefs_set_roster_header_char(ch);
        }
    }

    if (g_key_file_has_key(theme, "ui", "roster.contact.char", NULL)) {
        auto_gchar gchar* ch = g_key_file_get_string(theme, "ui", "roster.contact.char", NULL);
        if (ch && g_utf8_strlen(ch, 4) == 1) {
            prefs_set_roster_contact_char(ch);
        }
    }

    if (g_key_file_has_key(theme, "ui", "roster.resource.char", NULL)) {
        auto_gchar gchar* ch = g_key_file_get_string(theme, "ui", "roster.resource.char", NULL);
        if (ch && g_utf8_strlen(ch, 4) == 1) {
            prefs_set_roster_resource_char(ch);
        }
    } else {
        prefs_clear_roster_resource_char();
    }

    if (g_key_file_has_key(theme, "ui", "roster.rooms.char", NULL)) {
        auto_gchar gchar* ch = g_key_file_get_string(theme, "ui", "roster.rooms.char", NULL);
        if (ch && g_utf8_strlen(ch, 4) == 1) {
            prefs_set_roster_room_char(ch);
        }
    }

    if (g_key_file_has_key(theme, "ui", "roster.rooms.private.char", NULL)) {
        auto_gchar gchar* ch = g_key_file_get_string(theme, "ui", "roster.rooms.private.char", NULL);
        if (ch && g_utf8_strlen(ch, 4) == 1) {
            prefs_set_roster_room_private_char(ch);
        }
    }

    if (g_key_file_has_key(theme, "ui", "roster.private.char", NULL)) {
        auto_gchar gchar* ch = g_key_file_get_string(theme, "ui", "roster.private.char", NULL);
        if (ch && g_utf8_strlen(ch, 4) == 1) {
            prefs_set_roster_private_char(ch);
        }
    }

    if (g_key_file_has_key(theme, "ui", "otr.char", NULL)) {
        auto_gchar gchar* ch = g_key_file_get_string(theme, "ui", "otr.char", NULL);
        if (ch && g_utf8_strlen(ch, 4) == 1) {
            prefs_set_otr_char(ch);
        }
    }

    if (g_key_file_has_key(theme, "ui", "pgp.char", NULL)) {
        auto_gchar gchar* ch = g_key_file_get_string(theme, "ui", "pgp.char", NULL);
        if (ch && g_utf8_strlen(ch, 4) == 1) {
            prefs_set_pgp_char(ch);
        }
    }

    if (g_key_file_has_key(theme, "ui", "omemo.char", NULL)) {
        auto_gchar gchar* ch = g_key_file_get_string(theme, "ui", "omemo.char", NULL);
        if (ch && g_utf8_strlen(ch, 4) == 1) {
            prefs_set_omemo_char(ch);
        }
    }

    if (g_key_file_has_key(theme, "ui", "correction.char", NULL)) {
        auto_gchar gchar* ch = g_key_file_get_string(theme, "ui", "correction.char", NULL);
        if (ch && g_utf8_strlen(ch, 4) == 1) {
            prefs_set_correction_char(ch);
        }
    }

    // load window positions
    if (g_key_file_has_key(theme, "ui", "titlebar.position", NULL) && g_key_file_has_key(theme, "ui", "mainwin.position", NULL) && g_key_file_has_key(theme, "ui", "statusbar.position", NULL) && g_key_file_has_key(theme, "ui", "inputwin.position", NULL)) {

        int titlebar_pos = g_key_file_get_integer(theme, "ui", "titlebar.position", NULL);
        int mainwin_pos = g_key_file_get_integer(theme, "ui", "mainwin.position", NULL);
        int statusbar_pos = g_key_file_get_integer(theme, "ui", "statusbar.position", NULL);
        int inputwin_pos = g_key_file_get_integer(theme, "ui", "inputwin.position", NULL);

        ProfWinPlacement* placement = prefs_create_profwin_placement(titlebar_pos, mainwin_pos, statusbar_pos, inputwin_pos);

        prefs_save_win_placement(placement);
        prefs_free_win_placement(placement);
    }
}

static void
_theme_list_dir(const gchar* const dir, GSList** result)
{
    GDir* themes = g_dir_open(dir, 0, NULL);
    if (themes) {
        const gchar* theme = g_dir_read_name(themes);
        while (theme) {
            *result = g_slist_append(*result, strdup(theme));
            theme = g_dir_read_name(themes);
        }
        g_dir_close(themes);
    }
}

static GString*
_theme_find(const char* const theme_name)
{
    GString* path = NULL;
    auto_gchar gchar* themes_dir = files_get_config_path(DIR_THEMES);

    if (themes_dir) {
        path = g_string_new(themes_dir);
        g_string_append(path, "/");
        g_string_append(path, theme_name);
        if (!g_file_test(path->str, G_FILE_TEST_EXISTS)) {
            g_string_free(path, true);
            path = NULL;
        }
    }

#ifdef THEMES_PATH
    if (path == NULL) {
        path = g_string_new(THEMES_PATH);
        g_string_append(path, "/");
        g_string_append(path, theme_name);
        if (!g_file_test(path->str, G_FILE_TEST_EXISTS)) {
            g_string_free(path, true);
            path = NULL;
        }
    }
#endif /* THEMES_PATH */

    return path;
}

theme_item_t
theme_roster_unread_presence_attrs(const char* const presence)
{
    if (g_strcmp0(presence, "online") == 0) {
        return THEME_ROSTER_ONLINE_UNREAD;
    } else if (g_strcmp0(presence, "away") == 0) {
        return THEME_ROSTER_AWAY_UNREAD;
    } else if (g_strcmp0(presence, "chat") == 0) {
        return THEME_ROSTER_CHAT_UNREAD;
    } else if (g_strcmp0(presence, "dnd") == 0) {
        return THEME_ROSTER_DND_UNREAD;
    } else if (g_strcmp0(presence, "xa") == 0) {
        return THEME_ROSTER_XA_UNREAD;
    } else {
        return THEME_ROSTER_OFFLINE_UNREAD;
    }
}

theme_item_t
theme_roster_active_presence_attrs(const char* const presence)
{
    if (g_strcmp0(presence, "online") == 0) {
        return THEME_ROSTER_ONLINE_ACTIVE;
    } else if (g_strcmp0(presence, "away") == 0) {
        return THEME_ROSTER_AWAY_ACTIVE;
    } else if (g_strcmp0(presence, "chat") == 0) {
        return THEME_ROSTER_CHAT_ACTIVE;
    } else if (g_strcmp0(presence, "dnd") == 0) {
        return THEME_ROSTER_DND_ACTIVE;
    } else if (g_strcmp0(presence, "xa") == 0) {
        return THEME_ROSTER_XA_ACTIVE;
    } else {
        return THEME_ROSTER_OFFLINE_ACTIVE;
    }
}

theme_item_t
theme_roster_presence_attrs(const char* const presence)
{
    if (g_strcmp0(presence, "online") == 0) {
        return THEME_ROSTER_ONLINE;
    } else if (g_strcmp0(presence, "away") == 0) {
        return THEME_ROSTER_AWAY;
    } else if (g_strcmp0(presence, "chat") == 0) {
        return THEME_ROSTER_CHAT;
    } else if (g_strcmp0(presence, "dnd") == 0) {
        return THEME_ROSTER_DND;
    } else if (g_strcmp0(presence, "xa") == 0) {
        return THEME_ROSTER_XA;
    } else {
        return THEME_ROSTER_OFFLINE;
    }
}

theme_item_t
theme_main_presence_attrs(const char* const presence)
{
    if (g_strcmp0(presence, "online") == 0) {
        return THEME_ONLINE;
    } else if (g_strcmp0(presence, "away") == 0) {
        return THEME_AWAY;
    } else if (g_strcmp0(presence, "chat") == 0) {
        return THEME_CHAT;
    } else if (g_strcmp0(presence, "dnd") == 0) {
        return THEME_DND;
    } else if (g_strcmp0(presence, "xa") == 0) {
        return THEME_XA;
    } else {
        return THEME_OFFLINE;
    }
}

static void
_theme_prep_bgnd(char* setting, char* def, GString* lookup_str)
{
    auto_gchar gchar* val = g_key_file_get_string(theme, "colours", setting, NULL);
    if (!val) {
        g_string_append(lookup_str, def);
    } else {
        if (g_str_has_prefix(val, "bold_")) {
            g_string_append(lookup_str, &val[5]);
        } else {
            g_string_append(lookup_str, val);
        }
    }
}

/* return value needs to be freed */
char*
theme_get_bkgnd(void)
{
    char* val = g_key_file_get_string(theme, "colours", "bkgnd", NULL);
    return val;
}

/* gets the foreground color from the theme. or uses the one defined in 'defaults' */
static void
_theme_prep_fgnd(char* setting, GString* lookup_str, gboolean* bold)
{
    auto_gchar gchar* conf_str = g_key_file_get_string(theme, "colours", setting, NULL);
    gchar* val = conf_str;

    if (!val)
        val = g_hash_table_lookup(defaults, setting);

    if (g_str_has_prefix(val, "bold_")) {
        g_string_append(lookup_str, &val[5]);
        *bold = TRUE;
    } else {
        g_string_append(lookup_str, val);
        *bold = FALSE;
    }
}

char*
theme_get_string(char* str)
{
    gchar* res = g_key_file_get_string(theme, "colours", str, NULL);
    if (!res) {
        return strdup(g_hash_table_lookup(defaults, str));
    } else {
        return res;
    }
}

void
theme_free_string(char* str)
{
    if (str) {
        g_free(str);
    }
}

int
theme_hash_attrs(const char* str)
{
    color_profile profile = COLOR_PROFILE_DEFAULT;

    auto_gchar gchar* color_pref = prefs_get_string(PREF_COLOR_NICK);
    if (strcmp(color_pref, "redgreen") == 0) {
        profile = COLOR_PROFILE_REDGREEN_BLINDNESS;
    } else if (strcmp(color_pref, "blue") == 0) {
        profile = COLOR_PROFILE_BLUE_BLINDNESS;
    }

    return COLOR_PAIR(color_pair_cache_hash_str(str, profile));
}

/* returns the colours (fgnd and bknd) for a certain attribute ie main.text */
int
theme_attrs(theme_item_t attrs)
{
    int result = 0;

    GString* lookup_str = g_string_new("");
    gboolean bold = FALSE;

    // get foreground colour
    switch (attrs) {
    case THEME_TEXT:
        _theme_prep_fgnd("main.text", lookup_str, &bold);
        break;
    case THEME_TEXT_HISTORY:
        _theme_prep_fgnd("main.text.history", lookup_str, &bold);
        break;
    case THEME_TEXT_ME:
        _theme_prep_fgnd("main.text.me", lookup_str, &bold);
        break;
    case THEME_TEXT_THEM:
        _theme_prep_fgnd("main.text.them", lookup_str, &bold);
        break;
    case THEME_SPLASH:
        _theme_prep_fgnd("main.splash", lookup_str, &bold);
        break;
    case THEME_TRACKBAR:
        _theme_prep_fgnd("main.trackbar", lookup_str, &bold);
        break;
    case THEME_HELP_HEADER:
        _theme_prep_fgnd("main.help.header", lookup_str, &bold);
        break;
    case THEME_ERROR:
        _theme_prep_fgnd("error", lookup_str, &bold);
        break;
    case THEME_INCOMING:
        _theme_prep_fgnd("incoming", lookup_str, &bold);
        break;
    case THEME_MENTION:
        _theme_prep_fgnd("mention", lookup_str, &bold);
        break;
    case THEME_TRIGGER:
        _theme_prep_fgnd("trigger", lookup_str, &bold);
        break;
    case THEME_INPUT_TEXT:
        _theme_prep_fgnd("input.text", lookup_str, &bold);
        break;
    case THEME_TIME:
        _theme_prep_fgnd("main.time", lookup_str, &bold);
        break;
    case THEME_TITLE_TEXT:
        _theme_prep_fgnd("titlebar.text", lookup_str, &bold);
        break;
    case THEME_TITLE_BRACKET:
        _theme_prep_fgnd("titlebar.brackets", lookup_str, &bold);
        break;
    case THEME_TITLE_SCROLLED:
        _theme_prep_fgnd("titlebar.scrolled", lookup_str, &bold);
        break;
    case THEME_TITLE_UNENCRYPTED:
        _theme_prep_fgnd("titlebar.unencrypted", lookup_str, &bold);
        break;
    case THEME_TITLE_ENCRYPTED:
        _theme_prep_fgnd("titlebar.encrypted", lookup_str, &bold);
        break;
    case THEME_TITLE_UNTRUSTED:
        _theme_prep_fgnd("titlebar.untrusted", lookup_str, &bold);
        break;
    case THEME_TITLE_TRUSTED:
        _theme_prep_fgnd("titlebar.trusted", lookup_str, &bold);
        break;
    case THEME_TITLE_ONLINE:
        _theme_prep_fgnd("titlebar.online", lookup_str, &bold);
        break;
    case THEME_TITLE_OFFLINE:
        _theme_prep_fgnd("titlebar.offline", lookup_str, &bold);
        break;
    case THEME_TITLE_AWAY:
        _theme_prep_fgnd("titlebar.away", lookup_str, &bold);
        break;
    case THEME_TITLE_CHAT:
        _theme_prep_fgnd("titlebar.chat", lookup_str, &bold);
        break;
    case THEME_TITLE_DND:
        _theme_prep_fgnd("titlebar.dnd", lookup_str, &bold);
        break;
    case THEME_TITLE_XA:
        _theme_prep_fgnd("titlebar.xa", lookup_str, &bold);
        break;
    case THEME_STATUS_TEXT:
        _theme_prep_fgnd("statusbar.text", lookup_str, &bold);
        break;
    case THEME_STATUS_BRACKET:
        _theme_prep_fgnd("statusbar.brackets", lookup_str, &bold);
        break;
    case THEME_STATUS_ACTIVE:
        _theme_prep_fgnd("statusbar.active", lookup_str, &bold);
        break;
    case THEME_STATUS_CURRENT:
        _theme_prep_fgnd("statusbar.current", lookup_str, &bold);
        break;
    case THEME_STATUS_NEW:
        _theme_prep_fgnd("statusbar.new", lookup_str, &bold);
        break;
    case THEME_STATUS_TIME:
        _theme_prep_fgnd("statusbar.time", lookup_str, &bold);
        break;
    case THEME_ME:
        _theme_prep_fgnd("me", lookup_str, &bold);
        break;
    case THEME_THEM:
        _theme_prep_fgnd("them", lookup_str, &bold);
        break;
    case THEME_RECEIPT_SENT:
        _theme_prep_fgnd("receipt.sent", lookup_str, &bold);
        break;
    case THEME_ROOMINFO:
        _theme_prep_fgnd("roominfo", lookup_str, &bold);
        break;
    case THEME_ROOMMENTION:
        _theme_prep_fgnd("roommention", lookup_str, &bold);
        break;
    case THEME_ROOMMENTION_TERM:
        _theme_prep_fgnd("roommention.term", lookup_str, &bold);
        break;
    case THEME_ROOMTRIGGER:
        _theme_prep_fgnd("roomtrigger", lookup_str, &bold);
        break;
    case THEME_ROOMTRIGGER_TERM:
        _theme_prep_fgnd("roomtrigger.term", lookup_str, &bold);
        break;
    case THEME_ONLINE:
        _theme_prep_fgnd("online", lookup_str, &bold);
        break;
    case THEME_OFFLINE:
        _theme_prep_fgnd("offline", lookup_str, &bold);
        break;
    case THEME_AWAY:
        _theme_prep_fgnd("away", lookup_str, &bold);
        break;
    case THEME_CHAT:
        _theme_prep_fgnd("chat", lookup_str, &bold);
        break;
    case THEME_DND:
        _theme_prep_fgnd("dnd", lookup_str, &bold);
        break;
    case THEME_XA:
        _theme_prep_fgnd("xa", lookup_str, &bold);
        break;
    case THEME_TYPING:
        _theme_prep_fgnd("typing", lookup_str, &bold);
        break;
    case THEME_GONE:
        _theme_prep_fgnd("gone", lookup_str, &bold);
        break;
    case THEME_SUBSCRIBED:
        _theme_prep_fgnd("subscribed", lookup_str, &bold);
        break;
    case THEME_UNSUBSCRIBED:
        _theme_prep_fgnd("unsubscribed", lookup_str, &bold);
        break;
    case THEME_OTR_STARTED_TRUSTED:
        _theme_prep_fgnd("otr.started.trusted", lookup_str, &bold);
        break;
    case THEME_OTR_STARTED_UNTRUSTED:
        _theme_prep_fgnd("otr.started.untrusted", lookup_str, &bold);
        break;
    case THEME_OTR_ENDED:
        _theme_prep_fgnd("otr.ended", lookup_str, &bold);
        break;
    case THEME_OTR_TRUSTED:
        _theme_prep_fgnd("otr.trusted", lookup_str, &bold);
        break;
    case THEME_OTR_UNTRUSTED:
        _theme_prep_fgnd("otr.untrusted", lookup_str, &bold);
        break;
    case THEME_ROSTER_HEADER:
        _theme_prep_fgnd("roster.header", lookup_str, &bold);
        break;
    case THEME_ROSTER_ONLINE:
        _theme_prep_fgnd("roster.online", lookup_str, &bold);
        break;
    case THEME_ROSTER_OFFLINE:
        _theme_prep_fgnd("roster.offline", lookup_str, &bold);
        break;
    case THEME_ROSTER_CHAT:
        _theme_prep_fgnd("roster.chat", lookup_str, &bold);
        break;
    case THEME_ROSTER_AWAY:
        _theme_prep_fgnd("roster.away", lookup_str, &bold);
        break;
    case THEME_ROSTER_DND:
        _theme_prep_fgnd("roster.dnd", lookup_str, &bold);
        break;
    case THEME_ROSTER_XA:
        _theme_prep_fgnd("roster.xa", lookup_str, &bold);
        break;
    case THEME_ROSTER_ONLINE_ACTIVE:
        _theme_prep_fgnd("roster.online.active", lookup_str, &bold);
        break;
    case THEME_ROSTER_OFFLINE_ACTIVE:
        _theme_prep_fgnd("roster.offline.active", lookup_str, &bold);
        break;
    case THEME_ROSTER_CHAT_ACTIVE:
        _theme_prep_fgnd("roster.chat.active", lookup_str, &bold);
        break;
    case THEME_ROSTER_AWAY_ACTIVE:
        _theme_prep_fgnd("roster.away.active", lookup_str, &bold);
        break;
    case THEME_ROSTER_DND_ACTIVE:
        _theme_prep_fgnd("roster.dnd.active", lookup_str, &bold);
        break;
    case THEME_ROSTER_XA_ACTIVE:
        _theme_prep_fgnd("roster.xa.active", lookup_str, &bold);
        break;
    case THEME_ROSTER_ONLINE_UNREAD:
        _theme_prep_fgnd("roster.online.unread", lookup_str, &bold);
        break;
    case THEME_ROSTER_OFFLINE_UNREAD:
        _theme_prep_fgnd("roster.offline.unread", lookup_str, &bold);
        break;
    case THEME_ROSTER_CHAT_UNREAD:
        _theme_prep_fgnd("roster.chat.unread", lookup_str, &bold);
        break;
    case THEME_ROSTER_AWAY_UNREAD:
        _theme_prep_fgnd("roster.away.unread", lookup_str, &bold);
        break;
    case THEME_ROSTER_DND_UNREAD:
        _theme_prep_fgnd("roster.dnd.unread", lookup_str, &bold);
        break;
    case THEME_ROSTER_XA_UNREAD:
        _theme_prep_fgnd("roster.xa.unread", lookup_str, &bold);
        break;
    case THEME_ROSTER_ROOM:
        _theme_prep_fgnd("roster.room", lookup_str, &bold);
        break;
    case THEME_ROSTER_ROOM_UNREAD:
        _theme_prep_fgnd("roster.room.unread", lookup_str, &bold);
        break;
    case THEME_ROSTER_ROOM_TRIGGER:
        _theme_prep_fgnd("roster.room.trigger", lookup_str, &bold);
        break;
    case THEME_ROSTER_ROOM_MENTION:
        _theme_prep_fgnd("roster.room.mention", lookup_str, &bold);
        break;
    case THEME_OCCUPANTS_HEADER:
        _theme_prep_fgnd("occupants.header", lookup_str, &bold);
        break;
    case THEME_UNTRUSTED:
        _theme_prep_fgnd("untrusted", lookup_str, &bold);
        break;
    case THEME_CMD_WINS_UNREAD:
        _theme_prep_fgnd("cmd.wins.unread", lookup_str, &bold);
        break;
    case THEME_WHITE:
        g_string_append(lookup_str, "white");
        bold = FALSE;
        break;
    case THEME_WHITE_BOLD:
        g_string_append(lookup_str, "white");
        bold = TRUE;
        break;
    case THEME_GREEN:
        g_string_append(lookup_str, "green");
        bold = FALSE;
        break;
    case THEME_GREEN_BOLD:
        g_string_append(lookup_str, "green");
        bold = TRUE;
        break;
    case THEME_RED:
        g_string_append(lookup_str, "red");
        bold = FALSE;
        break;
    case THEME_RED_BOLD:
        g_string_append(lookup_str, "red");
        bold = TRUE;
        break;
    case THEME_YELLOW:
        g_string_append(lookup_str, "yellow");
        bold = FALSE;
        break;
    case THEME_YELLOW_BOLD:
        g_string_append(lookup_str, "yellow");
        bold = TRUE;
        break;
    case THEME_BLUE:
        g_string_append(lookup_str, "blue");
        bold = FALSE;
        break;
    case THEME_BLUE_BOLD:
        g_string_append(lookup_str, "blue");
        bold = TRUE;
        break;
    case THEME_CYAN:
        g_string_append(lookup_str, "cyan");
        bold = FALSE;
        break;
    case THEME_CYAN_BOLD:
        g_string_append(lookup_str, "cyan");
        bold = TRUE;
        break;
    case THEME_BLACK:
        g_string_append(lookup_str, "black");
        bold = FALSE;
        break;
    case THEME_BLACK_BOLD:
        g_string_append(lookup_str, "black");
        bold = TRUE;
        break;
    case THEME_MAGENTA:
        g_string_append(lookup_str, "magenta");
        bold = FALSE;
        break;
    case THEME_MAGENTA_BOLD:
        g_string_append(lookup_str, "magenta");
        bold = TRUE;
        break;
    default:
        g_string_append(lookup_str, "default");
        bold = FALSE;
        break;
    }

    g_string_append(lookup_str, "_");

    // append background str
    switch (attrs) {
    case THEME_TITLE_TEXT:
    case THEME_TITLE_BRACKET:
    case THEME_TITLE_SCROLLED:
    case THEME_TITLE_UNENCRYPTED:
    case THEME_TITLE_ENCRYPTED:
    case THEME_TITLE_UNTRUSTED:
    case THEME_TITLE_TRUSTED:
    case THEME_TITLE_ONLINE:
    case THEME_TITLE_OFFLINE:
    case THEME_TITLE_AWAY:
    case THEME_TITLE_CHAT:
    case THEME_TITLE_DND:
    case THEME_TITLE_XA:
        _theme_prep_bgnd("titlebar", "blue", lookup_str);
        break;
    case THEME_STATUS_TEXT:
    case THEME_STATUS_BRACKET:
    case THEME_STATUS_ACTIVE:
    case THEME_STATUS_CURRENT:
    case THEME_STATUS_NEW:
    case THEME_STATUS_TIME:
        _theme_prep_bgnd("statusbar", "blue", lookup_str);
        break;
    default:
        _theme_prep_bgnd("bkgnd", "default", lookup_str);
        break;
    }

    // lookup colour pair
    result = color_pair_cache_get(lookup_str->str);
    if (result < 0) {
        log_error("Unable to load colour theme");
        result = 0;
    }

    g_string_free(lookup_str, TRUE);

    if (bold) {
        return COLOR_PAIR(result) | A_BOLD;
    } else {
        return COLOR_PAIR(result);
    }
}
