/*
 * window_list.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 * Copyright (C) 2019 - 2025 Michael Vetter <jubalh@iodoru.org>
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

#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include <glib.h>

#include "common.h"
#include "config/preferences.h"
#include "config/theme.h"
#include "plugins/plugins.h"
#include "ui/ui.h"
#include "ui/window_list.h"
#include "xmpp/xmpp.h"
#include "xmpp/roster_list.h"
#include "tools/http_upload.h"

#ifdef HAVE_OMEMO
#include "omemo/omemo.h"
#endif

static GHashTable* windows;
static GList *keys, *values;
static int current;
static Autocomplete wins_ac;
static Autocomplete wins_close_ac;

static int _wins_cmp_num(gconstpointer a, gconstpointer b);
static int _wins_get_next_available_num(GList* used);

static void
_wins_htable_update(void)
{
    if (values)
        g_list_free(values);
    if (keys)
        g_list_free(keys);
    keys = g_hash_table_get_keys(windows);
    keys = g_list_sort(keys, _wins_cmp_num);
    values = g_hash_table_get_values(windows);
}

static gboolean
_wins_htable_insert(GHashTable* hash_table,
                    gpointer key,
                    gpointer value)
{
    gboolean ret = g_hash_table_insert(hash_table, key, value);
    _wins_htable_update();
    return ret;
}

void
wins_init(void)
{
    windows = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, (GDestroyNotify)win_free);

    ProfWin* console = win_create_console();
    _wins_htable_insert(windows, GINT_TO_POINTER(1), console);

    current = 1;

    wins_ac = autocomplete_new();
    autocomplete_add(wins_ac, "console");

    wins_close_ac = autocomplete_new();
    autocomplete_add(wins_close_ac, "all");
    autocomplete_add(wins_close_ac, "read");
}

ProfWin*
wins_get_console(void)
{
    return g_hash_table_lookup(windows, GINT_TO_POINTER(1));
}

gboolean
wins_chat_exists(const char* const barejid)
{
    ProfChatWin* chatwin = wins_get_chat(barejid);
    return (chatwin != NULL);
}

ProfChatWin*
wins_get_chat(const char* const barejid)
{
    GList* curr = values;

    while (curr) {
        ProfWin* window = curr->data;
        if (window->type == WIN_CHAT) {
            ProfChatWin* chatwin = (ProfChatWin*)window;
            if (g_strcmp0(chatwin->barejid, barejid) == 0) {
                return chatwin;
            }
        }
        curr = g_list_next(curr);
    }

    return NULL;
}

static gint
_cmp_unsubscribed_wins(ProfChatWin* a, ProfChatWin* b)
{
    return g_strcmp0(a->barejid, b->barejid);
}

GList*
wins_get_chat_unsubscribed(void)
{
    GList* result = NULL;
    GList* curr = values;

    while (curr) {
        ProfWin* window = curr->data;
        if (window->type == WIN_CHAT) {
            ProfChatWin* chatwin = (ProfChatWin*)window;
            PContact contact = roster_get_contact(chatwin->barejid);
            if (contact == NULL) {
                result = g_list_insert_sorted(result, chatwin, (GCompareFunc)_cmp_unsubscribed_wins);
            }
        }
        curr = g_list_next(curr);
    }

    return result;
}

ProfConfWin*
wins_get_conf(const char* const roomjid)
{
    GList* curr = values;

    while (curr) {
        ProfWin* window = curr->data;
        if (window->type == WIN_CONFIG) {
            ProfConfWin* confwin = (ProfConfWin*)window;
            if (g_strcmp0(confwin->roomjid, roomjid) == 0) {
                return confwin;
            }
        }
        curr = g_list_next(curr);
    }

    return NULL;
}

ProfMucWin*
wins_get_muc(const char* const roomjid)
{
    GList* curr = values;

    while (curr) {
        ProfWin* window = curr->data;
        if (window->type == WIN_MUC) {
            ProfMucWin* mucwin = (ProfMucWin*)window;
            if (g_strcmp0(mucwin->roomjid, roomjid) == 0) {
                return mucwin;
            }
        }
        curr = g_list_next(curr);
    }

    return NULL;
}

ProfPrivateWin*
wins_get_private(const char* const fulljid)
{
    GList* curr = values;

    while (curr) {
        ProfWin* window = curr->data;
        if (window->type == WIN_PRIVATE) {
            ProfPrivateWin* privatewin = (ProfPrivateWin*)window;
            if (g_strcmp0(privatewin->fulljid, fulljid) == 0) {
                return privatewin;
            }
        }
        curr = g_list_next(curr);
    }

    return NULL;
}

ProfPluginWin*
wins_get_plugin(const char* const tag)
{
    GList* curr = values;

    while (curr) {
        ProfWin* window = curr->data;
        if (window->type == WIN_PLUGIN) {
            ProfPluginWin* pluginwin = (ProfPluginWin*)window;
            if (g_strcmp0(pluginwin->tag, tag) == 0) {
                return pluginwin;
            }
        }
        curr = g_list_next(curr);
    }

    return NULL;
}

void
wins_close_plugin(char* tag)
{
    ProfWin* toclose = wins_get_by_string(tag);
    if (toclose == NULL) {
        return;
    }

    int index = wins_get_num(toclose);
    ui_close_win(index);

    wins_tidy();
}

GList*
wins_get_private_chats(const char* const roomjid)
{
    GList* result = NULL;
    auto_gchar gchar* prefix = g_strdup_printf("%s/", roomjid);
    GList* curr = values;

    while (curr) {
        ProfWin* window = curr->data;
        if (window->type == WIN_PRIVATE) {
            ProfPrivateWin* privatewin = (ProfPrivateWin*)window;
            if (roomjid == NULL || g_str_has_prefix(privatewin->fulljid, prefix)) {
                result = g_list_append(result, privatewin);
            }
        }
        curr = g_list_next(curr);
    }

    return result;
}

void
wins_private_nick_change(const char* const roomjid, const char* const oldnick, const char* const newnick)
{
    auto_jid Jid* oldjid = jid_create_from_bare_and_resource(roomjid, oldnick);

    ProfPrivateWin* privwin = wins_get_private(oldjid->fulljid);
    if (privwin) {
        free(privwin->fulljid);

        auto_jid Jid* newjid = jid_create_from_bare_and_resource(roomjid, newnick);
        privwin->fulljid = strdup(newjid->fulljid);
        win_println((ProfWin*)privwin, THEME_THEM, "!", "** %s is now known as %s.", oldjid->resourcepart, newjid->resourcepart);

        autocomplete_remove(wins_ac, oldjid->fulljid);
        autocomplete_remove(wins_close_ac, oldjid->fulljid);
        autocomplete_add(wins_ac, newjid->fulljid);
        autocomplete_add(wins_close_ac, newjid->fulljid);
    }
}

void
wins_change_nick(const char* const barejid, const char* const oldnick, const char* const newnick)
{
    ProfChatWin* chatwin = wins_get_chat(barejid);
    if (chatwin) {
        if (oldnick) {
            autocomplete_remove(wins_ac, oldnick);
            autocomplete_remove(wins_close_ac, oldnick);
        }
        autocomplete_add(wins_ac, newnick);
        autocomplete_add(wins_close_ac, newnick);
    }
}

void
wins_remove_nick(const char* const barejid, const char* const oldnick)
{
    ProfChatWin* chatwin = wins_get_chat(barejid);
    if (chatwin) {
        if (oldnick) {
            autocomplete_remove(wins_ac, oldnick);
            autocomplete_remove(wins_close_ac, oldnick);
        }
    }
}

ProfWin*
wins_get_current(void)
{
    if (windows) {
        return g_hash_table_lookup(windows, GINT_TO_POINTER(current));
    } else {
        return NULL;
    }
}

GList*
wins_get_nums(void)
{
    return g_list_copy(keys);
}

void
wins_set_current_by_num(int i)
{
    ProfWin* window = g_hash_table_lookup(windows, GINT_TO_POINTER(i));
    if (window) {
        current = i;
        if (window->type == WIN_CHAT) {
            ProfChatWin* chatwin = (ProfChatWin*)window;
            assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
            chatwin->unread = 0;
            plugins_on_chat_win_focus(chatwin->barejid);
        } else if (window->type == WIN_MUC) {
            ProfMucWin* mucwin = (ProfMucWin*)window;
            assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
            mucwin->unread = 0;
            mucwin->unread_mentions = FALSE;
            mucwin->unread_triggers = FALSE;
            plugins_on_room_win_focus(mucwin->roomjid);
        } else if (window->type == WIN_PRIVATE) {
            ProfPrivateWin* privatewin = (ProfPrivateWin*)window;
            privatewin->unread = 0;
        }

        // if we switched to console
        if (current == 0) {
            // remove all alerts
            cons_clear_alerts();
        } else {
            // remove alert from window where we switch to
            cons_remove_alert(window);
            // if there a no more alerts left
            if (!cons_has_alerts()) {
                // dont highlight console (no news there)
                ProfWin* conswin = wins_get_console();
                status_bar_active(1, conswin->type, "console");
            }
        }
    }
}

ProfWin*
wins_get_by_num(int i)
{
    return g_hash_table_lookup(windows, GINT_TO_POINTER(i));
}

ProfWin*
wins_get_by_string(const char* str)
{
    if (g_strcmp0(str, "console") == 0) {
        ProfWin* conswin = wins_get_console();
        return conswin;
    }

    if (g_strcmp0(str, "xmlconsole") == 0) {
        ProfXMLWin* xmlwin = wins_get_xmlconsole();
        return (ProfWin*)xmlwin;
    }

    ProfChatWin* chatwin = wins_get_chat(str);
    if (chatwin) {
        return (ProfWin*)chatwin;
    }

    jabber_conn_status_t conn_status = connection_get_status();
    if (conn_status == JABBER_CONNECTED) {
        char* barejid = roster_barejid_from_name(str);
        if (barejid) {
            ProfChatWin* chatwin = wins_get_chat(barejid);
            if (chatwin) {
                return (ProfWin*)chatwin;
            }
        }
    }

    ProfMucWin* mucwin = wins_get_muc(str);
    if (mucwin) {
        return (ProfWin*)mucwin;
    }

    ProfPrivateWin* privwin = wins_get_private(str);
    if (privwin) {
        return (ProfWin*)privwin;
    }

    ProfPluginWin* pluginwin = wins_get_plugin(str);
    if (pluginwin) {
        return (ProfWin*)pluginwin;
    }

    return NULL;
}

ProfWin*
wins_get_next(void)
{
    GList* curr = keys;
    // find our place in the list
    while (curr) {
        if (current == GPOINTER_TO_INT(curr->data)) {
            break;
        }
        curr = g_list_next(curr);
    }

    // if there is a next window return it
    curr = g_list_next(curr);
    if (curr) {
        int next = GPOINTER_TO_INT(curr->data);
        return wins_get_by_num(next);
    }
    // otherwise return the first window (console)
    return wins_get_console();
}

ProfWin*
wins_get_previous(void)
{
    GList* curr = keys;
    // find our place in the list
    while (curr) {
        if (current == GPOINTER_TO_INT(curr->data)) {
            break;
        }
        curr = g_list_next(curr);
    }

    // if there is a previous window return it
    curr = g_list_previous(curr);
    if (curr) {
        int previous = GPOINTER_TO_INT(curr->data);
        return wins_get_by_num(previous);
    }
    // otherwise return the last window
    int new_num = GPOINTER_TO_INT(g_list_last(keys)->data);
    return wins_get_by_num(new_num);
}

int
wins_get_num(ProfWin* window)
{
    GList* curr = keys;

    while (curr) {
        gconstpointer num_p = curr->data;
        ProfWin* curr_win = g_hash_table_lookup(windows, num_p);
        if (curr_win == window) {
            return GPOINTER_TO_INT(num_p);
        }
        curr = g_list_next(curr);
    }

    return -1;
}

int
wins_get_current_num(void)
{
    return current;
}

void
wins_close_by_num(int i)
{
    // console cannot be closed
    if (i != 1) {

        // go to console if closing current window
        if (i == current) {
            current = 1;
            ProfWin* window = wins_get_current();
            win_update_virtual(window);
        }

        ProfWin* window = wins_get_by_num(i);
        if (window) {
            // cancel upload processes of this window
            http_upload_cancel_processes(window);

            switch (window->type) {
            case WIN_CHAT:
            {
                ProfChatWin* chatwin = (ProfChatWin*)window;
                autocomplete_remove(wins_ac, chatwin->barejid);
                autocomplete_remove(wins_close_ac, chatwin->barejid);

                jabber_conn_status_t conn_status = connection_get_status();
                if (conn_status == JABBER_CONNECTED) {
                    PContact contact = roster_get_contact(chatwin->barejid);
                    if (contact) {
                        const char* nick = p_contact_name(contact);
                        if (nick) {
                            autocomplete_remove(wins_ac, nick);
                            autocomplete_remove(wins_close_ac, nick);
                        }
                    }
                }
                autocomplete_free(window->urls_ac);
                autocomplete_free(window->quotes_ac);
                break;
            }
            case WIN_MUC:
            {
                ProfMucWin* mucwin = (ProfMucWin*)window;
                autocomplete_remove(wins_ac, mucwin->roomjid);
                autocomplete_remove(wins_close_ac, mucwin->roomjid);

                if (mucwin->last_msg_timestamp) {
                    g_date_time_unref(mucwin->last_msg_timestamp);
                }
                autocomplete_free(window->urls_ac);
                autocomplete_free(window->quotes_ac);
                break;
            }
            case WIN_PRIVATE:
            {
                ProfPrivateWin* privwin = (ProfPrivateWin*)window;
                autocomplete_remove(wins_ac, privwin->fulljid);
                autocomplete_remove(wins_close_ac, privwin->fulljid);
                autocomplete_free(window->urls_ac);
                autocomplete_free(window->quotes_ac);
                break;
            }
            case WIN_XML:
            {
                autocomplete_remove(wins_ac, "xmlconsole");
                autocomplete_remove(wins_close_ac, "xmlconsole");
                break;
            }
            case WIN_PLUGIN:
            {
                ProfPluginWin* pluginwin = (ProfPluginWin*)window;
                plugins_close_win(pluginwin->plugin_name, pluginwin->tag);
                autocomplete_remove(wins_ac, pluginwin->tag);
                autocomplete_remove(wins_close_ac, pluginwin->tag);
                break;
            }
            case WIN_CONFIG:
            default:
                break;
            }
        }

        g_hash_table_remove(windows, GINT_TO_POINTER(i));
        _wins_htable_update();
        status_bar_inactive(i);
    }
}

gboolean
wins_is_current(ProfWin* window)
{
    ProfWin* current_window = wins_get_current();

    if (current_window == window) {
        return TRUE;
    } else {
        return FALSE;
    }
}

ProfWin*
wins_new_xmlconsole(void)
{
    int result = _wins_get_next_available_num(keys);
    ProfWin* newwin = win_create_xmlconsole();
    _wins_htable_insert(windows, GINT_TO_POINTER(result), newwin);
    autocomplete_add(wins_ac, "xmlconsole");
    autocomplete_add(wins_close_ac, "xmlconsole");
    return newwin;
}

ProfWin*
wins_new_chat(const char* const barejid)
{
    int result = _wins_get_next_available_num(keys);
    ProfWin* newwin = win_create_chat(barejid);
    _wins_htable_insert(windows, GINT_TO_POINTER(result), newwin);

    autocomplete_add(wins_ac, barejid);
    autocomplete_add(wins_close_ac, barejid);
    PContact contact = roster_get_contact(barejid);
    if (contact) {
        const char* nick = p_contact_name(contact);
        if (nick) {
            autocomplete_add(wins_ac, nick);
            autocomplete_add(wins_close_ac, nick);
        }
    }
    newwin->urls_ac = autocomplete_new();
    newwin->quotes_ac = autocomplete_new();

    return newwin;
}

ProfWin*
wins_new_muc(const char* const roomjid)
{
    int result = _wins_get_next_available_num(keys);
    ProfWin* newwin = win_create_muc(roomjid);
    _wins_htable_insert(windows, GINT_TO_POINTER(result), newwin);
    autocomplete_add(wins_ac, roomjid);
    autocomplete_add(wins_close_ac, roomjid);
    newwin->urls_ac = autocomplete_new();
    newwin->quotes_ac = autocomplete_new();

    return newwin;
}

ProfWin*
wins_new_config(const char* const roomjid, DataForm* form, ProfConfWinCallback submit, ProfConfWinCallback cancel, const void* userdata)
{
    int result = _wins_get_next_available_num(keys);
    ProfWin* newwin = win_create_config(roomjid, form, submit, cancel, userdata);
    _wins_htable_insert(windows, GINT_TO_POINTER(result), newwin);

    return newwin;
}

ProfWin*
wins_new_private(const char* const fulljid)
{
    int result = _wins_get_next_available_num(keys);
    ProfWin* newwin = win_create_private(fulljid);
    _wins_htable_insert(windows, GINT_TO_POINTER(result), newwin);
    autocomplete_add(wins_ac, fulljid);
    autocomplete_add(wins_close_ac, fulljid);
    newwin->urls_ac = autocomplete_new();
    newwin->quotes_ac = autocomplete_new();

    return newwin;
}

ProfWin*
wins_new_plugin(const char* const plugin_name, const char* const tag)
{
    int result = _wins_get_next_available_num(keys);
    ProfWin* newwin = win_create_plugin(plugin_name, tag);
    _wins_htable_insert(windows, GINT_TO_POINTER(result), newwin);
    autocomplete_add(wins_ac, tag);
    autocomplete_add(wins_close_ac, tag);
    return newwin;
}

ProfWin*
wins_new_vcard(vCard* vcard)
{
    int result = _wins_get_next_available_num(keys);
    ProfWin* newwin = win_create_vcard(vcard);
    _wins_htable_insert(windows, GINT_TO_POINTER(result), newwin);

    return newwin;
}

gboolean
wins_do_notify_remind(void)
{
    GList* curr = values;

    while (curr) {
        ProfWin* window = curr->data;
        if (win_notify_remind(window)) {
            return TRUE;
        }
        curr = g_list_next(curr);
    }
    return FALSE;
}

int
wins_get_total_unread(void)
{
    int result = 0;
    GList* curr = values;

    while (curr) {
        ProfWin* window = curr->data;
        result += win_unread(window);
        curr = g_list_next(curr);
    }
    return result;
}

void
wins_resize_all(void)
{
    GList* curr = values;
    while (curr) {
        ProfWin* window = curr->data;
        win_resize(window);
        curr = g_list_next(curr);
    }

    ProfWin* current_win = wins_get_current();
    win_update_virtual(current_win);
}

void
wins_hide_subwin(ProfWin* window)
{
    win_hide_subwin(window);

    ProfWin* current_win = wins_get_current();
    win_refresh_without_subwin(current_win);
}

void
wins_show_subwin(ProfWin* window)
{
    win_show_subwin(window);

    // only mucwin and console have occupants/roster subwin
    if (window->type != WIN_MUC && window->type != WIN_CONSOLE) {
        return;
    }

    ProfWin* current_win = wins_get_current();
    win_refresh_with_subwin(current_win);
}

ProfXMLWin*
wins_get_xmlconsole(void)
{
    GList* curr = values;

    while (curr) {
        ProfWin* window = curr->data;
        if (window->type == WIN_XML) {
            ProfXMLWin* xmlwin = (ProfXMLWin*)window;
            assert(xmlwin->memcheck == PROFXMLWIN_MEMCHECK);
            return xmlwin;
        }
        curr = g_list_next(curr);
    }

    return NULL;
}

ProfVcardWin*
wins_get_vcard(void)
{
    GList* curr = values;

    while (curr) {
        ProfWin* window = curr->data;
        if (window->type == WIN_VCARD) {
            ProfVcardWin* vcardwin = (ProfVcardWin*)window;
            assert(vcardwin->memcheck == PROFVCARDWIN_MEMCHECK);
            return vcardwin;
        }
        curr = g_list_next(curr);
    }

    return NULL;
}

GSList*
wins_get_chat_recipients(void)
{
    GSList* result = NULL;
    GList* curr = values;

    while (curr) {
        ProfWin* window = curr->data;
        if (window->type == WIN_CHAT) {
            ProfChatWin* chatwin = (ProfChatWin*)window;
            result = g_slist_append(result, chatwin->barejid);
        }
        curr = g_list_next(curr);
    }
    return result;
}

GSList*
wins_get_prune_wins(void)
{
    GSList* result = NULL;
    GList* curr = values;

    while (curr) {
        ProfWin* window = curr->data;
        if (win_unread(window) == 0 && window->type != WIN_MUC && window->type != WIN_CONFIG && window->type != WIN_XML && window->type != WIN_CONSOLE) {
            result = g_slist_append(result, window);
        }
        curr = g_list_next(curr);
    }
    return result;
}

void
wins_lost_connection(void)
{
    GList* curr = values;

    while (curr) {
        ProfWin* window = curr->data;
        if (window->type != WIN_CONSOLE) {
            win_println(window, THEME_ERROR, "-", "Lost connection.");

            // if current win, set current_win_dirty
            if (wins_is_current(window)) {
                win_update_virtual(window);
            }
        }
        curr = g_list_next(curr);
    }
}

void
wins_reestablished_connection(void)
{
    GList* curr = values;

    while (curr) {
        ProfWin* window = curr->data;
        if (window->type != WIN_CONSOLE) {
            win_println(window, THEME_TEXT, "-", "Connection re-established.");

#ifdef HAVE_OMEMO
            if (window->type == WIN_CHAT) {
                ProfChatWin* chatwin = (ProfChatWin*)window;
                assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
                if (chatwin->is_omemo) {
                    win_println(window, THEME_TEXT, "-", "Restarted OMEMO session.");
                    omemo_start_session(chatwin->barejid);
                }
            } else if (window->type == WIN_MUC) {
                ProfMucWin* mucwin = (ProfMucWin*)window;
                assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
                if (mucwin->is_omemo) {
                    win_println(window, THEME_TEXT, "-", "Restarted OMEMO session.");
                    omemo_start_muc_sessions(mucwin->roomjid);
                }
            }
#endif

            // if current win, set current_win_dirty
            if (wins_is_current(window)) {
                win_update_virtual(window);
            }
        }
        curr = g_list_next(curr);
    }
}

void
wins_swap(int source_win, int target_win)
{
    ProfWin* source = g_hash_table_lookup(windows, GINT_TO_POINTER(source_win));
    ProfWin* console = wins_get_console();

    if (source) {
        ProfWin* target = g_hash_table_lookup(windows, GINT_TO_POINTER(target_win));

        // target window empty
        if (target == NULL) {
            g_hash_table_steal(windows, GINT_TO_POINTER(source_win));
            _wins_htable_insert(windows, GINT_TO_POINTER(target_win), source);
            status_bar_inactive(source_win);
            auto_char char* identifier = win_get_tab_identifier(source);
            if (win_unread(source) > 0) {
                status_bar_new(target_win, source->type, identifier);
            } else {
                status_bar_active(target_win, source->type, identifier);
            }
            if (wins_get_current_num() == source_win) {
                wins_set_current_by_num(target_win);
                ui_focus_win(console);
            }

            // target window occupied
        } else {
            g_hash_table_steal(windows, GINT_TO_POINTER(source_win));
            g_hash_table_steal(windows, GINT_TO_POINTER(target_win));
            g_hash_table_insert(windows, GINT_TO_POINTER(source_win), target);
            _wins_htable_insert(windows, GINT_TO_POINTER(target_win), source);
            auto_char char* source_identifier = win_get_tab_identifier(source);
            auto_char char* target_identifier = win_get_tab_identifier(target);
            if (win_unread(source) > 0) {
                status_bar_new(target_win, source->type, source_identifier);
            } else {
                status_bar_active(target_win, source->type, source_identifier);
            }
            if (win_unread(target) > 0) {
                status_bar_new(source_win, target->type, target_identifier);
            } else {
                status_bar_active(source_win, target->type, target_identifier);
            }
            if ((wins_get_current_num() == source_win) || (wins_get_current_num() == target_win)) {
                ui_focus_win(console);
            }
        }
    }
}

static int
_wins_cmp_num(gconstpointer a, gconstpointer b)
{
    int real_a = GPOINTER_TO_INT(a);
    int real_b = GPOINTER_TO_INT(b);

    if (real_a == 0) {
        real_a = 10;
    }

    if (real_b == 0) {
        real_b = 10;
    }

    if (real_a < real_b) {
        return -1;
    } else if (real_a == real_b) {
        return 0;
    } else {
        return 1;
    }
}

static int
_wins_get_next_available_num(GList* used)
{
    // only console used
    if (g_list_length(used) == 1) {
        return 2;
    } else {
        GList* curr = used;
        int result = 0;
        int last_num = 1;
        // skip console
        curr = g_list_next(curr);
        while (curr) {
            int curr_num = GPOINTER_TO_INT(curr->data);

            if (((last_num != 9) && ((last_num + 1) != curr_num)) || ((last_num == 9) && (curr_num != 0))) {
                result = last_num + 1;
                if (result == 10) {
                    result = 0;
                }
                return (result);

            } else {
                last_num = curr_num;
                if (last_num == 0) {
                    last_num = 10;
                }
            }
            curr = g_list_next(curr);
        }
        result = last_num + 1;
        if (result == 10) {
            result = 0;
        }

        return result;
    }
}

gboolean
wins_tidy(void)
{
    gboolean tidy_required = FALSE;
    // get last used
    GList* last = g_list_last(keys);
    int last_num = GPOINTER_TO_INT(last->data);

    // find first free num TODO - Will sort again
    int next_available = _wins_get_next_available_num(keys);

    // found gap (next available before last window)
    if (_wins_cmp_num(GINT_TO_POINTER(next_available), GINT_TO_POINTER(last_num)) < 0) {
        tidy_required = TRUE;
    }

    if (tidy_required) {
        status_bar_set_all_inactive();
        GHashTable* new_windows = g_hash_table_new_full(g_direct_hash,
                                                        g_direct_equal, NULL, (GDestroyNotify)win_free);

        int num = 1;
        GList* curr = keys;
        while (curr) {
            ProfWin* window = g_hash_table_lookup(windows, curr->data);
            auto_char char* identifier = win_get_tab_identifier(window);
            g_hash_table_steal(windows, curr->data);
            if (num == 10) {
                g_hash_table_insert(new_windows, GINT_TO_POINTER(0), window);
                if (win_unread(window) > 0) {
                    status_bar_new(0, window->type, identifier);
                } else {
                    status_bar_active(0, window->type, identifier);
                }
            } else {
                g_hash_table_insert(new_windows, GINT_TO_POINTER(num), window);
                if (win_unread(window) > 0) {
                    status_bar_new(num, window->type, identifier);
                } else {
                    status_bar_active(num, window->type, identifier);
                }
            }
            num++;
            curr = g_list_next(curr);
        }

        g_hash_table_destroy(windows);
        windows = new_windows;
        _wins_htable_update();
        current = 1;
        ProfWin* console = wins_get_console();
        ui_focus_win(console);
        return TRUE;
    } else {
        return FALSE;
    }
}

GSList*
wins_create_summary(gboolean unread)
{
    if (unread && wins_get_total_unread() == 0) {
        return NULL;
    }

    GSList* result = NULL;
    GList* curr = keys;

    while (curr) {
        ProfWin* window = g_hash_table_lookup(windows, curr->data);
        if (!unread || (unread && win_unread(window) > 0)) {
            int ui_index = GPOINTER_TO_INT(curr->data);
            auto_gchar gchar* winstring = win_to_string(window);
            if (!winstring) {
                continue;
            }
            gchar* line = g_strdup_printf("%d: %s", ui_index, winstring);
            result = g_slist_append(result, line);
        }

        curr = g_list_next(curr);
    }

    return result;
}

GSList*
wins_create_summary_attention()
{
    GSList* result = NULL;

    GList* curr = keys;

    while (curr) {
        ProfWin* window = g_hash_table_lookup(windows, curr->data);
        gboolean has_attention = FALSE;
        if (window->type == WIN_CHAT) {
            ProfChatWin* chatwin = (ProfChatWin*)window;
            assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
            has_attention = chatwin->has_attention;
        } else if (window->type == WIN_MUC) {
            ProfMucWin* mucwin = (ProfMucWin*)window;
            assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
            has_attention = mucwin->has_attention;
        }
        if (has_attention) {
            int ui_index = GPOINTER_TO_INT(curr->data);
            auto_gchar gchar* winstring = win_to_string(window);
            if (!winstring) {
                continue;
            }
            gchar* line = g_strdup_printf("%d: %s", ui_index, winstring);
            result = g_slist_append(result, line);
        }
        curr = g_list_next(curr);
    }

    return result;
}

char*
win_autocomplete(const char* const search_str, gboolean previous, void* context)
{
    return autocomplete_complete(wins_ac, search_str, TRUE, previous);
}

char*
win_close_autocomplete(const char* const search_str, gboolean previous, void* context)
{
    return autocomplete_complete(wins_close_ac, search_str, TRUE, previous);
}

void
win_reset_search_attempts(void)
{
    autocomplete_reset(wins_ac);
}

void
win_close_reset_search_attempts(void)
{
    autocomplete_reset(wins_close_ac);
}

void
wins_destroy(void)
{
    if (values)
        g_list_free(values);
    if (keys)
        g_list_free(keys);
    values = keys = NULL;
    g_hash_table_destroy(windows);
    windows = NULL;
    autocomplete_free(wins_ac);
    wins_ac = NULL;
    autocomplete_free(wins_close_ac);
    wins_close_ac = NULL;
}

ProfWin*
wins_get_next_unread(void)
{
    // get and sort win nums
    GList* curr = keys;

    while (curr) {
        int curr_win_num = GPOINTER_TO_INT(curr->data);
        ProfWin* window = wins_get_by_num(curr_win_num);

        // test if window has unread messages
        if (win_unread(window) > 0) {
            return window;
        }

        curr = g_list_next(curr);
    }

    return NULL;
}

ProfWin*
wins_get_next_attention(void)
{
    // get and sort win nums
    GList* curr = values;

    ProfWin* current_window = wins_get_by_num(current);

    // search the current window
    while (curr) {
        ProfWin* window = curr->data;
        if (current_window == window) {
            current_window = window;
            curr = g_list_next(curr);
            break;
        }
        curr = g_list_next(curr);
    }

    // Start from current window
    while (current_window && curr) {
        ProfWin* window = curr->data;
        if (win_has_attention(window)) {
            return window;
        }
        curr = g_list_next(curr);
    }
    // Start from begin
    curr = values;
    while (current_window && curr) {
        ProfWin* window = curr->data;
        if (current_window == window) {
            // we are at current again
            break;
        }
        if (win_has_attention(window)) {
            return window;
        }
        curr = g_list_next(curr);
    }

    return NULL;
}

void
wins_add_urls_ac(const ProfWin* const win, const ProfMessage* const message, const gboolean flip)
{
    GRegex* regex;
    GMatchInfo* match_info;

    // https://stackoverflow.com/questions/43588699/regex-for-matching-any-url-character
    regex = g_regex_new("(https?|aesgcm)://[\\w\\-.~:/?#\\[\\]@!$&'()*+,;=%]+", 0, 0, NULL);
    g_regex_match(regex, message->plain, 0, &match_info);

    while (g_match_info_matches(match_info)) {
        auto_gchar gchar* word = g_match_info_fetch(match_info, 0);

        if (flip) {
            autocomplete_add_unsorted(win->urls_ac, word, FALSE);
        } else {
            autocomplete_add_unsorted(win->urls_ac, word, TRUE);
        }
        // for people who run profanity a long time, we don't want to waste a lot of memory
        autocomplete_remove_older_than_max_reverse(win->urls_ac, 20);

        g_match_info_next(match_info, NULL);
    }

    g_match_info_free(match_info);
    g_regex_unref(regex);
}

void
wins_add_quotes_ac(const ProfWin* const win, const char* const message, const gboolean flip)
{
    if (flip) {
        autocomplete_add_unsorted(win->quotes_ac, message, FALSE);
    } else {
        autocomplete_add_unsorted(win->quotes_ac, message, TRUE);
    }

    // for people who run profanity a long time, we don't want to waste a lot of memory
    autocomplete_remove_older_than_max_reverse(win->quotes_ac, 20);
}

char*
wins_get_url(const char* const search_str, gboolean previous, void* context)
{
    ProfWin* win = (ProfWin*)context;

    return autocomplete_complete(win->urls_ac, search_str, FALSE, previous);
}

char*
wins_get_quote(const char* const search_str, gboolean previous, void* context)
{
    ProfWin* win = (ProfWin*)context;

    return autocomplete_complete(win->quotes_ac, search_str, FALSE, previous);
}
