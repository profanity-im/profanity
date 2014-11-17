/*
 * theme.c
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
} colour_prefs;

static NCURSES_COLOR_T _lookup_colour(const char * const colour);
static void _set_colour(gchar *val, NCURSES_COLOR_T *pref, NCURSES_COLOR_T def, theme_item_t theme_item);
static void _load_colours(void);
static gchar * _get_themes_dir(void);
void _theme_list_dir(const gchar * const dir, GSList **result);
static GString * _theme_find(const char * const theme_name);

void
theme_init(const char * const theme_name)
{
    if (!theme_load(theme_name) && !theme_load("default")) {
        log_error("Theme initialisation failed");
    }
}

gboolean
theme_load(const char * const theme_name)
{
    // use default theme
    if (theme_name == NULL || strcmp(theme_name, "default") == 0) {
        if (theme != NULL) {
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

        if (theme_loc != NULL) {
            g_string_free(theme_loc, TRUE);
        }
        theme_loc = new_theme_file;
        log_info("Loading theme \"%s\"", theme_name);
        if (theme != NULL) {
            g_key_file_free(theme);
        }
        theme = g_key_file_new();
        g_key_file_load_from_file(theme, theme_loc->str, G_KEY_FILE_KEEP_COMMENTS,
            NULL);
    }

    _load_colours();
    return TRUE;
}

GSList *
theme_list(void)
{
    GSList *result = NULL;
    _theme_list_dir(_get_themes_dir(), &result);
#ifdef THEMES_PATH
    _theme_list_dir(THEMES_PATH, &result);
#endif
    return result;
}

void
theme_close(void)
{
    if (theme != NULL) {
        g_key_file_free(theme);
    }
    if (theme_loc != NULL) {
        g_string_free(theme_loc, TRUE);
    }
    if (bold_items) {
        g_hash_table_destroy(bold_items);
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

    // room chat
    init_pair(27, colour_prefs.roominfo, colour_prefs.bkgnd);
    init_pair(28, colour_prefs.roommention, colour_prefs.bkgnd);

    // statuses
    init_pair(29, colour_prefs.online, colour_prefs.bkgnd);
    init_pair(30, colour_prefs.offline, colour_prefs.bkgnd);
    init_pair(31, colour_prefs.away, colour_prefs.bkgnd);
    init_pair(32, colour_prefs.chat, colour_prefs.bkgnd);
    init_pair(33, colour_prefs.dnd, colour_prefs.bkgnd);
    init_pair(34, colour_prefs.xa, colour_prefs.bkgnd);

    // states
    init_pair(35, colour_prefs.typing, colour_prefs.bkgnd);
    init_pair(36, colour_prefs.gone, colour_prefs.bkgnd);

    // subscription status
    init_pair(37, colour_prefs.subscribed, colour_prefs.bkgnd);
    init_pair(38, colour_prefs.unsubscribed, colour_prefs.bkgnd);

    // otr messages
    init_pair(39, colour_prefs.otrstartedtrusted, colour_prefs.bkgnd);
    init_pair(40, colour_prefs.otrstarteduntrusted, colour_prefs.bkgnd);
    init_pair(41, colour_prefs.otrended, colour_prefs.bkgnd);
    init_pair(42, colour_prefs.otrtrusted, colour_prefs.bkgnd);
    init_pair(43, colour_prefs.otruntrusted, colour_prefs.bkgnd);

    // subwin headers
    init_pair(44, colour_prefs.rosterheader, colour_prefs.bkgnd);
    init_pair(45, colour_prefs.occupantsheader, colour_prefs.bkgnd);

    // raw
    init_pair(46, COLOR_WHITE, colour_prefs.bkgnd);
    init_pair(47, COLOR_GREEN, colour_prefs.bkgnd);
    init_pair(48, COLOR_RED, colour_prefs.bkgnd);
    init_pair(49, COLOR_YELLOW, colour_prefs.bkgnd);
    init_pair(50, COLOR_BLUE, colour_prefs.bkgnd);
    init_pair(51, COLOR_CYAN, colour_prefs.bkgnd);
    init_pair(52, COLOR_BLACK, colour_prefs.bkgnd);
    init_pair(53, COLOR_MAGENTA, colour_prefs.bkgnd);
}

static NCURSES_COLOR_T
_lookup_colour(const char * const colour)
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
_set_colour(gchar *val, NCURSES_COLOR_T *pref, NCURSES_COLOR_T def, theme_item_t theme_item)
{
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
}

static void
_load_colours(void)
{
    if (bold_items) {
        g_hash_table_destroy(bold_items);
    }
    bold_items = g_hash_table_new(g_direct_hash, g_direct_equal);
    g_hash_table_insert(bold_items, GINT_TO_POINTER(THEME_WHITE_BOLD), GINT_TO_POINTER(THEME_WHITE_BOLD));
    g_hash_table_insert(bold_items, GINT_TO_POINTER(THEME_GREEN_BOLD), GINT_TO_POINTER(THEME_GREEN_BOLD));
    g_hash_table_insert(bold_items, GINT_TO_POINTER(THEME_RED_BOLD), GINT_TO_POINTER(THEME_RED_BOLD));
    g_hash_table_insert(bold_items, GINT_TO_POINTER(THEME_YELLOW_BOLD), GINT_TO_POINTER(THEME_YELLOW_BOLD));
    g_hash_table_insert(bold_items, GINT_TO_POINTER(THEME_BLUE_BOLD), GINT_TO_POINTER(THEME_BLUE_BOLD));
    g_hash_table_insert(bold_items, GINT_TO_POINTER(THEME_CYAN_BOLD), GINT_TO_POINTER(THEME_CYAN_BOLD));
    g_hash_table_insert(bold_items, GINT_TO_POINTER(THEME_BLACK_BOLD), GINT_TO_POINTER(THEME_BLACK_BOLD));
    g_hash_table_insert(bold_items, GINT_TO_POINTER(THEME_MAGENTA_BOLD), GINT_TO_POINTER(THEME_MAGENTA_BOLD));

    gchar *bkgnd_val = g_key_file_get_string(theme, "colours", "bkgnd", NULL);
    _set_colour(bkgnd_val, &colour_prefs.bkgnd, -1, THEME_NONE);
    g_free(bkgnd_val);

    gchar *titlebar_val = g_key_file_get_string(theme, "colours", "titlebar", NULL);
    _set_colour(titlebar_val, &colour_prefs.titlebar, COLOR_BLUE, THEME_NONE);
    g_free(titlebar_val);

    gchar *statusbar_val = g_key_file_get_string(theme, "colours", "statusbar", NULL);
    _set_colour(statusbar_val, &colour_prefs.statusbar, COLOR_BLUE, THEME_NONE);
    g_free(statusbar_val);

    gchar *titlebartext_val = g_key_file_get_string(theme, "colours", "titlebar.text", NULL);
    _set_colour(titlebartext_val, &colour_prefs.titlebartext, COLOR_WHITE, THEME_TITLE_TEXT);
    g_free(titlebartext_val);

    gchar *titlebarbrackets_val = g_key_file_get_string(theme, "colours", "titlebar.brackets", NULL);
    _set_colour(titlebarbrackets_val, &colour_prefs.titlebarbrackets, COLOR_CYAN, THEME_TITLE_BRACKET);
    g_free(titlebarbrackets_val);

    gchar *titlebarunencrypted_val = g_key_file_get_string(theme, "colours", "titlebar.unencrypted", NULL);
    _set_colour(titlebarunencrypted_val, &colour_prefs.titlebarunencrypted, COLOR_RED, THEME_TITLE_UNENCRYPTED);
    g_free(titlebarunencrypted_val);

    gchar *titlebarencrypted_val = g_key_file_get_string(theme, "colours", "titlebar.encrypted", NULL);
    _set_colour(titlebarencrypted_val, &colour_prefs.titlebarencrypted, COLOR_WHITE, THEME_TITLE_ENCRYPTED);
    g_free(titlebarencrypted_val);

    gchar *titlebaruntrusted_val = g_key_file_get_string(theme, "colours", "titlebar.untrusted", NULL);
    _set_colour(titlebaruntrusted_val, &colour_prefs.titlebaruntrusted, COLOR_YELLOW, THEME_TITLE_UNTRUSTED);
    g_free(titlebaruntrusted_val);

    gchar *titlebartrusted_val = g_key_file_get_string(theme, "colours", "titlebar.trusted", NULL);
    _set_colour(titlebartrusted_val, &colour_prefs.titlebartrusted, COLOR_WHITE, THEME_TITLE_TRUSTED);
    g_free(titlebartrusted_val);

    gchar *titlebaronline_val = g_key_file_get_string(theme, "colours", "titlebar.online", NULL);
    _set_colour(titlebaronline_val, &colour_prefs.titlebaronline, COLOR_WHITE, THEME_TITLE_ONLINE);
    g_free(titlebaronline_val);

    gchar *titlebaroffline_val = g_key_file_get_string(theme, "colours", "titlebar.offline", NULL);
    _set_colour(titlebaroffline_val, &colour_prefs.titlebaroffline, COLOR_WHITE, THEME_TITLE_OFFLINE);
    g_free(titlebaroffline_val);

    gchar *titlebaraway_val = g_key_file_get_string(theme, "colours", "titlebar.away", NULL);
    _set_colour(titlebaraway_val, &colour_prefs.titlebaraway, COLOR_WHITE, THEME_TITLE_AWAY);
    g_free(titlebaraway_val);

    gchar *titlebarchat_val = g_key_file_get_string(theme, "colours", "titlebar.chat", NULL);
    _set_colour(titlebarchat_val, &colour_prefs.titlebarchat, COLOR_WHITE, THEME_TITLE_CHAT);
    g_free(titlebarchat_val);

    gchar *titlebardnd_val = g_key_file_get_string(theme, "colours", "titlebar.dnd", NULL);
    _set_colour(titlebardnd_val, &colour_prefs.titlebardnd, COLOR_WHITE, THEME_TITLE_DND);
    g_free(titlebardnd_val);

    gchar *titlebarxa_val = g_key_file_get_string(theme, "colours", "titlebar.xa", NULL);
    _set_colour(titlebarxa_val, &colour_prefs.titlebarxa, COLOR_WHITE, THEME_TITLE_XA);
    g_free(titlebarxa_val);

    gchar *statusbartext_val = g_key_file_get_string(theme, "colours", "statusbar.text", NULL);
    _set_colour(statusbartext_val, &colour_prefs.statusbartext, COLOR_WHITE, THEME_STATUS_TEXT);
    g_free(statusbartext_val);

    gchar *statusbarbrackets_val = g_key_file_get_string(theme, "colours", "statusbar.brackets", NULL);
    _set_colour(statusbarbrackets_val, &colour_prefs.statusbarbrackets, COLOR_CYAN, THEME_STATUS_BRACKET);
    g_free(statusbarbrackets_val);

    gchar *statusbaractive_val = g_key_file_get_string(theme, "colours", "statusbar.active", NULL);
    _set_colour(statusbaractive_val, &colour_prefs.statusbaractive, COLOR_CYAN, THEME_STATUS_ACTIVE);
    g_free(statusbaractive_val);

    gchar *statusbarnew_val = g_key_file_get_string(theme, "colours", "statusbar.new", NULL);
    _set_colour(statusbarnew_val, &colour_prefs.statusbarnew, COLOR_WHITE, THEME_STATUS_NEW);
    g_free(statusbarnew_val);

    gchar *maintext_val = g_key_file_get_string(theme, "colours", "main.text", NULL);
    _set_colour(maintext_val, &colour_prefs.maintext, COLOR_WHITE, THEME_TEXT);
    g_free(maintext_val);

    gchar *maintextme_val = g_key_file_get_string(theme, "colours", "main.text.me", NULL);
    _set_colour(maintextme_val, &colour_prefs.maintextme, COLOR_WHITE, THEME_TEXT_ME);
    g_free(maintextme_val);

    gchar *maintextthem_val = g_key_file_get_string(theme, "colours", "main.text.them", NULL);
    _set_colour(maintextthem_val, &colour_prefs.maintextthem, COLOR_WHITE, THEME_TEXT_THEM);
    g_free(maintextthem_val);

    gchar *splashtext_val = g_key_file_get_string(theme, "colours", "main.splash", NULL);
    _set_colour(splashtext_val, &colour_prefs.splashtext, COLOR_CYAN, THEME_SPLASH);
    g_free(splashtext_val);

    gchar *inputtext_val = g_key_file_get_string(theme, "colours", "input.text", NULL);
    _set_colour(inputtext_val, &colour_prefs.inputtext, COLOR_WHITE, THEME_INPUT_TEXT);
    g_free(inputtext_val);

    gchar *timetext_val = g_key_file_get_string(theme, "colours", "main.time", NULL);
    _set_colour(timetext_val, &colour_prefs.timetext, COLOR_WHITE, THEME_TIME);
    g_free(timetext_val);

    gchar *subscribed_val = g_key_file_get_string(theme, "colours", "subscribed", NULL);
    _set_colour(subscribed_val, &colour_prefs.subscribed, COLOR_GREEN, THEME_SUBSCRIBED);
    g_free(subscribed_val);

    gchar *unsubscribed_val = g_key_file_get_string(theme, "colours", "unsubscribed", NULL);
    _set_colour(unsubscribed_val, &colour_prefs.unsubscribed, COLOR_RED, THEME_UNSUBSCRIBED);
    g_free(unsubscribed_val);

    gchar *otrstartedtrusted_val = g_key_file_get_string(theme, "colours", "otr.started.trusted", NULL);
    _set_colour(otrstartedtrusted_val, &colour_prefs.otrstartedtrusted, COLOR_GREEN, THEME_OTR_STARTED_TRUSTED);
    g_free(otrstartedtrusted_val);

    gchar *otrstarteduntrusted_val = g_key_file_get_string(theme, "colours", "otr.started.untrusted", NULL);
    _set_colour(otrstarteduntrusted_val, &colour_prefs.otrstarteduntrusted, COLOR_YELLOW, THEME_OTR_STARTED_UNTRUSTED);
    g_free(otrstarteduntrusted_val);

    gchar *otrended_val = g_key_file_get_string(theme, "colours", "otr.ended", NULL);
    _set_colour(otrended_val, &colour_prefs.otrended, COLOR_RED, THEME_OTR_ENDED);
    g_free(otrended_val);

    gchar *otrtrusted_val = g_key_file_get_string(theme, "colours", "otr.trusted", NULL);
    _set_colour(otrtrusted_val, &colour_prefs.otrtrusted, COLOR_GREEN, THEME_OTR_TRUSTED);
    g_free(otrtrusted_val);

    gchar *otruntrusted_val = g_key_file_get_string(theme, "colours", "otr.untrusted", NULL);
    _set_colour(otruntrusted_val, &colour_prefs.otruntrusted, COLOR_YELLOW, THEME_OTR_UNTRUSTED);
    g_free(otruntrusted_val);

    gchar *online_val = g_key_file_get_string(theme, "colours", "online", NULL);
    _set_colour(online_val, &colour_prefs.online, COLOR_GREEN, THEME_ONLINE);
    g_free(online_val);

    gchar *away_val = g_key_file_get_string(theme, "colours", "away", NULL);
    _set_colour(away_val, &colour_prefs.away, COLOR_CYAN, THEME_AWAY);
    g_free(away_val);

    gchar *chat_val = g_key_file_get_string(theme, "colours", "chat", NULL);
    _set_colour(chat_val, &colour_prefs.chat, COLOR_GREEN, THEME_CHAT);
    g_free(chat_val);

    gchar *dnd_val = g_key_file_get_string(theme, "colours", "dnd", NULL);
    _set_colour(dnd_val, &colour_prefs.dnd, COLOR_RED, THEME_DND);
    g_free(dnd_val);

    gchar *xa_val = g_key_file_get_string(theme, "colours", "xa", NULL);
    _set_colour(xa_val, &colour_prefs.xa, COLOR_CYAN, THEME_XA);
    g_free(xa_val);

    gchar *offline_val = g_key_file_get_string(theme, "colours", "offline", NULL);
    _set_colour(offline_val, &colour_prefs.offline, COLOR_RED, THEME_OFFLINE);
    g_free(offline_val);

    gchar *typing_val = g_key_file_get_string(theme, "colours", "typing", NULL);
    _set_colour(typing_val, &colour_prefs.typing, COLOR_YELLOW, THEME_TYPING);
    g_free(typing_val);

    gchar *gone_val = g_key_file_get_string(theme, "colours", "gone", NULL);
    _set_colour(gone_val, &colour_prefs.gone, COLOR_RED, THEME_GONE);
    g_free(gone_val);

    gchar *error_val = g_key_file_get_string(theme, "colours", "error", NULL);
    _set_colour(error_val, &colour_prefs.error, COLOR_RED, THEME_ERROR);
    g_free(error_val);

    gchar *incoming_val = g_key_file_get_string(theme, "colours", "incoming", NULL);
    _set_colour(incoming_val, &colour_prefs.incoming, COLOR_YELLOW, THEME_INCOMING);
    g_free(incoming_val);

    gchar *roominfo_val = g_key_file_get_string(theme, "colours", "roominfo", NULL);
    _set_colour(roominfo_val, &colour_prefs.roominfo, COLOR_YELLOW, THEME_ROOMINFO);
    g_free(roominfo_val);

    gchar *roommention_val = g_key_file_get_string(theme, "colours", "roommention", NULL);
    _set_colour(roommention_val, &colour_prefs.roommention, COLOR_YELLOW, THEME_ROOMMENTION);
    g_free(roommention_val);

    gchar *me_val = g_key_file_get_string(theme, "colours", "me", NULL);
    _set_colour(me_val, &colour_prefs.me, COLOR_YELLOW, THEME_ME);
    g_free(me_val);

    gchar *them_val = g_key_file_get_string(theme, "colours", "them", NULL);
    _set_colour(them_val, &colour_prefs.them, COLOR_GREEN, THEME_THEM);
    g_free(them_val);

    gchar *rosterheader_val = g_key_file_get_string(theme, "colours", "roster.header", NULL);
    _set_colour(rosterheader_val, &colour_prefs.rosterheader, COLOR_YELLOW, THEME_ROSTER_HEADER);
    g_free(rosterheader_val);

    gchar *occupantsheader_val = g_key_file_get_string(theme, "colours", "occupants.header", NULL);
    _set_colour(occupantsheader_val, &colour_prefs.occupantsheader, COLOR_YELLOW, THEME_OCCUPANTS_HEADER);
    g_free(occupantsheader_val);
}

static gchar *
_get_themes_dir(void)
{
    gchar *xdg_config = xdg_get_config_home();
    GString *themes_dir = g_string_new(xdg_config);
    g_free(xdg_config);
    g_string_append(themes_dir, "/profanity/themes");
    return g_string_free(themes_dir, FALSE);
}

void
_theme_list_dir(const gchar * const dir, GSList **result)
{
    GDir *themes = g_dir_open(dir, 0, NULL);
    if (themes != NULL) {
        const gchar *theme = g_dir_read_name(themes);
        while (theme != NULL) {
            *result = g_slist_append(*result, strdup(theme));
            theme = g_dir_read_name(themes);
        }
        g_dir_close(themes);
    }
}

static GString *
_theme_find(const char * const theme_name)
{
    GString *path = NULL;
    gchar *themes_dir = _get_themes_dir();

    if (themes_dir != NULL) {
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
theme_main_presence_attrs(const char * const presence)
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
    case THEME_ROOMINFO:                result = COLOR_PAIR(27); break;
    case THEME_ROOMMENTION:             result = COLOR_PAIR(28); break;
    case THEME_ONLINE:                  result = COLOR_PAIR(29); break;
    case THEME_OFFLINE:                 result = COLOR_PAIR(30); break;
    case THEME_AWAY:                    result = COLOR_PAIR(31); break;
    case THEME_CHAT:                    result = COLOR_PAIR(32); break;
    case THEME_DND:                     result = COLOR_PAIR(33); break;
    case THEME_XA:                      result = COLOR_PAIR(34); break;
    case THEME_TYPING:                  result = COLOR_PAIR(35); break;
    case THEME_GONE:                    result = COLOR_PAIR(36); break;
    case THEME_SUBSCRIBED:              result = COLOR_PAIR(37); break;
    case THEME_UNSUBSCRIBED:            result = COLOR_PAIR(38); break;
    case THEME_OTR_STARTED_TRUSTED:     result = COLOR_PAIR(39); break;
    case THEME_OTR_STARTED_UNTRUSTED:   result = COLOR_PAIR(40); break;
    case THEME_OTR_ENDED:               result = COLOR_PAIR(41); break;
    case THEME_OTR_TRUSTED:             result = COLOR_PAIR(42); break;
    case THEME_OTR_UNTRUSTED:           result = COLOR_PAIR(43); break;
    case THEME_ROSTER_HEADER:           result = COLOR_PAIR(44); break;
    case THEME_OCCUPANTS_HEADER:        result = COLOR_PAIR(45); break;
    case THEME_WHITE:                   result = COLOR_PAIR(46); break;
    case THEME_WHITE_BOLD:              result = COLOR_PAIR(46); break;
    case THEME_GREEN:                   result = COLOR_PAIR(47); break;
    case THEME_GREEN_BOLD:              result = COLOR_PAIR(47); break;
    case THEME_RED:                     result = COLOR_PAIR(48); break;
    case THEME_RED_BOLD:                result = COLOR_PAIR(48); break;
    case THEME_YELLOW:                  result = COLOR_PAIR(49); break;
    case THEME_YELLOW_BOLD:             result = COLOR_PAIR(49); break;
    case THEME_BLUE:                    result = COLOR_PAIR(50); break;
    case THEME_BLUE_BOLD:               result = COLOR_PAIR(50); break;
    case THEME_CYAN:                    result = COLOR_PAIR(51); break;
    case THEME_CYAN_BOLD:               result = COLOR_PAIR(51); break;
    case THEME_BLACK:                   result = COLOR_PAIR(52); break;
    case THEME_BLACK_BOLD:              result = COLOR_PAIR(52); break;
    case THEME_MAGENTA:                 result = COLOR_PAIR(53); break;
    case THEME_MAGENTA_BOLD:            result = COLOR_PAIR(53); break;
    default:                           break;
    }

    if (g_hash_table_lookup(bold_items, GINT_TO_POINTER(attrs))) {
        return result | A_BOLD;
    } else {
        return result;

    }
}