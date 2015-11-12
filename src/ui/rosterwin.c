/*
 * rosterwin.c
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

#include <assert.h>
#include <stdlib.h>

#include "contact.h"
#include "ui/ui.h"
#include "ui/window.h"
#include "window_list.h"
#include "config/preferences.h"
#include "roster_list.h"

static void
_rosterwin_contact(ProfLayoutSplit *layout, PContact contact)
{
    const char *name = p_contact_name_or_jid(contact);
    const char *presence = p_contact_presence(contact);

    if ((g_strcmp0(presence, "offline") != 0) || ((g_strcmp0(presence, "offline") == 0) &&
            (prefs_get_boolean(PREF_ROSTER_OFFLINE)))) {
        theme_item_t presence_colour = theme_main_presence_attrs(presence);

        wattron(layout->subwin, theme_attrs(presence_colour));
        GString *msg = g_string_new("   ");
        g_string_append(msg, name);
        win_printline_nowrap(layout->subwin, msg->str);
        g_string_free(msg, TRUE);
        wattroff(layout->subwin, theme_attrs(presence_colour));

        if (prefs_get_boolean(PREF_ROSTER_RESOURCE)) {
            GList *resources = p_contact_get_available_resources(contact);
            GList *curr_resource = resources;
            while (curr_resource) {
                Resource *resource = curr_resource->data;
                const char *resource_presence = string_from_resource_presence(resource->presence);
                theme_item_t resource_presence_colour = theme_main_presence_attrs(resource_presence);

                wattron(layout->subwin, theme_attrs(resource_presence_colour));
                GString *msg = g_string_new("     ");
                g_string_append(msg, resource->name);
                win_printline_nowrap(layout->subwin, msg->str);
                g_string_free(msg, TRUE);
                wattroff(layout->subwin, theme_attrs(resource_presence_colour));

                curr_resource = g_list_next(curr_resource);
            }
            g_list_free(resources);
        }
    }
}

static void
_rosterwin_contacts_by_presence(ProfLayoutSplit *layout, const char *const presence, char *title)
{
    GSList *contacts = roster_get_contacts_by_presence(presence);

    // if this group has contacts, or if we want to show empty groups
    if (contacts || prefs_get_boolean(PREF_ROSTER_EMPTY)) {
        wattron(layout->subwin, theme_attrs(THEME_ROSTER_HEADER));
        win_printline_nowrap(layout->subwin, title);
        wattroff(layout->subwin, theme_attrs(THEME_ROSTER_HEADER));
    }

    if (contacts) {
        GSList *curr_contact = contacts;
        while (curr_contact) {
            PContact contact = curr_contact->data;
            _rosterwin_contact(layout, contact);
            curr_contact = g_slist_next(curr_contact);
        }
    }
    g_slist_free(contacts);
}

static void
_rosterwin_contacts_by_group(ProfLayoutSplit *layout, char *group)
{
    wattron(layout->subwin, theme_attrs(THEME_ROSTER_HEADER));
    GString *title = g_string_new(" -");
    g_string_append(title, group);
    win_printline_nowrap(layout->subwin, title->str);
    g_string_free(title, TRUE);
    wattroff(layout->subwin, theme_attrs(THEME_ROSTER_HEADER));

    GSList *contacts = roster_get_group(group);
    if (contacts) {
        GSList *curr_contact = contacts;
        while (curr_contact) {
            PContact contact = curr_contact->data;
            _rosterwin_contact(layout, contact);
            curr_contact = g_slist_next(curr_contact);
        }
    }
    g_slist_free(contacts);
}

static void
_rosterwin_contacts_by_no_group(ProfLayoutSplit *layout)
{
    GSList *contacts = roster_get_nogroup();
    if (contacts) {
        wattron(layout->subwin, theme_attrs(THEME_ROSTER_HEADER));
        win_printline_nowrap(layout->subwin, " -no group");
        wattroff(layout->subwin, theme_attrs(THEME_ROSTER_HEADER));

        GSList *curr_contact = contacts;
        while (curr_contact) {
            PContact contact = curr_contact->data;
            _rosterwin_contact(layout, contact);
            curr_contact = g_slist_next(curr_contact);
        }
    }
    g_slist_free(contacts);
}

void
rosterwin_roster(void)
{
    ProfWin *console = wins_get_console();
    if (console) {
        ProfLayoutSplit *layout = (ProfLayoutSplit*)console->layout;
        assert(layout->memcheck == LAYOUT_SPLIT_MEMCHECK);

       char *by = prefs_get_string(PREF_ROSTER_BY);
        if (g_strcmp0(by, "presence") == 0) {
            werase(layout->subwin);
            _rosterwin_contacts_by_presence(layout, "chat", " -Available for chat");
            _rosterwin_contacts_by_presence(layout, "online", " -Online");
            _rosterwin_contacts_by_presence(layout, "away", " -Away");
            _rosterwin_contacts_by_presence(layout, "xa", " -Extended Away");
            _rosterwin_contacts_by_presence(layout, "dnd", " -Do not disturb");
            if (prefs_get_boolean(PREF_ROSTER_OFFLINE)) {
                _rosterwin_contacts_by_presence(layout, "offline", " -Offline");
            }
        } else if (g_strcmp0(by, "group") == 0) {
            werase(layout->subwin);
            GSList *groups = roster_get_groups();
            GSList *curr_group = groups;
            while (curr_group) {
                _rosterwin_contacts_by_group(layout, curr_group->data);
                curr_group = g_slist_next(curr_group);
            }
            g_slist_free_full(groups, free);
            _rosterwin_contacts_by_no_group(layout);
        } else {
            GSList *contacts = roster_get_contacts();
            if (contacts) {
                werase(layout->subwin);

                wattron(layout->subwin, theme_attrs(THEME_ROSTER_HEADER));
                win_printline_nowrap(layout->subwin, " -Roster");
                wattroff(layout->subwin, theme_attrs(THEME_ROSTER_HEADER));

                GSList *curr_contact = contacts;
                while (curr_contact) {
                    PContact contact = curr_contact->data;
                    _rosterwin_contact(layout, contact);
                    curr_contact = g_slist_next(curr_contact);
                }
            }
            g_slist_free(contacts);
        }
        free(by);
    }
}
