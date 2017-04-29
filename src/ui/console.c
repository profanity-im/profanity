/*
 * console.c
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


#include <string.h>
#include <stdlib.h>
#include <assert.h>

#ifdef HAVE_NCURSESW_NCURSES_H
#include <ncursesw/ncurses.h>
#elif HAVE_NCURSES_H
#include <ncurses.h>
#endif

#include "common.h"
#include "log.h"
#include "config/preferences.h"
#include "config/theme.h"
#include "command/cmd_defs.h"
#include "ui/window_list.h"
#include "ui/ui.h"
#include "ui/window.h"
#include "ui/statusbar.h"
#include "xmpp/xmpp.h"
#include "xmpp/muc.h"
#include "xmpp/roster_list.h"

#ifdef HAVE_GIT_VERSION
#include "gitversion.h"
#endif

static void _cons_splash_logo(void);
void _show_roster_contacts(GSList *list, gboolean show_groups);

void
cons_debug(const char *const msg, ...)
{
    ProfWin *console = wins_get_console();
    if (strcmp(PACKAGE_STATUS, "development") == 0) {
        va_list arg;
        va_start(arg, msg);
        GString *fmt_msg = g_string_new(NULL);
        g_string_vprintf(fmt_msg, msg, arg);
        win_println(console, THEME_DEFAULT, '-', "%s", fmt_msg->str);
        g_string_free(fmt_msg, TRUE);
        va_end(arg);
    }
}

void
cons_show(const char *const msg, ...)
{
    ProfWin *console = wins_get_console();
    va_list arg;
    va_start(arg, msg);
    GString *fmt_msg = g_string_new(NULL);
    g_string_vprintf(fmt_msg, msg, arg);
    win_println(console, THEME_DEFAULT, '-', "%s", fmt_msg->str);
    g_string_free(fmt_msg, TRUE);
    va_end(arg);
}

void
cons_show_padded(int pad, const char *const msg, ...)
{
    ProfWin *console = wins_get_console();
    va_list arg;
    va_start(arg, msg);
    GString *fmt_msg = g_string_new(NULL);
    g_string_vprintf(fmt_msg, msg, arg);
    win_println_indent(console, pad, "%s", fmt_msg->str);
    g_string_free(fmt_msg, TRUE);
    va_end(arg);
}

void
cons_show_help(const char *const cmd, CommandHelp *help)
{
    ProfWin *console = wins_get_console();

    cons_show("");
    win_println(console, THEME_HELP_HEADER, '-', "%s", &cmd[1]);
    win_print(console, THEME_HELP_HEADER, '-', "");
    int i;
    for (i = 0; i < strlen(cmd) - 1 ; i++) {
        win_append(console, THEME_HELP_HEADER, "-");
    }
    win_appendln(console, THEME_HELP_HEADER, "");
    cons_show("");

    win_println(console, THEME_HELP_HEADER, '-', "Synopsis");
    ui_show_lines(console, help->synopsis);
    cons_show("");

    win_println(console, THEME_HELP_HEADER, '-', "Description");
    win_println(console, THEME_DEFAULT, '-', "%s", help->desc);

    int maxlen = 0;
    for (i = 0; help->args[i][0] != NULL; i++) {
        if (strlen(help->args[i][0]) > maxlen)
            maxlen = strlen(help->args[i][0]);
    }

    if (i > 0) {
        cons_show("");
        win_println(console, THEME_HELP_HEADER, '-', "Arguments");
        for (i = 0; help->args[i][0] != NULL; i++) {
            win_println_indent(console, maxlen + 3, "%-*s: %s", maxlen + 1, help->args[i][0], help->args[i][1]);
        }
    }

    if (g_strv_length((gchar**)help->examples) > 0) {
        cons_show("");
        win_println(console, THEME_HELP_HEADER, '-', "Examples");
        ui_show_lines(console, help->examples);
    }
}

void
cons_bad_cmd_usage(const char *const cmd)
{
    GString *msg = g_string_new("");
    g_string_printf(msg, "Invalid usage, see '/help %s' for details.", &cmd[1]);

    cons_show("");
    cons_show(msg->str);

    g_string_free(msg, TRUE);
}

void
cons_show_error(const char *const msg, ...)
{
    ProfWin *console = wins_get_console();
    va_list arg;
    va_start(arg, msg);
    GString *fmt_msg = g_string_new(NULL);
    g_string_vprintf(fmt_msg, msg, arg);
    win_println(console, THEME_ERROR, '-', "%s", fmt_msg->str);
    g_string_free(fmt_msg, TRUE);
    va_end(arg);

    cons_alert();
}

void
cons_show_tlscert_summary(TLSCertificate *cert)
{
    if (!cert) {
        return;
    }

    cons_show("Subject     : %s", cert->subject_commonname);
    cons_show("Issuer      : %s", cert->issuer_commonname);
    cons_show("Fingerprint : %s", cert->fingerprint);
}

void
cons_show_tlscert(TLSCertificate *cert)
{
    if (!cert) {
        return;
    }

    cons_show("Certificate:");

    cons_show("  Subject:");
    if (cert->subject_commonname) {
        cons_show("    Common name        : %s", cert->subject_commonname);
    }
    if (cert->subject_distinguishedname) {
        cons_show("    Distinguished name : %s", cert->subject_distinguishedname);
    }
    if (cert->subject_organisation) {
        cons_show("    Organisation       : %s", cert->subject_organisation);
    }
    if (cert->subject_organisation_unit) {
        cons_show("    Organisation unit  : %s", cert->subject_organisation_unit);
    }
    if (cert->subject_email) {
        cons_show("    Email              : %s", cert->subject_email);
    }
    if (cert->subject_state) {
        cons_show("    State              : %s", cert->subject_state);
    }
    if (cert->subject_country) {
        cons_show("    Country            : %s", cert->subject_country);
    }
    if (cert->subject_serialnumber) {
        cons_show("    Serial number      : %s", cert->subject_serialnumber);
    }

    cons_show("  Issuer:");
    if (cert->issuer_commonname) {
        cons_show("    Common name        : %s", cert->issuer_commonname);
    }
    if (cert->issuer_distinguishedname) {
        cons_show("    Distinguished name : %s", cert->issuer_distinguishedname);
    }
    if (cert->issuer_organisation) {
        cons_show("    Organisation       : %s", cert->issuer_organisation);
    }
    if (cert->issuer_organisation_unit) {
        cons_show("    Organisation unit  : %s", cert->issuer_organisation_unit);
    }
    if (cert->issuer_email) {
        cons_show("    Email              : %s", cert->issuer_email);
    }
    if (cert->issuer_state) {
        cons_show("    State              : %s", cert->issuer_state);
    }
    if (cert->issuer_country) {
        cons_show("    Country            : %s", cert->issuer_country);
    }
    if (cert->issuer_serialnumber) {
        cons_show("    Serial number      : %s", cert->issuer_serialnumber);
    }

    cons_show("  Version             : %d", cert->version);

    if (cert->serialnumber) {
        cons_show("  Serial number       : %s", cert->serialnumber);
    }

    if (cert->key_alg) {
        cons_show("  Key algorithm       : %s", cert->key_alg);
    }
    if (cert->signature_alg) {
        cons_show("  Signature algorithm : %s", cert->signature_alg);
    }

    cons_show("  Start               : %s", cert->notbefore);
    cons_show("  End                 : %s", cert->notafter);

    cons_show("  Fingerprint         : %s", cert->fingerprint);
}

void
cons_show_typing(const char *const barejid)
{
    ProfWin *console = wins_get_console();
    const char * display_usr = NULL;
    PContact contact = roster_get_contact(barejid);
    if (contact) {
        if (p_contact_name(contact)) {
            display_usr = p_contact_name(contact);
        } else {
            display_usr = barejid;
        }
    } else {
        display_usr = barejid;
    }

    win_println(console, THEME_TYPING, '-', "!! %s is typing a message...", display_usr);
    cons_alert();
}

char*
_room_triggers_to_string(GList *triggers)
{
    GString *triggers_str = g_string_new("");
    GList *curr = triggers;
    while (curr) {
        g_string_append_printf(triggers_str, "\"%s\"", (char*)curr->data);
        curr = g_list_next(curr);
        if (curr) {
            g_string_append(triggers_str, ", ");
        }
    }

    char *result = triggers_str->str;
    g_string_free(triggers_str, FALSE);
    return result;
}

void
cons_show_incoming_room_message(const char *const nick, const char *const room, const int win_index, gboolean mention,
    GList *triggers, int unread)
{
    ProfWin *const console = wins_get_console();

    int ui_index = win_index;
    if (ui_index == 10) {
        ui_index = 0;
    }

    char *muc_show = prefs_get_string(PREF_CONSOLE_MUC);

    if (g_strcmp0(muc_show, "all") == 0) {
        if (mention) {
            win_println(console, THEME_MENTION, '-', "<< room mention: %s in %s (win %d)", nick, room, ui_index);
        } else if (triggers) {
            char *triggers_str = _room_triggers_to_string(triggers);
            win_println(console, THEME_TRIGGER, '-', "<< room trigger %s: %s in %s (win %d)", triggers_str, nick, room, ui_index);
            free(triggers_str);
        } else {
            win_println(console, THEME_INCOMING, '-', "<< room message: %s in %s (win %d)", nick, room, ui_index);
        }
        cons_alert();

    } else if (g_strcmp0(muc_show, "first") == 0) {
        if (mention) {
            win_println(console, THEME_MENTION, '-', "<< room mention: %s in %s (win %d)", nick, room, ui_index);
            cons_alert();
        } else if (triggers) {
            char *triggers_str = _room_triggers_to_string(triggers);
            win_println(console, THEME_TRIGGER, '-', "<< room trigger %s: %s in %s (win %d)", triggers_str, nick, room, ui_index);
            free(triggers_str);
            cons_alert();
        } else if (unread == 0) {
            win_println(console, THEME_INCOMING, '-', "<< room message: %s (win %d)", room, ui_index);
            cons_alert();
        }
    }
    prefs_free_string(muc_show);
}

void
cons_show_incoming_message(const char *const short_from, const int win_index, int unread)
{
    ProfWin *console = wins_get_console();

    int ui_index = win_index;
    if (ui_index == 10) {
        ui_index = 0;
    }

    char *chat_show = prefs_get_string(PREF_CONSOLE_CHAT);
    if (g_strcmp0(chat_show, "all") == 0) {
        win_println(console, THEME_INCOMING, '-', "<< chat message: %s (win %d)", short_from, ui_index);
        cons_alert();
    } else if ((g_strcmp0(chat_show, "first") == 0) && unread == 0) {
        win_println(console, THEME_INCOMING, '-', "<< chat message: %s (win %d)", short_from, ui_index);
        cons_alert();
    }

    prefs_free_string(chat_show);
}

void
cons_show_incoming_private_message(const char *const nick, const char *const room, const int win_index, int unread)
{
    ProfWin *console = wins_get_console();

    int ui_index = win_index;
    if (ui_index == 10) {
        ui_index = 0;
    }

    char *priv_show = prefs_get_string(PREF_CONSOLE_PRIVATE);
    if (g_strcmp0(priv_show, "all") == 0) {
        win_println(console, THEME_INCOMING, '-', "<< private message: %s in %s (win %d)", nick, room, ui_index);
        cons_alert();
    } else if ((g_strcmp0(priv_show, "first") == 0) && unread == 0) {
        win_println(console, THEME_INCOMING, '-', "<< private message: %s in %s (win %d)", nick, room, ui_index);
        cons_alert();
    }

    prefs_free_string(priv_show);
}

void
cons_about(void)
{
    ProfWin *console = wins_get_console();
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    if (prefs_get_boolean(PREF_SPLASH)) {
        _cons_splash_logo();
    } else {

        if (strcmp(PACKAGE_STATUS, "development") == 0) {
#ifdef HAVE_GIT_VERSION
            win_println(console, THEME_DEFAULT, '-', "Welcome to Profanity, version %sdev.%s.%s", PACKAGE_VERSION, PROF_GIT_BRANCH, PROF_GIT_REVISION);
#else
            win_println(console, THEME_DEFAULT, "Welcome to Profanity, version %sdev", PACKAGE_VERSION);
#endif
        } else {
            win_println(console, THEME_DEFAULT, '-', "Welcome to Profanity, version %s", PACKAGE_VERSION);
        }
    }

    win_println(console, THEME_DEFAULT, '-', "Copyright (C) 2012 - 2017 James Booth <%s>.", PACKAGE_BUGREPORT);
    win_println(console, THEME_DEFAULT, '-', "License GPLv3+: GNU GPL version 3 or later <https://www.gnu.org/licenses/gpl.html>");
    win_println(console, THEME_DEFAULT, '-', "");
    win_println(console, THEME_DEFAULT, '-', "This is free software; you are free to change and redistribute it.");
    win_println(console, THEME_DEFAULT, '-', "There is NO WARRANTY, to the extent permitted by law.");
    win_println(console, THEME_DEFAULT, '-', "");
    win_println(console, THEME_DEFAULT, '-', "Type '/help' to show complete help.");
    win_println(console, THEME_DEFAULT, '-', "");

    if (prefs_get_boolean(PREF_VERCHECK)) {
        cons_check_version(FALSE);
    }

    pnoutrefresh(console->layout->win, 0, 0, 1, 0, rows-3, cols-1);

    cons_alert();
}

void
cons_check_version(gboolean not_available_msg)
{
    ProfWin *console = wins_get_console();
    char *latest_release = release_get_latest();

    if (latest_release) {
        gboolean relase_valid = g_regex_match_simple("^\\d+\\.\\d+\\.\\d+$", latest_release, 0, 0);

        if (relase_valid) {
            if (release_is_new(latest_release)) {
                win_println(console, THEME_DEFAULT, '-', "A new version of Profanity is available: %s", latest_release);
                win_println(console, THEME_DEFAULT, '-', "Check <http://www.profanity.im> for details.");
                win_println(console, THEME_DEFAULT, '-', "");
            } else {
                if (not_available_msg) {
                    win_println(console, THEME_DEFAULT, '-', "No new version available.");
                    win_println(console, THEME_DEFAULT, '-', "");
                }
            }

            cons_alert();
        }
        free(latest_release);
    }
}

void
cons_show_login_success(ProfAccount *account, gboolean secured)
{
    ProfWin *console = wins_get_console();

    const char *fulljid = connection_get_fulljid();
    win_print(console, THEME_DEFAULT, '-', "%s logged in successfully, ", fulljid);

    resource_presence_t presence = accounts_get_login_presence(account->name);
    const char *presence_str = string_from_resource_presence(presence);

    theme_item_t presence_colour = theme_main_presence_attrs(presence_str);
    win_append(console, presence_colour, "%s", presence_str);
    win_append(console, THEME_DEFAULT, " (priority %d)", accounts_get_priority_for_presence_type(account->name, presence));
    win_appendln(console, THEME_DEFAULT, ".");
    if (!secured) {
        cons_show_error("TLS connection not established");
    }
    cons_alert();
}

void
cons_show_wins(gboolean unread)
{
    ProfWin *console = wins_get_console();
    cons_show("");
    GSList *window_strings = wins_create_summary(unread);

    if (unread && window_strings == NULL) {
        cons_show("No windows with unread messages.");
        return;
    } else if (unread) {
        cons_show("Unread:");
    } else {
        cons_show("Active windows:");
    }

    GSList *curr = window_strings;
    while (curr) {
        win_println(console, THEME_DEFAULT, '-', "%s", curr->data);
        curr = g_slist_next(curr);
    }
    g_slist_free_full(window_strings, free);

    cons_alert();
}

void
cons_show_room_invites(GList *invites)
{
    cons_show("");
    if (invites == NULL) {
        cons_show("No outstanding chat room invites.");
    } else {
        cons_show("Chat room invites, use /join or /decline commands:");
        while (invites) {
            cons_show("  %s", invites->data);
            invites = g_list_next(invites);
        }
    }

    cons_alert();
}

void
cons_show_info(PContact pcontact)
{
    ProfWin *console = wins_get_console();
    win_show_info(console, pcontact);

    cons_alert();
}

void
cons_show_caps(const char *const fulljid, resource_presence_t presence)
{
    ProfWin *console = wins_get_console();
    cons_show("");

    EntityCapabilities *caps = caps_lookup(fulljid);
    if (caps) {
        const char *resource_presence = string_from_resource_presence(presence);

        theme_item_t presence_colour = theme_main_presence_attrs(resource_presence);
        win_print(console, presence_colour, '-', "%s", fulljid);
        win_appendln(console, THEME_DEFAULT, ":");

        // show identity
        if (caps->identity) {
            DiscoIdentity *identity = caps->identity;
            win_print(console, THEME_DEFAULT, '-', "Identity: ");
            if (identity->name) {
                win_append(console, THEME_DEFAULT, "%s", identity->name);
                if (identity->category || identity->type) {
                    win_append(console, THEME_DEFAULT, " ");
                }
            }
            if (identity->type) {
                win_append(console, THEME_DEFAULT, "%s", identity->type);
                if (identity->category) {
                    win_append(console, THEME_DEFAULT, " ");
                }
            }
            if (identity->category) {
                win_append(console, THEME_DEFAULT, "%s", identity->category);
            }
            win_newline(console);
        }

        if (caps->software_version) {
            SoftwareVersion *software_version = caps->software_version;
            if (software_version->software) {
                win_print(console, THEME_DEFAULT, '-', "Software: %s", software_version->software);
            }
            if (software_version->software_version) {
                win_append(console, THEME_DEFAULT, ", %s", software_version->software_version);
            }
            if (software_version->software || software_version->software_version) {
                win_newline(console);
            }
            if (software_version->os) {
                win_print(console, THEME_DEFAULT, '-', "OS: %s", software_version->os);
            }
            if (software_version->os_version) {
                win_append(console, THEME_DEFAULT, ", %s", software_version->os_version);
            }
            if (software_version->os || software_version->os_version) {
                win_newline(console);
            }
        }

        if (caps->features) {
            win_println(console, THEME_DEFAULT, '-', "Features:");
            GSList *feature = caps->features;
            while (feature) {
                win_println(console, THEME_DEFAULT, '-', " %s", feature->data);
                feature = g_slist_next(feature);
            }
        }
        caps_destroy(caps);

    } else {
        cons_show("No capabilities found for %s", fulljid);
    }

    cons_alert();
}

void
cons_show_received_subs(void)
{
    GList *received = presence_get_subscription_requests();
    if (received == NULL) {
        cons_show("No outstanding subscription requests.");
    } else {
        cons_show("Outstanding subscription requests from:",
            g_list_length(received));
        while (received) {
            cons_show("  %s", received->data);
            received = g_list_next(received);
        }
        g_list_free_full(received, g_free);
    }

    cons_alert();
}

void
cons_show_sent_subs(void)
{
   if (roster_has_pending_subscriptions()) {
        GSList *contacts = roster_get_contacts(ROSTER_ORD_NAME);
        PContact contact = NULL;
        cons_show("Awaiting subscription responses from:");
        GSList *curr = contacts;
        while (curr) {
            contact = (PContact) curr->data;
            if (p_contact_pending_out(contact)) {
                cons_show("  %s", p_contact_barejid(contact));
            }
            curr = g_slist_next(curr);
        }
        g_slist_free(contacts);
    } else {
        cons_show("No pending requests sent.");
    }
    cons_alert();
}

void
cons_show_room_list(GSList *rooms, const char *const conference_node)
{
    ProfWin *console = wins_get_console();
    if (rooms && (g_slist_length(rooms) > 0)) {
        cons_show("Chat rooms at %s:", conference_node);
        while (rooms) {
            DiscoItem *room = rooms->data;
            win_print(console, THEME_DEFAULT, '-', "  %s", room->jid);
            if (room->name) {
                win_append(console, THEME_DEFAULT, ", (%s)", room->name);
            }
            win_newline(console);
            rooms = g_slist_next(rooms);
        }
    } else {
        cons_show("No chat rooms at %s", conference_node);
    }

    cons_alert();
}

void
cons_show_bookmarks(const GList *list)
{
    ProfWin *console = wins_get_console();

    if (list == NULL) {
        cons_show("");
        cons_show("No bookmarks found.");
    } else {
        cons_show("");
        cons_show("Bookmarks:");

        while (list) {
            Bookmark *item = list->data;

            theme_item_t presence_colour = THEME_TEXT;

            if (muc_active(item->barejid)) {
                presence_colour = THEME_ONLINE;
            }
            win_print(console, presence_colour, '-', "  %s", item->barejid);
            if (item->nick) {
                win_append(console, presence_colour, "/%s", item->nick);
            }
            if (item->autojoin) {
                win_append(console, presence_colour, " (autojoin)");
            }
            if (item->password) {
                win_append(console, presence_colour, " (private)");
            }
            if (muc_active(item->barejid)) {
                ProfWin *roomwin = (ProfWin*)wins_get_muc(item->barejid);
                if (roomwin) {
                    int num = wins_get_num(roomwin);
                    win_append(console, presence_colour, " (win %d)", num);
                }
            }
            win_newline(console);
            list = g_list_next(list);
        }
    }

    cons_show("");
    if (prefs_get_boolean(PREF_BOOKMARK_INVITE)) {
        cons_show("Automatic invite bookmarking (/bookmark invites): ON");
    } else {
        cons_show("Automatic invite bookmarking (/bookmark invites): OFF");
    }

    cons_alert();
}

void
cons_show_disco_info(const char *jid, GSList *identities, GSList *features)
{
    if ((identities && (g_slist_length(identities) > 0)) ||
        (features && (g_slist_length(features) > 0))) {
        cons_show("");
        cons_show("Service discovery info for %s", jid);

        if (identities) {
            cons_show("  Identities");
        }
        while (identities) {
            DiscoIdentity *identity = identities->data;  // anme trpe, cat
            GString *identity_str = g_string_new("    ");
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
            cons_show(identity_str->str);
            g_string_free(identity_str, FALSE);
            identities = g_slist_next(identities);
        }

        if (features) {
            cons_show("  Features:");
        }
        while (features) {
            cons_show("    %s", features->data);
            features = g_slist_next(features);
        }

        cons_alert();
    }
}

void
cons_show_disco_items(GSList *items, const char *const jid)
{
    ProfWin *console = wins_get_console();
    if (items && (g_slist_length(items) > 0)) {
        cons_show("");
        cons_show("Service discovery items for %s:", jid);
        while (items) {
            DiscoItem *item = items->data;
            win_print(console, THEME_DEFAULT, '-', "  %s", item->jid);
            if (item->name) {
                win_append(console, THEME_DEFAULT, ", (%s)", item->name);
            }
            win_appendln(console, THEME_DEFAULT, "");
            items = g_slist_next(items);
        }
    } else {
        cons_show("");
        cons_show("No service discovery items for %s", jid);
    }

    cons_alert();
}

void
cons_show_status(const char *const barejid)
{
    ProfWin *console = wins_get_console();
    PContact pcontact = roster_get_contact(barejid);

    if (pcontact) {
        win_show_contact(console, pcontact);
    } else {
        cons_show("No such contact \"%s\" in roster.", barejid);
    }

    cons_alert();
}

void
cons_show_room_invite(const char *const invitor, const char * const room, const char *const reason)
{
    char *display_from = NULL;
    PContact contact = roster_get_contact(invitor);
    if (contact) {
        if (p_contact_name(contact)) {
            display_from = strdup(p_contact_name(contact));
        } else {
            display_from = strdup(invitor);
        }
    } else {
        display_from = strdup(invitor);
    }

    cons_show("");
    cons_show("Chat room invite received:");
    cons_show("  From   : %s", display_from);
    cons_show("  Room   : %s", room);

    if (reason) {
        cons_show("  Message: %s", reason);
    }

    cons_show("Use '/join %s' to accept the invitation", room);
    cons_show("Use '/decline %s' to decline the invitation", room);

    if (prefs_get_boolean(PREF_NOTIFY_INVITE)) {
        notify_invite(display_from, room, reason);
    }

    free(display_from);

    cons_alert();
}

void
cons_show_account_list(gchar **accounts)
{
    ProfWin *console = wins_get_console();
    int size = g_strv_length(accounts);
    if (size > 0) {
        cons_show("Accounts:");
        int i = 0;
        for (i = 0; i < size; i++) {
            if ((connection_get_status() == JABBER_CONNECTED) &&
                    (g_strcmp0(session_get_account_name(), accounts[i]) == 0)) {
                resource_presence_t presence = accounts_get_last_presence(accounts[i]);
                theme_item_t presence_colour = theme_main_presence_attrs(string_from_resource_presence(presence));
                win_println(console, presence_colour, '-', "%s", accounts[i]);
            } else {
                cons_show(accounts[i]);
            }
        }
        cons_show("");
    } else {
        cons_show("No accounts created yet.");
        cons_show("");
    }

    cons_alert();
}

void
cons_show_account(ProfAccount *account)
{
    ProfWin *console = wins_get_console();
    cons_show("");
    cons_show("Account %s:", account->name);
    if (account->enabled) {
        cons_show   ("enabled           : TRUE");
    } else {
        cons_show   ("enabled           : FALSE");
    }
    cons_show       ("jid               : %s", account->jid);
    if (account->eval_password) {
        cons_show   ("eval_password     : %s", account->eval_password);
    } else if (account->password) {
        cons_show   ("password          : [redacted]");
    }
    if (account->resource) {
        cons_show   ("resource          : %s", account->resource);
    }
    if (account->server) {
        cons_show   ("server            : %s", account->server);
    }
    if (account->port != 0) {
        cons_show   ("port              : %d", account->port);
    }
    if (account->muc_service) {
        cons_show   ("muc service       : %s", account->muc_service);
    }
    if (account->muc_nick) {
        cons_show   ("muc nick          : %s", account->muc_nick);
    }
    if (account->tls_policy) {
        cons_show   ("TLS policy        : %s", account->tls_policy);
    }
    if (account->last_presence) {
        cons_show   ("Last presence     : %s", account->last_presence);
    }
    if (account->login_presence) {
        cons_show   ("Login presence    : %s", account->login_presence);
    }
    if (account->startscript) {
        cons_show   ("Start script      : %s", account->startscript);
    }
    if (account->theme) {
        cons_show   ("Theme             : %s", account->theme);
    }
    if (account->otr_policy) {
        cons_show   ("OTR policy        : %s", account->otr_policy);
    }
    if (g_list_length(account->otr_manual) > 0) {
        GString *manual = g_string_new("OTR manual        : ");
        GList *curr = account->otr_manual;
        while (curr) {
            g_string_append(manual, curr->data);
            if (curr->next) {
                g_string_append(manual, ", ");
            }
            curr = curr->next;
        }
        cons_show(manual->str);
        g_string_free(manual, TRUE);
    }
    if (g_list_length(account->otr_opportunistic) > 0) {
        GString *opportunistic = g_string_new("OTR opportunistic : ");
        GList *curr = account->otr_opportunistic;
        while (curr) {
            g_string_append(opportunistic, curr->data);
            if (curr->next) {
                g_string_append(opportunistic, ", ");
            }
            curr = curr->next;
        }
        cons_show(opportunistic->str);
        g_string_free(opportunistic, TRUE);
    }
    if (g_list_length(account->otr_always) > 0) {
        GString *always = g_string_new("OTR always        : ");
        GList *curr = account->otr_always;
        while (curr) {
            g_string_append(always, curr->data);
            if (curr->next) {
                g_string_append(always, ", ");
            }
            curr = curr->next;
        }
        cons_show(always->str);
        g_string_free(always, TRUE);
    }

    if (account->pgp_keyid) {
        cons_show   ("PGP Key ID        : %s", account->pgp_keyid);
    }

    cons_show       ("Priority          : chat:%d, online:%d, away:%d, xa:%d, dnd:%d",
        account->priority_chat, account->priority_online, account->priority_away,
        account->priority_xa, account->priority_dnd);

    if ((connection_get_status() == JABBER_CONNECTED) &&
            (g_strcmp0(session_get_account_name(), account->name) == 0)) {
        GList *resources = connection_get_available_resources();
        GList *ordered_resources = NULL;

        GList *curr = resources;
        if (curr) {
            win_println(console, THEME_DEFAULT, '-', "Resources:");

            // sort in order of availability
            while (curr) {
                Resource *resource = curr->data;
                ordered_resources = g_list_insert_sorted(ordered_resources,
                    resource, (GCompareFunc)resource_compare_availability);
                curr = g_list_next(curr);
            }
        }

        g_list_free(resources);

        curr = ordered_resources;
        while (curr) {
            Resource *resource = curr->data;
            const char *resource_presence = string_from_resource_presence(resource->presence);
            theme_item_t presence_colour = theme_main_presence_attrs(resource_presence);
            win_print(console, presence_colour, '-', "  %s (%d), %s", resource->name, resource->priority, resource_presence);

            if (resource->status) {
                win_append(console, presence_colour, ", \"%s\"", resource->status);
            }
            win_appendln(console, THEME_DEFAULT, "");
            Jid *jidp = jid_create_from_bare_and_resource(account->jid, resource->name);
            EntityCapabilities *caps = caps_lookup(jidp->fulljid);
            jid_destroy(jidp);

            if (caps) {
                // show identity
                if (caps->identity) {
                    DiscoIdentity *identity = caps->identity;
                    win_print(console, THEME_DEFAULT, '-', "    Identity: ");
                    if (identity->name) {
                        win_append(console, THEME_DEFAULT, "%s", identity->name);
                        if (identity->category || identity->type) {
                            win_append(console, THEME_DEFAULT, " ");
                        }
                    }
                    if (identity->type) {
                        win_append(console, THEME_DEFAULT, "%s", identity->type);
                        if (identity->category) {
                            win_append(console, THEME_DEFAULT, " ");
                        }
                    }
                    if (identity->category) {
                        win_append(console, THEME_DEFAULT, "%s", identity->category);
                    }
                    win_newline(console);
                }

                if (caps->software_version) {
                    SoftwareVersion *software_version = caps->software_version;
                    if (software_version->software) {
                        win_print(console, THEME_DEFAULT, '-', "    Software: %s", software_version->software);
                    }
                    if (software_version->software_version) {
                        win_append(console, THEME_DEFAULT, ", %s", software_version->software_version);
                    }
                    if (software_version->software || software_version->software_version) {
                        win_newline(console);
                    }
                    if (software_version->os) {
                        win_print(console, THEME_DEFAULT, '-', "    OS: %s", software_version->os);
                    }
                    if (software_version->os_version) {
                        win_append(console, THEME_DEFAULT, ", %s", software_version->os_version);
                    }
                    if (software_version->os || software_version->os_version) {
                        win_newline(console);
                    }
                }

                caps_destroy(caps);
            }

            curr = g_list_next(curr);
        }
        g_list_free(ordered_resources);
    }

    cons_alert();
}

void
cons_show_aliases(GList *aliases)
{
    if (aliases == NULL) {
        cons_show("No aliases configured.");
        return;
    }

    GList *curr = aliases;
    if (curr) {
        cons_show("Command aliases:");
    }
    while (curr) {
        ProfAlias *alias = curr->data;
        cons_show("  /%s -> %s", alias->name, alias->value);
        curr = g_list_next(curr);
    }
    cons_show("");
}

void
cons_theme_setting(void)
{
    char *theme = prefs_get_string(PREF_THEME);
    if (theme == NULL) {
        cons_show("Theme (/theme)                      : default");
    } else {
        cons_show("Theme (/theme)                      : %s", theme);
    }
    prefs_free_string(theme);
}

void
cons_privileges_setting(void)
{
    if (prefs_get_boolean(PREF_MUC_PRIVILEGES))
        cons_show("MUC privileges (/privileges)        : ON");
    else
        cons_show("MUC privileges (/privileges)        : OFF");
}

void
cons_beep_setting(void)
{
    if (prefs_get_boolean(PREF_BEEP))
        cons_show("Terminal beep (/beep)               : ON");
    else
        cons_show("Terminal beep (/beep)               : OFF");
}

void
cons_resource_setting(void)
{
    if (prefs_get_boolean(PREF_RESOURCE_TITLE))
        cons_show("Resource title (/resource)          : ON");
    else
        cons_show("Resource title (/resource)          : OFF");
    if (prefs_get_boolean(PREF_RESOURCE_MESSAGE))
        cons_show("Resource message (/resource)        : ON");
    else
        cons_show("Resource message (/resource)        : OFF");
}

void
cons_wrap_setting(void)
{
    if (prefs_get_boolean(PREF_WRAP))
        cons_show("Word wrap (/wrap)                   : ON");
    else
        cons_show("Word wrap (/wrap)                   : OFF");
}

void
cons_winstidy_setting(void)
{
    if (prefs_get_boolean(PREF_WINS_AUTO_TIDY))
        cons_show("Window Auto Tidy (/wins)            : ON");
    else
        cons_show("Window Auto Tidy (/wins)            : OFF");
}

void
cons_encwarn_setting(void)
{
    if (prefs_get_boolean(PREF_ENC_WARN)) {
        cons_show("Warn unencrypted (/encwarn)         : ON");
    } else {
        cons_show("Warn unencrypted (/encwarn)         : OFF");
    }
}

void
cons_console_setting(void)
{
    char *chatsetting = prefs_get_string(PREF_CONSOLE_CHAT);
    cons_show("Console chat messages (/console)    : %s", chatsetting);
    prefs_free_string(chatsetting);

    char *mucsetting = prefs_get_string(PREF_CONSOLE_MUC);
    cons_show("Console MUC messages (/console)     : %s", mucsetting);
    prefs_free_string(mucsetting);

    char *privsetting = prefs_get_string(PREF_CONSOLE_PRIVATE);
    cons_show("Console private messages (/console) : %s", privsetting);
    prefs_free_string(privsetting);
}

void
cons_tlsshow_setting(void)
{
    if (prefs_get_boolean(PREF_TLS_SHOW)) {
        cons_show("TLS show (/tls)                     : ON");
    } else {
        cons_show("TLS show (/tls)                     : OFF");
    }
}

void
cons_presence_setting(void)
{
    if (prefs_get_boolean(PREF_PRESENCE))
        cons_show("Titlebar presence (/presence)       : ON");
    else
        cons_show("Titlebar presence (/presence)       : OFF");

    char *console = prefs_get_string(PREF_STATUSES_CONSOLE);
    char *chat = prefs_get_string(PREF_STATUSES_CHAT);
    char *room = prefs_get_string(PREF_STATUSES_MUC);

    cons_show("Console presence (/presence)        : %s", console);
    cons_show("Chat presence (/presence)           : %s", chat);
    cons_show("Room presence (/presence)           : %s", room);

    prefs_free_string(console);
    prefs_free_string(chat);
    prefs_free_string(room);
}

void
cons_flash_setting(void)
{
    if (prefs_get_boolean(PREF_FLASH))
        cons_show("Terminal flash (/flash)             : ON");
    else
        cons_show("Terminal flash (/flash)             : OFF");
}

void
cons_tray_setting(void)
{
    if (prefs_get_boolean(PREF_TRAY))
        cons_show("Tray icon (/tray)                   : ON");
    else
        cons_show("Tray icon (/tray)                   : OFF");

    if (prefs_get_boolean(PREF_TRAY_READ))
        cons_show("Tray icon read (/tray)              : ON");
    else
        cons_show("Tray icon read (/tray)              : OFF");

    int seconds = prefs_get_tray_timer();
    if (seconds == 1) {
        cons_show("Tray timer (/tray)                  : 1 second");
    } else {
        cons_show("Tray timer (/tray)                  : %d seconds", seconds);
    }
}

void
cons_splash_setting(void)
{
    if (prefs_get_boolean(PREF_SPLASH))
        cons_show("Splash screen (/splash)             : ON");
    else
        cons_show("Splash screen (/splash)             : OFF");
}

void
cons_occupants_setting(void)
{
    if (prefs_get_boolean(PREF_OCCUPANTS))
        cons_show("Occupants (/occupants)              : show");
    else
        cons_show("Occupants (/occupants)              : hide");

    if (prefs_get_boolean(PREF_OCCUPANTS_JID))
        cons_show("Occupant jids (/occupants)          : show");
    else
        cons_show("Occupant jids (/occupants)          : hide");

    int size = prefs_get_occupants_size();
    cons_show("Occupants size (/occupants)         : %d", size);
}

void
cons_autoconnect_setting(void)
{
    char *pref_connect_account = prefs_get_string(PREF_CONNECT_ACCOUNT);
    if (pref_connect_account)
        cons_show("Autoconnect (/autoconnect)      : %s", pref_connect_account);
    else
        cons_show("Autoconnect (/autoconnect)      : OFF");

    prefs_free_string(pref_connect_account);
}

void
cons_time_setting(void)
{
    char *pref_time_console = prefs_get_string(PREF_TIME_CONSOLE);
    if (g_strcmp0(pref_time_console, "off") == 0)
        cons_show("Time console (/time)                : OFF");
    else
        cons_show("Time console (/time)                : %s", pref_time_console);
    prefs_free_string(pref_time_console);

    char *pref_time_chat = prefs_get_string(PREF_TIME_CHAT);
    if (g_strcmp0(pref_time_chat, "off") == 0)
        cons_show("Time chat (/time)                   : OFF");
    else
        cons_show("Time chat (/time)                   : %s", pref_time_chat);
    prefs_free_string(pref_time_chat);

    char *pref_time_muc = prefs_get_string(PREF_TIME_MUC);
    if (g_strcmp0(pref_time_muc, "off") == 0)
        cons_show("Time MUC (/time)                    : OFF");
    else
        cons_show("Time MUC (/time)                    : %s", pref_time_muc);
    prefs_free_string(pref_time_muc);

    char *pref_time_mucconf = prefs_get_string(PREF_TIME_MUCCONFIG);
    if (g_strcmp0(pref_time_mucconf, "off") == 0)
        cons_show("Time MUC config (/time)             : OFF");
    else
        cons_show("Time MUC config (/time)             : %s", pref_time_mucconf);
    prefs_free_string(pref_time_mucconf);

    char *pref_time_private = prefs_get_string(PREF_TIME_PRIVATE);
    if (g_strcmp0(pref_time_private, "off") == 0)
        cons_show("Time private (/time)                : OFF");
    else
        cons_show("Time private (/time)                : %s", pref_time_private);
    prefs_free_string(pref_time_private);

    char *pref_time_xml = prefs_get_string(PREF_TIME_XMLCONSOLE);
    if (g_strcmp0(pref_time_xml, "off") == 0)
        cons_show("Time XML Console (/time)            : OFF");
    else
        cons_show("Time XML Console (/time)            : %s", pref_time_xml);
    prefs_free_string(pref_time_xml);

    char *pref_time_statusbar = prefs_get_string(PREF_TIME_STATUSBAR);
    if (g_strcmp0(pref_time_statusbar, "off") == 0)
        cons_show("Time statusbar (/time)              : OFF");
    else
        cons_show("Time statusbar (/time)              : %s", pref_time_statusbar);
    prefs_free_string(pref_time_statusbar);

    char *pref_time_lastactivity = prefs_get_string(PREF_TIME_LASTACTIVITY);
    cons_show("Time last activity (/time)          : %s", pref_time_lastactivity);
    prefs_free_string(pref_time_lastactivity);
}

void
cons_vercheck_setting(void)
{
    if (prefs_get_boolean(PREF_VERCHECK))
        cons_show("Version checking (/vercheck)        : ON");
    else
        cons_show("Version checking (/vercheck)        : OFF");
}

void
cons_wintitle_setting(void)
{
    if (prefs_get_boolean(PREF_WINTITLE_SHOW)) {
        cons_show("Window title show (/wintitle)       : ON");
    } else {
        cons_show("Window title show (/wintitle)       : OFF");
    }
    if (prefs_get_boolean(PREF_WINTITLE_GOODBYE)) {
        cons_show("Window title goodbye (/wintitle)    : ON");
    } else {
        cons_show("Window title goodbye (/wintitle)    : OFF");
    }
}

void
cons_roster_setting(void)
{
    if (prefs_get_boolean(PREF_ROSTER))
        cons_show("Roster (/roster)                    : show");
    else
        cons_show("Roster (/roster)                    : hide");

    if (prefs_get_boolean(PREF_ROSTER_OFFLINE))
        cons_show("Roster offline (/roster)            : show");
    else
        cons_show("Roster offline (/roster)            : hide");

    char header_ch = prefs_get_roster_header_char();
    if (header_ch)
        cons_show("Roster header char (/roster)        : %c", header_ch);
    else
        cons_show("Roster header char (/roster)        : none");

    char contact_ch = prefs_get_roster_contact_char();
    if (contact_ch)
        cons_show("Roster contact char (/roster)       : %c", contact_ch);
    else
        cons_show("Roster contact char (/roster)       : none");

    char resource_ch = prefs_get_roster_resource_char();
    if (resource_ch)
        cons_show("Roster resource char (/roster)      : %c", resource_ch);
    else
        cons_show("Roster resource char (/roster)      : none");

    char room_ch = prefs_get_roster_room_char();
    if (room_ch)
        cons_show("Roster room char (/roster)          : %c", room_ch);
    else
        cons_show("Roster room char (/roster)          : none");

    char room_priv_ch = prefs_get_roster_room_private_char();
    if (room_priv_ch)
        cons_show("Roster room private char (/roster)  : %c", room_priv_ch);
    else
        cons_show("Roster room private char (/roster)  : none");

    char private_ch = prefs_get_roster_private_char();
    if (private_ch)
        cons_show("Roster private char (/roster)       : %c", private_ch);
    else
        cons_show("Roster private char (/roster)       : none");

    gint contact_indent = prefs_get_roster_contact_indent();
    cons_show("Roster contact indent (/roster)     : %d", contact_indent);

    if (prefs_get_boolean(PREF_ROSTER_RESOURCE))
        cons_show("Roster resource (/roster)           : show");
    else
        cons_show("Roster resource (/roster)           : hide");

    gint resource_indent = prefs_get_roster_resource_indent();
    cons_show("Roster resource indent (/roster)    : %d", resource_indent);

    if (prefs_get_boolean(PREF_ROSTER_RESOURCE_JOIN))
        cons_show("Roster resource join (/roster)      : on");
    else
        cons_show("Roster resource join (/roster)      : off");

    if (prefs_get_boolean(PREF_ROSTER_PRESENCE))
        cons_show("Roster presence (/roster)           : show");
    else
        cons_show("Roster presence (/roster)           : hide");

    gint presence_indent = prefs_get_roster_presence_indent();
    cons_show("Roster presence indent (/roster)    : %d", presence_indent);

    if (prefs_get_boolean(PREF_ROSTER_STATUS))
        cons_show("Roster status (/roster)             : show");
    else
        cons_show("Roster status (/roster)             : hide");

    if (prefs_get_boolean(PREF_ROSTER_EMPTY))
        cons_show("Roster empty (/roster)              : show");
    else
        cons_show("Roster empty (/roster)              : hide");

    if (prefs_get_boolean(PREF_ROSTER_PRIORITY))
        cons_show("Roster priority (/roster)           : show");
    else
        cons_show("Roster priority (/roster)           : hide");

    if (prefs_get_boolean(PREF_ROSTER_CONTACTS))
        cons_show("Roster contacts (/roster)           : show");
    else
        cons_show("Roster contacts (/roster)           : hide");

    if (prefs_get_boolean(PREF_ROSTER_UNSUBSCRIBED))
        cons_show("Roster unsubscribed (/roster)       : show");
    else
        cons_show("Roster unsubscribed (/roster)       : hide");

    char *count = prefs_get_string(PREF_ROSTER_COUNT);
    if (g_strcmp0(count, "off") == 0) {
        cons_show("Roster count (/roster)              : OFF");
    } else {
        cons_show("Roster count (/roster)              : %s", count);
    }
    prefs_free_string(count);

    if (prefs_get_boolean(PREF_ROSTER_COUNT_ZERO))
        cons_show("Roster count zero (/roster)         : ON");
    else
        cons_show("Roster count zero (/roster)         : OFF");

    char *by = prefs_get_string(PREF_ROSTER_BY);
    cons_show("Roster by (/roster)                 : %s", by);
    prefs_free_string(by);

    char *order = prefs_get_string(PREF_ROSTER_ORDER);
    cons_show("Roster order (/roster)              : %s", order);
    prefs_free_string(order);

    char *unread = prefs_get_string(PREF_ROSTER_UNREAD);
    if (g_strcmp0(unread, "before") == 0) {
        cons_show("Roster unread (/roster)             : before");
    } else if (g_strcmp0(unread, "after") == 0) {
        cons_show("Roster unread (/roster)             : after");
    } else {
        cons_show("Roster unread (/roster)             : OFF");
    }

    if (prefs_get_boolean(PREF_ROSTER_ROOMS))
        cons_show("Roster rooms (/roster)              : show");
    else
        cons_show("Roster rooms (/roster)              : hide");

    char *priv = prefs_get_string(PREF_ROSTER_PRIVATE);
    if (g_strcmp0(priv, "room") == 0) {
        cons_show("Roster private (/roster)            : room");
    } else if (g_strcmp0(priv, "group") == 0) {
        cons_show("Roster private (/roster)            : group");
    } else {
        cons_show("Roster private (/roster)            : OFF");
    }

    char *rooms_pos = prefs_get_string(PREF_ROSTER_ROOMS_POS);
    cons_show("Roster rooms position (/roster)     : %s", rooms_pos);
    prefs_free_string(rooms_pos);

    char *rooms_by = prefs_get_string(PREF_ROSTER_ROOMS_BY);
    cons_show("Roster rooms by (/roster)           : %s", rooms_by);
    prefs_free_string(rooms_by);

    char *rooms_order = prefs_get_string(PREF_ROSTER_ROOMS_ORDER);
    cons_show("Roster rooms order (/roster)        : %s", rooms_order);
    prefs_free_string(rooms_order);

    char *roomsunread = prefs_get_string(PREF_ROSTER_ROOMS_UNREAD);
    if (g_strcmp0(roomsunread, "before") == 0) {
        cons_show("Roster rooms unread (/roster)       : before");
    } else if (g_strcmp0(roomsunread, "after") == 0) {
        cons_show("Roster rooms unread (/roster)       : after");
    } else {
        cons_show("Roster rooms unread (/roster)       : OFF");
    }

    int size = prefs_get_roster_size();
    cons_show("Roster size (/roster)               : %d", size);

    if (prefs_get_boolean(PREF_ROSTER_WRAP))
        cons_show("Roster wrap (/roster)               : ON");
    else
        cons_show("Roster wrap (/roster)               : OFF");
}

void
cons_show_ui_prefs(void)
{
    cons_show("UI preferences:");
    cons_show("");
    cons_theme_setting();
    cons_beep_setting();
    cons_flash_setting();
    cons_splash_setting();
    cons_winpos_setting();
    cons_wrap_setting();
    cons_winstidy_setting();
    cons_time_setting();
    cons_resource_setting();
    cons_vercheck_setting();
    cons_console_setting();
    cons_occupants_setting();
    cons_roster_setting();
    cons_privileges_setting();
    cons_wintitle_setting();
    cons_encwarn_setting();
    cons_presence_setting();
    cons_inpblock_setting();
    cons_tlsshow_setting();

    cons_alert();
}

void
cons_notify_setting(void)
{
    if (!is_notify_enabled()) {
        cons_show("Notification support was not included in this build.");
        return;
    }

    if (prefs_get_boolean(PREF_NOTIFY_CHAT))
        cons_show("Chat message (/notify chat)         : ON");
    else
        cons_show("Chat message (/notify chat)         : OFF");

    if (prefs_get_boolean(PREF_NOTIFY_CHAT_CURRENT))
        cons_show("Chat current (/notify chat)         : ON");
    else
        cons_show("Chat current (/notify chat)         : OFF");

    if (prefs_get_boolean(PREF_NOTIFY_CHAT_TEXT))
        cons_show("Chat text (/notify chat)            : ON");
    else
        cons_show("Chat text (/notify chat)            : OFF");

    if (prefs_get_boolean(PREF_NOTIFY_ROOM))
        cons_show("Room message (/notify room)         : ON");
    else
        cons_show("Room message (/notify room)         : OFF");

    if (prefs_get_boolean(PREF_NOTIFY_ROOM_MENTION))
        cons_show("Room mention (/notify room)         : ON");
    else
        cons_show("Room mention (/notify room)         : OFF");

    if (prefs_get_boolean(PREF_NOTIFY_MENTION_CASE_SENSITIVE))
        cons_show("Room mention case (/notify room)    : Case sensitive");
    else
        cons_show("Room mention case (/notify room)    : Case insensitive");

    if (prefs_get_boolean(PREF_NOTIFY_MENTION_WHOLE_WORD))
        cons_show("Room mention word (/notify room)    : Whole word only");
    else
        cons_show("Room mention word (/notify room)    : Part of word");

    if (prefs_get_boolean(PREF_NOTIFY_ROOM_TRIGGER))
        cons_show("Room trigger (/notify room)         : ON");
    else
        cons_show("Room trigger (/notify room)         : OFF");

    if (prefs_get_boolean(PREF_NOTIFY_ROOM_CURRENT))
        cons_show("Room current (/notify room)         : ON");
    else
        cons_show("Room current (/notify room)         : OFF");

    if (prefs_get_boolean(PREF_NOTIFY_ROOM_TEXT))
        cons_show("Room text (/notify room)            : ON");
    else
        cons_show("Room text (/notify room)            : OFF");

    if (prefs_get_boolean(PREF_NOTIFY_TYPING))
        cons_show("Composing (/notify typing)          : ON");
    else
        cons_show("Composing (/notify typing)          : OFF");

    if (prefs_get_boolean(PREF_NOTIFY_TYPING_CURRENT))
        cons_show("Composing current (/notify typing)  : ON");
    else
        cons_show("Composing current (/notify typing)  : OFF");

    if (prefs_get_boolean(PREF_NOTIFY_INVITE))
        cons_show("Room invites (/notify invite)       : ON");
    else
        cons_show("Room invites (/notify invite)       : OFF");

    if (prefs_get_boolean(PREF_NOTIFY_SUB))
        cons_show("Subscription requests (/notify sub) : ON");
    else
        cons_show("Subscription requests (/notify sub) : OFF");

    gint remind_period = prefs_get_notify_remind();
    if (remind_period == 0) {
        cons_show("Reminder period (/notify remind)    : OFF");
    } else if (remind_period == 1) {
        cons_show("Reminder period (/notify remind)    : 1 second");
    } else {
        cons_show("Reminder period (/notify remind)    : %d seconds", remind_period);
    }
}

void
cons_show_desktop_prefs(void)
{
    cons_show("Desktop notification preferences:");
    cons_show("");
    cons_notify_setting();
    cons_tray_setting();

    cons_alert();
}

void
cons_states_setting(void)
{
    if (prefs_get_boolean(PREF_STATES))
        cons_show("Send chat states (/states)    : ON");
    else
        cons_show("Send chat states (/states)    : OFF");
}

void
cons_outtype_setting(void)
{
    if (prefs_get_boolean(PREF_OUTTYPE))
        cons_show("Send composing (/outtype)     : ON");
    else
        cons_show("Send composing (/outtype)     : OFF");
}

void
cons_intype_setting(void)
{
    if (prefs_get_boolean(PREF_INTYPE))
        cons_show("Show typing (/intype)         : ON");
    else
        cons_show("Show typing (/intype)         : OFF");
}

void
cons_gone_setting(void)
{
    gint gone_time = prefs_get_gone();
    if (gone_time == 0) {
        cons_show("Leave conversation (/gone)    : OFF");
    } else if (gone_time == 1) {
        cons_show("Leave conversation (/gone)    : 1 minute");
    } else {
        cons_show("Leave conversation (/gone)    : %d minutes", gone_time);
    }
}

void
cons_history_setting(void)
{
    if (prefs_get_boolean(PREF_HISTORY))
        cons_show("Chat history (/history)       : ON");
    else
        cons_show("Chat history (/history)       : OFF");
}

void
cons_carbons_setting(void)
{
    if (prefs_get_boolean(PREF_CARBONS))
        cons_show("Message carbons (/carbons)    : ON");
    else
        cons_show("Message carbons (/carbons)    : OFF");
}

void
cons_receipts_setting(void)
{
    if (prefs_get_boolean(PREF_RECEIPTS_REQUEST))
        cons_show("Request receipts (/receipts)  : ON");
    else
        cons_show("Request receipts (/receipts)  : OFF");

    if (prefs_get_boolean(PREF_RECEIPTS_SEND))
        cons_show("Send receipts (/receipts)     : ON");
    else
        cons_show("Send receipts (/receipts)     : OFF");

}

void
cons_show_chat_prefs(void)
{
    cons_show("Chat preferences:");
    cons_show("");
    cons_states_setting();
    cons_outtype_setting();
    cons_intype_setting();
    cons_gone_setting();
    cons_history_setting();
    cons_carbons_setting();
    cons_receipts_setting();

    cons_alert();
}

void
cons_inpblock_setting(void)
{
    cons_show("Input timeout (/inpblock)           : %d milliseconds", prefs_get_inpblock());
    if (prefs_get_boolean(PREF_INPBLOCK_DYNAMIC)) {
        cons_show("Dynamic timeout (/inpblock)         : ON");
    } else {
        cons_show("Dynamic timeout (/inpblock)         : OFF");
    }
}

void
cons_winpos_setting(void)
{
    ProfWinPlacement *placement = prefs_get_win_placement();
    cons_show("Title bar postion (/titlebar)       : %d", placement->titlebar_pos);
    cons_show("Main window postion (/mainwin)      : %d", placement->mainwin_pos);
    cons_show("Status bar postion (/statusbar)     : %d", placement->statusbar_pos);
    cons_show("Input window postion (/inputwin)    : %d", placement->inputwin_pos);
    prefs_free_win_placement(placement);
}

void
cons_log_setting(void)
{
    cons_show("Log file location           : %s", get_log_file_location());
    cons_show("Max log size (/log maxsize) : %d bytes", prefs_get_max_log_size());

    if (prefs_get_boolean(PREF_LOG_ROTATE))
        cons_show("Log rotation (/log rotate)  : ON");
    else
        cons_show("Log rotation (/log rotate)  : OFF");

    if (prefs_get_boolean(PREF_LOG_SHARED))
        cons_show("Shared log (/log shared)    : ON");
    else
        cons_show("Shared log (/log shared)    : OFF");
}

void
cons_chlog_setting(void)
{
    if (prefs_get_boolean(PREF_CHLOG))
        cons_show("Chat logging (/chlog)       : ON");
    else
        cons_show("Chat logging (/chlog)       : OFF");
}

void
cons_grlog_setting(void)
{
    if (prefs_get_boolean(PREF_GRLOG))
        cons_show("Groupchat logging (/grlog)  : ON");
    else
        cons_show("Groupchat logging (/grlog)  : OFF");
}

void
cons_show_log_prefs(void)
{
    cons_show("Logging preferences:");
    cons_show("");
    cons_log_setting();
    cons_chlog_setting();
    cons_grlog_setting();

    cons_alert();
}

void
cons_autoaway_setting(void)
{
    char *pref_autoaway_mode = prefs_get_string(PREF_AUTOAWAY_MODE);
    if (strcmp(pref_autoaway_mode, "off") == 0) {
        cons_show("Autoaway (/autoaway mode)                 : OFF");
    } else {
        cons_show("Autoaway (/autoaway mode)                 : %s", pref_autoaway_mode);
    }
    prefs_free_string(pref_autoaway_mode);

    int away_time = prefs_get_autoaway_time();
    if (away_time == 1) {
        cons_show("Autoaway away minutes (/autoaway time)    : 1 minute");
    } else {
        cons_show("Autoaway away minutes (/autoaway time)    : %d minutes", away_time);
    }

    int xa_time = prefs_get_autoxa_time();
    if (xa_time == 0) {
        cons_show("Autoaway xa minutes (/autoaway time)      : OFF");
    } else if (xa_time == 1) {
        cons_show("Autoaway xa minutes (/autoaway time)      : 1 minute");
    } else {
        cons_show("Autoaway xa minutes (/autoaway time)      : %d minutes", xa_time);
    }

    char *pref_autoaway_message = prefs_get_string(PREF_AUTOAWAY_MESSAGE);
    if ((pref_autoaway_message == NULL) || (strcmp(pref_autoaway_message, "") == 0)) {
        cons_show("Autoaway away message (/autoaway message) : OFF");
    } else {
        cons_show("Autoaway away message (/autoaway message) : \"%s\"", pref_autoaway_message);
    }
    prefs_free_string(pref_autoaway_message);

    char *pref_autoxa_message = prefs_get_string(PREF_AUTOXA_MESSAGE);
    if ((pref_autoxa_message == NULL) || (strcmp(pref_autoxa_message, "") == 0)) {
        cons_show("Autoaway xa message (/autoaway message)   : OFF");
    } else {
        cons_show("Autoaway xa message (/autoaway message)   : \"%s\"", pref_autoxa_message);
    }
    prefs_free_string(pref_autoxa_message);

    if (prefs_get_boolean(PREF_AUTOAWAY_CHECK)) {
        cons_show("Autoaway check (/autoaway check)          : ON");
    } else {
        cons_show("Autoaway check (/autoaway check)          : OFF");
    }
}

void
cons_show_presence_prefs(void)
{
    cons_show("Presence preferences:");
    cons_show("");
    cons_autoaway_setting();

    if (prefs_get_boolean(PREF_LASTACTIVITY)) {
        cons_show("Send last activity (/lastactivity)        : ON");
    } else {
        cons_show("Send last activity (/lastactivity)        : OFF");
    }

    cons_alert();
}

void
cons_reconnect_setting(void)
{
    gint reconnect_interval = prefs_get_reconnect();
    if (reconnect_interval == 0) {
        cons_show("Reconnect interval (/reconnect) : OFF");
    } else if (reconnect_interval == 1) {
        cons_show("Reconnect interval (/reconnect) : 1 second");
    } else {
        cons_show("Reconnect interval (/reconnect) : %d seconds", reconnect_interval);
    }
}

void
cons_autoping_setting(void)
{
    gint autoping_interval = prefs_get_autoping();
    if (autoping_interval == 0) {
        cons_show("Autoping interval (/autoping)   : OFF");
    } else if (autoping_interval == 1) {
        cons_show("Autoping interval (/autoping)   : 1 second");
    } else {
        cons_show("Autoping interval (/autoping)   : %d seconds", autoping_interval);
    }

    gint autoping_timeout = prefs_get_autoping_timeout();
    if (autoping_timeout == 0) {
        cons_show("Autoping timeout (/autoping)    : OFF");
    } else if (autoping_timeout == 1) {
        cons_show("Autoping timeout (/autoping)    : 1 second");
    } else {
        cons_show("Autoping timeout (/autoping)    : %d seconds", autoping_timeout);
    }
}

void
cons_show_connection_prefs(void)
{
    cons_show("Connection preferences:");
    cons_show("");
    cons_reconnect_setting();
    cons_autoping_setting();
    cons_autoconnect_setting();

    cons_alert();
}

void
cons_show_otr_prefs(void)
{
    cons_show("OTR preferences:");
    cons_show("");

    char *policy_value = prefs_get_string(PREF_OTR_POLICY);
    cons_show("OTR policy (/otr policy) : %s", policy_value);
    prefs_free_string(policy_value);

    char *log_value = prefs_get_string(PREF_OTR_LOG);
    if (strcmp(log_value, "on") == 0) {
        cons_show("OTR logging (/otr log)   : ON");
    } else if (strcmp(log_value, "off") == 0) {
        cons_show("OTR logging (/otr log)   : OFF");
    } else {
        cons_show("OTR logging (/otr log)   : Redacted");
    }
    prefs_free_string(log_value);

    char ch = prefs_get_otr_char();
    cons_show("OTR char (/otr char)     : %c", ch);

    cons_alert();
}

void
cons_show_pgp_prefs(void)
{
    cons_show("PGP preferences:");
    cons_show("");

    char *log_value = prefs_get_string(PREF_PGP_LOG);
    if (strcmp(log_value, "on") == 0) {
        cons_show("PGP logging (/pgp log)   : ON");
    } else if (strcmp(log_value, "off") == 0) {
        cons_show("PGP logging (/pgp log)   : OFF");
    } else {
        cons_show("PGP logging (/pgp log)   : Redacted");
    }
    prefs_free_string(log_value);

    char ch = prefs_get_pgp_char();
    cons_show("PGP char (/pgp char)     : %c", ch);

    cons_alert();
}

void
cons_show_themes(GSList *themes)
{
    cons_show("");

    if (themes == NULL) {
        cons_show("No available themes.");
    } else {
        cons_show("Available themes:");
        while (themes) {
            cons_show(themes->data);
            themes = g_slist_next(themes);
        }
    }

    cons_alert();
}

void
cons_show_scripts(GSList *scripts)
{
    cons_show("");

    if (scripts == NULL) {
        cons_show("No scripts available.");
    } else {
        cons_show("Scripts:");
        while (scripts) {
            cons_show(scripts->data);
            scripts = g_slist_next(scripts);
        }
    }

    cons_alert();
}

void
cons_show_script(const char *const script, GSList *commands)
{
    cons_show("");

    if (commands == NULL) {
        cons_show("Script not found: %s", script);
    } else {
        cons_show("%s:", script);
        while (commands) {
            cons_show("  %s", commands->data);
            commands = g_slist_next(commands);
        }
    }

    cons_alert();
}

void
cons_prefs(void)
{
    cons_show("");
    cons_show_ui_prefs();
    cons_show("");
    cons_show_desktop_prefs();
    cons_show("");
    cons_show_chat_prefs();
    cons_show("");
    cons_show_log_prefs();
    cons_show("");
    cons_show_presence_prefs();
    cons_show("");
    cons_show_connection_prefs();
    cons_show("");
    cons_show_otr_prefs();
    cons_show("");
    cons_show_pgp_prefs();
    cons_show("");

    cons_alert();
}

void
cons_help(void)
{
    int pad = strlen("/help commands connection") + 3;

    cons_show("");
    cons_show("Choose a help option:");
    cons_show("");
    cons_show_padded(pad, "/help commands            : List all commands.");
    cons_show_padded(pad, "/help commands chat       : List chat commands.");
    cons_show_padded(pad, "/help commands groupchat  : List groupchat commands.");
    cons_show_padded(pad, "/help commands roster     : List commands for manipulating your roster.");
    cons_show_padded(pad, "/help commands presence   : List commands to change your presence.");
    cons_show_padded(pad, "/help commands discovery  : List service discovery commands.");
    cons_show_padded(pad, "/help commands connection : List commands related to managing your connection.");
    cons_show_padded(pad, "/help commands ui         : List commands for manipulating the user interface.");
    cons_show_padded(pad, "/help commands plugins    : List plugin commands.");
    cons_show_padded(pad, "/help [command]           : Detailed help on a specific command.");
    cons_show_padded(pad, "/help navigation          : How to navigate around Profanity.");
    cons_show("");

    cons_alert();
}

void
cons_navigation_help(void)
{
    ProfWin *console = wins_get_console();
    cons_show("");
    win_println(console, THEME_HELP_HEADER, '-', "Navigation");
    cons_show("Alt-1..Alt-0, F1..F10    : Choose window.");
    cons_show("Alt-LEFT, Alt-RIGHT      : Previous/next chat window.");
    cons_show("PAGEUP, PAGEDOWN         : Page the main window.");
    cons_show("Alt-PAGEUP, Alt-PAGEDOWN : Page occupants/roster panel.");
    cons_show("");
    cons_show("/win <n>     : Focus window n, where n is the window number.");
    cons_show("/win <name>  : Focus window with name, where name is the recipient, room or window title.");
    cons_show("");
    cons_show("See '/help win' for more information.");

    cons_alert();
}

void
cons_show_roster_group(const char *const group, GSList *list)
{
    cons_show("");

    if (list) {
        cons_show("%s:", group);
    } else {
        cons_show("No group named %s exists.", group);
    }

    _show_roster_contacts(list, FALSE);

    cons_alert();
}

void
cons_show_roster(GSList *list)
{
    cons_show("");
    cons_show("Roster: jid (nick) - subscription - groups");

    _show_roster_contacts(list, TRUE);

    cons_alert();
}

void
cons_show_contact_online(PContact contact, Resource *resource, GDateTime *last_activity)
{
    const char *show = string_from_resource_presence(resource->presence);
    char *display_str = p_contact_create_display_string(contact, resource->name);

    ProfWin *console = wins_get_console();
    win_show_status_string(console, display_str, show, resource->status, last_activity,
        "++", "online");

    free(display_str);
}

void
cons_show_contact_offline(PContact contact, char *resource, char *status)
{
    char *display_str = p_contact_create_display_string(contact, resource);

    ProfWin *console = wins_get_console();
    win_show_status_string(console, display_str, "offline", status, NULL, "--",
        "offline");
    free(display_str);
}

void
cons_show_contacts(GSList *list)
{
    ProfWin *console = wins_get_console();
    GSList *curr = list;

    while(curr) {
        PContact contact = curr->data;
        if ((strcmp(p_contact_subscription(contact), "to") == 0) ||
            (strcmp(p_contact_subscription(contact), "both") == 0)) {
            win_show_contact(console, contact);
        }
        curr = g_slist_next(curr);
    }
    cons_alert();
}

void
cons_alert(void)
{
    ProfWin *current = wins_get_current();
    if (current->type != WIN_CONSOLE) {
        status_bar_new(1);
    }
}

char*
cons_get_string(ProfConsoleWin *conswin)
{
    assert(conswin != NULL);

    return strdup("Console");
}

void
_cons_theme_bar_prop(theme_item_t theme, char *prop)
{
    ProfWin *console = wins_get_console();

    GString *propstr = g_string_new(" ");
    g_string_append_printf(propstr, "%-24s", prop);
    win_print(console, THEME_TEXT, '-', "%s", propstr->str);
    g_string_free(propstr, TRUE);

    GString *valstr = g_string_new(" ");
    char *setting = theme_get_string(prop);
    g_string_append_printf(valstr, "%s ", setting);
    theme_free_string(setting);
    win_append(console, theme, "%s", valstr->str);
    win_appendln(console, THEME_TEXT, "");
    g_string_free(valstr, TRUE);
}

void
_cons_theme_prop(theme_item_t theme, char *prop)
{
    ProfWin *console = wins_get_console();

    GString *propstr = g_string_new(" ");
    g_string_append_printf(propstr, "%-24s", prop);
    win_print(console, THEME_TEXT, '-', "%s", propstr->str);
    g_string_free(propstr, TRUE);

    GString *valstr = g_string_new("");
    char *setting = theme_get_string(prop);
    g_string_append_printf(valstr, "%s", setting);
    theme_free_string(setting);
    win_appendln(console, theme, "%s", valstr->str);
    g_string_free(valstr, TRUE);
}

void
cons_theme_properties(void)
{
    cons_show("Current colours:");
    _cons_theme_bar_prop(THEME_TITLE_TEXT, "titlebar.text");
    _cons_theme_bar_prop(THEME_TITLE_BRACKET, "titlebar.brackets");

    _cons_theme_bar_prop(THEME_TITLE_UNENCRYPTED, "titlebar.unencrypted");
    _cons_theme_bar_prop(THEME_TITLE_ENCRYPTED, "titlebar.encrypted");
    _cons_theme_bar_prop(THEME_TITLE_UNTRUSTED, "titlebar.untrusted");
    _cons_theme_bar_prop(THEME_TITLE_TRUSTED, "titlebar.trusted");

    _cons_theme_bar_prop(THEME_TITLE_CHAT, "titlebar.chat");
    _cons_theme_bar_prop(THEME_TITLE_ONLINE, "titlebar.online");
    _cons_theme_bar_prop(THEME_TITLE_AWAY, "titlebar.away");
    _cons_theme_bar_prop(THEME_TITLE_XA, "titlebar.xa");
    _cons_theme_bar_prop(THEME_TITLE_DND, "titlebar.dnd");
    _cons_theme_bar_prop(THEME_TITLE_OFFLINE, "titlebar.offline");

    _cons_theme_bar_prop(THEME_STATUS_TEXT, "statusbar.text");
    _cons_theme_bar_prop(THEME_STATUS_BRACKET, "statusbar.brackets");
    _cons_theme_bar_prop(THEME_STATUS_ACTIVE, "statusbar.active");
    _cons_theme_bar_prop(THEME_STATUS_NEW, "statusbar.new");
    _cons_theme_bar_prop(THEME_STATUS_TIME, "statusbar.time");

    _cons_theme_prop(THEME_TIME, "main.time");
    _cons_theme_prop(THEME_TEXT, "main.text");
    _cons_theme_prop(THEME_SPLASH, "main.splash");
    _cons_theme_prop(THEME_ERROR, "error");
    _cons_theme_prop(THEME_OTR_STARTED_TRUSTED, "otr.started.trusted");
    _cons_theme_prop(THEME_OTR_STARTED_UNTRUSTED, "otr.started.untrusted");
    _cons_theme_prop(THEME_OTR_ENDED, "otr.ended");
    _cons_theme_prop(THEME_OTR_TRUSTED, "otr.trusted");
    _cons_theme_prop(THEME_OTR_UNTRUSTED, "otr.untrusted");

    _cons_theme_prop(THEME_ME, "me");
    _cons_theme_prop(THEME_TEXT_ME, "main.text.me");
    _cons_theme_prop(THEME_THEM, "them");
    _cons_theme_prop(THEME_TEXT_THEM, "main.text.them");

    _cons_theme_prop(THEME_CHAT, "chat");
    _cons_theme_prop(THEME_ONLINE, "online");
    _cons_theme_prop(THEME_AWAY, "away");
    _cons_theme_prop(THEME_XA, "xa");
    _cons_theme_prop(THEME_DND, "dnd");
    _cons_theme_prop(THEME_OFFLINE, "offline");
    _cons_theme_prop(THEME_SUBSCRIBED, "subscribed");
    _cons_theme_prop(THEME_UNSUBSCRIBED, "unsubscribed");

    _cons_theme_prop(THEME_INCOMING, "incoming");
    _cons_theme_prop(THEME_MENTION, "mention");
    _cons_theme_prop(THEME_TRIGGER, "trigger");
    _cons_theme_prop(THEME_TYPING, "typing");
    _cons_theme_prop(THEME_GONE, "gone");

    _cons_theme_prop(THEME_ROOMINFO, "roominfo");
    _cons_theme_prop(THEME_ROOMMENTION, "roommention");
    _cons_theme_prop(THEME_ROOMMENTION_TERM, "roommention.term");
    _cons_theme_prop(THEME_ROOMTRIGGER, "roomtrigger");
    _cons_theme_prop(THEME_ROOMTRIGGER_TERM, "roomtrigger.term");

    _cons_theme_prop(THEME_ROSTER_HEADER, "roster.header");
    _cons_theme_prop(THEME_ROSTER_CHAT, "roster.chat");
    _cons_theme_prop(THEME_ROSTER_ONLINE, "roster.online");
    _cons_theme_prop(THEME_ROSTER_AWAY, "roster.away");
    _cons_theme_prop(THEME_ROSTER_XA, "roster.xa");
    _cons_theme_prop(THEME_ROSTER_DND, "roster.dnd");
    _cons_theme_prop(THEME_ROSTER_OFFLINE, "roster.offline");
    _cons_theme_prop(THEME_ROSTER_CHAT_ACTIVE, "roster.chat.active");
    _cons_theme_prop(THEME_ROSTER_ONLINE_ACTIVE, "roster.online.active");
    _cons_theme_prop(THEME_ROSTER_AWAY_ACTIVE, "roster.away.active");
    _cons_theme_prop(THEME_ROSTER_XA_ACTIVE, "roster.xa.active");
    _cons_theme_prop(THEME_ROSTER_DND_ACTIVE, "roster.dnd.active");
    _cons_theme_prop(THEME_ROSTER_OFFLINE_ACTIVE, "roster.offline.active");
    _cons_theme_prop(THEME_ROSTER_CHAT_UNREAD, "roster.chat.unread");
    _cons_theme_prop(THEME_ROSTER_ONLINE_UNREAD, "roster.online.unread");
    _cons_theme_prop(THEME_ROSTER_AWAY_UNREAD, "roster.away.unread");
    _cons_theme_prop(THEME_ROSTER_XA_UNREAD, "roster.xa.unread");
    _cons_theme_prop(THEME_ROSTER_DND_UNREAD, "roster.dnd.unread");
    _cons_theme_prop(THEME_ROSTER_OFFLINE_UNREAD, "roster.offline.unread");
    _cons_theme_prop(THEME_ROSTER_ROOM, "roster.room");
    _cons_theme_prop(THEME_ROSTER_ROOM_UNREAD, "roster.room.unread");
    _cons_theme_prop(THEME_ROSTER_ROOM_TRIGGER, "roster.room.trigger");
    _cons_theme_prop(THEME_ROSTER_ROOM_MENTION, "roster.room.mention");

    _cons_theme_prop(THEME_OCCUPANTS_HEADER, "occupants.header");

    _cons_theme_prop(THEME_RECEIPT_SENT, "receipt.sent");

    _cons_theme_prop(THEME_INPUT_TEXT, "input.text");

    cons_show("");
}

void
cons_theme_colours(void)
{
    /*
     *     { "default", -1 },
    { "white", COLOR_WHITE },
    { "green", COLOR_GREEN },
    { "red", COLOR_RED },
    { "yellow", COLOR_YELLOW },
    { "blue", COLOR_BLUE },
    { "cyan", COLOR_CYAN },
    { "black", COLOR_BLACK },
    { "magenta", COLOR_MAGENTA },

     */

    ProfWin *console = wins_get_console();
    cons_show("Available colours:");

    win_print(console,      THEME_WHITE, '-',   " white   ");
    win_appendln(console,   THEME_WHITE_BOLD,   " bold_white");

    win_print(console,      THEME_GREEN, '-',   " green   ");
    win_appendln(console,   THEME_GREEN_BOLD,   " bold_green");

    win_print(console,      THEME_RED, '-',     " red     ");
    win_appendln(console,   THEME_RED_BOLD,     " bold_red");

    win_print(console,      THEME_YELLOW, '-',  " yellow  ");
    win_appendln(console,   THEME_YELLOW_BOLD,  " bold_yellow");

    win_print(console,      THEME_BLUE, '-',    " blue    ");
    win_appendln(console,   THEME_BLUE_BOLD,    " bold_blue");

    win_print(console,      THEME_CYAN, '-',    " cyan    ");
    win_appendln(console,   THEME_CYAN_BOLD,    " bold_cyan");

    win_print(console,      THEME_MAGENTA, '-', " magenta ");
    win_appendln(console,   THEME_MAGENTA_BOLD, " bold_magenta");

    win_print(console,      THEME_BLACK, '-',   " black   ");
    win_appendln(console,   THEME_BLACK_BOLD,   " bold_black");

    cons_show("");
}

