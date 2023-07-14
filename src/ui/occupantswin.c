/*
 * occupantswin.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 * Copyright (C) 2019 - 2023 Michael Vetter <jubalh@iodoru.org>
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

#include <assert.h>

#include "config/preferences.h"
#include "ui/ui.h"
#include "ui/window.h"
#include "ui/window_list.h"

static void
_occuptantswin_occupant(ProfLayoutSplit* layout, GList* item, gboolean showjid, gboolean isoffline)
{
    int colour = 0;                                     // init to workaround compiler warning
    theme_item_t presence_colour = THEME_ROSTER_ONLINE; // init to workaround compiler warning
    Occupant* occupant = item->data;

    if (isoffline) {
        wattron(layout->subwin, theme_attrs(THEME_ROSTER_OFFLINE));
    } else if (prefs_get_boolean(PREF_OCCUPANTS_COLOR_NICK)) {
        colour = theme_hash_attrs(occupant->nick);

        wattron(layout->subwin, colour);
    } else {
        const char* presence_str = string_from_resource_presence(occupant->presence);
        presence_colour = theme_main_presence_attrs(presence_str);

        wattron(layout->subwin, theme_attrs(presence_colour));
    }

    GString* spaces = g_string_new(" ");

    int indent = prefs_get_occupants_indent();
    int current_indent = 0;
    if (indent > 0) {
        current_indent += indent;
        while (indent > 0) {
            g_string_append(spaces, " ");
            indent--;
        }
    }

    GString* msg = g_string_new(spaces->str);

    auto_char char* ch = prefs_get_occupants_char();
    if (ch) {
        g_string_append_printf(msg, "%s", ch);
    }

    gboolean wrap = prefs_get_boolean(PREF_OCCUPANTS_WRAP);

    if (isoffline) {
        auto_jid Jid* jid = jid_create(item->data);
        g_string_append(msg, jid->barejid);
    } else {
        g_string_append(msg, occupant->nick);
    }
    win_sub_newline_lazy(layout->subwin);
    win_sub_print(layout->subwin, msg->str, FALSE, wrap, current_indent);
    g_string_free(msg, TRUE);

    if (showjid && !isoffline && occupant->jid) {
        GString* msg = g_string_new(spaces->str);
        g_string_append(msg, " ");

        g_string_append(msg, occupant->jid);
        win_sub_newline_lazy(layout->subwin);
        win_sub_print(layout->subwin, msg->str, FALSE, wrap, current_indent);
        g_string_free(msg, TRUE);
    }

    g_string_free(spaces, TRUE);

    if (isoffline) {
        wattroff(layout->subwin, theme_attrs(THEME_ROSTER_OFFLINE));
    } else if (prefs_get_boolean(PREF_OCCUPANTS_COLOR_NICK)) {
        wattroff(layout->subwin, colour);
    } else {
        wattroff(layout->subwin, theme_attrs(presence_colour));
    }
}

void
occupantswin_occupants(const char* const roomjid)
{
    ProfMucWin* mucwin = wins_get_muc(roomjid);
    if (mucwin) {
        GList* occupants = muc_roster(roomjid);
        if (occupants) {
            ProfLayoutSplit* layout = (ProfLayoutSplit*)mucwin->window.layout;
            assert(layout->memcheck == LAYOUT_SPLIT_MEMCHECK);

            werase(layout->subwin);

            GString* prefix = g_string_new(" ");

            auto_char char* ch = prefs_get_occupants_header_char();
            if (ch) {
                g_string_append_printf(prefix, "%s", ch);
            }

            if (prefs_get_boolean(PREF_MUC_PRIVILEGES)) {

                GList* online_occupants = { NULL };

                GString* role = g_string_new(prefix->str);
                g_string_append(role, "Moderators");

                wattron(layout->subwin, theme_attrs(THEME_OCCUPANTS_HEADER));
                win_sub_newline_lazy(layout->subwin);
                win_sub_print(layout->subwin, role->str, TRUE, FALSE, 0);
                wattroff(layout->subwin, theme_attrs(THEME_OCCUPANTS_HEADER));
                g_string_free(role, TRUE);
                GList* roster_curr = occupants;
                while (roster_curr) {
                    Occupant* occupant = roster_curr->data;
                    if (occupant->role == MUC_ROLE_MODERATOR) {
                        _occuptantswin_occupant(layout, roster_curr, mucwin->showjid, false);
                    }
                    roster_curr = g_list_next(roster_curr);
                    online_occupants = g_list_append(online_occupants, occupant->jid);
                }

                role = g_string_new(prefix->str);
                g_string_append(role, "Participants");

                wattron(layout->subwin, theme_attrs(THEME_OCCUPANTS_HEADER));
                win_sub_newline_lazy(layout->subwin);
                win_sub_print(layout->subwin, role->str, TRUE, FALSE, 0);
                wattroff(layout->subwin, theme_attrs(THEME_OCCUPANTS_HEADER));
                g_string_free(role, TRUE);
                roster_curr = occupants;
                while (roster_curr) {
                    Occupant* occupant = roster_curr->data;
                    if (occupant->role == MUC_ROLE_PARTICIPANT) {
                        _occuptantswin_occupant(layout, roster_curr, mucwin->showjid, false);
                    }
                    roster_curr = g_list_next(roster_curr);
                    online_occupants = g_list_append(online_occupants, occupant->jid);
                }

                role = g_string_new(prefix->str);
                g_string_append(role, "Visitors");

                wattron(layout->subwin, theme_attrs(THEME_OCCUPANTS_HEADER));
                win_sub_newline_lazy(layout->subwin);
                win_sub_print(layout->subwin, role->str, TRUE, FALSE, 0);
                wattroff(layout->subwin, theme_attrs(THEME_OCCUPANTS_HEADER));
                g_string_free(role, TRUE);
                roster_curr = occupants;
                while (roster_curr) {
                    Occupant* occupant = roster_curr->data;
                    if (occupant->role == MUC_ROLE_VISITOR) {
                        _occuptantswin_occupant(layout, roster_curr, mucwin->showjid, false);
                    }
                    roster_curr = g_list_next(roster_curr);
                    online_occupants = g_list_append(online_occupants, occupant->jid);
                }

                if (mucwin->showoffline) {
                    GList* members = muc_members(roomjid);
                    // offline_occupants is used to display the same account on multiple devices once
                    GList* offline_occupants = { NULL };

                    role = g_string_new(prefix->str);
                    g_string_append(role, "Offline");

                    wattron(layout->subwin, theme_attrs(THEME_OCCUPANTS_HEADER));
                    win_sub_newline_lazy(layout->subwin);
                    win_sub_print(layout->subwin, role->str, TRUE, FALSE, 0);
                    wattroff(layout->subwin, theme_attrs(THEME_OCCUPANTS_HEADER));
                    g_string_free(role, TRUE);
                    roster_curr = members;
                    while (roster_curr) {
                        auto_jid Jid* jid = jid_create(roster_curr->data);
                        gboolean found = false;
                        GList* iter = online_occupants;
                        for (; iter != NULL; iter = iter->next) {
                            if (strstr(iter->data, jid->barejid)) {
                                found = true;
                                break;
                            }
                        }

                        if (!found && g_list_index(offline_occupants, jid->barejid) == -1) {
                            _occuptantswin_occupant(layout, roster_curr, mucwin->showjid, true);
                            offline_occupants = g_list_append(offline_occupants, jid->barejid);
                        }

                        roster_curr = g_list_next(roster_curr);
                    }
                    g_list_free(members);
                    g_list_free(offline_occupants);
                }
                g_list_free(online_occupants);

            } else {
                GString* role = g_string_new(prefix->str);
                g_string_append(role, "Occupants\n");

                wattron(layout->subwin, theme_attrs(THEME_OCCUPANTS_HEADER));
                win_sub_newline_lazy(layout->subwin);
                win_sub_print(layout->subwin, role->str, TRUE, FALSE, 0);
                wattroff(layout->subwin, theme_attrs(THEME_OCCUPANTS_HEADER));
                g_string_free(role, TRUE);

                GList* roster_curr = occupants;
                while (roster_curr) {
                    _occuptantswin_occupant(layout, roster_curr, mucwin->showjid, false);
                    roster_curr = g_list_next(roster_curr);
                }
            }

            g_string_free(prefix, TRUE);
        }

        g_list_free(occupants);
    }
}

void
occupantswin_occupants_all(void)
{
    GList* rooms = muc_rooms();
    GList* curr = rooms;

    while (curr) {
        char* roomjid = curr->data;
        ProfMucWin* mw = wins_get_muc(roomjid);
        if (mw != NULL) {
            mucwin_update_occupants(mw);
        }

        curr = g_list_next(curr);
    }
}
