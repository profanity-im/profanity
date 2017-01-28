/*
 * mucwin.c
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

#define _GNU_SOURCE 1

#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "log.h"
#include "config/preferences.h"
#include "plugins/plugins.h"
#include "ui/window.h"
#include "ui/win_types.h"
#include "ui/window_list.h"

void
mucwin_role_change(ProfMucWin *mucwin, const char *const role, const char *const actor, const char *const reason)
{
    assert(mucwin != NULL);

    ProfWin *window = (ProfWin*)mucwin;
    win_print(window, THEME_ROOMINFO, '!', "Your role has been changed to: %s", role);
    if (actor) {
        win_append(window, THEME_ROOMINFO, ", by: %s", actor);
    }
    if (reason) {
        win_append(window, THEME_ROOMINFO, ", reason: %s", reason);
    }
    win_appendln(window, THEME_ROOMINFO, "");
}

void
mucwin_affiliation_change(ProfMucWin *mucwin, const char *const affiliation, const char *const actor,
    const char *const reason)
{
    assert(mucwin != NULL);

    ProfWin *window = (ProfWin*)mucwin;
    win_print(window, THEME_ROOMINFO, '!', "Your affiliation has been changed to: %s", affiliation);
    if (actor) {
        win_append(window, THEME_ROOMINFO, ", by: %s", actor);
    }
    if (reason) {
        win_append(window, THEME_ROOMINFO, ", reason: %s", reason);
    }
    win_appendln(window, THEME_ROOMINFO, "");
}

void
mucwin_role_and_affiliation_change(ProfMucWin *mucwin, const char *const role, const char *const affiliation,
    const char *const actor, const char *const reason)
{
    assert(mucwin != NULL);

    ProfWin *window = (ProfWin*)mucwin;
    win_print(window, THEME_ROOMINFO, '!', "Your role and affiliation have been changed, role: %s, affiliation: %s", role, affiliation);
    if (actor) {
        win_append(window, THEME_ROOMINFO, ", by: %s", actor);
    }
    if (reason) {
        win_append(window, THEME_ROOMINFO, ", reason: %s", reason);
    }
    win_appendln(window, THEME_ROOMINFO, "");
}


void
mucwin_occupant_role_change(ProfMucWin *mucwin, const char *const nick, const char *const role,
    const char *const actor, const char *const reason)
{
    assert(mucwin != NULL);

    ProfWin *window = (ProfWin*)mucwin;
    win_print(window, THEME_ROOMINFO, '!', "%s's role has been changed to: %s", nick, role);
    if (actor) {
        win_append(window, THEME_ROOMINFO, ", by: %s", actor);
    }
    if (reason) {
        win_append(window, THEME_ROOMINFO, ", reason: %s", reason);
    }
    win_appendln(window, THEME_ROOMINFO, "");
}

void
mucwin_occupant_affiliation_change(ProfMucWin *mucwin, const char *const nick, const char *const affiliation,
    const char *const actor, const char *const reason)
{
    assert(mucwin != NULL);

    ProfWin *window = (ProfWin*)mucwin;
    win_print(window, THEME_ROOMINFO, '!', "%s's affiliation has been changed to: %s", nick, affiliation);
    if (actor) {
        win_append(window, THEME_ROOMINFO, ", by: %s", actor);
    }
    if (reason) {
        win_append(window, THEME_ROOMINFO, ", reason: %s", reason);
    }
    win_appendln(window, THEME_ROOMINFO, "");
}

void
mucwin_occupant_role_and_affiliation_change(ProfMucWin *mucwin, const char *const nick, const char *const role,
    const char *const affiliation, const char *const actor, const char *const reason)
{
    assert(mucwin != NULL);

    ProfWin *window = (ProfWin*)mucwin;
    win_print(window, THEME_ROOMINFO, '!', "%s's role and affiliation have been changed, role: %s, affiliation: %s", nick, role, affiliation);
    if (actor) {
        win_append(window, THEME_ROOMINFO, ", by: %s", actor);
    }
    if (reason) {
        win_append(window, THEME_ROOMINFO, ", reason: %s", reason);
    }
    win_appendln(window, THEME_ROOMINFO, "");
}

void
mucwin_room_info_error(ProfMucWin *mucwin, const char *const error)
{
    assert(mucwin != NULL);

    ProfWin *window = (ProfWin*)mucwin;
    win_println(window, THEME_DEFAULT, '!', "Room info request failed: %s", error);
    win_println(window, THEME_DEFAULT, '-', "");
}

void
mucwin_room_disco_info(ProfMucWin *mucwin, GSList *identities, GSList *features)
{
    assert(mucwin != NULL);

    ProfWin *window = (ProfWin*)mucwin;
    if ((identities && (g_slist_length(identities) > 0)) ||
        (features && (g_slist_length(features) > 0))) {
        if (identities) {
            win_println(window, THEME_DEFAULT, '!', "Identities:");
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
            win_println(window, THEME_DEFAULT, '!', "%s", identity_str->str);
            g_string_free(identity_str, TRUE);
            identities = g_slist_next(identities);
        }

        if (features) {
            win_println(window, THEME_DEFAULT, '!', "Features:");
        }
        while (features) {
            win_println(window, THEME_DEFAULT, '!', "  %s", features->data);
            features = g_slist_next(features);
        }
        win_println(window, THEME_DEFAULT, '-', "");
    }
}

void
mucwin_roster(ProfMucWin *mucwin, GList *roster, const char *const presence)
{
    assert(mucwin != NULL);

    ProfWin *window = (ProfWin*)mucwin;
    if ((roster == NULL) || (g_list_length(roster) == 0)) {
        if (presence == NULL) {
            win_println(window, THEME_ROOMINFO, '!', "Room is empty.");
        } else {
            win_println(window, THEME_ROOMINFO, '!', "No occupants %s.", presence);
        }
    } else {
        int length = g_list_length(roster);
        if (presence == NULL) {
            win_print(window, THEME_ROOMINFO, '!', "%d occupants: ", length);
        } else {
            win_print(window, THEME_ROOMINFO, '!', "%d %s: ", length, presence);
        }

        while (roster) {
            Occupant *occupant = roster->data;
            const char *presence_str = string_from_resource_presence(occupant->presence);

            theme_item_t presence_colour = theme_main_presence_attrs(presence_str);
            win_append(window, presence_colour, "%s", occupant->nick);

            if (roster->next) {
                win_append(window, THEME_DEFAULT, ", ");
            }

            roster = g_list_next(roster);
        }
        win_appendln(window, THEME_ONLINE, "");
    }
}

void
mucwin_occupant_offline(ProfMucWin *mucwin, const char *const nick)
{
    assert(mucwin != NULL);

    ProfWin *window = (ProfWin*)mucwin;
    win_println(window, THEME_OFFLINE, '!', "<- %s has left the room.", nick);
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

    win_println(window, THEME_OFFLINE, '!', "<- %s", message->str);
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

    win_println(window, THEME_OFFLINE, '!', "<- %s", message->str);
    g_string_free(message, TRUE);
}

void
mucwin_occupant_online(ProfMucWin *mucwin, const char *const nick, const char *const role,
    const char *const affiliation, const char *const show, const char *const status)
{
    assert(mucwin != NULL);

    ProfWin *window = (ProfWin*)mucwin;
    win_print(window, THEME_ONLINE, '!', "-> %s has joined the room", nick);
    if (prefs_get_boolean(PREF_MUC_PRIVILEGES)) {
        if (role) {
            win_append(window, THEME_ONLINE, ", role: %s", role);
        }
        if (affiliation) {
            win_append(window, THEME_ONLINE, ", affiliation: %s", affiliation);
        }
    }
    win_appendln(window, THEME_ROOMINFO, "");
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
    win_println(window, THEME_THEM, '!', "** %s is now known as %s", old_nick, nick);
}

void
mucwin_nick_change(ProfMucWin *mucwin, const char *const nick)
{
    assert(mucwin != NULL);

    ProfWin *window = (ProfWin*)mucwin;
    win_println(window, THEME_ME, '!', "** You are now known as %s", nick);
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

    win_print_history(window, timestamp, "%s", line->str);
    g_string_free(line, TRUE);

    plugins_on_room_history_message(mucwin->roomjid, nick, message, timestamp);
}

static void
_mucwin_print_mention(ProfWin *window, const char *const message, const char *const nick, GSList *mentions)
{
    int last_pos = 0;
    int pos = 0;
    GSList *curr = mentions;
    while (curr) {
        pos = GPOINTER_TO_INT(curr->data);

        char *before_str = g_strndup(message + last_pos, pos - last_pos);
        win_append_highlight(window, THEME_ROOMMENTION, "%s", before_str);
        g_free(before_str);
        char *nick_str = g_strndup(message + pos, strlen(nick));
        win_append_highlight(window, THEME_ROOMMENTION_TERM, "%s", nick_str);
        g_free(nick_str);

        last_pos = pos + strlen(nick);

        curr = g_slist_next(curr);
    }
    if (last_pos < strlen(message)) {
        win_appendln_highlight(window, THEME_ROOMMENTION, "%s", &message[last_pos]);
    } else {
        win_appendln_highlight(window, THEME_ROOMMENTION, "");
    }
}

gint
_cmp_trigger_weight(gconstpointer a, gconstpointer b)
{
    int alen = strlen((char*)a);
    int blen = strlen((char*)b);

    if (alen > blen) return -1;
    if (alen < blen) return 1;

    return 0;
}

static void
_mucwin_print_triggers(ProfWin *window, const char *const message, GList *triggers)
{
    GList *weighted_triggers = NULL;
    GList *curr = triggers;
    while (curr) {
        weighted_triggers = g_list_insert_sorted(weighted_triggers, curr->data, (GCompareFunc)_cmp_trigger_weight);
        curr = g_list_next(curr);
    }

    char *message_lower = g_utf8_strdown(message, -1);

    // find earliest trigger in message
    int first_trigger_pos = -1;
    int first_trigger_len = -1;
    curr = weighted_triggers;
    while (curr) {
        char *trigger_lower = g_utf8_strdown(curr->data, -1);
        char *trigger_ptr = g_strstr_len(message_lower, -1, trigger_lower);

        // not found, try next
        if (trigger_ptr == NULL) {
            curr = g_list_next(curr);
            continue;
        }

        // found, repace vars if earlier than previous
        int trigger_pos = trigger_ptr - message_lower;
        if (first_trigger_pos == -1 || trigger_pos < first_trigger_pos) {
            first_trigger_pos = trigger_pos;
            first_trigger_len = strlen(trigger_lower);
        }

        g_free(trigger_lower);
        curr = g_list_next(curr);
    }

    g_free(message_lower);
    g_list_free(weighted_triggers);

    // no triggers found
    if (first_trigger_pos == -1) {
        win_appendln_highlight(window, THEME_ROOMTRIGGER, "%s", message);
    } else {
        if (first_trigger_pos > 0) {
            char message_section[strlen(message) + 1];
            int i = 0;
            while (i < first_trigger_pos) {
                message_section[i] = message[i];
                i++;
            }
            message_section[i] = '\0';
            win_append_highlight(window, THEME_ROOMTRIGGER, "%s", message_section);
        }
        char trigger_section[first_trigger_len + 1];
        int i = 0;
        while (i < first_trigger_len) {
            trigger_section[i] = message[first_trigger_pos + i];
            i++;
        }
        trigger_section[i] = '\0';

        if (first_trigger_pos + first_trigger_len < strlen(message)) {
            win_append_highlight(window, THEME_ROOMTRIGGER_TERM, "%s", trigger_section);
            _mucwin_print_triggers(window, &message[first_trigger_pos + first_trigger_len], triggers);
        } else {
            win_appendln_highlight(window, THEME_ROOMTRIGGER_TERM, "%s", trigger_section);
        }
    }
}

void
mucwin_message(ProfMucWin *mucwin, const char *const nick, const char *const message, GSList *mentions, GList *triggers)
{
    assert(mucwin != NULL);

    ProfWin *window = (ProfWin*)mucwin;
    char *mynick = muc_nick(mucwin->roomjid);

    char ch = '-';
    if (mucwin->message_char) {
        ch = mucwin->message_char[0];
    }

    if (g_strcmp0(nick, mynick) != 0) {
        if (g_slist_length(mentions) > 0) {
            win_print_them(window, THEME_ROOMMENTION, ch, nick);
            _mucwin_print_mention(window, message, mynick, mentions);
        } else if (triggers) {
            win_print_them(window, THEME_ROOMTRIGGER, ch, nick);
            _mucwin_print_triggers(window, message, triggers);
        } else {
            win_println_them_message(window, ch, nick, "%s", message);
        }
    } else {
        win_println_me_message(window, ch, mynick, "%s", message);
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

    win_println(window, THEME_DEFAULT, '-', "");
    win_println(window, THEME_ROOMINFO, '!', "Room locked, requires configuration.");
    win_println(window, THEME_ROOMINFO, '!', "Use '/room accept' to accept the defaults");
    win_println(window, THEME_ROOMINFO, '!', "Use '/room destroy' to cancel and destroy the room");
    win_println(window, THEME_ROOMINFO, '!', "Use '/room config' to edit the room configuration");
    win_println(window, THEME_DEFAULT, '-', "");

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
            win_print(window, THEME_ROOMINFO, '!', "*%s has set the room subject: ", nick);
            win_appendln(window, THEME_DEFAULT, "%s", subject);
        } else {
            win_print(window, THEME_ROOMINFO, '!', "Room subject: ");
            win_appendln(window, THEME_DEFAULT, "%s", subject);
        }
    } else {
        if (nick) {
            win_println(window, THEME_ROOMINFO, '!', "*%s has cleared the room subject.", nick);
        } else {
            win_println(window, THEME_ROOMINFO, '!', "Room subject cleared");
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
    win_println(window, THEME_ERROR, '!', "Error kicking %s: %s", nick, error);
}

void
mucwin_broadcast(ProfMucWin *mucwin, const char *const message)
{
    assert(mucwin != NULL);

    ProfWin *window = (ProfWin*)mucwin;
    int num = wins_get_num(window);

    win_print(window, THEME_ROOMINFO, '!', "Room message: ");
    win_appendln(window, THEME_DEFAULT, "%s", message);

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
    win_println(window, THEME_ERROR, '!', "Error retrieving %s list: %s", affiliation, error);
}

void
mucwin_handle_affiliation_list(ProfMucWin *mucwin, const char *const affiliation, GSList *jids)
{
    assert(mucwin != NULL);

    ProfWin *window = (ProfWin*)mucwin;
    if (jids) {
        win_println(window, THEME_DEFAULT, '!', "Affiliation: %s", affiliation);
        GSList *curr_jid = jids;
        while (curr_jid) {
            const char *jid = curr_jid->data;
            win_println(window, THEME_DEFAULT, '!', "  %s", jid);
            curr_jid = g_slist_next(curr_jid);
        }
        win_println(window, THEME_DEFAULT, '!', "");
    } else {
        win_println(window, THEME_DEFAULT, '!', "No users found with affiliation: %s", affiliation);
        win_println(window, THEME_DEFAULT, '!', "");
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
                win_println(window, THEME_DEFAULT, '!', "No owners found.");
                break;
            case MUC_AFFILIATION_ADMIN:
                win_println(window, THEME_DEFAULT, '!', "No admins found.");
                break;
            case MUC_AFFILIATION_MEMBER:
                win_println(window, THEME_DEFAULT, '!', "No members found.");
                break;
            case MUC_AFFILIATION_OUTCAST:
                win_println(window, THEME_DEFAULT, '!', "No outcasts found.");
                break;
            default:
                break;
        }
        win_println(window, THEME_DEFAULT, '-', "");
    } else {
        switch (affiliation) {
            case MUC_AFFILIATION_OWNER:
                win_println(window, THEME_DEFAULT, '!', "Owners:");
                break;
            case MUC_AFFILIATION_ADMIN:
                win_println(window, THEME_DEFAULT, '!', "Admins:");
                break;
            case MUC_AFFILIATION_MEMBER:
                win_println(window, THEME_DEFAULT, '!', "Members:");
                break;
            case MUC_AFFILIATION_OUTCAST:
                win_println(window, THEME_DEFAULT, '!', "Outcasts:");
                break;
            default:
                break;
        }

        GSList *curr_occupant = occupants;
        while(curr_occupant) {
            Occupant *occupant = curr_occupant->data;
            if (occupant->affiliation == affiliation) {
                if (occupant->jid) {
                    win_println(window, THEME_DEFAULT, '!', "  %s (%s)", occupant->nick, occupant->jid);
                } else {
                    win_println(window, THEME_DEFAULT, '!', "  %s", occupant->nick);
                }
            }

            curr_occupant = g_slist_next(curr_occupant);
        }

        win_println(window, THEME_DEFAULT, '-', "");
    }
}

void
mucwin_role_list_error(ProfMucWin *mucwin, const char *const role, const char *const error)
{
    assert(mucwin != NULL);

    ProfWin *window = (ProfWin*)mucwin;
    win_println(window, THEME_ERROR, '!', "Error retrieving %s list: %s", role, error);
}

void
mucwin_handle_role_list(ProfMucWin *mucwin, const char *const role, GSList *nicks)
{
    assert(mucwin != NULL);

    ProfWin *window = (ProfWin*)mucwin;
    if (nicks) {
        win_println(window, THEME_DEFAULT, '!', "Role: %s", role);
        GSList *curr_nick = nicks;
        while (curr_nick) {
            const char *nick = curr_nick->data;
            Occupant *occupant = muc_roster_item(mucwin->roomjid, nick);
            if (occupant) {
                if (occupant->jid) {
                    win_println(window, THEME_DEFAULT, '!', "  %s (%s)", nick, occupant->jid);
                } else {
                    win_println(window, THEME_DEFAULT, '!', "  %s", nick);
                }
            } else {
                win_println(window, THEME_DEFAULT, '!', "  %s", nick);
            }
            curr_nick = g_slist_next(curr_nick);
        }
        win_println(window, THEME_DEFAULT, '!', "");
    } else {
        win_println(window, THEME_DEFAULT, '!', "No occupants found with role: %s", role);
        win_println(window, THEME_DEFAULT, '!', "");
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
                win_println(window, THEME_DEFAULT, '!', "No moderators found.");
                break;
            case MUC_ROLE_PARTICIPANT:
                win_println(window, THEME_DEFAULT, '!', "No participants found.");
                break;
            case MUC_ROLE_VISITOR:
                win_println(window, THEME_DEFAULT, '!', "No visitors found.");
                break;
            default:
                break;
        }
        win_println(window, THEME_DEFAULT, '-', "");
    } else {
        switch (role) {
            case MUC_ROLE_MODERATOR:
                win_println(window, THEME_DEFAULT, '!', "Moderators:");
                break;
            case MUC_ROLE_PARTICIPANT:
                win_println(window, THEME_DEFAULT, '!', "Participants:");
                break;
            case MUC_ROLE_VISITOR:
                win_println(window, THEME_DEFAULT, '!', "Visitors:");
                break;
            default:
                break;
        }

        GSList *curr_occupant = occupants;
        while(curr_occupant) {
            Occupant *occupant = curr_occupant->data;
            if (occupant->role == role) {
                if (occupant->jid) {
                    win_println(window, THEME_DEFAULT, '!', "  %s (%s)", occupant->nick, occupant->jid);
                } else {
                    win_println(window, THEME_DEFAULT, '!', "  %s", occupant->nick);
                }
            }

            curr_occupant = g_slist_next(curr_occupant);
        }

        win_println(window, THEME_DEFAULT, '-', "");
    }
}

void
mucwin_affiliation_set_error(ProfMucWin *mucwin, const char *const jid, const char *const affiliation,
    const char *const error)
{
    assert(mucwin != NULL);

    ProfWin *window = (ProfWin*)mucwin;
    win_println(window, THEME_ERROR, '!', "Error setting %s affiliation for %s: %s", affiliation, jid, error);
}

void
mucwin_role_set_error(ProfMucWin *mucwin, const char *const nick, const char *const role,
    const char *const error)
{
    assert(mucwin != NULL);

    ProfWin *window = (ProfWin*)mucwin;
    win_println(window, THEME_ERROR, '!', "Error setting %s role for %s: %s", role, nick, error);
}

void
mucwin_info(ProfMucWin *mucwin)
{
    assert(mucwin != NULL);

    char *role = muc_role_str(mucwin->roomjid);
    char *affiliation = muc_affiliation_str(mucwin->roomjid);

    ProfWin *window = (ProfWin*) mucwin;
    win_println(window, THEME_DEFAULT, '!', "Room: %s", mucwin->roomjid);
    win_println(window, THEME_DEFAULT, '!', "Affiliation: %s", affiliation);
    win_println(window, THEME_DEFAULT, '!', "Role: %s", role);
    win_println(window, THEME_DEFAULT, '-', "");
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

char*
mucwin_get_string(ProfMucWin *mucwin)
{
    assert(mucwin != NULL);

    GString *res = g_string_new("Room ");
    g_string_append(res, mucwin->roomjid);

    if (mucwin->unread > 0) {
        g_string_append_printf(res, ", %d unread", mucwin->unread);
    }

    char *resstr = res->str;
    g_string_free(res, FALSE);

    return resstr;
}

void
mucwin_set_enctext(ProfMucWin *mucwin, const char *const enctext)
{
    if (mucwin->enctext) {
        free(mucwin->enctext);
    }
    mucwin->enctext = strdup(enctext);
}

void
mucwin_unset_enctext(ProfMucWin *mucwin)
{
    if (mucwin->enctext) {
        free(mucwin->enctext);
        mucwin->enctext = NULL;
    }
}

void
mucwin_set_message_char(ProfMucWin *mucwin, const char *const ch)
{
    if (mucwin->message_char) {
        free(mucwin->message_char);
    }
    mucwin->message_char = strdup(ch);
}

void
mucwin_unset_message_char(ProfMucWin *mucwin)
{
    if (mucwin->message_char) {
        free(mucwin->message_char);
        mucwin->message_char = NULL;
    }
}