static void
_cons_splash_logo(void)
{
    ProfWin *console = wins_get_console();
    win_println(console, THEME_DEFAULT, '-', "Welcome to");

    win_println(console, THEME_SPLASH, '-', "                   ___            _           ");
    win_println(console, THEME_SPLASH, '-', "                  / __)          (_)_         ");
    win_println(console, THEME_SPLASH, '-', " ____   ____ ___ | |__ ____ ____  _| |_ _   _ ");
    win_println(console, THEME_SPLASH, '-', "|  _ \\ / ___) _ \\|  __) _  |  _ \\| |  _) | | |");
    win_println(console, THEME_SPLASH, '-', "| | | | |  | |_| | | ( ( | | | | | | |_| |_| |");
    win_println(console, THEME_SPLASH, '-', "| ||_/|_|   \\___/|_|  \\_||_|_| |_|_|\\___)__  |");
    win_println(console, THEME_SPLASH, '-', "|_|                                    (____/ ");
    win_println(console, THEME_SPLASH, '-', "");

    if (strcmp(PACKAGE_STATUS, "development") == 0) {
#ifdef HAVE_GIT_VERSION
        win_println(console, THEME_DEFAULT, '-', "Version %sdev.%s.%s", PACKAGE_VERSION, PROF_GIT_BRANCH, PROF_GIT_REVISION);
#else
        win_println(console, THEME_DEFAULT, "Version %sdev", PACKAGE_VERSION);
#endif
    } else {
        win_println(console, THEME_DEFAULT, '-', "Version %s", PACKAGE_VERSION);
    }
}

