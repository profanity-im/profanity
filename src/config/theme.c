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

struct colour_string_t {
    char *str;
    NCURSES_COLOR_T colour;
};

static int num_colours = 9;
static struct colour_string_t colours[] = {
    { "default", -1 },
    { "white", COLOR_WHITE },
    { "green", COLOR_GREEN },
    { "red", COLOR_RED },
    { "yellow", COLOR_YELLOW },
    { "blue", COLOR_BLUE },
    { "cyan", COLOR_CYAN },
    { "black", COLOR_BLACK },
    { "magenta", COLOR_MAGENTA },
};

// colour preferences
static struct colours_t {
        NCURSES_COLOR_T bkgnd;
        NCURSES_COLOR_T titlebar;
        NCURSES_COLOR_T statusbar;
        NCURSES_COLOR_T titlebartext;
        NCURSES_COLOR_T titlebarbrackets;
        NCURSES_COLOR_T titlebarunencrypted;
        NCURSES_COLOR_T titlebarencrypted;
        NCURSES_COLOR_T titlebaruntrusted;
        NCURSES_COLOR_T titlebartrusted;
        NCURSES_COLOR_T titlebaronline;
        NCURSES_COLOR_T titlebaroffline;
        NCURSES_COLOR_T titlebaraway;
        NCURSES_COLOR_T titlebarxa;
        NCURSES_COLOR_T titlebardnd;
        NCURSES_COLOR_T titlebarchat;
        NCURSES_COLOR_T statusbartext;
        NCURSES_COLOR_T statusbarbrackets;
        NCURSES_COLOR_T statusbaractive;
        NCURSES_COLOR_T statusbarnew;
        NCURSES_COLOR_T maintext;
        NCURSES_COLOR_T maintextme;
        NCURSES_COLOR_T maintextthem;
        NCURSES_COLOR_T inputtext;
        NCURSES_COLOR_T timetext;
        NCURSES_COLOR_T splashtext;
        NCURSES_COLOR_T subscribed;
        NCURSES_COLOR_T unsubscribed;
        NCURSES_COLOR_T online;
        NCURSES_COLOR_T away;
        NCURSES_COLOR_T xa;
        NCURSES_COLOR_T dnd;
        NCURSES_COLOR_T chat;
        NCURSES_COLOR_T offline;
        NCURSES_COLOR_T typing;
        NCURSES_COLOR_T gone;
        NCURSES_COLOR_T error;
        NCURSES_COLOR_T incoming;
        NCURSES_COLOR_T roominfo;
        NCURSES_COLOR_T roommention;
        NCURSES_COLOR_T me;
        NCURSES_COLOR_T them;
        NCURSES_COLOR_T otrstartedtrusted;
        NCURSES_COLOR_T otrstarteduntrusted;
        NCURSES_COLOR_T otrended;
        NCURSES_COLOR_T otrtrusted;
        NCURSES_COLOR_T otruntrusted;
        NCURSES_COLOR_T rosterheader;
        NCURSES_COLOR_T occupantsheader;
        NCURSES_COLOR_T receiptsent;
} colour_prefs;

static NCURSES_COLOR_T _lookup_colour(const char *const colour);
static void _set_colour(gchar *val, NCURSES_COLOR_T *pref, NCURSES_COLOR_T def, theme_item_t theme_item);
static void _load_colours(void);
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
    } else {
        _load_colours();
    }
}

