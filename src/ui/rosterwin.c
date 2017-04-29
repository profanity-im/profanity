/*
 * rosterwin.c
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

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "config/preferences.h"
#include "ui/ui.h"
#include "ui/window.h"
#include "ui/window_list.h"
#include "xmpp/roster_list.h"
#include "xmpp/contact.h"

typedef enum {
    ROSTER_CONTACT,
    ROSTER_CONTACT_ACTIVE,
    ROSTER_CONTACT_UNREAD
} roster_contact_theme_t;

static void _rosterwin_contacts_all(ProfLayoutSplit *layout);
static void _rosterwin_contacts_by_presence(ProfLayoutSplit *layout, const char *const presence, char *title);
static void _rosterwin_contacts_by_group(ProfLayoutSplit *layout, char *group);
static void _rosteriwin_unsubscribed(ProfLayoutSplit *layout);
static void _rosterwin_contacts_header(ProfLayoutSplit *layout, const char *title, GSList *contacts);
static void _rosterwin_unsubscribed_header(ProfLayoutSplit *layout, GList *wins);

static void _rosterwin_contact(ProfLayoutSplit *layout, PContact contact);
static void _rosterwin_unsubscribed_item(ProfLayoutSplit *layout, ProfChatWin *chatwin);
static void _rosterwin_presence(ProfLayoutSplit *layout, const char *presence, const char *status,
    int current_indent);
static void _rosterwin_resources(ProfLayoutSplit *layout, PContact contact, int current_indent,
    roster_contact_theme_t theme_type, int unread);

static void _rosterwin_rooms(ProfLayoutSplit *layout, char *title, GList *rooms);
static void _rosterwin_rooms_by_service(ProfLayoutSplit *layout);
static void _rosterwin_rooms_header(ProfLayoutSplit *layout, GList *rooms, char *title);
static void _rosterwin_room(ProfLayoutSplit *layout, ProfMucWin *mucwin);

static void _rosterwin_private_chats(ProfLayoutSplit *layout, GList *orphaned_privchats);
static void _rosterwin_private_header(ProfLayoutSplit *layout, GList *privs);

static GSList* _filter_contacts(GSList *contacts);
static GSList* _filter_contacts_with_presence(GSList *contacts, const char *const presence);
static theme_item_t _get_roster_theme(roster_contact_theme_t theme_type, const char *presence);
static int _compare_rooms_name(ProfMucWin *a, ProfMucWin *b);
static int _compare_rooms_unread(ProfMucWin *a, ProfMucWin *b);

void
rosterwin_roster(void)
{
    ProfWin *console = wins_get_console();
    if (!console) {
        return;
    }

    jabber_conn_status_t conn_status = connection_get_status();
    if (conn_status != JABBER_CONNECTED) {
        return;
    }

    ProfLayoutSplit *layout = (ProfLayoutSplit*)console->layout;
    assert(layout->memcheck == LAYOUT_SPLIT_MEMCHECK);
    werase(layout->subwin);

    char *roomspos = prefs_get_string(PREF_ROSTER_ROOMS_POS);
    if (prefs_get_boolean(PREF_ROSTER_ROOMS) && (g_strcmp0(roomspos, "first") == 0)) {
        char *roomsbypref = prefs_get_string(PREF_ROSTER_ROOMS_BY);
        if (g_strcmp0(roomsbypref, "service") == 0) {
            _rosterwin_rooms_by_service(layout);
        } else {
            GList *rooms = muc_rooms();
            _rosterwin_rooms(layout, "Rooms", rooms);
            g_list_free(rooms);
        }
        prefs_free_string(roomsbypref);

        GList *orphaned_privchats = NULL;
        GList *privchats = wins_get_private_chats(NULL);
        GList *curr = privchats;
        while (curr) {
            ProfPrivateWin *privwin = curr->data;
            Jid *jidp = jid_create(privwin->fulljid);
            if (!muc_active(jidp->barejid)) {
                orphaned_privchats = g_list_append(orphaned_privchats, privwin);
            }
            jid_destroy(jidp);
            curr = g_list_next(curr);
        }

        char *privpref = prefs_get_string(PREF_ROSTER_PRIVATE);
        if (g_strcmp0(privpref, "group") == 0 || orphaned_privchats) {
            _rosterwin_private_chats(layout, orphaned_privchats);
        }
        prefs_free_string(privpref);
        g_list_free(orphaned_privchats);

    }

    if (prefs_get_boolean(PREF_ROSTER_CONTACTS)) {
        char *by = prefs_get_string(PREF_ROSTER_BY);
        if (g_strcmp0(by, "presence") == 0) {
            _rosterwin_contacts_by_presence(layout, "chat", "Available for chat");
            _rosterwin_contacts_by_presence(layout, "online", "Online");
            _rosterwin_contacts_by_presence(layout, "away", "Away");
            _rosterwin_contacts_by_presence(layout, "xa", "Extended Away");
            _rosterwin_contacts_by_presence(layout, "dnd", "Do not disturb");
            _rosterwin_contacts_by_presence(layout, "offline", "Offline");
        } else if (g_strcmp0(by, "group") == 0) {
            GList *groups = roster_get_groups();
            GList *curr_group = groups;
            while (curr_group) {
                _rosterwin_contacts_by_group(layout, curr_group->data);
                curr_group = g_list_next(curr_group);
            }
            g_list_free_full(groups, free);
            _rosterwin_contacts_by_group(layout, NULL);
        } else {
            _rosterwin_contacts_all(layout);
        }

        if (prefs_get_boolean(PREF_ROSTER_UNSUBSCRIBED)) {
            _rosteriwin_unsubscribed(layout);
        }
        prefs_free_string(by);
    }

    if (prefs_get_boolean(PREF_ROSTER_ROOMS) && (g_strcmp0(roomspos, "last") == 0)) {
        char *roomsbypref = prefs_get_string(PREF_ROSTER_ROOMS_BY);
        if (g_strcmp0(roomsbypref, "service") == 0) {
            _rosterwin_rooms_by_service(layout);
        } else {
            GList *rooms = muc_rooms();
            _rosterwin_rooms(layout, "Rooms", rooms);
            g_list_free(rooms);
        }
        prefs_free_string(roomsbypref);

        GList *orphaned_privchats = NULL;
        GList *privchats = wins_get_private_chats(NULL);
        GList *curr = privchats;
        while (curr) {
            ProfPrivateWin *privwin = curr->data;
            Jid *jidp = jid_create(privwin->fulljid);
            if (!muc_active(jidp->barejid)) {
                orphaned_privchats = g_list_append(orphaned_privchats, privwin);
            }
            jid_destroy(jidp);
            curr = g_list_next(curr);
        }

        char *privpref = prefs_get_string(PREF_ROSTER_PRIVATE);
        if (g_strcmp0(privpref, "group") == 0 || orphaned_privchats) {
            _rosterwin_private_chats(layout, orphaned_privchats);
        }
        prefs_free_string(privpref);
        g_list_free(orphaned_privchats);
    }

    prefs_free_string(roomspos);
}

static void
_rosterwin_contacts_all(ProfLayoutSplit *layout)
{
    GSList *contacts = NULL;

    char *order = prefs_get_string(PREF_ROSTER_ORDER);
    if (g_strcmp0(order, "presence") == 0) {
        contacts = roster_get_contacts(ROSTER_ORD_PRESENCE);
    } else {
        contacts = roster_get_contacts(ROSTER_ORD_NAME);
    }
    prefs_free_string(order);

    GSList *filtered_contacts = _filter_contacts(contacts);
    g_slist_free(contacts);

    _rosterwin_contacts_header(layout, "Roster", filtered_contacts);

    if (filtered_contacts) {
        GSList *curr_contact = filtered_contacts;
        while (curr_contact) {
            PContact contact = curr_contact->data;
            _rosterwin_contact(layout, contact);
            curr_contact = g_slist_next(curr_contact);
        }
    }
    g_slist_free(filtered_contacts);
}

static void
_rosteriwin_unsubscribed(ProfLayoutSplit *layout)
{
    GList *wins = wins_get_chat_unsubscribed();
    if (wins) {
        _rosterwin_unsubscribed_header(layout, wins);
    }

    GList *curr = wins;
    while (curr) {
        ProfChatWin *chatwin = curr->data;
        _rosterwin_unsubscribed_item(layout, chatwin);
        curr = g_list_next(curr);
    }

    g_list_free(wins);
}

static void
_rosterwin_contacts_by_presence(ProfLayoutSplit *layout, const char *const presence, char *title)
{
    GSList *contacts = roster_get_contacts_by_presence(presence);
    GSList *filtered_contacts = _filter_contacts_with_presence(contacts, presence);
    g_slist_free(contacts);

    // if this group has contacts, or if we want to show empty groups
    if (filtered_contacts || prefs_get_boolean(PREF_ROSTER_EMPTY)) {
        _rosterwin_contacts_header(layout, title, filtered_contacts);
    }

    if (filtered_contacts) {
        GSList *curr_contact = filtered_contacts;
        while (curr_contact) {
            PContact contact = curr_contact->data;
            _rosterwin_contact(layout, contact);
            curr_contact = g_slist_next(curr_contact);
        }
    }
    g_slist_free(filtered_contacts);
}

static void
_rosterwin_contacts_by_group(ProfLayoutSplit *layout, char *group)
{
    GSList *contacts = NULL;

    char *order = prefs_get_string(PREF_ROSTER_ORDER);
    if (g_strcmp0(order, "presence") == 0) {
        contacts = roster_get_group(group, ROSTER_ORD_PRESENCE);
    } else {
        contacts = roster_get_group(group, ROSTER_ORD_NAME);
    }
    prefs_free_string(order);

    GSList *filtered_contacts = _filter_contacts(contacts);
    g_slist_free(contacts);

    if (filtered_contacts || prefs_get_boolean(PREF_ROSTER_EMPTY)) {
        if (group) {
            _rosterwin_contacts_header(layout, group, filtered_contacts);
        } else {
            _rosterwin_contacts_header(layout, "no group", filtered_contacts);
        }

        GSList *curr_contact = filtered_contacts;
        while (curr_contact) {
            PContact contact = curr_contact->data;
            _rosterwin_contact(layout, contact);
            curr_contact = g_slist_next(curr_contact);
        }
    }
    g_slist_free(filtered_contacts);
}

static void
_rosterwin_unsubscribed_item(ProfLayoutSplit *layout, ProfChatWin *chatwin)
{
    const char *const name = chatwin->barejid;
    const char *const presence = "offline";
    int unread = 0;

    roster_contact_theme_t theme_type = ROSTER_CONTACT;
    if (chatwin->unread > 0) {
        theme_type = ROSTER_CONTACT_UNREAD;
        unread = chatwin->unread;
    } else {
        theme_type = ROSTER_CONTACT_ACTIVE;
    }

    theme_item_t presence_colour = _get_roster_theme(theme_type, presence);

    wattron(layout->subwin, theme_attrs(presence_colour));
    GString *msg = g_string_new(" ");
    int indent = prefs_get_roster_contact_indent();
    int current_indent = 0;
    if (indent > 0) {
        current_indent += indent;
        while (indent > 0) {
            g_string_append(msg, " ");
            indent--;
        }
    }
    char ch = prefs_get_roster_contact_char();
    if (ch) {
        g_string_append_printf(msg, "%c", ch);
    }

    char *unreadpos = prefs_get_string(PREF_ROSTER_UNREAD);
    if ((g_strcmp0(unreadpos, "before") == 0) && unread > 0) {
        g_string_append_printf(msg, "(%d) ", unread);
        unread = 0;
    }
    g_string_append(msg, name);
    if ((g_strcmp0(unreadpos, "after") == 0) && unread > 0) {
        g_string_append_printf(msg, " (%d)", unread);
        unread = 0;
    }
    prefs_free_string(unreadpos);

    win_sub_newline_lazy(layout->subwin);
    gboolean wrap = prefs_get_boolean(PREF_ROSTER_WRAP);
    win_sub_print(layout->subwin, msg->str, FALSE, wrap, current_indent);
    g_string_free(msg, TRUE);
    wattroff(layout->subwin, theme_attrs(presence_colour));
}

static void
_rosterwin_contact(ProfLayoutSplit *layout, PContact contact)
{
    const char *name = p_contact_name_or_jid(contact);
    const char *presence = p_contact_presence(contact);
    const char *status = p_contact_status(contact);
    const char *barejid = p_contact_barejid(contact);
    int unread = 0;

    roster_contact_theme_t theme_type = ROSTER_CONTACT;
    ProfChatWin *chatwin = wins_get_chat(barejid);
    if (chatwin) {
        if (chatwin->unread > 0) {
            theme_type = ROSTER_CONTACT_UNREAD;
            unread = chatwin->unread;
        } else {
            theme_type = ROSTER_CONTACT_ACTIVE;
        }
    }

    theme_item_t presence_colour = _get_roster_theme(theme_type, presence);

    wattron(layout->subwin, theme_attrs(presence_colour));
    GString *msg = g_string_new(" ");
    int indent = prefs_get_roster_contact_indent();
    int current_indent = 0;
    if (indent > 0) {
        current_indent += indent;
        while (indent > 0) {
            g_string_append(msg, " ");
            indent--;
        }
    }
    char ch = prefs_get_roster_contact_char();
    if (ch) {
        g_string_append_printf(msg, "%c", ch);
    }

    char *unreadpos = prefs_get_string(PREF_ROSTER_UNREAD);
    if ((g_strcmp0(unreadpos, "before") == 0) && unread > 0) {
        g_string_append_printf(msg, "(%d) ", unread);
        unread = 0;
    }
    g_string_append(msg, name);
    if (g_strcmp0(unreadpos, "after") == 0) {
        if (!prefs_get_boolean(PREF_ROSTER_RESOURCE)) {
            if (unread > 0) {
                g_string_append_printf(msg, " (%d)", unread);
            }
            unread = 0;
        }
    }
    prefs_free_string(unreadpos);

    win_sub_newline_lazy(layout->subwin);
    gboolean wrap = prefs_get_boolean(PREF_ROSTER_WRAP);
    win_sub_print(layout->subwin, msg->str, FALSE, wrap, current_indent);
    g_string_free(msg, TRUE);
    wattroff(layout->subwin, theme_attrs(presence_colour));

    if (prefs_get_boolean(PREF_ROSTER_RESOURCE)) {
        _rosterwin_resources(layout, contact, current_indent, theme_type, unread);
    } else if (prefs_get_boolean(PREF_ROSTER_PRESENCE) || prefs_get_boolean(PREF_ROSTER_STATUS)) {
        if (unread > 0) {
            GString *unreadmsg = g_string_new("");
            g_string_append_printf(unreadmsg, " (%d)", unread);
            wattron(layout->subwin, theme_attrs(presence_colour));
            win_sub_print(layout->subwin, unreadmsg->str, FALSE, wrap, current_indent);
            g_string_free(unreadmsg, TRUE);
            wattroff(layout->subwin, theme_attrs(presence_colour));
        }

        _rosterwin_presence(layout, presence, status, current_indent);
    }
}

static void
_rosterwin_presence(ProfLayoutSplit *layout, const char *presence, const char *status,
    int current_indent)
{
    // don't show presence for offline contacts
    gboolean is_offline = g_strcmp0(presence, "offline") == 0;
    if (is_offline) {
        return;
    }

    char *by = prefs_get_string(PREF_ROSTER_BY);
    gboolean by_presence = g_strcmp0(by, "presence") == 0;
    prefs_free_string(by);

    int presence_indent = prefs_get_roster_presence_indent();
    if (presence_indent > 0) {
        current_indent += presence_indent;
    }

    gboolean wrap = prefs_get_boolean(PREF_ROSTER_WRAP);
    theme_item_t colour = _get_roster_theme(ROSTER_CONTACT, presence);

    // show only status when grouped by presence
    if (by_presence) {
        if (status && prefs_get_boolean(PREF_ROSTER_STATUS)) {

            wattron(layout->subwin, theme_attrs(colour));
            if (presence_indent == -1) {
                GString *msg = g_string_new("");
                g_string_append_printf(msg, ": \"%s\"", status);
                win_sub_print(layout->subwin, msg->str, FALSE, wrap, current_indent);
                g_string_free(msg, TRUE);
                wattroff(layout->subwin, theme_attrs(colour));
            } else {
                GString *msg = g_string_new(" ");
                while (current_indent > 0) {
                    g_string_append(msg, " ");
                    current_indent--;
                }
                g_string_append_printf(msg, "\"%s\"", status);
                win_sub_newline_lazy(layout->subwin);
                win_sub_print(layout->subwin, msg->str, FALSE, wrap, current_indent);
                g_string_free(msg, TRUE);
                wattroff(layout->subwin, theme_attrs(colour));
            }
        }

    // show both presence and status when not grouped by presence
    } else if (prefs_get_boolean(PREF_ROSTER_PRESENCE) || (status && prefs_get_boolean(PREF_ROSTER_STATUS))) {
        wattron(layout->subwin, theme_attrs(colour));
        if (presence_indent == -1) {
            GString *msg = g_string_new("");
            if (prefs_get_boolean(PREF_ROSTER_PRESENCE)) {
                g_string_append_printf(msg, ": %s", presence);
                if (status && prefs_get_boolean(PREF_ROSTER_STATUS)) {
                    g_string_append_printf(msg, " \"%s\"", status);
                }
            } else if (status && prefs_get_boolean(PREF_ROSTER_STATUS)) {
                g_string_append_printf(msg, ": \"%s\"", status);
            }
            win_sub_print(layout->subwin, msg->str, FALSE, wrap, current_indent);
            g_string_free(msg, TRUE);
            wattroff(layout->subwin, theme_attrs(colour));
        } else {
            GString *msg = g_string_new(" ");
            while (current_indent > 0) {
                g_string_append(msg, " ");
                current_indent--;
            }
            if (prefs_get_boolean(PREF_ROSTER_PRESENCE)) {
                g_string_append(msg, presence);
                if (status && prefs_get_boolean(PREF_ROSTER_STATUS)) {
                    g_string_append_printf(msg, " \"%s\"", status);
                }
            } else if (status && prefs_get_boolean(PREF_ROSTER_STATUS)) {
                g_string_append_printf(msg, "\"%s\"", status);
            }
            win_sub_newline_lazy(layout->subwin);
            win_sub_print(layout->subwin, msg->str, FALSE, wrap, current_indent);
            g_string_free(msg, TRUE);
            wattroff(layout->subwin, theme_attrs(colour));
        }
    }
}

static void
_rosterwin_resources(ProfLayoutSplit *layout, PContact contact, int current_indent, roster_contact_theme_t theme_type,
    int unread)
{
    gboolean join = prefs_get_boolean(PREF_ROSTER_RESOURCE_JOIN);

    GList *resources = p_contact_get_available_resources(contact);
    if (resources) {

        // resource on same line as contact
        if (join && (g_list_length(resources) == 1)) {
            Resource *resource = resources->data;
            const char *resource_presence = string_from_resource_presence(resource->presence);
            theme_item_t resource_presence_colour = _get_roster_theme(theme_type, resource_presence);

            wattron(layout->subwin, theme_attrs(resource_presence_colour));
            GString *msg = g_string_new("");
            char ch = prefs_get_roster_resource_char();
            if (ch) {
                g_string_append_printf(msg, "%c", ch);
            } else {
                g_string_append(msg, " ");
            }
            g_string_append(msg, resource->name);
            if (prefs_get_boolean(PREF_ROSTER_PRIORITY)) {
                g_string_append_printf(msg, " %d", resource->priority);
            }

            char *unreadpos = prefs_get_string(PREF_ROSTER_UNREAD);
            if ((g_strcmp0(unreadpos, "after") == 0) && unread > 0) {
                g_string_append_printf(msg, " (%d)", unread);
            }
            prefs_free_string(unreadpos);

            gboolean wrap = prefs_get_boolean(PREF_ROSTER_WRAP);
            win_sub_print(layout->subwin, msg->str, FALSE, wrap, 0);
            g_string_free(msg, TRUE);
            wattroff(layout->subwin, theme_attrs(resource_presence_colour));

            if (prefs_get_boolean(PREF_ROSTER_PRESENCE) || prefs_get_boolean(PREF_ROSTER_STATUS)) {
                _rosterwin_presence(layout, resource_presence, resource->status, current_indent);
            }

        // resource(s) on new lines
        } else {
            gboolean wrap = prefs_get_boolean(PREF_ROSTER_WRAP);

            char *unreadpos = prefs_get_string(PREF_ROSTER_UNREAD);
            if ((g_strcmp0(unreadpos, "after") == 0) && unread > 0) {
                GString *unreadmsg = g_string_new("");
                g_string_append_printf(unreadmsg, " (%d)", unread);

                const char *presence = p_contact_presence(contact);
                theme_item_t presence_colour = _get_roster_theme(theme_type, presence);

                wattron(layout->subwin, theme_attrs(presence_colour));
                win_sub_print(layout->subwin, unreadmsg->str, FALSE, wrap, current_indent);
                wattroff(layout->subwin, theme_attrs(presence_colour));
            }
            prefs_free_string(unreadpos);

            int resource_indent = prefs_get_roster_resource_indent();
            if (resource_indent > 0) {
                current_indent += resource_indent;
            }

            GList *curr_resource = resources;
            while (curr_resource) {
                Resource *resource = curr_resource->data;
                const char *resource_presence = string_from_resource_presence(resource->presence);
                theme_item_t resource_presence_colour = _get_roster_theme(ROSTER_CONTACT, resource_presence);

                wattron(layout->subwin, theme_attrs(resource_presence_colour));
                GString *msg = g_string_new(" ");
                int this_indent = current_indent;
                while (this_indent > 0) {
                    g_string_append(msg, " ");
                    this_indent--;
                }
                char ch = prefs_get_roster_resource_char();
                if (ch) {
                    g_string_append_printf(msg, "%c", ch);
                }
                g_string_append(msg, resource->name);
                if (prefs_get_boolean(PREF_ROSTER_PRIORITY)) {
                    g_string_append_printf(msg, " %d", resource->priority);
                }
                win_sub_newline_lazy(layout->subwin);
                win_sub_print(layout->subwin, msg->str, FALSE, wrap, current_indent);
                g_string_free(msg, TRUE);
                wattroff(layout->subwin, theme_attrs(resource_presence_colour));

                if (prefs_get_boolean(PREF_ROSTER_PRESENCE) || prefs_get_boolean(PREF_ROSTER_STATUS)) {
                    _rosterwin_presence(layout, resource_presence, resource->status, current_indent);
                }

                curr_resource = g_list_next(curr_resource);
            }
        }
    } else if (prefs_get_boolean(PREF_ROSTER_PRESENCE) || prefs_get_boolean(PREF_ROSTER_STATUS)) {
        const char *presence = p_contact_presence(contact);
        const char *status = p_contact_status(contact);
        theme_item_t presence_colour = _get_roster_theme(theme_type, presence);
        gboolean wrap = prefs_get_boolean(PREF_ROSTER_WRAP);

        char *unreadpos = prefs_get_string(PREF_ROSTER_UNREAD);
        if ((g_strcmp0(unreadpos, "after") == 0) && unread > 0) {
            GString *unreadmsg = g_string_new("");
            g_string_append_printf(unreadmsg, " (%d)", unread);

            wattron(layout->subwin, theme_attrs(presence_colour));
            win_sub_print(layout->subwin, unreadmsg->str, FALSE, wrap, current_indent);
            wattroff(layout->subwin, theme_attrs(presence_colour));
        }
        prefs_free_string(unreadpos);
        _rosterwin_presence(layout, presence, status, current_indent);
    } else {
        gboolean wrap = prefs_get_boolean(PREF_ROSTER_WRAP);

        char *unreadpos = prefs_get_string(PREF_ROSTER_UNREAD);
        if ((g_strcmp0(unreadpos, "after") == 0) && unread > 0) {
            GString *unreadmsg = g_string_new("");
            g_string_append_printf(unreadmsg, " (%d)", unread);
            const char *presence = p_contact_presence(contact);
            theme_item_t presence_colour = _get_roster_theme(theme_type, presence);

            wattron(layout->subwin, theme_attrs(presence_colour));
            win_sub_print(layout->subwin, unreadmsg->str, FALSE, wrap, current_indent);
            wattroff(layout->subwin, theme_attrs(presence_colour));
        }
        prefs_free_string(unreadpos);
    }

    g_list_free(resources);

}

static void
_rosterwin_rooms(ProfLayoutSplit *layout, char *title, GList *rooms)
{
    GList *rooms_sorted = NULL;
    GList *curr_room = rooms;
    while (curr_room) {
        ProfMucWin *mucwin = wins_get_muc(curr_room->data);
        if (mucwin) {
            char *order = prefs_get_string(PREF_ROSTER_ROOMS_ORDER);
            if (g_strcmp0(order, "unread") == 0) {
                rooms_sorted = g_list_insert_sorted(rooms_sorted, mucwin, (GCompareFunc)_compare_rooms_unread);
            } else {
                rooms_sorted = g_list_insert_sorted(rooms_sorted, mucwin, (GCompareFunc)_compare_rooms_name);
            }
            prefs_free_string(order);
        }
        curr_room = g_list_next(curr_room);
    }

    // if there are active rooms, or if we want to show empty groups
    if (rooms_sorted || prefs_get_boolean(PREF_ROSTER_EMPTY)) {
        _rosterwin_rooms_header(layout, rooms_sorted, title);

        GList *curr_room = rooms_sorted;
        while (curr_room) {
            _rosterwin_room(layout, curr_room->data);
            curr_room = g_list_next(curr_room);
        }
    }

    g_list_free(rooms_sorted);
}

static void
_rosterwin_rooms_by_service(ProfLayoutSplit *layout)
{
    GList *rooms = muc_rooms();
    GList *curr = rooms;
    GList *services = NULL;
    while (curr) {
        char *roomjid = curr->data;
        Jid *jidp = jid_create(roomjid);

        if (!g_list_find_custom(services, jidp->domainpart, (GCompareFunc)g_strcmp0)) {
            services = g_list_insert_sorted(services, strdup(jidp->domainpart), (GCompareFunc)g_strcmp0);
        }

        jid_destroy(jidp);
        curr = g_list_next(curr);
    }

    GList *curr_service = services;
    while (curr_service) {
        char *service = curr_service->data;
        GList *filtered_rooms = NULL;

        curr = rooms;
        while (curr) {
            char *roomjid = curr->data;
            Jid *jidp = jid_create(roomjid);

            if (g_strcmp0(curr_service->data, jidp->domainpart) == 0) {
                filtered_rooms = g_list_append(filtered_rooms, strdup(jidp->barejid));
            }

            jid_destroy(jidp);
            curr = g_list_next(curr);
        }

        _rosterwin_rooms(layout, service, filtered_rooms);

        g_list_free_full(filtered_rooms, free);
        curr_service = g_list_next(curr_service);
    }

    g_list_free(rooms);
    g_list_free_full(services, free);
}

static void
_rosterwin_room(ProfLayoutSplit *layout, ProfMucWin *mucwin)
{
    GString *msg = g_string_new(" ");

    if (mucwin->unread_mentions) {
        wattron(layout->subwin, theme_attrs(THEME_ROSTER_ROOM_MENTION));
    } else if (mucwin->unread_triggers) {
        wattron(layout->subwin, theme_attrs(THEME_ROSTER_ROOM_TRIGGER));
    } else if (mucwin->unread > 0) {
        wattron(layout->subwin, theme_attrs(THEME_ROSTER_ROOM_UNREAD));
    } else {
        wattron(layout->subwin, theme_attrs(THEME_ROSTER_ROOM));
    }

    int indent = prefs_get_roster_contact_indent();
    int current_indent = 0;
    if (indent > 0) {
        current_indent += indent;
        while (indent > 0) {
            g_string_append(msg, " ");
            indent--;
        }
    }
    char ch = prefs_get_roster_room_char();
    if (ch) {
        g_string_append_printf(msg, "%c", ch);
    }

    char *unreadpos = prefs_get_string(PREF_ROSTER_ROOMS_UNREAD);
    if ((g_strcmp0(unreadpos, "before") == 0) && mucwin->unread > 0) {
        g_string_append_printf(msg, "(%d) ", mucwin->unread);
    }
    char *roombypref = prefs_get_string(PREF_ROSTER_ROOMS_BY);
    if (g_strcmp0(roombypref, "service") == 0) {
        Jid *jidp = jid_create(mucwin->roomjid);
        g_string_append(msg, jidp->localpart);
        jid_destroy(jidp);
    } else {
        g_string_append(msg, mucwin->roomjid);
    }
    prefs_free_string(roombypref);
    if ((g_strcmp0(unreadpos, "after") == 0) && mucwin->unread > 0) {
        g_string_append_printf(msg, " (%d)", mucwin->unread);
    }
    prefs_free_string(unreadpos);

    win_sub_newline_lazy(layout->subwin);
    gboolean wrap = prefs_get_boolean(PREF_ROSTER_WRAP);
    win_sub_print(layout->subwin, msg->str, FALSE, wrap, current_indent);
    g_string_free(msg, TRUE);

    if (mucwin->unread_mentions) {
        wattroff(layout->subwin, theme_attrs(THEME_ROSTER_ROOM_MENTION));
    } else if (mucwin->unread_triggers) {
        wattroff(layout->subwin, theme_attrs(THEME_ROSTER_ROOM_TRIGGER));
    } else if (mucwin->unread > 0) {
        wattroff(layout->subwin, theme_attrs(THEME_ROSTER_ROOM_UNREAD));
    } else {
        wattroff(layout->subwin, theme_attrs(THEME_ROSTER_ROOM));
    }

    char *privpref = prefs_get_string(PREF_ROSTER_PRIVATE);
    if (g_strcmp0(privpref, "room") == 0) {
        GList *privs = wins_get_private_chats(mucwin->roomjid);
        GList *curr = privs;
        while (curr) {
            ProfPrivateWin *privwin = curr->data;
            win_sub_newline_lazy(layout->subwin);

            GString *privmsg = g_string_new(" ");
            indent = prefs_get_roster_contact_indent();
            current_indent = 0;
            if (indent > 0) {
                current_indent += indent;
                while (indent > 0) {
                    g_string_append(privmsg, " ");
                    indent--;
                }
            }

            // TODO add preference
            indent = prefs_get_roster_resource_indent();
            if (indent > 0) {
                current_indent += indent;
                while (indent > 0) {
                    g_string_append(privmsg, " ");
                    indent--;
                }
            }

            unreadpos = prefs_get_string(PREF_ROSTER_ROOMS_UNREAD);
            if ((g_strcmp0(unreadpos, "before") == 0) && privwin->unread > 0) {
                g_string_append_printf(privmsg, "(%d) ", privwin->unread);
            }

            ch = prefs_get_roster_room_private_char();
            if (ch) {
                g_string_append_printf(privmsg, "%c", ch);
            }

            char *nick = privwin->fulljid + strlen(mucwin->roomjid) + 1;
            g_string_append(privmsg, nick);

            if ((g_strcmp0(unreadpos, "after") == 0) && privwin->unread > 0) {
                g_string_append_printf(privmsg, " (%d)", privwin->unread);
            }
            prefs_free_string(unreadpos);

            const char *presence = "offline";

            Occupant *occupant = muc_roster_item(mucwin->roomjid, nick);
            if (occupant) {
                presence = string_from_resource_presence(occupant->presence);
            }

            theme_item_t colour;
            if (privwin->unread > 0) {
                colour = _get_roster_theme(ROSTER_CONTACT_UNREAD, presence);
            } else {
                colour = _get_roster_theme(ROSTER_CONTACT_ACTIVE, presence);
            }

            wattron(layout->subwin, theme_attrs(colour));
            win_sub_print(layout->subwin, privmsg->str, FALSE, wrap, current_indent);
            wattroff(layout->subwin, theme_attrs(colour));

            g_string_free(privmsg, TRUE);
            curr = g_list_next(curr);
        }

        g_list_free(privs);
    }

    prefs_free_string(privpref);
}

static void
_rosterwin_private_chats(ProfLayoutSplit *layout, GList *orphaned_privchats)
{
    GList *privs = NULL;

    char *privpref = prefs_get_string(PREF_ROSTER_PRIVATE);
    if (g_strcmp0(privpref, "group") == 0) {
        privs = wins_get_private_chats(NULL);
    } else {
        GList *curr = orphaned_privchats;
        while (curr) {
            privs = g_list_append(privs, curr->data);
            curr = g_list_next(curr);
        }
    }

    if (privs || prefs_get_boolean(PREF_ROSTER_EMPTY)) {
        _rosterwin_private_header(layout, privs);

        GList *curr = privs;
        while (curr) {
            ProfPrivateWin *privwin = curr->data;
            win_sub_newline_lazy(layout->subwin);

            GString *privmsg = g_string_new(" ");
            int indent = prefs_get_roster_contact_indent();
            int current_indent = 0;
            if (indent > 0) {
                current_indent += indent;
                while (indent > 0) {
                    g_string_append(privmsg, " ");
                    indent--;
                }
            }

            char *unreadpos = prefs_get_string(PREF_ROSTER_ROOMS_UNREAD);
            if ((g_strcmp0(unreadpos, "before") == 0) && privwin->unread > 0) {
                g_string_append_printf(privmsg, "(%d) ", privwin->unread);
            }

            char ch = prefs_get_roster_private_char();
            if (ch) {
                g_string_append_printf(privmsg, "%c", ch);
            }

            g_string_append(privmsg, privwin->fulljid);

            if ((g_strcmp0(unreadpos, "after") == 0) && privwin->unread > 0) {
                g_string_append_printf(privmsg, " (%d)", privwin->unread);
            }
            prefs_free_string(unreadpos);

            Jid *jidp = jid_create(privwin->fulljid);
            Occupant *occupant = muc_roster_item(jidp->barejid, jidp->resourcepart);
            jid_destroy(jidp);

            const char *presence = "offline";
            if (occupant) {
                presence = string_from_resource_presence(occupant->presence);
            }

            theme_item_t colour;
            if (privwin->unread > 0) {
                colour = _get_roster_theme(ROSTER_CONTACT_UNREAD, presence);
            } else {
                colour = _get_roster_theme(ROSTER_CONTACT_ACTIVE, presence);
            }

            gboolean wrap = prefs_get_boolean(PREF_ROSTER_WRAP);

            wattron(layout->subwin, theme_attrs(colour));
            win_sub_print(layout->subwin, privmsg->str, FALSE, wrap, current_indent);
            wattroff(layout->subwin, theme_attrs(colour));

            g_string_free(privmsg, TRUE);
            curr = g_list_next(curr);
        }

        g_list_free(privs);
    }
}

static theme_item_t
_get_roster_theme(roster_contact_theme_t theme_type, const char *presence)
{
    switch (theme_type) {
    case ROSTER_CONTACT:        return theme_roster_presence_attrs(presence);
    case ROSTER_CONTACT_ACTIVE: return theme_roster_active_presence_attrs(presence);
    case ROSTER_CONTACT_UNREAD: return theme_roster_unread_presence_attrs(presence);
    default:                    return theme_roster_presence_attrs(presence);
    }
}

static int
_compare_rooms_name(ProfMucWin *a, ProfMucWin *b)
{
    return g_strcmp0(a->roomjid, b->roomjid);
}

static int
_compare_rooms_unread(ProfMucWin *a, ProfMucWin *b)
{
    if (a->unread > b->unread) {
        return -1;
    } else if (a->unread == b->unread) {
        return g_strcmp0(a->roomjid, b->roomjid);
    } else {
        return 1;
    }
}

static void
_rosterwin_unsubscribed_header(ProfLayoutSplit *layout, GList *wins)
{
    win_sub_newline_lazy(layout->subwin);

    GString *header = g_string_new(" ");
    char ch = prefs_get_roster_header_char();
    if (ch) {
        g_string_append_printf(header, "%c", ch);
    }

    g_string_append(header, "Unsubscribed");

    char *countpref = prefs_get_string(PREF_ROSTER_COUNT);
    if (g_strcmp0(countpref, "items") == 0) {
        int itemcount = g_list_length(wins);
        if (itemcount == 0 && prefs_get_boolean(PREF_ROSTER_COUNT_ZERO)) {
            g_string_append_printf(header, " (%d)", itemcount);
        } else {
            g_string_append_printf(header, " (%d)", itemcount);
        }
    } else if (g_strcmp0(countpref, "unread") == 0) {
        int unreadcount = 0;
        GList *curr = wins;
        while (curr) {
            ProfChatWin *chatwin = curr->data;
            unreadcount += chatwin->unread;
            curr = g_list_next(curr);
        }
        if (unreadcount == 0 && prefs_get_boolean(PREF_ROSTER_COUNT_ZERO)) {
            g_string_append_printf(header, " (%d)", unreadcount);
        } else if (unreadcount > 0) {
            g_string_append_printf(header, " (%d)", unreadcount);
        }
    }
    prefs_free_string(countpref);

    gboolean wrap = prefs_get_boolean(PREF_ROSTER_WRAP);

    wattron(layout->subwin, theme_attrs(THEME_ROSTER_HEADER));
    win_sub_print(layout->subwin, header->str, FALSE, wrap, 1);
    wattroff(layout->subwin, theme_attrs(THEME_ROSTER_HEADER));

    g_string_free(header, TRUE);
}

static void
_rosterwin_contacts_header(ProfLayoutSplit *layout, const char *const title, GSList *contacts)
{
    win_sub_newline_lazy(layout->subwin);

    GString *header = g_string_new(" ");
    char ch = prefs_get_roster_header_char();
    if (ch) {
        g_string_append_printf(header, "%c", ch);
    }

    g_string_append(header, title);

    char *countpref = prefs_get_string(PREF_ROSTER_COUNT);
    if (g_strcmp0(countpref, "items") == 0) {
        int itemcount = g_slist_length(contacts);
        if (itemcount == 0 && prefs_get_boolean(PREF_ROSTER_COUNT_ZERO)) {
            g_string_append_printf(header, " (%d)", itemcount);
        } else {
            g_string_append_printf(header, " (%d)", itemcount);
        }
    } else if (g_strcmp0(countpref, "unread") == 0) {
        int unreadcount = 0;
        GSList *curr = contacts;
        while (curr) {
            PContact contact = curr->data;
            const char *barejid = p_contact_barejid(contact);
            ProfChatWin *chatwin = wins_get_chat(barejid);
            if (chatwin) {
                unreadcount += chatwin->unread;
            }
            curr = g_slist_next(curr);
        }
        if (unreadcount == 0 && prefs_get_boolean(PREF_ROSTER_COUNT_ZERO)) {
            g_string_append_printf(header, " (%d)", unreadcount);
        } else if (unreadcount > 0) {
            g_string_append_printf(header, " (%d)", unreadcount);
        }
    }
    prefs_free_string(countpref);

    gboolean wrap = prefs_get_boolean(PREF_ROSTER_WRAP);

    wattron(layout->subwin, theme_attrs(THEME_ROSTER_HEADER));
    win_sub_print(layout->subwin, header->str, FALSE, wrap, 1);
    wattroff(layout->subwin, theme_attrs(THEME_ROSTER_HEADER));

    g_string_free(header, TRUE);
}

static void
_rosterwin_rooms_header(ProfLayoutSplit *layout, GList *rooms, char *title)
{
    win_sub_newline_lazy(layout->subwin);
    GString *header = g_string_new(" ");
    char ch = prefs_get_roster_header_char();
    if (ch) {
        g_string_append_printf(header, "%c", ch);
    }
    g_string_append(header, title);

    char *countpref = prefs_get_string(PREF_ROSTER_COUNT);
    if (g_strcmp0(countpref, "items") == 0) {
        int count = g_list_length(rooms);
        if (count == 0 && prefs_get_boolean(PREF_ROSTER_COUNT_ZERO)) {
            g_string_append_printf(header, " (%d)", count);
        } else if (count > 0) {
            g_string_append_printf(header, " (%d)", count);
        }
    } else if (g_strcmp0(countpref, "unread") == 0) {
        int unread = 0;
        GList *curr = rooms;
        while (curr) {
            ProfMucWin *mucwin = curr->data;
            unread += mucwin->unread;

            // include private chats
            char *prefpriv = prefs_get_string(PREF_ROSTER_PRIVATE);
            if (g_strcmp0(prefpriv, "room") == 0) {
                GList *privwins = wins_get_private_chats(mucwin->roomjid);
                GList *curr_priv = privwins;
                while (curr_priv) {
                    ProfPrivateWin *privwin = curr_priv->data;
                    unread += privwin->unread;
                    curr_priv = g_list_next(curr_priv);
                }
                g_list_free(privwins);
            }
            prefs_free_string(prefpriv);

            curr = g_list_next(curr);
        }

        if (unread == 0 && prefs_get_boolean(PREF_ROSTER_COUNT_ZERO)) {
            g_string_append_printf(header, " (%d)", unread);
        } else if (unread > 0) {
            g_string_append_printf(header, " (%d)", unread);
        }
    }
    prefs_free_string(countpref);

    gboolean wrap = prefs_get_boolean(PREF_ROSTER_WRAP);

    wattron(layout->subwin, theme_attrs(THEME_ROSTER_HEADER));
    win_sub_print(layout->subwin, header->str, FALSE, wrap, 1);
    wattroff(layout->subwin, theme_attrs(THEME_ROSTER_HEADER));

    g_string_free(header, TRUE);
}

static void
_rosterwin_private_header(ProfLayoutSplit *layout, GList *privs)
{
    win_sub_newline_lazy(layout->subwin);

    GString *title_str = g_string_new(" ");
    char ch = prefs_get_roster_header_char();
    if (ch) {
        g_string_append_printf(title_str, "%c", ch);
    }
    g_string_append(title_str, "Private chats");

    char *countpref = prefs_get_string(PREF_ROSTER_COUNT);
    if (g_strcmp0(countpref, "items") == 0) {
        int itemcount = g_list_length(privs);
        if (itemcount == 0 && prefs_get_boolean(PREF_ROSTER_COUNT_ZERO)) {
            g_string_append_printf(title_str, " (%d)", itemcount);
        } else if (itemcount > 0) {
            g_string_append_printf(title_str, " (%d)", itemcount);
        }
    } else if (g_strcmp0(countpref, "unread") == 0) {
        int unreadcount = 0;
        GList *curr = privs;
        while (curr) {
            ProfPrivateWin *privwin = curr->data;
            unreadcount += privwin->unread;
            curr = g_list_next(curr);
        }
        if (unreadcount == 0 && prefs_get_boolean(PREF_ROSTER_COUNT_ZERO)) {
            g_string_append_printf(title_str, " (%d)", unreadcount);
        } else if (unreadcount > 0) {
            g_string_append_printf(title_str, " (%d)", unreadcount);
        }
    }
    prefs_free_string(countpref);

    gboolean wrap = prefs_get_boolean(PREF_ROSTER_WRAP);

    wattron(layout->subwin, theme_attrs(THEME_ROSTER_HEADER));
    win_sub_print(layout->subwin, title_str->str, FALSE, wrap, 1);
    wattroff(layout->subwin, theme_attrs(THEME_ROSTER_HEADER));

    g_string_free(title_str, TRUE);
}

static GSList*
_filter_contacts(GSList *contacts)
{
    GSList *filtered_contacts = NULL;

    // if show offline, include all contacts
    if (prefs_get_boolean(PREF_ROSTER_OFFLINE)) {
        GSList *curr = contacts;
        while (curr) {
            filtered_contacts = g_slist_append(filtered_contacts, curr->data);
            curr = g_slist_next(curr);
        }
    // if dont show offline
    } else {
        GSList *curr = contacts;
        while (curr) {
            PContact contact = curr->data;
            const char *presence = p_contact_presence(contact);

            // include if offline and unread messages
            if (g_strcmp0(presence, "offline") == 0) {
                ProfChatWin *chatwin = wins_get_chat(p_contact_barejid(contact));
                if (chatwin && chatwin->unread > 0) {
                    filtered_contacts = g_slist_append(filtered_contacts, contact);
                }

            // include if not offline
            } else {
                filtered_contacts = g_slist_append(filtered_contacts, contact);
            }
            curr = g_slist_next(curr);
        }
    }

    return filtered_contacts;
}

static GSList*
_filter_contacts_with_presence(GSList *contacts, const char *const presence)
{
    GSList *filtered_contacts = NULL;

    // handling offline contacts
    if (g_strcmp0(presence, "offline") == 0) {

        // if show offline, include all contacts
        if (prefs_get_boolean(PREF_ROSTER_OFFLINE)) {
            GSList *curr = contacts;
            while (curr) {
                filtered_contacts = g_slist_append(filtered_contacts, curr->data);
                curr = g_slist_next(curr);
            }

        // otherwise show if unread messages
        } else {
            GSList *curr = contacts;
            while (curr) {
                PContact contact = curr->data;
                ProfChatWin *chatwin = wins_get_chat(p_contact_barejid(contact));
                if (chatwin && chatwin->unread > 0) {
                    filtered_contacts = g_slist_append(filtered_contacts, contact);
                }
                curr = g_slist_next(curr);
            }
        }

    // any other presence, include all
    } else {
        GSList *curr = contacts;
        while (curr) {
            filtered_contacts = g_slist_append(filtered_contacts, curr->data);
            curr = g_slist_next(curr);
        }
    }

    return filtered_contacts;
}