void
_show_roster_contacts(GSList *list, gboolean show_groups)
{
    ProfWin *console = wins_get_console();
    GSList *curr = list;
    while(curr) {

        PContact contact = curr->data;
        GString *title = g_string_new("  ");
        title = g_string_append(title, p_contact_barejid(contact));
        if (p_contact_name(contact)) {
            title = g_string_append(title, " (");
            title = g_string_append(title, p_contact_name(contact));
            title = g_string_append(title, ")");
        }

        const char *presence = p_contact_presence(contact);
        theme_item_t presence_colour = THEME_TEXT;
        if (p_contact_subscribed(contact)) {
            presence_colour = theme_main_presence_attrs(presence);
        } else {
            presence_colour = theme_main_presence_attrs("offline");
        }
        win_print(console, presence_colour, '-', "%s", title->str);

        g_string_free(title, TRUE);

        win_append(console, THEME_DEFAULT, " - ");
        GString *sub = g_string_new("");
        sub = g_string_append(sub, p_contact_subscription(contact));
        if (p_contact_pending_out(contact)) {
            sub = g_string_append(sub, ", request sent");
        }
        if (presence_sub_request_exists(p_contact_barejid(contact))) {
            sub = g_string_append(sub, ", request received");
        }
        if (p_contact_subscribed(contact)) {
            presence_colour = THEME_SUBSCRIBED;
        } else {
            presence_colour = THEME_UNSUBSCRIBED;
        }

        if (show_groups) {
            win_append(console, presence_colour, "%s", sub->str);
        } else {
            win_appendln(console, presence_colour, "%s", sub->str);
        }

        g_string_free(sub, TRUE);

        if (show_groups) {
            GSList *groups = p_contact_groups(contact);
            if (groups) {
                GString *groups_str = g_string_new(" - ");
                while (groups) {
                    g_string_append(groups_str, groups->data);
                    if (g_slist_next(groups)) {
                        g_string_append(groups_str, ", ");
                    }
                    groups = g_slist_next(groups);
                }
                win_appendln(console, THEME_DEFAULT, "%s", groups_str->str);
                g_string_free(groups_str, TRUE);
            } else {
                win_appendln(console, THEME_DEFAULT, " ");
            }
        }

        curr = g_slist_next(curr);
    }
}