gboolean
theme_load(const char *const theme_name)
{
    if (_theme_load_file(theme_name)) {
        _load_colours();
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
}

void
theme_init_colours(void)
{
    // main text
    init_pair(1, colour_prefs.maintext, colour_prefs.bkgnd);
    init_pair(2, colour_prefs.maintextme, colour_prefs.bkgnd);
    init_pair(3, colour_prefs.maintextthem, colour_prefs.bkgnd);
    init_pair(4, colour_prefs.splashtext, colour_prefs.bkgnd);
    init_pair(5, colour_prefs.error, colour_prefs.bkgnd);
    init_pair(6, colour_prefs.incoming, colour_prefs.bkgnd);
    init_pair(7, colour_prefs.inputtext, colour_prefs.bkgnd);
    init_pair(8, colour_prefs.timetext, colour_prefs.bkgnd);

    // title bar
    init_pair(9, colour_prefs.titlebartext, colour_prefs.titlebar);
    init_pair(10, colour_prefs.titlebarbrackets, colour_prefs.titlebar);
    init_pair(11, colour_prefs.titlebarunencrypted, colour_prefs.titlebar);
    init_pair(12, colour_prefs.titlebarencrypted, colour_prefs.titlebar);
    init_pair(13, colour_prefs.titlebaruntrusted, colour_prefs.titlebar);
    init_pair(14, colour_prefs.titlebartrusted, colour_prefs.titlebar);
    init_pair(15, colour_prefs.titlebaronline, colour_prefs.titlebar);
    init_pair(16, colour_prefs.titlebaroffline, colour_prefs.titlebar);
    init_pair(17, colour_prefs.titlebaraway, colour_prefs.titlebar);
    init_pair(18, colour_prefs.titlebarchat, colour_prefs.titlebar);
    init_pair(19, colour_prefs.titlebardnd, colour_prefs.titlebar);
    init_pair(20, colour_prefs.titlebarxa, colour_prefs.titlebar);

    // status bar
    init_pair(21, colour_prefs.statusbartext, colour_prefs.statusbar);
    init_pair(22, colour_prefs.statusbarbrackets, colour_prefs.statusbar);
    init_pair(23, colour_prefs.statusbaractive, colour_prefs.statusbar);
    init_pair(24, colour_prefs.statusbarnew, colour_prefs.statusbar);

    // chat
    init_pair(25, colour_prefs.me, colour_prefs.bkgnd);
    init_pair(26, colour_prefs.them, colour_prefs.bkgnd);
    init_pair(27, colour_prefs.receiptsent, colour_prefs.bkgnd);

    // room chat
    init_pair(28, colour_prefs.roominfo, colour_prefs.bkgnd);
    init_pair(29, colour_prefs.roommention, colour_prefs.bkgnd);

    // statuses
    init_pair(30, colour_prefs.online, colour_prefs.bkgnd);
    init_pair(31, colour_prefs.offline, colour_prefs.bkgnd);
    init_pair(32, colour_prefs.away, colour_prefs.bkgnd);
    init_pair(33, colour_prefs.chat, colour_prefs.bkgnd);
    init_pair(34, colour_prefs.dnd, colour_prefs.bkgnd);
    init_pair(35, colour_prefs.xa, colour_prefs.bkgnd);

    // states
    init_pair(36, colour_prefs.typing, colour_prefs.bkgnd);
    init_pair(37, colour_prefs.gone, colour_prefs.bkgnd);

    // subscription status
    init_pair(38, colour_prefs.subscribed, colour_prefs.bkgnd);
    init_pair(39, colour_prefs.unsubscribed, colour_prefs.bkgnd);

    // otr messages
    init_pair(40, colour_prefs.otrstartedtrusted, colour_prefs.bkgnd);
    init_pair(41, colour_prefs.otrstarteduntrusted, colour_prefs.bkgnd);
    init_pair(42, colour_prefs.otrended, colour_prefs.bkgnd);
    init_pair(43, colour_prefs.otrtrusted, colour_prefs.bkgnd);
    init_pair(44, colour_prefs.otruntrusted, colour_prefs.bkgnd);

    // subwin headers
    init_pair(45, colour_prefs.rosterheader, colour_prefs.bkgnd);
    init_pair(46, colour_prefs.occupantsheader, colour_prefs.bkgnd);

    // raw
    init_pair(47, COLOR_WHITE, colour_prefs.bkgnd);
    init_pair(48, COLOR_GREEN, colour_prefs.bkgnd);
    init_pair(49, COLOR_RED, colour_prefs.bkgnd);
    init_pair(50, COLOR_YELLOW, colour_prefs.bkgnd);
    init_pair(51, COLOR_BLUE, colour_prefs.bkgnd);
    init_pair(52, COLOR_CYAN, colour_prefs.bkgnd);
    init_pair(53, COLOR_BLACK, colour_prefs.bkgnd);
    init_pair(54, COLOR_MAGENTA, colour_prefs.bkgnd);
}

static NCURSES_COLOR_T
_lookup_colour(const char *const colour)
{
    int i;
    for (i = 0; i < num_colours; i++) {
        if (strcmp(colours[i].str, colour) == 0) {
            return colours[i].colour;
        }
    }

    return -99;
}

static void
_set_colour(char *setting, NCURSES_COLOR_T *pref, NCURSES_COLOR_T def, theme_item_t theme_item)
{
    gchar *val = g_key_file_get_string(theme, "colours", setting, NULL);

    if(!val) {
        *pref = def;
    } else {
        gchar *true_val = val;
        if (g_str_has_prefix(val, "bold_")) {
            true_val = &val[5];
            if (theme_item != THEME_NONE) {
                g_hash_table_insert(bold_items, GINT_TO_POINTER(theme_item), GINT_TO_POINTER(theme_item));
            }
        }
        NCURSES_COLOR_T col = _lookup_colour(true_val);
        if (col == -99) {
            *pref = def;
        } else {
            *pref = col;
        }
    }

    g_free(val);
}

static void
_load_colours(void)
{
    if (bold_items) {
        g_hash_table_destroy(bold_items);
    }
    bold_items = g_hash_table_new(g_direct_hash, g_direct_equal);
    g_hash_table_insert(bold_items, GINT_TO_POINTER(THEME_WHITE_BOLD),      GINT_TO_POINTER(THEME_WHITE_BOLD));
    g_hash_table_insert(bold_items, GINT_TO_POINTER(THEME_GREEN_BOLD),      GINT_TO_POINTER(THEME_GREEN_BOLD));
    g_hash_table_insert(bold_items, GINT_TO_POINTER(THEME_RED_BOLD),        GINT_TO_POINTER(THEME_RED_BOLD));
    g_hash_table_insert(bold_items, GINT_TO_POINTER(THEME_YELLOW_BOLD),     GINT_TO_POINTER(THEME_YELLOW_BOLD));
    g_hash_table_insert(bold_items, GINT_TO_POINTER(THEME_BLUE_BOLD),       GINT_TO_POINTER(THEME_BLUE_BOLD));
    g_hash_table_insert(bold_items, GINT_TO_POINTER(THEME_CYAN_BOLD),       GINT_TO_POINTER(THEME_CYAN_BOLD));
    g_hash_table_insert(bold_items, GINT_TO_POINTER(THEME_BLACK_BOLD),      GINT_TO_POINTER(THEME_BLACK_BOLD));
    g_hash_table_insert(bold_items, GINT_TO_POINTER(THEME_MAGENTA_BOLD),    GINT_TO_POINTER(THEME_MAGENTA_BOLD));

    _set_colour("bkgnd",                    &colour_prefs.bkgnd,                -1,             THEME_NONE);
    _set_colour("titlebar",                 &colour_prefs.titlebar,             COLOR_BLUE,     THEME_NONE);
    _set_colour("statusbar",                &colour_prefs.statusbar,            COLOR_BLUE,     THEME_NONE);
    _set_colour("titlebar.text",            &colour_prefs.titlebartext,         COLOR_WHITE,    THEME_TITLE_TEXT);
    _set_colour("titlebar.brackets",        &colour_prefs.titlebarbrackets,     COLOR_CYAN,     THEME_TITLE_BRACKET);
    _set_colour("titlebar.unencrypted",     &colour_prefs.titlebarunencrypted,  COLOR_RED,      THEME_TITLE_UNENCRYPTED);
    _set_colour("titlebar.encrypted",       &colour_prefs.titlebarencrypted,    COLOR_WHITE,    THEME_TITLE_ENCRYPTED);
    _set_colour("titlebar.untrusted",       &colour_prefs.titlebaruntrusted,    COLOR_YELLOW,   THEME_TITLE_UNTRUSTED);
    _set_colour("titlebar.trusted",         &colour_prefs.titlebartrusted,      COLOR_WHITE,    THEME_TITLE_TRUSTED);
    _set_colour("titlebar.online",          &colour_prefs.titlebaronline,       COLOR_WHITE,    THEME_TITLE_ONLINE);
    _set_colour("titlebar.offline",         &colour_prefs.titlebaroffline,      COLOR_WHITE,    THEME_TITLE_OFFLINE);
    _set_colour("titlebar.away",            &colour_prefs.titlebaraway,         COLOR_WHITE,    THEME_TITLE_AWAY);
    _set_colour("titlebar.chat",            &colour_prefs.titlebarchat,         COLOR_WHITE,    THEME_TITLE_CHAT);
    _set_colour("titlebar.dnd",             &colour_prefs.titlebardnd,          COLOR_WHITE,    THEME_TITLE_DND);
    _set_colour("titlebar.xa",              &colour_prefs.titlebarxa,           COLOR_WHITE,    THEME_TITLE_XA);
    _set_colour("statusbar.text",           &colour_prefs.statusbartext,        COLOR_WHITE,    THEME_STATUS_TEXT);
    _set_colour("statusbar.brackets",       &colour_prefs.statusbarbrackets,    COLOR_CYAN,     THEME_STATUS_BRACKET);
    _set_colour("statusbar.active",         &colour_prefs.statusbaractive,      COLOR_CYAN,     THEME_STATUS_ACTIVE);
    _set_colour("statusbar.new",            &colour_prefs.statusbarnew,         COLOR_WHITE,    THEME_STATUS_NEW);
    _set_colour("main.text",                &colour_prefs.maintext,             COLOR_WHITE,    THEME_TEXT);
    _set_colour("main.text.me",             &colour_prefs.maintextme,           COLOR_WHITE,    THEME_TEXT_ME);
    _set_colour("main.text.them",           &colour_prefs.maintextthem,         COLOR_WHITE,    THEME_TEXT_THEM);
    _set_colour("main.splash",              &colour_prefs.splashtext,           COLOR_CYAN,     THEME_SPLASH);
    _set_colour("input.text",               &colour_prefs.inputtext,            COLOR_WHITE,    THEME_INPUT_TEXT);
    _set_colour("main.time",                &colour_prefs.timetext,             COLOR_WHITE,    THEME_TIME);
    _set_colour("subscribed",               &colour_prefs.subscribed,           COLOR_GREEN,    THEME_SUBSCRIBED);
    _set_colour("unsubscribed",             &colour_prefs.unsubscribed,         COLOR_RED,      THEME_UNSUBSCRIBED);
    _set_colour("otr.started.trusted",      &colour_prefs.otrstartedtrusted,    COLOR_GREEN,    THEME_OTR_STARTED_TRUSTED);
    _set_colour("otr.started.untrusted",    &colour_prefs.otrstarteduntrusted,  COLOR_YELLOW,   THEME_OTR_STARTED_UNTRUSTED);
    _set_colour("otr.ended",                &colour_prefs.otrended,             COLOR_RED,      THEME_OTR_ENDED);
    _set_colour("otr.trusted",              &colour_prefs.otrtrusted,           COLOR_GREEN,    THEME_OTR_TRUSTED);
    _set_colour("otr.untrusted",            &colour_prefs.otruntrusted,         COLOR_YELLOW,   THEME_OTR_UNTRUSTED);
    _set_colour("online",                   &colour_prefs.online,               COLOR_GREEN,    THEME_ONLINE);
    _set_colour("away",                     &colour_prefs.away,                 COLOR_CYAN,     THEME_AWAY);
    _set_colour("chat",                     &colour_prefs.chat,                 COLOR_GREEN,    THEME_CHAT);
    _set_colour("dnd",                      &colour_prefs.dnd,                  COLOR_RED,      THEME_DND);
    _set_colour("xa",                       &colour_prefs.xa,                   COLOR_CYAN,     THEME_XA);
    _set_colour("offline",                  &colour_prefs.offline,              COLOR_RED,      THEME_OFFLINE);
    _set_colour("typing",                   &colour_prefs.typing,               COLOR_YELLOW,   THEME_TYPING);
    _set_colour("gone",                     &colour_prefs.gone,                 COLOR_RED,      THEME_GONE);
    _set_colour("error",                    &colour_prefs.error,                COLOR_RED,      THEME_ERROR);
    _set_colour("incoming",                 &colour_prefs.incoming,             COLOR_YELLOW,   THEME_INCOMING);
    _set_colour("roominfo",                 &colour_prefs.roominfo,             COLOR_YELLOW,   THEME_ROOMINFO);
    _set_colour("roommention",              &colour_prefs.roommention,          COLOR_YELLOW,   THEME_ROOMMENTION);
    _set_colour("me",                       &colour_prefs.me,                   COLOR_YELLOW,   THEME_ME);
    _set_colour("them",                     &colour_prefs.them,                 COLOR_GREEN,    THEME_THEM);
    _set_colour("roster.header",            &colour_prefs.rosterheader,         COLOR_YELLOW,   THEME_ROSTER_HEADER);
    _set_colour("occupants.header",         &colour_prefs.occupantsheader,      COLOR_YELLOW,   THEME_OCCUPANTS_HEADER);
    _set_colour("receipt.sent",             &colour_prefs.receiptsent,          COLOR_RED,      THEME_RECEIPT_SENT);
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
    _set_boolean_preference("roster.presence", PREF_ROSTER_PRESENCE);
    _set_boolean_preference("roster.status", PREF_ROSTER_STATUS);
    _set_boolean_preference("roster.empty", PREF_ROSTER_EMPTY);
    _set_string_preference("roster.by", PREF_ROSTER_BY);
    _set_string_preference("roster.order", PREF_ROSTER_ORDER);
    _set_boolean_preference("roster.count", PREF_ROSTER_COUNT);
    _set_boolean_preference("roster.priority", PREF_ROSTER_PRIORITY);
    if (g_key_file_has_key(theme, "ui", "roster.size", NULL)) {
        gint roster_size = g_key_file_get_integer(theme, "ui", "roster.size", NULL);
        prefs_set_roster_size(roster_size);
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

int
theme_attrs(theme_item_t attrs)
{
    int result = 0;

    switch (attrs) {
    case THEME_TEXT:                    result = COLOR_PAIR(1); break;
    case THEME_TEXT_ME:                 result = COLOR_PAIR(2); break;
    case THEME_TEXT_THEM:               result = COLOR_PAIR(3); break;
    case THEME_SPLASH:                  result = COLOR_PAIR(4); break;
    case THEME_ERROR:                   result = COLOR_PAIR(5); break;
    case THEME_INCOMING:                result = COLOR_PAIR(6); break;
    case THEME_INPUT_TEXT:              result = COLOR_PAIR(7); break;
    case THEME_TIME:                    result = COLOR_PAIR(8); break;
    case THEME_TITLE_TEXT:              result = COLOR_PAIR(9); break;
    case THEME_TITLE_BRACKET:           result = COLOR_PAIR(10); break;
    case THEME_TITLE_UNENCRYPTED:       result = COLOR_PAIR(11); break;
    case THEME_TITLE_ENCRYPTED:         result = COLOR_PAIR(12); break;
    case THEME_TITLE_UNTRUSTED:         result = COLOR_PAIR(13); break;
    case THEME_TITLE_TRUSTED:           result = COLOR_PAIR(14); break;
    case THEME_TITLE_ONLINE:            result = COLOR_PAIR(15); break;
    case THEME_TITLE_OFFLINE:           result = COLOR_PAIR(16); break;
    case THEME_TITLE_AWAY:              result = COLOR_PAIR(17); break;
    case THEME_TITLE_CHAT:              result = COLOR_PAIR(18); break;
    case THEME_TITLE_DND:               result = COLOR_PAIR(19); break;
    case THEME_TITLE_XA:                result = COLOR_PAIR(20); break;
    case THEME_STATUS_TEXT:             result = COLOR_PAIR(21); break;
    case THEME_STATUS_BRACKET:          result = COLOR_PAIR(22); break;
    case THEME_STATUS_ACTIVE:           result = COLOR_PAIR(23); break;
    case THEME_STATUS_NEW:              result = COLOR_PAIR(24); break;
    case THEME_ME:                      result = COLOR_PAIR(25); break;
    case THEME_THEM:                    result = COLOR_PAIR(26); break;
    case THEME_RECEIPT_SENT:            result = COLOR_PAIR(27); break;
    case THEME_ROOMINFO:                result = COLOR_PAIR(28); break;
    case THEME_ROOMMENTION:             result = COLOR_PAIR(29); break;
    case THEME_ONLINE:                  result = COLOR_PAIR(30); break;
    case THEME_OFFLINE:                 result = COLOR_PAIR(31); break;
    case THEME_AWAY:                    result = COLOR_PAIR(32); break;
    case THEME_CHAT:                    result = COLOR_PAIR(33); break;
    case THEME_DND:                     result = COLOR_PAIR(34); break;
    case THEME_XA:                      result = COLOR_PAIR(35); break;
    case THEME_TYPING:                  result = COLOR_PAIR(36); break;
    case THEME_GONE:                    result = COLOR_PAIR(37); break;
    case THEME_SUBSCRIBED:              result = COLOR_PAIR(38); break;
    case THEME_UNSUBSCRIBED:            result = COLOR_PAIR(39); break;
    case THEME_OTR_STARTED_TRUSTED:     result = COLOR_PAIR(40); break;
    case THEME_OTR_STARTED_UNTRUSTED:   result = COLOR_PAIR(41); break;
    case THEME_OTR_ENDED:               result = COLOR_PAIR(42); break;
    case THEME_OTR_TRUSTED:             result = COLOR_PAIR(43); break;
    case THEME_OTR_UNTRUSTED:           result = COLOR_PAIR(44); break;
    case THEME_ROSTER_HEADER:           result = COLOR_PAIR(45); break;
    case THEME_OCCUPANTS_HEADER:        result = COLOR_PAIR(46); break;
    case THEME_WHITE:                   result = COLOR_PAIR(47); break;
    case THEME_WHITE_BOLD:              result = COLOR_PAIR(47); break;
    case THEME_GREEN:                   result = COLOR_PAIR(48); break;
    case THEME_GREEN_BOLD:              result = COLOR_PAIR(48); break;
    case THEME_RED:                     result = COLOR_PAIR(49); break;
    case THEME_RED_BOLD:                result = COLOR_PAIR(49); break;
    case THEME_YELLOW:                  result = COLOR_PAIR(50); break;
    case THEME_YELLOW_BOLD:             result = COLOR_PAIR(50); break;
    case THEME_BLUE:                    result = COLOR_PAIR(51); break;
    case THEME_BLUE_BOLD:               result = COLOR_PAIR(51); break;
    case THEME_CYAN:                    result = COLOR_PAIR(52); break;
    case THEME_CYAN_BOLD:               result = COLOR_PAIR(52); break;
    case THEME_BLACK:                   result = COLOR_PAIR(53); break;
    case THEME_BLACK_BOLD:              result = COLOR_PAIR(53); break;
    case THEME_MAGENTA:                 result = COLOR_PAIR(54); break;
    case THEME_MAGENTA_BOLD:            result = COLOR_PAIR(54); break;
    default:                            break;
    }

    if (g_hash_table_lookup(bold_items, GINT_TO_POINTER(attrs))) {
        return result | A_BOLD;
    } else {
        return result;

    }
}
