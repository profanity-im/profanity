/*
 * occupantswin.c
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

#include "config/preferences.h"
#include "ui/ui.h"
#include "ui/window.h"
#include "ui/window_list.h"

static void
_occuptantswin_occupant(ProfLayoutSplit *layout, Occupant *occupant, gboolean showjid)
{
    const char *presence_str = string_from_resource_presence(occupant->presence);
    theme_item_t presence_colour = theme_main_presence_attrs(presence_str);
    wattron(layout->subwin, theme_attrs(presence_colour));

    GString *msg = g_string_new("   ");
    g_string_append(msg, occupant->nick);
    win_sub_print(layout->subwin, msg->str, TRUE, FALSE, 0);
    g_string_free(msg, TRUE);

    if (showjid && occupant->jid) {
        GString *msg = g_string_new("     ");
        g_string_append(msg, occupant->jid);
        win_sub_print(layout->subwin, msg->str, TRUE, FALSE, 0);
        g_string_free(msg, TRUE);
    }

    wattroff(layout->subwin, theme_attrs(presence_colour));
}

void
occupantswin_occupants(const char *const roomjid)
{
    ProfMucWin *mucwin = wins_get_muc(roomjid);
    if (mucwin) {
        GList *occupants = muc_roster(roomjid);
        if (occupants) {
            ProfLayoutSplit *layout = (ProfLayoutSplit*)mucwin->window.layout;
            assert(layout->memcheck == LAYOUT_SPLIT_MEMCHECK);

            werase(layout->subwin);

            if (prefs_get_boolean(PREF_MUC_PRIVILEGES)) {
                wattron(layout->subwin, theme_attrs(THEME_OCCUPANTS_HEADER));
                win_sub_print(layout->subwin, " -Moderators", TRUE, FALSE, 0);
                wattroff(layout->subwin, theme_attrs(THEME_OCCUPANTS_HEADER));
                GList *roster_curr = occupants;
                while (roster_curr) {
                    Occupant *occupant = roster_curr->data;
                    if (occupant->role == MUC_ROLE_MODERATOR) {
                        _occuptantswin_occupant(layout, occupant, mucwin->showjid);
                    }
                    roster_curr = g_list_next(roster_curr);
                }

                wattron(layout->subwin, theme_attrs(THEME_OCCUPANTS_HEADER));
                win_sub_print(layout->subwin, " -Participants", TRUE, FALSE, 0);
                wattroff(layout->subwin, theme_attrs(THEME_OCCUPANTS_HEADER));
                roster_curr = occupants;
                while (roster_curr) {
                    Occupant *occupant = roster_curr->data;
                    if (occupant->role == MUC_ROLE_PARTICIPANT) {
                        _occuptantswin_occupant(layout, occupant, mucwin->showjid);
                    }
                    roster_curr = g_list_next(roster_curr);
                }

                wattron(layout->subwin, theme_attrs(THEME_OCCUPANTS_HEADER));
                win_sub_print(layout->subwin, " -Visitors", TRUE, FALSE, 0);
                wattroff(layout->subwin, theme_attrs(THEME_OCCUPANTS_HEADER));
                roster_curr = occupants;
                while (roster_curr) {
                    Occupant *occupant = roster_curr->data;
                    if (occupant->role == MUC_ROLE_VISITOR) {
                        _occuptantswin_occupant(layout, occupant, mucwin->showjid);
                    }
                    roster_curr = g_list_next(roster_curr);
                }
            } else {
                wattron(layout->subwin, theme_attrs(THEME_OCCUPANTS_HEADER));
                win_sub_print(layout->subwin, " -Occupants\n", TRUE, FALSE, 0);
                wattroff(layout->subwin, theme_attrs(THEME_OCCUPANTS_HEADER));
                GList *roster_curr = occupants;
                while (roster_curr) {
                    Occupant *occupant = roster_curr->data;
                    _occuptantswin_occupant(layout, occupant, mucwin->showjid);
                    roster_curr = g_list_next(roster_curr);
                }
            }
        }

        g_list_free(occupants);
    }
}
