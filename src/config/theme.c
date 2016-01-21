/*
 * theme.c
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
#include <string.h>

#include <glib.h>
#ifdef HAVE_NCURSESW_NCURSES_H
#include <ncursesw/ncurses.h>
#elif HAVE_NCURSES_H
#include <ncurses.h>
#endif

#include "common.h"
#include "log.h"
#include "theme.h"
#include "preferences.h"

static GString *theme_loc;
static GKeyFile *theme;
static GHashTable *bold_items;
static GHashTable *str_to_pair;

struct colour_string_t {
    char *str;
    NCURSES_COLOR_T colour;
};

static void _load_preferences(void);
static gchar* _get_themes_dir(void);
void _theme_list_dir(const gchar *const dir, GSList **result);
static GString* _theme_find(const char *const theme_name);
static gboolean _theme_load_file(const char *const theme_name);

void
theme_init(const char *const theme_name)
{
    if (!_theme_load_file(theme_name) && !_theme_load_file("default")) {
        log_error("Theme initialisation failed");
    }

    str_to_pair = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
}

gboolean
theme_load(const char *const theme_name)
{
    if (_theme_load_file(theme_name)) {
        _load_preferences();
        return TRUE;
    } else {
        return FALSE;
    }
}

static gboolean
_theme_load_file(const char *const theme_name)
{
    // use default theme
    if (theme_name == NULL || strcmp(theme_name, "default") == 0) {
        if (theme) {
            g_key_file_free(theme);
        }
        theme = g_key_file_new();

    // load theme from file
    } else {
        GString *new_theme_file = _theme_find(theme_name);
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
    GSList *result = NULL;
    char *themes_dir = _get_themes_dir();
    _theme_list_dir(themes_dir, &result);
    free(themes_dir);
#ifdef THEMES_PATH
    _theme_list_dir(THEMES_PATH, &result);
#endif
    return result;
}

void
theme_close(void)
{
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
    if (str_to_pair) {
        g_hash_table_destroy(str_to_pair);
        str_to_pair = NULL;
    }
}

static void
_theme_init_pair(short pair, short fgnd, short bgnd, char *pair_str)
{
    init_pair(pair, fgnd, bgnd);
    g_hash_table_insert(str_to_pair, strdup(pair_str), GINT_TO_POINTER((int)pair));
}

void
theme_init_colours(void)
{
    assume_default_colors(-1, -1);
    g_hash_table_insert(str_to_pair, strdup("default_default"), 0);

    _theme_init_pair(1, -1, COLOR_BLACK,                "default_black");
    _theme_init_pair(2, -1, COLOR_BLUE,                 "default_blue");
    _theme_init_pair(3, -1, COLOR_GREEN,                "default_green");
    _theme_init_pair(4, -1, COLOR_RED,                  "default_red");
    _theme_init_pair(5, -1, COLOR_CYAN,                 "default_cyan");
    _theme_init_pair(6, -1, COLOR_MAGENTA,              "default_magenta");
    _theme_init_pair(7, -1, COLOR_WHITE,                "default_white");
    _theme_init_pair(8, -1, COLOR_YELLOW,               "default_yellow");

    _theme_init_pair(9,  COLOR_BLACK, -1,               "black_default");
    _theme_init_pair(10, COLOR_BLACK, COLOR_BLACK,      "black_black");
    _theme_init_pair(11, COLOR_BLACK, COLOR_BLUE,       "black_blue");
    _theme_init_pair(12, COLOR_BLACK, COLOR_GREEN,      "black_green");
    _theme_init_pair(13, COLOR_BLACK, COLOR_RED,        "black_red");
    _theme_init_pair(14, COLOR_BLACK, COLOR_CYAN,       "black_cyan");
    _theme_init_pair(15, COLOR_BLACK, COLOR_MAGENTA,    "black_magenta");
    _theme_init_pair(16, COLOR_BLACK, COLOR_WHITE,      "black_white");
    _theme_init_pair(17, COLOR_BLACK, COLOR_YELLOW,     "black_yellow");

    _theme_init_pair(18, COLOR_BLUE, -1,                "blue_default");
    _theme_init_pair(19, COLOR_BLUE, COLOR_BLACK,       "blue_black");
    _theme_init_pair(20, COLOR_BLUE, COLOR_BLUE,        "blue_blue");
    _theme_init_pair(21, COLOR_BLUE, COLOR_GREEN,       "blue_green");
    _theme_init_pair(22, COLOR_BLUE, COLOR_RED,         "blue_red");
    _theme_init_pair(23, COLOR_BLUE, COLOR_CYAN,        "blue_cyan");
    _theme_init_pair(24, COLOR_BLUE, COLOR_MAGENTA,     "blue_magenta");
    _theme_init_pair(25, COLOR_BLUE, COLOR_WHITE,       "blue_white");
    _theme_init_pair(26, COLOR_BLUE, COLOR_YELLOW,      "blue_yellow");

    _theme_init_pair(27, COLOR_GREEN, -1,               "green_default");
    _theme_init_pair(28, COLOR_GREEN, COLOR_BLACK,      "green_black");
    _theme_init_pair(29, COLOR_GREEN, COLOR_BLUE,       "green_blue");
    _theme_init_pair(30, COLOR_GREEN, COLOR_GREEN,      "green_green");
    _theme_init_pair(31, COLOR_GREEN, COLOR_RED,        "green_red");
    _theme_init_pair(32, COLOR_GREEN, COLOR_CYAN,       "green_cyan");
    _theme_init_pair(33, COLOR_GREEN, COLOR_MAGENTA,    "green_magenta");
    _theme_init_pair(34, COLOR_GREEN, COLOR_WHITE,      "green_white");
    _theme_init_pair(35, COLOR_GREEN, COLOR_YELLOW,     "green_yellow");

    _theme_init_pair(36, COLOR_RED, -1,                 "red_default");
    _theme_init_pair(37, COLOR_RED, COLOR_BLACK,        "red_black");
    _theme_init_pair(38, COLOR_RED, COLOR_BLUE,         "red_blue");
    _theme_init_pair(39, COLOR_RED, COLOR_GREEN,        "red_green");
    _theme_init_pair(40, COLOR_RED, COLOR_RED,          "red_red");
    _theme_init_pair(41, COLOR_RED, COLOR_CYAN,         "red_cyan");
    _theme_init_pair(42, COLOR_RED, COLOR_MAGENTA,      "red_magenta");
    _theme_init_pair(43, COLOR_RED, COLOR_WHITE,        "red_white");
    _theme_init_pair(44, COLOR_RED, COLOR_YELLOW,       "red_yellow");

    _theme_init_pair(45, COLOR_CYAN, -1,                "cyan_default");
    _theme_init_pair(46, COLOR_CYAN, COLOR_BLACK,       "cyan_black");
    _theme_init_pair(47, COLOR_CYAN, COLOR_BLUE,        "cyan_blue");
    _theme_init_pair(48, COLOR_CYAN, COLOR_GREEN,       "cyan_green");
    _theme_init_pair(49, COLOR_CYAN, COLOR_RED,         "cyan_red");
    _theme_init_pair(50, COLOR_CYAN, COLOR_CYAN,        "cyan_cyan");
    _theme_init_pair(51, COLOR_CYAN, COLOR_MAGENTA,     "cyan_magenta");
    _theme_init_pair(52, COLOR_CYAN, COLOR_WHITE,       "cyan_white");
    _theme_init_pair(53, COLOR_CYAN, COLOR_YELLOW,      "cyan_yellow");

    _theme_init_pair(54, COLOR_MAGENTA, -1,             "magenta_default");
    _theme_init_pair(55, COLOR_MAGENTA, COLOR_BLACK,    "magenta_black");
    _theme_init_pair(56, COLOR_MAGENTA, COLOR_BLUE,     "magenta_blue");
    _theme_init_pair(57, COLOR_MAGENTA, COLOR_GREEN,    "magenta_green");
    _theme_init_pair(58, COLOR_MAGENTA, COLOR_RED,      "magenta_red");
    _theme_init_pair(59, COLOR_MAGENTA, COLOR_CYAN,     "magenta_cyan");
    _theme_init_pair(60, COLOR_MAGENTA, COLOR_MAGENTA,  "magenta_magenta");
    _theme_init_pair(61, COLOR_MAGENTA, COLOR_WHITE,    "magenta_white");
    _theme_init_pair(62, COLOR_MAGENTA, COLOR_YELLOW,   "magenta_yellow");

    _theme_init_pair(63, COLOR_WHITE, -1,               "white_default");
    _theme_init_pair(64, COLOR_WHITE, COLOR_BLACK,      "white_black");
    _theme_init_pair(65, COLOR_WHITE, COLOR_BLUE,       "white_blue");
    _theme_init_pair(66, COLOR_WHITE, COLOR_GREEN,      "white_green");
    _theme_init_pair(67, COLOR_WHITE, COLOR_RED,        "white_red");
    _theme_init_pair(68, COLOR_WHITE, COLOR_CYAN,       "white_cyan");
    _theme_init_pair(69, COLOR_WHITE, COLOR_MAGENTA,    "white_magenta");
    _theme_init_pair(70, COLOR_WHITE, COLOR_WHITE,      "white_white");
    _theme_init_pair(71, COLOR_WHITE, COLOR_YELLOW,     "white_yellow");

    _theme_init_pair(72, COLOR_YELLOW, -1,              "yellow_default");
    _theme_init_pair(73, COLOR_YELLOW, COLOR_BLACK,     "yellow_black");
    _theme_init_pair(74, COLOR_YELLOW, COLOR_BLUE,      "yellow_blue");
    _theme_init_pair(75, COLOR_YELLOW, COLOR_GREEN,     "yellow_green");
    _theme_init_pair(76, COLOR_YELLOW, COLOR_RED,       "yellow_red");
    _theme_init_pair(77, COLOR_YELLOW, COLOR_CYAN,      "yellow_cyan");
    _theme_init_pair(78, COLOR_YELLOW, COLOR_MAGENTA,   "yellow_magenta");
    _theme_init_pair(79, COLOR_YELLOW, COLOR_WHITE,     "yellow_white");
    _theme_init_pair(80, COLOR_YELLOW, COLOR_YELLOW,    "yellow_yellow");
}

static void
_set_string_preference(char *prefstr, preference_t pref)
{
    if (g_key_file_has_key(theme, "ui", prefstr, NULL)) {
        gchar *val = g_key_file_get_string(theme, "ui", prefstr, NULL);
        prefs_set_string(pref, val);
        g_free(val);
    }
}

static void
_set_boolean_preference(char *prefstr, preference_t pref)
{
    if (g_key_file_has_key(theme, "ui", prefstr, NULL)) {
        gboolean val = g_key_file_get_boolean(theme, "ui", prefstr, NULL);
        prefs_set_boolean(pref, val);
    }
}

static void
_load_preferences(void)
{
    _set_boolean_preference("beep", PREF_BEEP);
    _set_boolean_preference("flash", PREF_FLASH);
    _set_boolean_preference("splash", PREF_SPLASH);
    _set_boolean_preference("wrap", PREF_WRAP);
    _set_boolean_preference("wins.autotidy", PREF_WINS_AUTO_TIDY);
    _set_string_preference("time.console", PREF_TIME_CONSOLE);
    _set_string_preference("time.chat", PREF_TIME_CHAT);
    _set_string_preference("time.muc", PREF_TIME_MUC);
    _set_string_preference("time.mucconfig", PREF_TIME_MUCCONFIG);
    _set_string_preference("time.private", PREF_TIME_PRIVATE);
    _set_string_preference("time.xmlconsole", PREF_TIME_XMLCONSOLE);
    _set_string_preference("time.statusbar", PREF_TIME_STATUSBAR);
    _set_string_preference("time.lastactivity", PREF_TIME_LASTACTIVITY);

    _set_boolean_preference("resource.title", PREF_RESOURCE_TITLE);
    _set_boolean_preference("resource.message", PREF_RESOURCE_MESSAGE);

    _set_string_preference("statuses.console", PREF_STATUSES_CONSOLE);
    _set_string_preference("statuses.chat", PREF_STATUSES_CHAT);
    _set_string_preference("statuses.muc", PREF_STATUSES_MUC);

    _set_boolean_preference("occupants", PREF_OCCUPANTS);
    _set_boolean_preference("occupants.jid", PREF_OCCUPANTS_JID);
    if (g_key_file_has_key(theme, "ui", "occupants.size", NULL)) {
        gint occupants_size = g_key_file_get_integer(theme, "ui", "occupants.size", NULL);
        prefs_set_occupants_size(occupants_size);
    }

    _set_boolean_preference("roster", PREF_ROSTER);
    _set_boolean_preference("roster.offline", PREF_ROSTER_OFFLINE);
    _set_boolean_preference("roster.resource", PREF_ROSTER_RESOURCE);
    _set_boolean_preference("roster.resource.join", PREF_ROSTER_RESOURCE_JOIN);
    _set_boolean_preference("roster.presence", PREF_ROSTER_PRESENCE);
    _set_boolean_preference("roster.status", PREF_ROSTER_STATUS);
    _set_boolean_preference("roster.empty", PREF_ROSTER_EMPTY);
    _set_boolean_preference("roster.wrap", PREF_ROSTER_WRAP);
    _set_string_preference("roster.by", PREF_ROSTER_BY);
    _set_string_preference("roster.order", PREF_ROSTER_ORDER);
    _set_string_preference("roster.unread", PREF_ROSTER_UNREAD);
    _set_boolean_preference("roster.count", PREF_ROSTER_COUNT);
    _set_boolean_preference("roster.priority", PREF_ROSTER_PRIORITY);
    _set_boolean_preference("roster.contacts", PREF_ROSTER_CONTACTS);
    _set_boolean_preference("roster.rooms", PREF_ROSTER_ROOMS);
    _set_string_preference("roster.rooms.order", PREF_ROSTER_ROOMS_ORDER);
    _set_string_preference("roster.rooms.unread", PREF_ROSTER_ROOMS_UNREAD);
    _set_string_preference("roster.rooms.pos", PREF_ROSTER_ROOMS_POS);
    if (g_key_file_has_key(theme, "ui", "roster.size", NULL)) {
        gint roster_size = g_key_file_get_integer(theme, "ui", "roster.size", NULL);
        prefs_set_roster_size(roster_size);
    }
    if (g_key_file_has_key(theme, "ui", "roster.header.char", NULL)) {
        gchar *ch = g_key_file_get_string(theme, "ui", "roster.header.char", NULL);
        if (ch && strlen(ch) > 0) {
            prefs_set_roster_header_char(ch[0]);
            g_free(ch);
        }
    } else {
        prefs_clear_roster_header_char();
    }
    if (g_key_file_has_key(theme, "ui", "roster.contact.char", NULL)) {
        gchar *ch = g_key_file_get_string(theme, "ui", "roster.contact.char", NULL);
        if (ch && strlen(ch) > 0) {
            prefs_set_roster_contact_char(ch[0]);
            g_free(ch);
        }
    } else {
        prefs_clear_roster_contact_char();
    }
    if (g_key_file_has_key(theme, "ui", "roster.resource.char", NULL)) {
        gchar *ch = g_key_file_get_string(theme, "ui", "roster.resource.char", NULL);
        if (ch && strlen(ch) > 0) {
            prefs_set_roster_resource_char(ch[0]);
            g_free(ch);
        }
    } else {
        prefs_clear_roster_resource_char();
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

    _set_boolean_preference("privileges", PREF_MUC_PRIVILEGES);

    _set_boolean_preference("presence", PREF_PRESENCE);
    _set_boolean_preference("intype", PREF_INTYPE);

    _set_boolean_preference("enc.warn", PREF_ENC_WARN);
    _set_boolean_preference("tls.show", PREF_TLS_SHOW);

    if (g_key_file_has_key(theme, "ui", "otr.char", NULL)) {
        gchar *ch = g_key_file_get_string(theme, "ui", "otr.char", NULL);
        if (ch && strlen(ch) > 0) {
            prefs_set_otr_char(ch[0]);
            g_free(ch);
        }
    }
    if (g_key_file_has_key(theme, "ui", "pgp.char", NULL)) {
        gchar *ch = g_key_file_get_string(theme, "ui", "pgp.char", NULL);
        if (ch && strlen(ch) > 0) {
            prefs_set_pgp_char(ch[0]);
            g_free(ch);
        }
    }

    _set_string_preference("console.muc", PREF_CONSOLE_MUC);
}

static gchar*
_get_themes_dir(void)
{
    gchar *xdg_config = xdg_get_config_home();
    GString *themes_dir = g_string_new(xdg_config);
    g_free(xdg_config);
    g_string_append(themes_dir, "/profanity/themes");
    return g_string_free(themes_dir, FALSE);
}

void
_theme_list_dir(const gchar *const dir, GSList **result)
{
    GDir *themes = g_dir_open(dir, 0, NULL);
    if (themes) {
        const gchar *theme = g_dir_read_name(themes);
        while (theme) {
            *result = g_slist_append(*result, strdup(theme));
            theme = g_dir_read_name(themes);
        }
        g_dir_close(themes);
    }
}

static GString*
_theme_find(const char *const theme_name)
{
    GString *path = NULL;
    gchar *themes_dir = _get_themes_dir();

    if (themes_dir) {
        path = g_string_new(themes_dir);
        g_free(themes_dir);
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
theme_roster_unread_presence_attrs(const char *const presence)
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
theme_roster_active_presence_attrs(const char *const presence)
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
theme_roster_presence_attrs(const char *const presence)
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
theme_main_presence_attrs(const char *const presence)
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
_theme_prep_bgnd(char *setting, char *def, GString *lookup_str)
{
    gchar *val = g_key_file_get_string(theme, "colours", setting, NULL);
    if (!val) {
        g_string_append(lookup_str, def);
    } else {
        if (g_str_has_prefix(val, "bold_")) {
            g_string_append(lookup_str, &val[5]);
        } else {
            g_string_append(lookup_str, val);
        }
    }
    g_free(val);
}

static void
_theme_prep_fgnd(char *setting, char *def, GString *lookup_str, gboolean *bold)
{
    gchar *val = g_key_file_get_string(theme, "colours", setting, NULL);
    if (!val) {
        g_string_append(lookup_str, def);
    } else {
        if (g_str_has_prefix(val, "bold_")) {
            g_string_append(lookup_str, &val[5]);
            *bold = TRUE;
        } else {
            g_string_append(lookup_str, val);
            *bold = FALSE;
        }
    }
    g_free(val);
}

char*
theme_get_string(char *str)
{
    return g_key_file_get_string(theme, "colours", str, NULL);
}

void
theme_free_string(char *str)
{
    if (str) {
        g_free(str);
    }
}

int
theme_attrs(theme_item_t attrs)
{
    int result = 0;

    GString *lookup_str = g_string_new("");
    gboolean bold = FALSE;

    // get forground colour
    switch (attrs) {
    case THEME_TEXT:                    _theme_prep_fgnd("main.text",               "white",    lookup_str, &bold); break;
    case THEME_TEXT_ME:                 _theme_prep_fgnd("main.text.me",            "white",    lookup_str, &bold); break;
    case THEME_TEXT_THEM:               _theme_prep_fgnd("main.text.them",          "white",    lookup_str, &bold); break;
    case THEME_SPLASH:                  _theme_prep_fgnd("main.splash",             "cyan",     lookup_str, &bold); break;
    case THEME_ERROR:                   _theme_prep_fgnd("error",                   "red",      lookup_str, &bold); break;
    case THEME_INCOMING:                _theme_prep_fgnd("incoming",                "yellow",   lookup_str, &bold); break;
    case THEME_INPUT_TEXT:              _theme_prep_fgnd("input.text",              "white",    lookup_str, &bold); break;
    case THEME_TIME:                    _theme_prep_fgnd("main.time",               "white",    lookup_str, &bold); break;
    case THEME_TITLE_TEXT:              _theme_prep_fgnd("titlebar.text",           "white",    lookup_str, &bold); break;
    case THEME_TITLE_BRACKET:           _theme_prep_fgnd("titlebar.brackets",       "cyan",     lookup_str, &bold); break;
    case THEME_TITLE_UNENCRYPTED:       _theme_prep_fgnd("titlebar.unencrypted",    "red",      lookup_str, &bold); break;
    case THEME_TITLE_ENCRYPTED:         _theme_prep_fgnd("titlebar.encrypted",      "white",    lookup_str, &bold); break;
    case THEME_TITLE_UNTRUSTED:         _theme_prep_fgnd("titlebar.untrusted",      "yellow",   lookup_str, &bold); break;
    case THEME_TITLE_TRUSTED:           _theme_prep_fgnd("titlebar.trusted",        "white",    lookup_str, &bold); break;
    case THEME_TITLE_ONLINE:            _theme_prep_fgnd("titlebar.online",         "white",    lookup_str, &bold); break;
    case THEME_TITLE_OFFLINE:           _theme_prep_fgnd("titlebar.offline",        "white",    lookup_str, &bold); break;
    case THEME_TITLE_AWAY:              _theme_prep_fgnd("titlebar.away",           "white",    lookup_str, &bold); break;
    case THEME_TITLE_CHAT:              _theme_prep_fgnd("titlebar.chat",           "white",    lookup_str, &bold); break;
    case THEME_TITLE_DND:               _theme_prep_fgnd("titlebar.dnd",            "white",    lookup_str, &bold); break;
    case THEME_TITLE_XA:                _theme_prep_fgnd("titlebar.xa",             "white",    lookup_str, &bold); break;
    case THEME_STATUS_TEXT:             _theme_prep_fgnd("statusbar.text",          "white",    lookup_str, &bold); break;
    case THEME_STATUS_BRACKET:          _theme_prep_fgnd("statusbar.brackets",      "cyan",     lookup_str, &bold); break;
    case THEME_STATUS_ACTIVE:           _theme_prep_fgnd("statusbar.active",        "cyan",     lookup_str, &bold); break;
    case THEME_STATUS_NEW:              _theme_prep_fgnd("statusbar.new",           "white",    lookup_str, &bold); break;
    case THEME_ME:                      _theme_prep_fgnd("me",                      "yellow",   lookup_str, &bold); break;
    case THEME_THEM:                    _theme_prep_fgnd("them",                    "green",    lookup_str, &bold); break;
    case THEME_RECEIPT_SENT:            _theme_prep_fgnd("receipt.sent",            "red",      lookup_str, &bold); break;
    case THEME_ROOMINFO:                _theme_prep_fgnd("roominfo",                "yellow",   lookup_str, &bold); break;
    case THEME_ROOMMENTION:             _theme_prep_fgnd("roommention",             "yellow",   lookup_str, &bold); break;
    case THEME_ONLINE:                  _theme_prep_fgnd("online",                  "green",    lookup_str, &bold); break;
    case THEME_OFFLINE:                 _theme_prep_fgnd("offline",                 "red",      lookup_str, &bold); break;
    case THEME_AWAY:                    _theme_prep_fgnd("away",                    "cyan",     lookup_str, &bold); break;
    case THEME_CHAT:                    _theme_prep_fgnd("chat",                    "green",    lookup_str, &bold); break;
    case THEME_DND:                     _theme_prep_fgnd("dnd",                     "red",      lookup_str, &bold); break;
    case THEME_XA:                      _theme_prep_fgnd("xa",                      "cyan",     lookup_str, &bold); break;
    case THEME_TYPING:                  _theme_prep_fgnd("typing",                  "yellow",   lookup_str, &bold); break;
    case THEME_GONE:                    _theme_prep_fgnd("gone",                    "red",      lookup_str, &bold); break;
    case THEME_SUBSCRIBED:              _theme_prep_fgnd("subscribed",              "green",    lookup_str, &bold); break;
    case THEME_UNSUBSCRIBED:            _theme_prep_fgnd("unsubscribed",            "red",      lookup_str, &bold); break;
    case THEME_OTR_STARTED_TRUSTED:     _theme_prep_fgnd("otr.started.trusted",     "green",    lookup_str, &bold); break;
    case THEME_OTR_STARTED_UNTRUSTED:   _theme_prep_fgnd("otr.started.untrusted",   "yellow",   lookup_str, &bold); break;
    case THEME_OTR_ENDED:               _theme_prep_fgnd("otr.ended",               "red",      lookup_str, &bold); break;
    case THEME_OTR_TRUSTED:             _theme_prep_fgnd("otr.trusted",             "green",    lookup_str, &bold); break;
    case THEME_OTR_UNTRUSTED:           _theme_prep_fgnd("otr.untrusted",           "yellow",   lookup_str, &bold); break;
    case THEME_ROSTER_HEADER:           _theme_prep_fgnd("roster.header",           "yellow",   lookup_str, &bold); break;
    case THEME_ROSTER_ONLINE:           _theme_prep_fgnd("roster.online",           "green",    lookup_str, &bold); break;
    case THEME_ROSTER_OFFLINE:          _theme_prep_fgnd("roster.offline",          "red",      lookup_str, &bold); break;
    case THEME_ROSTER_CHAT:             _theme_prep_fgnd("roster.chat",             "green",    lookup_str, &bold); break;
    case THEME_ROSTER_AWAY:             _theme_prep_fgnd("roster.away",             "cyan",     lookup_str, &bold); break;
    case THEME_ROSTER_DND:              _theme_prep_fgnd("roster.dnd",              "red",      lookup_str, &bold); break;
    case THEME_ROSTER_XA:               _theme_prep_fgnd("roster.xa",               "cyan",     lookup_str, &bold); break;
    case THEME_ROSTER_ONLINE_ACTIVE:    _theme_prep_fgnd("roster.online.active",    "green",    lookup_str, &bold); break;
    case THEME_ROSTER_OFFLINE_ACTIVE:   _theme_prep_fgnd("roster.offline.active",   "red",      lookup_str, &bold); break;
    case THEME_ROSTER_CHAT_ACTIVE:      _theme_prep_fgnd("roster.chat.active",      "green",    lookup_str, &bold); break;
    case THEME_ROSTER_AWAY_ACTIVE:      _theme_prep_fgnd("roster.away.active",      "cyan",     lookup_str, &bold); break;
    case THEME_ROSTER_DND_ACTIVE:       _theme_prep_fgnd("roster.dnd.active",       "red",      lookup_str, &bold); break;
    case THEME_ROSTER_XA_ACTIVE:        _theme_prep_fgnd("roster.xa.active",        "cyan",     lookup_str, &bold); break;
    case THEME_ROSTER_ONLINE_UNREAD:    _theme_prep_fgnd("roster.online.unread",    "green",    lookup_str, &bold); break;
    case THEME_ROSTER_OFFLINE_UNREAD:   _theme_prep_fgnd("roster.offline.unread",   "red",      lookup_str, &bold); break;
    case THEME_ROSTER_CHAT_UNREAD:      _theme_prep_fgnd("roster.chat.unread",      "green",    lookup_str, &bold); break;
    case THEME_ROSTER_AWAY_UNREAD:      _theme_prep_fgnd("roster.away.unread",      "cyan",     lookup_str, &bold); break;
    case THEME_ROSTER_DND_UNREAD:       _theme_prep_fgnd("roster.dnd.unread",       "red",      lookup_str, &bold); break;
    case THEME_ROSTER_XA_UNREAD:        _theme_prep_fgnd("roster.xa.unread",        "cyan",     lookup_str, &bold); break;
    case THEME_ROSTER_ROOM:             _theme_prep_fgnd("roster.room",             "green",    lookup_str, &bold); break;
    case THEME_ROSTER_ROOM_UNREAD:      _theme_prep_fgnd("roster.room.unread",      "green",    lookup_str, &bold); break;
    case THEME_OCCUPANTS_HEADER:        _theme_prep_fgnd("occupants.header",        "yellow",   lookup_str, &bold); break;
    case THEME_WHITE:                   g_string_append(lookup_str, "white");   bold = FALSE;   break;
    case THEME_WHITE_BOLD:              g_string_append(lookup_str, "white");   bold = TRUE;    break;
    case THEME_GREEN:                   g_string_append(lookup_str, "green");   bold = FALSE;   break;
    case THEME_GREEN_BOLD:              g_string_append(lookup_str, "green");   bold = TRUE;    break;
    case THEME_RED:                     g_string_append(lookup_str, "red");     bold = FALSE;   break;
    case THEME_RED_BOLD:                g_string_append(lookup_str, "red");     bold = TRUE;    break;
    case THEME_YELLOW:                  g_string_append(lookup_str, "yellow");  bold = FALSE;   break;
    case THEME_YELLOW_BOLD:             g_string_append(lookup_str, "yellow");  bold = TRUE;    break;
    case THEME_BLUE:                    g_string_append(lookup_str, "blue");    bold = FALSE;   break;
    case THEME_BLUE_BOLD:               g_string_append(lookup_str, "blue");    bold = TRUE;    break;
    case THEME_CYAN:                    g_string_append(lookup_str, "cyan");    bold = FALSE;   break;
    case THEME_CYAN_BOLD:               g_string_append(lookup_str, "cyan");    bold = TRUE;    break;
    case THEME_BLACK:                   g_string_append(lookup_str, "black");   bold = FALSE;   break;
    case THEME_BLACK_BOLD:              g_string_append(lookup_str, "black");   bold = TRUE;    break;
    case THEME_MAGENTA:                 g_string_append(lookup_str, "magenta"); bold = FALSE;   break;
    case THEME_MAGENTA_BOLD:            g_string_append(lookup_str, "magenta"); bold = TRUE;    break;
    default:                            g_string_append(lookup_str, "default"); bold = FALSE;   break;
    }

    g_string_append(lookup_str, "_");

    // append background str
    switch (attrs) {
    case THEME_TITLE_TEXT:
    case THEME_TITLE_BRACKET:
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
    case THEME_STATUS_NEW:
        _theme_prep_bgnd("statusbar", "blue", lookup_str);
        break;
    default:
        _theme_prep_bgnd("bkgnd", "default", lookup_str);
        break;
    }

    // lookup colour pair
    result = GPOINTER_TO_INT(g_hash_table_lookup(str_to_pair, lookup_str->str));
    g_string_free(lookup_str, TRUE);
    if (bold) {
        return COLOR_PAIR(result) | A_BOLD;
    } else {
        return COLOR_PAIR(result);
    }
}
