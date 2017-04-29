/*
 * window_list.c
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

#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include <glib.h>

#include "common.h"
#include "config/preferences.h"
#include "config/theme.h"
#include "plugins/plugins.h"
#include "ui/ui.h"
#include "ui/statusbar.h"
#include "ui/window_list.h"
#include "xmpp/xmpp.h"
#include "xmpp/roster_list.h"

static GHashTable *windows;
static int current;
static Autocomplete wins_ac;
static Autocomplete wins_close_ac;

static int _wins_cmp_num(gconstpointer a, gconstpointer b);
static int _wins_get_next_available_num(GList *used);

void
wins_init(void)
{
    windows = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, (GDestroyNotify)win_free);

    ProfWin *console = win_create_console();
    g_hash_table_insert(windows, GINT_TO_POINTER(1), console);

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
wins_chat_exists(const char *const barejid)
{
    ProfChatWin *chatwin = wins_get_chat(barejid);
    return (chatwin != NULL);
}

ProfChatWin*
wins_get_chat(const char *const barejid)
{
    GList *values = g_hash_table_get_values(windows);
    GList *curr = values;

    while (curr) {
        ProfWin *window = curr->data;
        if (window->type == WIN_CHAT) {
            ProfChatWin *chatwin = (ProfChatWin*)window;
            if (g_strcmp0(chatwin->barejid, barejid) == 0) {
                g_list_free(values);
                return chatwin;
            }
        }
        curr = g_list_next(curr);
    }

    g_list_free(values);
    return NULL;
}

static gint
_cmp_unsubscribed_wins(ProfChatWin *a, ProfChatWin *b)
{
    return g_strcmp0(a->barejid, b->barejid);
}

GList*
wins_get_chat_unsubscribed(void)
{
    GList *result = NULL;
    GList *values = g_hash_table_get_values(windows);
    GList *curr = values;

    while (curr) {
        ProfWin *window = curr->data;
        if (window->type == WIN_CHAT) {
            ProfChatWin *chatwin = (ProfChatWin*)window;
            PContact contact = roster_get_contact(chatwin->barejid);
            if (contact == NULL) {
                result = g_list_insert_sorted(result, chatwin, (GCompareFunc)_cmp_unsubscribed_wins);
            }
        }
        curr = g_list_next(curr);
    }

    g_list_free(values);
    return result;
}

ProfMucConfWin*
wins_get_muc_conf(const char *const roomjid)
{
    GList *values = g_hash_table_get_values(windows);
    GList *curr = values;

    while (curr) {
        ProfWin *window = curr->data;
        if (window->type == WIN_MUC_CONFIG) {
            ProfMucConfWin *confwin = (ProfMucConfWin*)window;
            if (g_strcmp0(confwin->roomjid, roomjid) == 0) {
                g_list_free(values);
                return confwin;
            }
        }
        curr = g_list_next(curr);
    }

    g_list_free(values);
    return NULL;
}

ProfMucWin*
wins_get_muc(const char *const roomjid)
{
    GList *values = g_hash_table_get_values(windows);
    GList *curr = values;

    while (curr) {
        ProfWin *window = curr->data;
        if (window->type == WIN_MUC) {
            ProfMucWin *mucwin = (ProfMucWin*)window;
            if (g_strcmp0(mucwin->roomjid, roomjid) == 0) {
                g_list_free(values);
                return mucwin;
            }
        }
        curr = g_list_next(curr);
    }

    g_list_free(values);
    return NULL;
}

ProfPrivateWin*
wins_get_private(const char *const fulljid)
{
    GList *values = g_hash_table_get_values(windows);
    GList *curr = values;

    while (curr) {
        ProfWin *window = curr->data;
        if (window->type == WIN_PRIVATE) {
            ProfPrivateWin *privatewin = (ProfPrivateWin*)window;
            if (g_strcmp0(privatewin->fulljid, fulljid) == 0) {
                g_list_free(values);
                return privatewin;
            }
        }
        curr = g_list_next(curr);
    }

    g_list_free(values);
    return NULL;
}

ProfPluginWin*
wins_get_plugin(const char *const tag)
{
    GList *values = g_hash_table_get_values(windows);
    GList *curr = values;

    while (curr) {
        ProfWin *window = curr->data;
        if (window->type == WIN_PLUGIN) {
            ProfPluginWin *pluginwin = (ProfPluginWin*)window;
            if (g_strcmp0(pluginwin->tag, tag) == 0) {
                g_list_free(values);
                return pluginwin;
            }
        }
        curr = g_list_next(curr);
    }

    g_list_free(values);
    return NULL;
}

void
wins_close_plugin(char *tag)
{
    ProfWin *toclose = wins_get_by_string(tag);
    if (toclose == NULL) {
        return;
    }

    int index = wins_get_num(toclose);
    ui_close_win(index);

    if (prefs_get_boolean(PREF_WINS_AUTO_TIDY)) {
        wins_tidy();
    }
}

GList*
wins_get_private_chats(const char *const roomjid)
{
    GList *result = NULL;
    GString *prefix = g_string_new(roomjid);
    g_string_append(prefix, "/");
    GList *values = g_hash_table_get_values(windows);
    GList *curr = values;

    while (curr) {
        ProfWin *window = curr->data;
        if (window->type == WIN_PRIVATE) {
            ProfPrivateWin *privatewin = (ProfPrivateWin*)window;
            if (roomjid == NULL || g_str_has_prefix(privatewin->fulljid, prefix->str)) {
                result = g_list_append(result, privatewin);
            }
        }
        curr = g_list_next(curr);
    }

    g_list_free(values);
    g_string_free(prefix, TRUE);
    return result;
}

void
wins_private_nick_change(const char *const roomjid, const char *const oldnick, const char *const newnick)
{
    Jid *oldjid = jid_create_from_bare_and_resource(roomjid, oldnick);

    ProfPrivateWin *privwin = wins_get_private(oldjid->fulljid);
    if (privwin) {
        free(privwin->fulljid);

        Jid *newjid = jid_create_from_bare_and_resource(roomjid, newnick);
        privwin->fulljid = strdup(newjid->fulljid);
        win_println((ProfWin*)privwin, THEME_THEM, '!', "** %s is now known as %s.", oldjid->resourcepart, newjid->resourcepart);

        autocomplete_remove(wins_ac, oldjid->fulljid);
        autocomplete_remove(wins_close_ac, oldjid->fulljid);
        autocomplete_add(wins_ac, newjid->fulljid);
        autocomplete_add(wins_close_ac, newjid->fulljid);

        jid_destroy(newjid);
    }

    jid_destroy(oldjid);
}

void
wins_change_nick(const char *const barejid, const char *const oldnick, const char *const newnick)
{
    ProfChatWin *chatwin = wins_get_chat(barejid);
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
wins_remove_nick(const char *const barejid, const char *const oldnick)
{
    ProfChatWin *chatwin = wins_get_chat(barejid);
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
    return g_hash_table_get_keys(windows);
}

void
wins_set_current_by_num(int i)
{
    ProfWin *window = g_hash_table_lookup(windows, GINT_TO_POINTER(i));
    if (window) {
        current = i;
        if (window->type == WIN_CHAT) {
            ProfChatWin *chatwin = (ProfChatWin*) window;
            assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
            chatwin->unread = 0;
            plugins_on_chat_win_focus(chatwin->barejid);
        } else if (window->type == WIN_MUC) {
            ProfMucWin *mucwin = (ProfMucWin*) window;
            assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
            mucwin->unread = 0;
            mucwin->unread_mentions = FALSE;
            mucwin->unread_triggers = FALSE;
            plugins_on_room_win_focus(mucwin->roomjid);
        } else if (window->type == WIN_PRIVATE) {
            ProfPrivateWin *privatewin = (ProfPrivateWin*) window;
            privatewin->unread = 0;
        }
    }
}

ProfWin*
wins_get_by_num(int i)
{
    return g_hash_table_lookup(windows, GINT_TO_POINTER(i));
}

ProfWin*
wins_get_by_string(char *str)
{
    if (g_strcmp0(str, "console") == 0) {
        ProfWin *conswin = wins_get_console();
        if (conswin) {
            return conswin;
        } else {
            return NULL;
        }
    }

    if (g_strcmp0(str, "xmlconsole") == 0) {
        ProfXMLWin *xmlwin = wins_get_xmlconsole();
        if (xmlwin) {
            return (ProfWin*)xmlwin;
        } else {
            return NULL;
        }
    }

    ProfChatWin *chatwin = wins_get_chat(str);
    if (chatwin) {
        return (ProfWin*)chatwin;
    }

    jabber_conn_status_t conn_status = connection_get_status();
    if (conn_status == JABBER_CONNECTED) {
        char *barejid = roster_barejid_from_name(str);
        if (barejid) {
            ProfChatWin *chatwin = wins_get_chat(barejid);
            if (chatwin) {
                return (ProfWin*)chatwin;
            }
        }
    }

    ProfMucWin *mucwin = wins_get_muc(str);
    if (mucwin) {
        return (ProfWin*)mucwin;
    }

    ProfPrivateWin *privwin = wins_get_private(str);
    if (privwin) {
        return (ProfWin*)privwin;
    }

    ProfPluginWin *pluginwin = wins_get_plugin(str);
    if (pluginwin) {
        return (ProfWin*)pluginwin;
    }

    return NULL;
}

ProfWin*
wins_get_next(void)
{
    // get and sort win nums
    GList *keys = g_hash_table_get_keys(windows);
    keys = g_list_sort(keys, _wins_cmp_num);
    GList *curr = keys;

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
        g_list_free(keys);
        return wins_get_by_num(next);
    // otherwise return the first window (console)
    } else {
        g_list_free(keys);
        return wins_get_console();
    }
}

ProfWin*
wins_get_previous(void)
{
    // get and sort win nums
    GList *keys = g_hash_table_get_keys(windows);
    keys = g_list_sort(keys, _wins_cmp_num);
    GList *curr = keys;

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
        g_list_free(keys);
        return wins_get_by_num(previous);
    // otherwise return the last window
    } else {
        int new_num = GPOINTER_TO_INT(g_list_last(keys)->data);
        g_list_free(keys);
        return wins_get_by_num(new_num);
    }
}

int
wins_get_num(ProfWin *window)
{
    GList *keys = g_hash_table_get_keys(windows);
    GList *curr = keys;

    while (curr) {
        gconstpointer num_p = curr->data;
        ProfWin *curr_win = g_hash_table_lookup(windows, num_p);
        if (curr_win == window) {
            g_list_free(keys);
            return GPOINTER_TO_INT(num_p);
        }
        curr = g_list_next(curr);
    }

    g_list_free(keys);
    return -1;
}

int
wins_get_current_num(void)
{
    return current;
}

void
wins_close_current(void)
{
    wins_close_by_num(current);
}

void
wins_close_by_num(int i)
{
    // console cannot be closed
    if (i != 1) {

        // go to console if closing current window
        if (i == current) {
            current = 1;
            ProfWin *window = wins_get_current();
            win_update_virtual(window);
        }

        ProfWin *window = wins_get_by_num(i);
        if (window) {
            // cancel upload proccesses of this window
            GSList *upload_process = upload_processes;
            while (upload_process) {
                HTTPUpload *upload = upload_process->data;
                if (upload->window == window) {
                    upload->cancel = 1;
                    break;
                }
                upload_process = g_slist_next(upload_process);
            }

            switch (window->type) {
            case WIN_CHAT:
            {
                ProfChatWin *chatwin = (ProfChatWin*)window;
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

                break;
            }
            case WIN_MUC:
            {
                ProfMucWin *mucwin = (ProfMucWin*)window;
                autocomplete_remove(wins_ac, mucwin->roomjid);
                autocomplete_remove(wins_close_ac, mucwin->roomjid);
                break;
            }
            case WIN_PRIVATE:
            {
                ProfPrivateWin *privwin = (ProfPrivateWin*)window;
                autocomplete_remove(wins_ac, privwin->fulljid);
                autocomplete_remove(wins_close_ac, privwin->fulljid);
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
                ProfPluginWin *pluginwin = (ProfPluginWin*)window;
                plugins_close_win(pluginwin->plugin_name, pluginwin->tag);
                autocomplete_remove(wins_ac, pluginwin->tag);
                autocomplete_remove(wins_close_ac, pluginwin->tag);
                break;
            }
            case WIN_MUC_CONFIG:
            default:
                break;
            }
        }

        g_hash_table_remove(windows, GINT_TO_POINTER(i));
        status_bar_inactive(i);
    }
}

gboolean
wins_is_current(ProfWin *window)
{
    ProfWin *current_window = wins_get_current();

    if (current_window == window) {
        return TRUE;
    } else {
        return FALSE;
    }
}

ProfWin*
wins_new_xmlconsole(void)
{
    GList *keys = g_hash_table_get_keys(windows);
    int result = _wins_get_next_available_num(keys);
    g_list_free(keys);
    ProfWin *newwin = win_create_xmlconsole();
    g_hash_table_insert(windows, GINT_TO_POINTER(result), newwin);
    autocomplete_add(wins_ac, "xmlconsole");
    autocomplete_add(wins_close_ac, "xmlconsole");
    return newwin;
}

ProfWin*
wins_new_chat(const char *const barejid)
{
    GList *keys = g_hash_table_get_keys(windows);
    int result = _wins_get_next_available_num(keys);
    g_list_free(keys);
    ProfWin *newwin = win_create_chat(barejid);
    g_hash_table_insert(windows, GINT_TO_POINTER(result), newwin);

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

    return newwin;
}

ProfWin*
wins_new_muc(const char *const roomjid)
{
    GList *keys = g_hash_table_get_keys(windows);
    int result = _wins_get_next_available_num(keys);
    g_list_free(keys);
    ProfWin *newwin = win_create_muc(roomjid);
    g_hash_table_insert(windows, GINT_TO_POINTER(result), newwin);
    autocomplete_add(wins_ac, roomjid);
    autocomplete_add(wins_close_ac, roomjid);
    return newwin;
}

ProfWin*
wins_new_muc_config(const char *const roomjid, DataForm *form)
{
    GList *keys = g_hash_table_get_keys(windows);
    int result = _wins_get_next_available_num(keys);
    g_list_free(keys);
    ProfWin *newwin = win_create_muc_config(roomjid, form);
    g_hash_table_insert(windows, GINT_TO_POINTER(result), newwin);
    return newwin;
}

ProfWin*
wins_new_private(const char *const fulljid)
{
    GList *keys = g_hash_table_get_keys(windows);
    int result = _wins_get_next_available_num(keys);
    g_list_free(keys);
    ProfWin *newwin = win_create_private(fulljid);
    g_hash_table_insert(windows, GINT_TO_POINTER(result), newwin);
    autocomplete_add(wins_ac, fulljid);
    autocomplete_add(wins_close_ac, fulljid);
    return newwin;
}

ProfWin *
wins_new_plugin(const char *const plugin_name, const char * const tag)
{
    GList *keys = g_hash_table_get_keys(windows);
    int result = _wins_get_next_available_num(keys);
    g_list_free(keys);
    ProfWin *newwin = win_create_plugin(plugin_name, tag);
    g_hash_table_insert(windows, GINT_TO_POINTER(result), newwin);
    autocomplete_add(wins_ac, tag);
    autocomplete_add(wins_close_ac, tag);
    return newwin;
}

gboolean
wins_do_notify_remind(void)
{
    GList *values = g_hash_table_get_values(windows);
    GList *curr = values;

    while (curr) {
        ProfWin *window = curr->data;
        if (win_notify_remind(window)) {
            g_list_free(values);
            return TRUE;
        }
        curr = g_list_next(curr);
    }
    g_list_free(values);
    return FALSE;
}

int
wins_get_total_unread(void)
{
    int result = 0;
    GList *values = g_hash_table_get_values(windows);
    GList *curr = values;

    while (curr) {
        ProfWin *window = curr->data;
        result += win_unread(window);
        curr = g_list_next(curr);
    }
    g_list_free(values);
    return result;
}

void
wins_resize_all(void)
{
    GList *values = g_hash_table_get_values(windows);
    GList *curr = values;
    while (curr) {
        ProfWin *window = curr->data;
        win_resize(window);
        curr = g_list_next(curr);
    }
    g_list_free(values);

    ProfWin *current_win = wins_get_current();
    win_update_virtual(current_win);
}

void
wins_hide_subwin(ProfWin *window)
{
    win_hide_subwin(window);

    ProfWin *current_win = wins_get_current();
    win_refresh_without_subwin(current_win);
}

void
wins_show_subwin(ProfWin *window)
{
    win_show_subwin(window);

    ProfWin *current_win = wins_get_current();
    win_refresh_with_subwin(current_win);
}

ProfXMLWin*
wins_get_xmlconsole(void)
{
    GList *values = g_hash_table_get_values(windows);
    GList *curr = values;

    while (curr) {
        ProfWin *window = curr->data;
        if (window->type == WIN_XML) {
            ProfXMLWin *xmlwin = (ProfXMLWin*)window;
            assert(xmlwin->memcheck == PROFXMLWIN_MEMCHECK);
            g_list_free(values);
            return xmlwin;
        }
        curr = g_list_next(curr);
    }

    g_list_free(values);
    return NULL;
}

GSList*
wins_get_chat_recipients(void)
{
    GSList *result = NULL;
    GList *values = g_hash_table_get_values(windows);
    GList *curr = values;

    while (curr) {
        ProfWin *window = curr->data;
        if (window->type == WIN_CHAT) {
            ProfChatWin *chatwin = (ProfChatWin*)window;
            result = g_slist_append(result, chatwin->barejid);
        }
        curr = g_list_next(curr);
    }
    g_list_free(values);
    return result;
}

GSList*
wins_get_prune_wins(void)
{
    GSList *result = NULL;
    GList *values = g_hash_table_get_values(windows);
    GList *curr = values;

    while (curr) {
        ProfWin *window = curr->data;
        if (win_unread(window) == 0 &&
                window->type != WIN_MUC &&
                window->type != WIN_MUC_CONFIG &&
                window->type != WIN_XML &&
                window->type != WIN_CONSOLE) {
            result = g_slist_append(result, window);
        }
        curr = g_list_next(curr);
    }
    g_list_free(values);
    return result;
}

void
wins_lost_connection(void)
{
    GList *values = g_hash_table_get_values(windows);
    GList *curr = values;

    while (curr) {
        ProfWin *window = curr->data;
        if (window->type != WIN_CONSOLE) {
            win_println(window, THEME_ERROR, '-', "Lost connection.");

            // if current win, set current_win_dirty
            if (wins_is_current(window)) {
                win_update_virtual(window);
            }
        }
        curr = g_list_next(curr);
    }
    g_list_free(values);
}

gboolean
wins_swap(int source_win, int target_win)
{
    ProfWin *source = g_hash_table_lookup(windows, GINT_TO_POINTER(source_win));
    ProfWin *console = wins_get_console();

    if (source) {
        ProfWin *target = g_hash_table_lookup(windows, GINT_TO_POINTER(target_win));

        // target window empty
        if (!target) {
            g_hash_table_steal(windows, GINT_TO_POINTER(source_win));
            g_hash_table_insert(windows, GINT_TO_POINTER(target_win), source);
            status_bar_inactive(source_win);
            if (win_unread(source) > 0) {
                status_bar_new(target_win);
            } else {
                status_bar_active(target_win);
            }
            if (wins_get_current_num() == source_win) {
                wins_set_current_by_num(target_win);
                ui_focus_win(console);
            }
            return TRUE;

        // target window occupied
        } else {
            g_hash_table_steal(windows, GINT_TO_POINTER(source_win));
            g_hash_table_steal(windows, GINT_TO_POINTER(target_win));
            g_hash_table_insert(windows, GINT_TO_POINTER(source_win), target);
            g_hash_table_insert(windows, GINT_TO_POINTER(target_win), source);
            if (win_unread(source) > 0) {
                status_bar_new(target_win);
            } else {
                status_bar_active(target_win);
            }
            if (win_unread(target) > 0) {
                status_bar_new(source_win);
            } else {
                status_bar_active(source_win);
            }
            if ((wins_get_current_num() == source_win) || (wins_get_current_num() == target_win)) {
                ui_focus_win(console);
            }
            return TRUE;
        }
    } else {
        return FALSE;
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
_wins_get_next_available_num(GList *used)
{
    // only console used
    if (g_list_length(used) == 1) {
        return 2;
    } else {
        GList *sorted = NULL;
        GList *curr = used;
        while (curr) {
            sorted = g_list_insert_sorted(sorted, curr->data, _wins_cmp_num);
            curr = g_list_next(curr);
        }

        int result = 0;
        int last_num = 1;
        curr = sorted;
        // skip console
        curr = g_list_next(curr);
        while (curr) {
            int curr_num = GPOINTER_TO_INT(curr->data);

            if (((last_num != 9) && ((last_num + 1) != curr_num)) ||
                    ((last_num == 9) && (curr_num != 0))) {
                result = last_num + 1;
                if (result == 10) {
                    result = 0;
                }
                g_list_free(sorted);
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

        g_list_free(sorted);
        return result;
    }
}

gboolean
wins_tidy(void)
{
    gboolean tidy_required = FALSE;
    // check for gaps
    GList *keys = g_hash_table_get_keys(windows);
    keys = g_list_sort(keys, _wins_cmp_num);

    // get last used
    GList *last = g_list_last(keys);
    int last_num = GPOINTER_TO_INT(last->data);

    // find first free num TODO - Will sort again
    int next_available = _wins_get_next_available_num(keys);

    // found gap (next available before last window)
    if (_wins_cmp_num(GINT_TO_POINTER(next_available), GINT_TO_POINTER(last_num)) < 0) {
        tidy_required = TRUE;
    }

    if (tidy_required) {
        status_bar_set_all_inactive();
        GHashTable *new_windows = g_hash_table_new_full(g_direct_hash,
            g_direct_equal, NULL, (GDestroyNotify)win_free);

        int num = 1;
        GList *curr = keys;
        while (curr) {
            ProfWin *window = g_hash_table_lookup(windows, curr->data);
            g_hash_table_steal(windows, curr->data);
            if (num == 10) {
                g_hash_table_insert(new_windows, GINT_TO_POINTER(0), window);
                if (win_unread(window) > 0) {
                    status_bar_new(0);
                } else {
                    status_bar_active(0);
                }
            } else {
                g_hash_table_insert(new_windows, GINT_TO_POINTER(num), window);
                if (win_unread(window) > 0) {
                    status_bar_new(num);
                } else {
                    status_bar_active(num);
                }
            }
            num++;
            curr = g_list_next(curr);
        }

        g_hash_table_destroy(windows);
        windows = new_windows;
        current = 1;
        ProfWin *console = wins_get_console();
        ui_focus_win(console);
        g_list_free(keys);
        return TRUE;
    } else {
        g_list_free(keys);
        return FALSE;
    }
}

GSList*
wins_create_summary(gboolean unread)
{
    if (unread && wins_get_total_unread() == 0) {
        return NULL;
    }

    GSList *result = NULL;

    GList *keys = g_hash_table_get_keys(windows);
    keys = g_list_sort(keys, _wins_cmp_num);
    GList *curr = keys;

    while (curr) {
        ProfWin *window = g_hash_table_lookup(windows, curr->data);
        if (!unread || (unread && win_unread(window) > 0)) {
            GString *line = g_string_new("");

            int ui_index = GPOINTER_TO_INT(curr->data);
            char *winstring = win_to_string(window);
            if (!winstring) {
                g_string_free(line, TRUE);
                continue;
            }

            g_string_append_printf(line, "%d: %s", ui_index, winstring);
            free(winstring);

            result = g_slist_append(result, strdup(line->str));
            g_string_free(line, TRUE);
        }

        curr = g_list_next(curr);
    }

    g_list_free(keys);

    return result;
}

char*
win_autocomplete(const char *const search_str, gboolean previous)
{
    return autocomplete_complete(wins_ac, search_str, TRUE, previous);
}

char*
win_close_autocomplete(const char *const search_str, gboolean previous)
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
    g_hash_table_destroy(windows);
    autocomplete_free(wins_ac);
    autocomplete_free(wins_close_ac);
}
