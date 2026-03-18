/*
 * win_types.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
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
#elif HAVE_CURSES_H
#include <curses.h>
#endif

#include "tools/autocomplete.h"
#include "ui/buffer.h"
#include "xmpp/chat_state.h"
#include "xmpp/vcard.h"

#define LAYOUT_SPLIT_MEMCHECK   12345671
#define PROFCHATWIN_MEMCHECK    22374522
#define PROFMUCWIN_MEMCHECK     52345276
#define PROFPRIVATEWIN_MEMCHECK 77437483
#define PROFCONFWIN_MEMCHECK    64334685
#define PROFXMLWIN_MEMCHECK     87333463
#define PROFPLUGINWIN_MEMCHECK  43434777
#define PROFVCARDWIN_MEMCHECK   68947523

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

typedef struct form_option_t
{
    char* label;
    char* value;
} FormOption;

typedef struct form_field_t
{
    char* label;
    char* type;
    form_field_type_t type_t;
    char* var;
    char* description;
    gboolean required;
    GSList* values;
    GSList* options;
    Autocomplete value_ac;
} FormField;

typedef struct data_form_t
{
    char* type;
    char* title;
    char* instructions;
    GSList* fields;
    GHashTable* var_to_tag;
    GHashTable* tag_to_var;
    Autocomplete tag_ac;
    gboolean modified;
} DataForm;

typedef enum {
    LAYOUT_SIMPLE,
    LAYOUT_SPLIT
} layout_type_t;

typedef struct prof_layout_t
{
    layout_type_t type;
    WINDOW* win;
    ProfBuff buffer;
    int y_pos;
    int paged;
} ProfLayout;

typedef struct prof_layout_simple_t
{
    ProfLayout base;
} ProfLayoutSimple;

typedef struct prof_layout_split_t
{
    ProfLayout base;
    WINDOW* subwin;
    int sub_y_pos;
    unsigned long memcheck;
} ProfLayoutSplit;

typedef enum {
    WIN_CONSOLE,
    WIN_CHAT,
    WIN_MUC,
    WIN_CONFIG,
    WIN_PRIVATE,
    WIN_XML,
    WIN_PLUGIN,
    WIN_VCARD
} win_type_t;

typedef enum {
    WIN_SCROLL_INNER,
    WIN_SCROLL_REACHED_TOP,
    WIN_SCROLL_REACHED_BOTTOM
} win_scroll_state_t;

typedef struct prof_win_t
{
    win_type_t type;
    win_scroll_state_t scroll_state;
    ProfLayout* layout;
    Autocomplete urls_ac;
    Autocomplete quotes_ac;
    GHashTable* warned_jids;
} ProfWin;

typedef struct prof_console_win_t
{
    ProfWin window;
} ProfConsoleWin;

typedef struct prof_chat_win_t
{
    ProfWin window;
    char* barejid;
    int unread;
    ChatState* state;
    gboolean is_otr;
    gboolean otr_is_trusted;
    gboolean pgp_send;
    gboolean pgp_recv;
    gboolean is_omemo;
    gboolean omemo_trusted;
    gboolean is_ox; // XEP-0373: OpenPGP for XMPP
    char* resource_override;
    gboolean history_shown;
    unsigned long memcheck;
    char* enctext;
    char* incoming_char;
    char* outgoing_char;
    // For LMC
    char* last_message;
    char* last_msg_id;
    gboolean has_attention;
} ProfChatWin;

typedef struct prof_muc_win_t
{
    ProfWin window;
    char* roomjid;
    char* room_name;
    int unread;
    gboolean unread_mentions;
    gboolean unread_triggers;
    gboolean showjid;
    gboolean showoffline;
    gboolean is_omemo;
    gboolean omemo_trusted;
    unsigned long memcheck;
    char* enctext;
    char* message_char;
    GDateTime* last_msg_timestamp;
    // For LMC
    char* last_message;
    char* last_msg_id;
    gboolean has_attention;
} ProfMucWin;

typedef struct prof_conf_win_t ProfConfWin;
typedef void (*ProfConfWinCallback)(ProfConfWin*);

struct prof_conf_win_t
{
    ProfWin window;
    char* roomjid;
    DataForm* form;
    unsigned long memcheck;
    ProfConfWinCallback submit;
    ProfConfWinCallback cancel;
    const void* userdata;
};

typedef struct prof_private_win_t
{
    ProfWin window;
    char* fulljid;
    int unread;
    unsigned long memcheck;
    gboolean occupant_offline;
    gboolean room_left;
} ProfPrivateWin;

typedef struct prof_xml_win_t
{
    ProfWin window;
    unsigned long memcheck;
} ProfXMLWin;

typedef struct prof_plugin_win_t
{
    ProfWin window;
    char* tag;
    char* plugin_name;
    unsigned long memcheck;
} ProfPluginWin;

typedef struct prof_vcard_win_t
{
    ProfWin window;
    vCard* vcard;
    unsigned long memcheck;
} ProfVcardWin;

#endif
