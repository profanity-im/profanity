/*
 * mucwin.c
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

#include "ui/win_types.h"
#include "window_list.h"
#include "log.h"
#include "config/preferences.h"
#include "ui/window.h"

void
mucwin_role_change(ProfMucWin *mucwin, const char *const role, const char *const actor, const char *const reason)
{
    assert(mucwin != NULL);

    ProfWin *window = (ProfWin*)mucwin;
    win_vprint(window, '!', 0, NULL, NO_EOL, THEME_ROOMINFO, "", "Your role has been changed to: %s", role);
    if (actor) {
        win_vprint(window, '!', 0, NULL, NO_DATE | NO_EOL, THEME_ROOMINFO, "", ", by: %s", actor);
    }
    if (reason) {
        win_vprint(window, '!', 0, NULL, NO_DATE | NO_EOL, THEME_ROOMINFO, "", ", reason: %s", reason);
    }
    win_print(window, '!', 0, NULL, NO_DATE, THEME_ROOMINFO, "", "");
}

void
mucwin_affiliation_change(ProfMucWin *mucwin, const char *const affiliation, const char *const actor,
    const char *const reason)
{
    assert(mucwin != NULL);

    ProfWin *window = (ProfWin*)mucwin;
    win_vprint(window, '!', 0, NULL, NO_EOL, THEME_ROOMINFO, "", "Your affiliation has been changed to: %s", affiliation);
    if (actor) {
        win_vprint(window, '!', 0, NULL, NO_DATE | NO_EOL, THEME_ROOMINFO, "", ", by: %s", actor);
    }
    if (reason) {
        win_vprint(window, '!', 0, NULL, NO_DATE | NO_EOL, THEME_ROOMINFO, "", ", reason: %s", reason);
    }
    win_print(window, '!', 0, NULL, NO_DATE, THEME_ROOMINFO, "", "");
}

void
mucwin_role_and_affiliation_change(ProfMucWin *mucwin, const char *const role, const char *const affiliation,
    const char *const actor, const char *const reason)
{
    assert(mucwin != NULL);

    ProfWin *window = (ProfWin*)mucwin;
    win_vprint(window, '!', 0, NULL, NO_EOL, THEME_ROOMINFO, "", "Your role and affiliation have been changed, role: %s, affiliation: %s", role, affiliation);
    if (actor) {
        win_vprint(window, '!', 0, NULL, NO_DATE | NO_EOL, THEME_ROOMINFO, "", ", by: %s", actor);
    }
    if (reason) {
        win_vprint(window, '!', 0, NULL, NO_DATE | NO_EOL, THEME_ROOMINFO, "", ", reason: %s", reason);
    }
    win_print(window, '!', 0, NULL, NO_DATE, THEME_ROOMINFO, "", "");
}


void
mucwin_occupant_role_change(ProfMucWin *mucwin, const char *const nick, const char *const role,
    const char *const actor, const char *const reason)
{
    assert(mucwin != NULL);

    ProfWin *window = (ProfWin*)mucwin;
    win_vprint(window, '!', 0, NULL, NO_EOL, THEME_ROOMINFO, "", "%s's role has been changed to: %s", nick, role);
    if (actor) {
        win_vprint(window, '!', 0, NULL, NO_DATE | NO_EOL, THEME_ROOMINFO, "", ", by: %s", actor);
    }
    if (reason) {
        win_vprint(window, '!', 0, NULL, NO_DATE | NO_EOL, THEME_ROOMINFO, "", ", reason: %s", reason);
    }
    win_print(window, '!', 0, NULL, NO_DATE, THEME_ROOMINFO, "", "");
}

void
mucwin_occupant_affiliation_change(ProfMucWin *mucwin, const char *const nick, const char *const affiliation,
    const char *const actor, const char *const reason)
{
    assert(mucwin != NULL);

    ProfWin *window = (ProfWin*)mucwin;
    win_vprint(window, '!', 0, NULL, NO_EOL, THEME_ROOMINFO, "", "%s's affiliation has been changed to: %s", nick, affiliation);
    if (actor) {
        win_vprint(window, '!', 0, NULL, NO_DATE | NO_EOL, THEME_ROOMINFO, "", ", by: %s", actor);
    }
    if (reason) {
        win_vprint(window, '!', 0, NULL, NO_DATE | NO_EOL, THEME_ROOMINFO, "", ", reason: %s", reason);
    }
    win_print(window, '!', 0, NULL, NO_DATE, THEME_ROOMINFO, "", "");
}

void
mucwin_occupant_role_and_affiliation_change(ProfMucWin *mucwin, const char *const nick, const char *const role,
    const char *const affiliation, const char *const actor, const char *const reason)
{
    assert(mucwin != NULL);

    ProfWin *window = (ProfWin*)mucwin;
    win_vprint(window, '!', 0, NULL, NO_EOL, THEME_ROOMINFO, "", "%s's role and affiliation have been changed, role: %s, affiliation: %s", nick, role, affiliation);
    if (actor) {
        win_vprint(window, '!', 0, NULL, NO_DATE | NO_EOL, THEME_ROOMINFO, "", ", by: %s", actor);
    }
    if (reason) {
        win_vprint(window, '!', 0, NULL, NO_DATE | NO_EOL, THEME_ROOMINFO, "", ", reason: %s", reason);
    }
    win_print(window, '!', 0, NULL, NO_DATE, THEME_ROOMINFO, "", "");
}

void
mucwin_room_info_error(ProfMucWin *mucwin, const char *const error)
{
    assert(mucwin != NULL);

    ProfWin *window = (ProfWin*)mucwin;
    win_vprint(window, '!', 0, NULL, 0, 0, "", "Room info request failed: %s", error);
    win_print(window, '-', 0, NULL, 0, 0, "", "");
}

void
mucwin_room_disco_info(ProfMucWin *mucwin, GSList *identities, GSList *features)
{
    assert(mucwin != NULL);

    ProfWin *window = (ProfWin*)mucwin;
    if ((identities && (g_slist_length(identities) > 0)) ||
        (features && (g_slist_length(features) > 0))) {
        if (identities) {
            win_print(window, '!', 0, NULL, 0, 0, "", "Identities:");
        }
        while (identities) {
            DiscoIdentity *identity = identities->data;  // anme trpe, cat
            GString *identity_str = g_string_new("  ");
            if (identity->name) {
                identity_str = g_string_append(identity_str, identity->name);
                identity_str = g_string_append(identity_str, " ");
            }
            if (identity->type) {
                identity_str = g_string_append(identity_str, identity->type);
                identity_str = g_string_append(identity_str, " ");
            }
            if (identity->category) {
                identity_str = g_string_append(identity_str, identity->category);
            }
            win_print(window, '!', 0, NULL, 0, 0, "", identity_str->str);
            g_string_free(identity_str, TRUE);
            identities = g_slist_next(identities);
        }

        if (features) {
            win_print(window, '!', 0, NULL, 0, 0, "", "Features:");
        }
        while (features) {
            win_vprint(window, '!', 0, NULL, 0, 0, "", "  %s", features->data);
            features = g_slist_next(features);
        }
        win_print(window, '-', 0, NULL, 0, 0, "", "");
    }
}

void
mucwin_roster(ProfMucWin *mucwin, GList *roster, const char *const presence)
{
    assert(mucwin != NULL);

    ProfWin *window = (ProfWin*)mucwin;
    if ((roster == NULL) || (g_list_length(roster) == 0)) {
        if (presence == NULL) {
            win_print(window, '!', 0, NULL, 0, THEME_ROOMINFO, "", "Room is empty.");
        } else {
            win_vprint(window, '!', 0, NULL, 0, THEME_ROOMINFO, "", "No occupants %s.", presence);
        }
    } else {
        int length = g_list_length(roster);
        if (presence == NULL) {
            win_vprint(window, '!', 0, NULL, NO_EOL, THEME_ROOMINFO, "", "%d occupants: ", length);
        } else {
            win_vprint(window, '!', 0, NULL, NO_EOL, THEME_ROOMINFO, "", "%d %s: ", length, presence);
        }

        while (roster) {
            Occupant *occupant = roster->data;
            const char *presence_str = string_from_resource_presence(occupant->presence);

            theme_item_t presence_colour = theme_main_presence_attrs(presence_str);
            win_vprint(window, '!', 0, NULL, NO_DATE | NO_EOL, presence_colour, "", "%s", occupant->nick);

            if (roster->next) {
                win_print(window, '!', 0, NULL, NO_DATE | NO_EOL, 0, "", ", ");
            }

            roster = g_list_next(roster);
        }
        win_print(window, '!', 0, NULL, NO_DATE, THEME_ONLINE, "", "");

    }
}

void
mucwin_occupant_offline(ProfMucWin *mucwin, const char *const nick)
{
    assert(mucwin != NULL);

    ProfWin *window = (ProfWin*)mucwin;
    win_vprint(window, '!', 0, NULL, 0, THEME_OFFLINE, "", "<- %s has left the room.", nick);
}

void
mucwin_occupant_kicked(ProfMucWin *mucwin, const char *const nick, const char *const actor,
    const char *const reason)
{
    assert(mucwin != NULL);

    ProfWin *window = (ProfWin*)mucwin;
    GString *message = g_string_new(nick);
    g_string_append(message, " has been kicked from the room");
    if (actor) {
        g_string_append(message, " by ");
        g_string_append(message, actor);
    }
    if (reason) {
        g_string_append(message, ", reason: ");
        g_string_append(message, reason);
    }

    win_vprint(window, '!', 0, NULL, 0, THEME_OFFLINE, "", "<- %s", message->str);
    g_string_free(message, TRUE);
}

void
mucwin_occupant_banned(ProfMucWin *mucwin, const char *const nick, const char *const actor,
    const char *const reason)
{
    assert(mucwin != NULL);

    ProfWin *window = (ProfWin*)mucwin;
    GString *message = g_string_new(nick);
    g_string_append(message, " has been banned from the room");
    if (actor) {
        g_string_append(message, " by ");
        g_string_append(message, actor);
    }
    if (reason) {
        g_string_append(message, ", reason: ");
        g_string_append(message, reason);
    }

    win_vprint(window, '!', 0, NULL, 0, THEME_OFFLINE, "", "<- %s", message->str);
    g_string_free(message, TRUE);
}

void
mucwin_occupant_online(ProfMucWin *mucwin, const char *const nick, const char *const role,
    const char *const affiliation, const char *const show, const char *const status)
{
    assert(mucwin != NULL);

    ProfWin *window = (ProfWin*)mucwin;
    win_vprint(window, '!', 0, NULL, NO_EOL, THEME_ONLINE, "", "-> %s has joined the room", nick);
    if (prefs_get_boolean(PREF_MUC_PRIVILEGES)) {
        if (role) {
            win_vprint(window, '!', 0, NULL, NO_DATE | NO_EOL, THEME_ONLINE, "", ", role: %s", role);
        }
        if (affiliation) {
            win_vprint(window, '!', 0, NULL, NO_DATE | NO_EOL, THEME_ONLINE, "", ", affiliation: %s", affiliation);
        }
    }
    win_print(window, '!', 0, NULL, NO_DATE, THEME_ROOMINFO, "", "");
}

void
mucwin_occupant_presence(ProfMucWin *mucwin, const char *const nick,
    const char *const show, const char *const status)
{
    assert(mucwin != NULL);

    ProfWin *window = (ProfWin*)mucwin;
    win_show_status_string(window, nick, show, status, NULL, "++", "online");
}

void
mucwin_occupant_nick_change(ProfMucWin *mucwin, const char *const old_nick, const char *const nick)
{
    assert(mucwin != NULL);

    ProfWin *window = (ProfWin*)mucwin;
    win_vprint(window, '!', 0, NULL, 0, THEME_THEM, "", "** %s is now known as %s", old_nick, nick);
}

void
mucwin_nick_change(ProfMucWin *mucwin, const char *const nick)
{
    assert(mucwin != NULL);

    ProfWin *window = (ProfWin*)mucwin;
    win_vprint(window, '!', 0, NULL, 0, THEME_ME, "", "** You are now known as %s", nick);
}

void
mucwin_history(ProfMucWin *mucwin, const char *const nick, GDateTime *timestamp, const char *const message)
{
    assert(mucwin != NULL);

    ProfWin *window = (ProfWin*)mucwin;
    GString *line = g_string_new("");

    if (strncmp(message, "/me ", 4) == 0) {
        g_string_append(line, "*");
        g_string_append(line, nick);
        g_string_append(line, " ");
        g_string_append(line, message + 4);
    } else {
        g_string_append(line, nick);
        g_string_append(line, ": ");
        g_string_append(line, message);
    }

    win_print(window, '-', 0, timestamp, NO_COLOUR_DATE, 0, "", line->str);
    g_string_free(line, TRUE);
}

void
mucwin_message(ProfMucWin *mucwin, const char *const nick, const char *const message)
{
    assert(mucwin != NULL);

    ProfWin *window = (ProfWin*)mucwin;
    int num = wins_get_num(window);
    char *my_nick = muc_nick(mucwin->roomjid);

    if (g_strcmp0(nick, my_nick) != 0) {
        if (g_strrstr(message, my_nick)) {
            win_print(window, '-', 0, NULL, NO_ME, THEME_ROOMMENTION, nick, message);
        } else {
            win_print(window, '-', 0, NULL, NO_ME, THEME_TEXT_THEM, nick, message);
        }
    } else {
        win_print(window, '-', 0, NULL, 0, THEME_TEXT_ME, nick, message);
    }

    // currently in groupchat window
    if (wins_is_current(window)) {
        status_bar_active(num);

    // not currently on groupchat window
    } else {
        status_bar_new(num);
        cons_show_incoming_message(nick, num);

        if (prefs_get_boolean(PREF_FLASH) && (strcmp(nick, my_nick) != 0)) {
            flash();
        }

        mucwin->unread++;
    }

    int ui_index = num;
    if (ui_index == 10) {
        ui_index = 0;
    }

    // don't notify self messages
    if (strcmp(nick, my_nick) == 0) {
        return;
    }

    if (prefs_get_boolean(PREF_BEEP)) {
        beep();
    }

    gboolean notify = FALSE;
    char *room_setting = prefs_get_string(PREF_NOTIFY_ROOM);
    if (g_strcmp0(room_setting, "on") == 0) {
        notify = TRUE;
    }
    if (g_strcmp0(room_setting, "mention") == 0) {
        char *message_lower = g_utf8_strdown(message, -1);
        char *nick_lower = g_utf8_strdown(nick, -1);
        if (g_strrstr(message_lower, nick_lower)) {
            notify = TRUE;
        }
        g_free(message_lower);
        g_free(nick_lower);
    }
    prefs_free_string(room_setting);

    if (notify) {
        gboolean is_current = wins_is_current(window);
        if ( !is_current || (is_current && prefs_get_boolean(PREF_NOTIFY_ROOM_CURRENT)) ) {
            Jid *jidp = jid_create(mucwin->roomjid);
            if (prefs_get_boolean(PREF_NOTIFY_ROOM_TEXT)) {
                notify_room_message(nick, jidp->localpart, ui_index, message);
            } else {
                notify_room_message(nick, jidp->localpart, ui_index, NULL);
            }
            jid_destroy(jidp);
        }
    }
}

void
mucwin_requires_config(ProfMucWin *mucwin)
{
    assert(mucwin != NULL);

    ProfWin *window = (ProfWin*)mucwin;
    int num = wins_get_num(window);
    int ui_index = num;
    if (ui_index == 10) {
        ui_index = 0;
    }

    win_print(window, '-', 0, NULL, 0, 0, "", "");
    win_vprint(window, '!', 0, NULL, 0, THEME_ROOMINFO, "", "Room locked, requires configuration.");
    win_vprint(window, '!', 0, NULL, 0, THEME_ROOMINFO, "", "Use '/room accept' to accept the defaults");
    win_vprint(window, '!', 0, NULL, 0, THEME_ROOMINFO, "", "Use '/room destroy' to cancel and destroy the room");
    win_vprint(window, '!', 0, NULL, 0, THEME_ROOMINFO, "", "Use '/room config' to edit the room configuration");
    win_print(window, '-', 0, NULL, 0, 0, "", "");

    // currently in groupchat window
    if (wins_is_current(window)) {
        status_bar_active(num);

    // not currently on groupchat window
    } else {
        status_bar_new(num);
    }
}

void
mucwin_subject(ProfMucWin *mucwin, const char *const nick, const char *const subject)
{
    assert(mucwin != NULL);

    ProfWin *window = (ProfWin*)mucwin;
    int num = wins_get_num(window);

    if (subject) {
        if (nick) {
            win_vprint(window, '!', 0, NULL, NO_EOL, THEME_ROOMINFO, "", "*%s has set the room subject: ", nick);
            win_vprint(window, '!', 0, NULL, NO_DATE, 0, "", "%s", subject);
        } else {
            win_vprint(window, '!', 0, NULL, NO_EOL, THEME_ROOMINFO, "", "Room subject: ");
            win_vprint(window, '!', 0, NULL, NO_DATE, 0, "", "%s", subject);
        }
    } else {
        if (nick) {
            win_vprint(window, '!', 0, NULL, 0, THEME_ROOMINFO, "", "*%s has cleared the room subject.", nick);
        } else {
            win_vprint(window, '!', 0, NULL, 0, THEME_ROOMINFO, "", "Room subject cleared");
        }
    }

    // currently in groupchat window
    if (wins_is_current(window)) {
        status_bar_active(num);

    // not currently on groupchat window
    } else {
        status_bar_active(num);
    }
}

void
mucwin_kick_error(ProfMucWin *mucwin, const char *const nick, const char *const error)
{
    assert(mucwin != NULL);

    ProfWin *window = (ProfWin*)mucwin;
    win_vprint(window, '!', 0, NULL, 0, THEME_ERROR, "", "Error kicking %s: %s", nick, error);
}

void
mucwin_broadcast(ProfMucWin *mucwin, const char *const message)
{
    assert(mucwin != NULL);

    ProfWin *window = (ProfWin*)mucwin;
    int num = wins_get_num(window);

    win_vprint(window, '!', 0, NULL, NO_EOL, THEME_ROOMINFO, "", "Room message: ");
    win_vprint(window, '!', 0, NULL, NO_DATE, 0, "", "%s", message);

    // currently in groupchat window
    if (wins_is_current(window)) {
        status_bar_active(num);

    // not currently on groupchat window
    } else {
        status_bar_new(num);
    }
}

void
mucwin_affiliation_list_error(ProfMucWin *mucwin, const char *const affiliation,
    const char *const error)
{
    assert(mucwin != NULL);

    ProfWin *window = (ProfWin*)mucwin;
    win_vprint(window, '!', 0, NULL, 0, THEME_ERROR, "", "Error retrieving %s list: %s", affiliation, error);
}

void
mucwin_handle_affiliation_list(ProfMucWin *mucwin, const char *const affiliation, GSList *jids)
{
    assert(mucwin != NULL);

    ProfWin *window = (ProfWin*)mucwin;
    if (jids) {
        win_vprint(window, '!', 0, NULL, 0, 0, "", "Affiliation: %s", affiliation);
        GSList *curr_jid = jids;
        while (curr_jid) {
            char *jid = curr_jid->data;
            win_vprint(window, '!', 0, NULL, 0, 0, "", "  %s", jid);
            curr_jid = g_slist_next(curr_jid);
        }
        win_print(window, '!', 0, NULL, 0, 0, "", "");
    } else {
        win_vprint(window, '!', 0, NULL, 0, 0, "", "No users found with affiliation: %s", affiliation);
        win_print(window, '!', 0, NULL, 0, 0, "", "");
    }
}

void
mucwin_show_affiliation_list(ProfMucWin *mucwin, muc_affiliation_t affiliation)
{
    assert(mucwin != NULL);

    ProfWin *window = (ProfWin*) mucwin;
    GSList *occupants = muc_occupants_by_affiliation(mucwin->roomjid, affiliation);

    if (!occupants) {
        switch (affiliation) {
            case MUC_AFFILIATION_OWNER:
                win_print(window, '!', 0, NULL, 0, 0, "", "No owners found.");
                break;
            case MUC_AFFILIATION_ADMIN:
                win_print(window, '!', 0, NULL, 0, 0, "", "No admins found.");
                break;
            case MUC_AFFILIATION_MEMBER:
                win_print(window, '!', 0, NULL, 0, 0, "", "No members found.");
                break;
            case MUC_AFFILIATION_OUTCAST:
                win_print(window, '!', 0, NULL, 0, 0, "", "No outcasts found.");
                break;
            default:
                break;
        }
        win_print(window, '-', 0, NULL, 0, 0, "", "");
    } else {
        switch (affiliation) {
            case MUC_AFFILIATION_OWNER:
                win_print(window, '!', 0, NULL, 0, 0, "", "Owners:");
                break;
            case MUC_AFFILIATION_ADMIN:
                win_print(window, '!', 0, NULL, 0, 0, "", "Admins:");
                break;
            case MUC_AFFILIATION_MEMBER:
                win_print(window, '!', 0, NULL, 0, 0, "", "Members:");
                break;
            case MUC_AFFILIATION_OUTCAST:
                win_print(window, '!', 0, NULL, 0, 0, "", "Outcasts:");
                break;
            default:
                break;
        }

        GSList *curr_occupant = occupants;
        while(curr_occupant) {
            Occupant *occupant = curr_occupant->data;
            if (occupant->affiliation == affiliation) {
                if (occupant->jid) {
                    win_vprint(window, '!', 0, NULL, 0, 0, "", "  %s (%s)", occupant->nick, occupant->jid);
                } else {
                    win_vprint(window, '!', 0, NULL, 0, 0, "", "  %s", occupant->nick);
                }
            }

            curr_occupant = g_slist_next(curr_occupant);
        }

        win_print(window, '-', 0, NULL, 0, 0, "", "");
    }
}

void
mucwin_role_list_error(ProfMucWin *mucwin, const char *const role, const char *const error)
{
    assert(mucwin != NULL);

    ProfWin *window = (ProfWin*)mucwin;
    win_vprint(window, '!', 0, NULL, 0, THEME_ERROR, "", "Error retrieving %s list: %s", role, error);
}

void
mucwin_handle_role_list(ProfMucWin *mucwin, const char *const role, GSList *nicks)
{
    assert(mucwin != NULL);

    ProfWin *window = (ProfWin*)mucwin;
    if (nicks) {
        win_vprint(window, '!', 0, NULL, 0, 0, "", "Role: %s", role);
        GSList *curr_nick = nicks;
        while (curr_nick) {
            char *nick = curr_nick->data;
            Occupant *occupant = muc_roster_item(mucwin->roomjid, nick);
            if (occupant) {
                if (occupant->jid) {
                    win_vprint(window, '!', 0, NULL, 0, 0, "", "  %s (%s)", nick, occupant->jid);
                } else {
                    win_vprint(window, '!', 0, NULL, 0, 0, "", "  %s", nick);
                }
            } else {
                win_vprint(window, '!', 0, NULL, 0, 0, "", "  %s", nick);
            }
            curr_nick = g_slist_next(curr_nick);
        }
        win_print(window, '!', 0, NULL, 0, 0, "", "");
    } else {
        win_vprint(window, '!', 0, NULL, 0, 0, "", "No occupants found with role: %s", role);
        win_print(window, '!', 0, NULL, 0, 0, "", "");
    }
}

void
mucwin_show_role_list(ProfMucWin *mucwin, muc_role_t role)
{
    assert(mucwin != NULL);

    ProfWin *window = (ProfWin*)mucwin;
    GSList *occupants = muc_occupants_by_role(mucwin->roomjid, role);

    if (!occupants) {
        switch (role) {
            case MUC_ROLE_MODERATOR:
                win_print(window, '!', 0, NULL, 0, 0, "", "No moderators found.");
                break;
            case MUC_ROLE_PARTICIPANT:
                win_print(window, '!', 0, NULL, 0, 0, "", "No participants found.");
                break;
            case MUC_ROLE_VISITOR:
                win_print(window, '!', 0, NULL, 0, 0, "", "No visitors found.");
                break;
            default:
                break;
        }
        win_print(window, '-', 0, NULL, 0, 0, "", "");
    } else {
        switch (role) {
            case MUC_ROLE_MODERATOR:
                win_print(window, '!', 0, NULL, 0, 0, "", "Moderators:");
                break;
            case MUC_ROLE_PARTICIPANT:
                win_print(window, '!', 0, NULL, 0, 0, "", "Participants:");
                break;
            case MUC_ROLE_VISITOR:
                win_print(window, '!', 0, NULL, 0, 0, "", "Visitors:");
                break;
            default:
                break;
        }

        GSList *curr_occupant = occupants;
        while(curr_occupant) {
            Occupant *occupant = curr_occupant->data;
            if (occupant->role == role) {
                if (occupant->jid) {
                    win_vprint(window, '!', 0, NULL, 0, 0, "", "  %s (%s)", occupant->nick, occupant->jid);
                } else {
                    win_vprint(window, '!', 0, NULL, 0, 0, "", "  %s", occupant->nick);
                }
            }

            curr_occupant = g_slist_next(curr_occupant);
        }

        win_print(window, '-', 0, NULL, 0, 0, "", "");
    }
}

void
mucwin_affiliation_set_error(ProfMucWin *mucwin, const char *const jid, const char *const affiliation,
    const char *const error)
{
    assert(mucwin != NULL);

    ProfWin *window = (ProfWin*)mucwin;
    win_vprint(window, '!', 0, NULL, 0, THEME_ERROR, "", "Error setting %s affiliation for %s: %s", affiliation, jid, error);
}

void
mucwin_role_set_error(ProfMucWin *mucwin, const char *const nick, const char *const role,
    const char *const error)
{
    assert(mucwin != NULL);

    ProfWin *window = (ProfWin*)mucwin;
    win_vprint(window, '!', 0, NULL, 0, THEME_ERROR, "", "Error setting %s role for %s: %s", role, nick, error);
}

void
mucwin_info(ProfMucWin *mucwin)
{
    assert(mucwin != NULL);

    char *role = muc_role_str(mucwin->roomjid);
    char *affiliation = muc_affiliation_str(mucwin->roomjid);

    ProfWin *window = (ProfWin*) mucwin;
    win_vprint(window, '!', 0, NULL, 0, 0, "", "Room: %s", mucwin->roomjid);
    win_vprint(window, '!', 0, NULL, 0, 0, "", "Affiliation: %s", affiliation);
    win_vprint(window, '!', 0, NULL, 0, 0, "", "Role: %s", role);
    win_print(window, '-', 0, NULL, 0, 0, "", "");
}

void
mucwin_update_occupants(ProfMucWin *mucwin)
{
    assert(mucwin != NULL);

    ProfWin *window = (ProfWin*)mucwin;
    if (win_has_active_subwin(window)) {
        occupantswin_occupants(mucwin->roomjid);
    }
}

void
mucwin_show_occupants(ProfMucWin *mucwin)
{
    assert(mucwin != NULL);

    ProfWin *window = (ProfWin*)mucwin;
    if (!win_has_active_subwin(window)) {
        wins_show_subwin(window);
        occupantswin_occupants(mucwin->roomjid);
    }
}

void
mucwin_hide_occupants(ProfMucWin *mucwin)
{
    assert(mucwin != NULL);

    ProfWin *window = (ProfWin*)mucwin;
    if (win_has_active_subwin(window)) {
        wins_hide_subwin(window);
    }
}

