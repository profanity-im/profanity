/*
 * win_types.h
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

#ifndef UI_WIN_TYPES_H
#define UI_WIN_TYPES_H

#include "config.h"

#include <wchar.h>
#include <glib.h>

#ifdef HAVE_NCURSESW_NCURSES_H
#include <ncursesw/ncurses.h>
#elif HAVE_NCURSES_H
#include <ncurses.h>
#endif

#include "tools/autocomplete.h"
#include "ui/buffer.h"
#include "xmpp/chat_state.h"

#define LAYOUT_SPLIT_MEMCHECK       12345671
#define PROFCHATWIN_MEMCHECK        22374522
#define PROFMUCWIN_MEMCHECK         52345276
#define PROFPRIVATEWIN_MEMCHECK     77437483
#define PROFCONFWIN_MEMCHECK        64334685
#define PROFXMLWIN_MEMCHECK         87333463
#define PROFPLUGINWIN_MEMCHECK      43434777

typedef enum {
    FIELD_HIDDEN,
    FIELD_TEXT_SINGLE,
    FIELD_TEXT_PRIVATE,
    FIELD_TEXT_MULTI,
    FIELD_BOOLEAN,
    FIELD_LIST_SINGLE,
    FIELD_LIST_MULTI,
    FIELD_JID_SINGLE,
    FIELD_JID_MULTI,
    FIELD_FIXED,
    FIELD_UNKNOWN
} form_field_type_t;

typedef struct form_option_t {
    char *label;
    char *value;
} FormOption;

typedef struct form_field_t {
    char *label;
    char *type;
    form_field_type_t type_t;
    char *var;
    char *description;
    gboolean required;
    GSList *values;
    GSList *options;
    Autocomplete value_ac;
} FormField;

typedef struct data_form_t {
    char *type;
    char *title;
    char *instructions;
    GSList *fields;
    GHashTable *var_to_tag;
    GHashTable *tag_to_var;
    Autocomplete tag_ac;
    gboolean modified;
} DataForm;

typedef enum {
    LAYOUT_SIMPLE,
    LAYOUT_SPLIT
} layout_type_t;

typedef struct prof_layout_t {
    layout_type_t type;
    WINDOW *win;
    ProfBuff buffer;
    int y_pos;
    int paged;
} ProfLayout;

typedef struct prof_layout_simple_t {
    ProfLayout base;
} ProfLayoutSimple;

typedef struct prof_layout_split_t {
    ProfLayout base;
    WINDOW *subwin;
    int sub_y_pos;
    unsigned long memcheck;
} ProfLayoutSplit;

typedef enum {
    WIN_CONSOLE,
    WIN_CHAT,
    WIN_MUC,
    WIN_MUC_CONFIG,
    WIN_PRIVATE,
    WIN_XML,
    WIN_PLUGIN
} win_type_t;

typedef struct prof_win_t {
    win_type_t type;
    ProfLayout *layout;
} ProfWin;

typedef struct prof_console_win_t {
    ProfWin window;
} ProfConsoleWin;

typedef struct prof_chat_win_t {
    ProfWin window;
    char *barejid;
    int unread;
    ChatState *state;
    gboolean is_otr;
    gboolean otr_is_trusted;
    gboolean pgp_send;
    gboolean pgp_recv;
    char *resource_override;
    gboolean history_shown;
    unsigned long memcheck;
    char *enctext;
    char *incoming_char;
    char *outgoing_char;
} ProfChatWin;

typedef struct prof_muc_win_t {
    ProfWin window;
    char *roomjid;
    int unread;
    gboolean unread_mentions;
    gboolean unread_triggers;
    gboolean showjid;
    unsigned long memcheck;
    char *enctext;
    char *message_char;
} ProfMucWin;

typedef struct prof_mucconf_win_t {
    ProfWin window;
    char *roomjid;
    DataForm *form;
    unsigned long memcheck;
} ProfMucConfWin;

typedef struct prof_private_win_t {
    ProfWin window;
    char *fulljid;
    int unread;
    unsigned long memcheck;
    gboolean occupant_offline;
    gboolean room_left;
} ProfPrivateWin;

typedef struct prof_xml_win_t {
    ProfWin window;
    unsigned long memcheck;
} ProfXMLWin;

typedef struct prof_plugin_win_t {
    ProfWin super;
    char *tag;
    char *plugin_name;
    unsigned long memcheck;
} ProfPluginWin;

#endif
