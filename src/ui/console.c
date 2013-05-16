/*
 * console.c
 *
 * Copyright (C) 2012, 2013 James Booth <boothj5@gmail.com>
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
 */

#include <string.h>
#include <stdlib.h>

#ifdef HAVE_NCURSESW_NCURSES_H
#include <ncursesw/ncurses.h>
#elif HAVE_NCURSES_H
#include <ncurses.h>
#endif

#include "command/command.h"
#include "common.h"
#include "config/preferences.h"
#include "config/theme.h"
#include "ui/notifier.h"
#include "ui/window.h"
#include "ui/ui.h"
#include "xmpp/xmpp.h"

#define CONS_WIN_TITLE "_cons"

static ProfWin* console;

static void _cons_splash_logo(void);
static void _cons_show_basic_help(void);
static void _cons_alert(void);

ProfWin *
cons_create(void)
{
    int cols = getmaxx(stdscr);
    console = win_create(CONS_WIN_TITLE, cols, WIN_CONSOLE);
    return console;
}

void
cons_show_time(void)
{
    win_print_time(console, '-');
    ui_console_dirty();
}

void
cons_show_word(const char * const word)
{
    wprintw(console->win, "%s", word);
    ui_console_dirty();
}

void
cons_debug(const char * const msg, ...)
{
    if (strcmp(PACKAGE_STATUS, "development") == 0) {
        va_list arg;
        va_start(arg, msg);
        GString *fmt_msg = g_string_new(NULL);
        g_string_vprintf(fmt_msg, msg, arg);
        win_print_time(console, '-');
        wprintw(console->win, "%s\n", fmt_msg->str);
        g_string_free(fmt_msg, TRUE);
        va_end(arg);

        ui_console_dirty();
        _cons_alert();

        ui_current_page_off();
        ui_refresh();
    }
}

void
cons_show(const char * const msg, ...)
{
    va_list arg;
    va_start(arg, msg);
    GString *fmt_msg = g_string_new(NULL);
    g_string_vprintf(fmt_msg, msg, arg);
    win_print_time(console, '-');
    wprintw(console->win, "%s\n", fmt_msg->str);
    g_string_free(fmt_msg, TRUE);
    va_end(arg);
    ui_console_dirty();
}

void
cons_show_error(const char * const msg, ...)
{
    va_list arg;
    va_start(arg, msg);
    GString *fmt_msg = g_string_new(NULL);
    g_string_vprintf(fmt_msg, msg, arg);
    win_print_time(console, '-');
    wattron(console->win, COLOUR_ERROR);
    wprintw(console->win, "%s\n", fmt_msg->str);
    wattroff(console->win, COLOUR_ERROR);
    g_string_free(fmt_msg, TRUE);
    va_end(arg);

    ui_console_dirty();
    _cons_alert();
}

void
cons_show_typing(const char * const short_from)
{
    win_print_time(console, '-');
    wattron(console->win, COLOUR_TYPING);
    wprintw(console->win, "!! %s is typing a message...\n", short_from);
    wattroff(console->win, COLOUR_TYPING);

    ui_console_dirty();
    _cons_alert();
}

void
cons_show_incoming_message(const char * const short_from, const int win_index)
{
    int ui_index = win_index + 1;
    if (ui_index == 10) {
        ui_index = 0;
    }
    win_print_time(console, '-');
    wattron(console->win, COLOUR_INCOMING);
    wprintw(console->win, "<< incoming from %s (%d)\n", short_from, ui_index);
    wattroff(console->win, COLOUR_INCOMING);

    ui_console_dirty();
    _cons_alert();
}

void
cons_about(void)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    if (prefs_get_boolean(PREF_SPLASH)) {
        _cons_splash_logo();
    } else {
        win_print_time(console, '-');

        if (strcmp(PACKAGE_STATUS, "development") == 0) {
            wprintw(console->win, "Welcome to Profanity, version %sdev\n", PACKAGE_VERSION);
        } else {
            wprintw(console->win, "Welcome to Profanity, version %s\n", PACKAGE_VERSION);
        }
    }

    win_print_time(console, '-');
    wprintw(console->win, "Copyright (C) 2012, 2013 James Booth <%s>.\n", PACKAGE_BUGREPORT);
    win_print_time(console, '-');
    wprintw(console->win, "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n");
    win_print_time(console, '-');
    wprintw(console->win, "\n");
    win_print_time(console, '-');
    wprintw(console->win, "This is free software; you are free to change and redistribute it.\n");
    win_print_time(console, '-');
    wprintw(console->win, "There is NO WARRANTY, to the extent permitted by law.\n");
    win_print_time(console, '-');
    wprintw(console->win, "\n");
    win_print_time(console, '-');
    wprintw(console->win, "Type '/help' to show complete help.\n");
    win_print_time(console, '-');
    wprintw(console->win, "\n");

    if (prefs_get_boolean(PREF_VERCHECK)) {
        cons_check_version(FALSE);
    }

    prefresh(console->win, 0, 0, 1, 0, rows-3, cols-1);

    ui_console_dirty();
    _cons_alert();
}

void
cons_check_version(gboolean not_available_msg)
{
    char *latest_release = release_get_latest();

    if (latest_release != NULL) {
        gboolean relase_valid = g_regex_match_simple("^\\d+\\.\\d+\\.\\d+$", latest_release, 0, 0);

        if (relase_valid) {
            if (release_is_new(latest_release)) {
                win_print_time(console, '-');
                wprintw(console->win, "A new version of Profanity is available: %s", latest_release);
                win_print_time(console, '-');
                wprintw(console->win, "Check <http://www.profanity.im> for details.\n");
                free(latest_release);
                win_print_time(console, '-');
                wprintw(console->win, "\n");
            } else {
                if (not_available_msg) {
                    cons_show("No new version available.");
                    cons_show("");
                }
            }

            ui_console_dirty();
            _cons_alert();
        }
    }
}

void
cons_show_login_success(ProfAccount *account)
{
    win_print_time(console, '-');
    wprintw(console->win, "%s logged in successfully, ", account->jid);

    resource_presence_t presence = accounts_get_login_presence(account->name);
    const char *presence_str = string_from_resource_presence(presence);

    win_presence_colour_on(console, presence_str);
    wprintw(console->win, "%s", presence_str);
    win_presence_colour_off(console, presence_str);
    wprintw(console->win, " (priority %d)",
        accounts_get_priority_for_presence_type(account->name, presence));
    wprintw(console->win, ".\n");
    ui_console_dirty();
    _cons_alert();
}

void
cons_show_wins(void)
{
    int i = 0;
    int count = 0;
    int ui_index = 0;

    cons_show("");
    cons_show("Active windows:");
    win_print_time(console, '-');
    wprintw(console->win, "1: Console\n");

    for (i = 1; i < NUM_WINS; i++) {
        if (windows[i] != NULL) {
            count++;
        }
    }

    if (count != 0) {
        for (i = 1; i < NUM_WINS; i++) {
            if (windows[i] != NULL) {
                ProfWin *window = windows[i];
                win_print_time(console, '-');
                ui_index = i + 1;
                if (ui_index == 10) {
                    ui_index = 0;
                }

                switch (window->type)
                {
                    case WIN_CHAT:
                        wprintw(console->win, "%d: Chat %s", ui_index, window->from);
                        PContact contact = roster_get_contact(window->from);

                        if (contact != NULL) {
                            if (p_contact_name(contact) != NULL) {
                                wprintw(console->win, " (%s)", p_contact_name(contact));
                            }
                            wprintw(console->win, " - %s", p_contact_presence(contact));
                        }

                        if (window->unread > 0) {
                            wprintw(console->win, ", %d unread", window->unread);
                        }

                        break;

                    case WIN_PRIVATE:
                        wprintw(console->win, "%d: Private %s", ui_index, window->from);

                        if (window->unread > 0) {
                            wprintw(console->win, ", %d unread", window->unread);
                        }

                        break;

                    case WIN_MUC:
                        wprintw(console->win, "%d: Room %s", ui_index, window->from);

                        if (window->unread > 0) {
                            wprintw(console->win, ", %d unread", window->unread);
                        }

                        break;

                    case WIN_DUCK:
                        wprintw(console->win, "%d: DuckDuckGo search", ui_index);

                        break;

                    default:
                        break;
                }

                wprintw(console->win, "\n");
            }
        }
    }

    cons_show("");
    ui_console_dirty();
    _cons_alert();
}

void
cons_show_room_invites(GSList *invites)
{
    cons_show("");
    if (invites == NULL) {
        cons_show("No outstanding chat room invites.");
    } else {
        cons_show("Chat room invites, use /join or /decline commands:");
        while (invites != NULL) {
            cons_show("  %s", invites->data);
            invites = g_slist_next(invites);
        }
    }

    ui_console_dirty();
    _cons_alert();
}

void
cons_show_info(PContact pcontact)
{
    const char *barejid = p_contact_barejid(pcontact);
    const char *name = p_contact_name(pcontact);
    const char *presence = p_contact_presence(pcontact);
    const char *sub = p_contact_subscription(pcontact);
    GList *resources = p_contact_get_available_resources(pcontact);
    GList *ordered_resources = NULL;
    GDateTime *last_activity = p_contact_last_activity(pcontact);
    WINDOW *win = console->win;

    win_print_time(console, '-');
    wprintw(win, "\n");
    win_print_time(console, '-');
    win_presence_colour_on(console, presence);
    wprintw(win, "%s", barejid);
    if (name != NULL) {
        wprintw(win, " (%s)", name);
    }
    win_presence_colour_off(console, presence);
    wprintw(win, ":\n");

    if (sub != NULL) {
        win_print_time(console, '-');
        wprintw(win, "Subscription: %s\n", sub);
    }

    if (last_activity != NULL) {
        GDateTime *now = g_date_time_new_now_local();
        GTimeSpan span = g_date_time_difference(now, last_activity);

        win_print_time(console, '-');
        wprintw(win, "Last activity: ");

        int hours = span / G_TIME_SPAN_HOUR;
        span = span - hours * G_TIME_SPAN_HOUR;
        if (hours > 0) {
            wprintw(win, "%dh", hours);
        }

        int minutes = span / G_TIME_SPAN_MINUTE;
        span = span - minutes * G_TIME_SPAN_MINUTE;
        wprintw(win, "%dm", minutes);

        int seconds = span / G_TIME_SPAN_SECOND;
        wprintw(win, "%ds", seconds);

        wprintw(win, "\n");

        g_date_time_unref(now);
    }

    if (resources != NULL) {
        win_print_time(console, '-');
        wprintw(win, "Resources:\n");

        // sort in order of availabiltiy
        while (resources != NULL) {
            Resource *resource = resources->data;
            ordered_resources = g_list_insert_sorted(ordered_resources,
                resource, (GCompareFunc)resource_compare_availability);
            resources = g_list_next(resources);
        }
    }

    while (ordered_resources != NULL) {
        Resource *resource = ordered_resources->data;
        const char *resource_presence = string_from_resource_presence(resource->presence);
        win_print_time(console, '-');
        win_presence_colour_on(console, resource_presence);
        wprintw(win, "  %s (%d), %s", resource->name, resource->priority, resource_presence);
        if (resource->status != NULL) {
            wprintw(win, ", \"%s\"", resource->status);
        }
        wprintw(win, "\n");
        win_presence_colour_off(console, resource_presence);

        if (resource->caps_str != NULL) {
            Capabilities *caps = caps_get(resource->caps_str);
            if (caps != NULL) {
                // show identity
                if ((caps->category != NULL) || (caps->type != NULL) || (caps->name != NULL)) {
                    win_print_time(console, '-');
                    wprintw(win, "    Identity: ");
                    if (caps->name != NULL) {
                        wprintw(win, "%s", caps->name);
                        if ((caps->category != NULL) || (caps->type != NULL)) {
                            wprintw(win, " ");
                        }
                    }
                    if (caps->type != NULL) {
                        wprintw(win, "%s", caps->type);
                        if (caps->category != NULL) {
                            wprintw(win, " ");
                        }
                    }
                    if (caps->category != NULL) {
                        wprintw(win, "%s", caps->category);
                    }
                    wprintw(win, "\n");
                }
                if (caps->software != NULL) {
                    win_print_time(console, '-');
                    wprintw(win, "    Software: %s", caps->software);
                }
                if (caps->software_version != NULL) {
                    wprintw(win, ", %s", caps->software_version);
                }
                if ((caps->software != NULL) || (caps->software_version != NULL)) {
                    wprintw(win, "\n");
                }
                if (caps->os != NULL) {
                    win_print_time(console, '-');
                    wprintw(win, "    OS: %s", caps->os);
                }
                if (caps->os_version != NULL) {
                    wprintw(win, ", %s", caps->os_version);
                }
                if ((caps->os != NULL) || (caps->os_version != NULL)) {
                    wprintw(win, "\n");
                }
            }
        }

        ordered_resources = g_list_next(ordered_resources);
    }

    ui_console_dirty();
    _cons_alert();
}

void
cons_show_caps(const char * const contact, Resource *resource)
{
    WINDOW *win = console->win;
    cons_show("");
    const char *resource_presence = string_from_resource_presence(resource->presence);
    win_print_time(console, '-');
    win_presence_colour_on(console, resource_presence);
    wprintw(console->win, "%s", contact);
    win_presence_colour_off(console, resource_presence);
    wprintw(win, ":\n");

    if (resource->caps_str != NULL) {
        Capabilities *caps = caps_get(resource->caps_str);
        if (caps != NULL) {
            // show identity
            if ((caps->category != NULL) || (caps->type != NULL) || (caps->name != NULL)) {
                win_print_time(console, '-');
                wprintw(win, "Identity: ");
                if (caps->name != NULL) {
                    wprintw(win, "%s", caps->name);
                    if ((caps->category != NULL) || (caps->type != NULL)) {
                        wprintw(win, " ");
                    }
                }
                if (caps->type != NULL) {
                    wprintw(win, "%s", caps->type);
                    if (caps->category != NULL) {
                        wprintw(win, " ");
                    }
                }
                if (caps->category != NULL) {
                    wprintw(win, "%s", caps->category);
                }
                wprintw(win, "\n");
            }
            if (caps->software != NULL) {
                win_print_time(console, '-');
                wprintw(win, "Software: %s", caps->software);
            }
            if (caps->software_version != NULL) {
                wprintw(win, ", %s", caps->software_version);
            }
            if ((caps->software != NULL) || (caps->software_version != NULL)) {
                wprintw(win, "\n");
            }
            if (caps->os != NULL) {
                win_print_time(console, '-');
                wprintw(win, "OS: %s", caps->os);
            }
            if (caps->os_version != NULL) {
                wprintw(win, ", %s", caps->os_version);
            }
            if ((caps->os != NULL) || (caps->os_version != NULL)) {
                wprintw(win, "\n");
            }

            if (caps->features != NULL) {
                win_print_time(console, '-');
                wprintw(win, "Features:\n");
                GSList *feature = caps->features;
                while (feature != NULL) {
                    win_print_time(console, '-');
                    wprintw(win, "  %s\n", feature->data);
                    feature = g_slist_next(feature);
                }
            }
        }
    }

    ui_console_dirty();
    _cons_alert();
}

void
cons_show_software_version(const char * const jid, const char * const  presence,
    const char * const name, const char * const version, const char * const os)
{
    if ((name != NULL) || (version != NULL) || (os != NULL)) {
        cons_show("");
        win_print_time(console, '-');
        win_presence_colour_on(console, presence);
        wprintw(console->win, "%s", jid);
        win_presence_colour_off(console, presence);
        wprintw(console->win, ":\n");
    }
    if (name != NULL) {
        cons_show("Name    : %s", name);
    }
    if (version != NULL) {
        cons_show("Version : %s", version);
    }
    if (os != NULL) {
        cons_show("OS      : %s", os);
    }

    ui_console_dirty();
    _cons_alert();
}

void
cons_show_received_subs(void)
{
    GSList *received = presence_get_subscription_requests();
    if (received == NULL) {
        cons_show("No outstanding subscription requests.");
    } else {
        cons_show("Outstanding subscription requests from:",
            g_slist_length(received));
        while (received != NULL) {
            cons_show("  %s", received->data);
            received = g_slist_next(received);
        }
        g_slist_free_full(received, g_free);
    }
}

void
cons_show_sent_subs(void)
{
   if (roster_has_pending_subscriptions()) {
        GSList *contacts = roster_get_contacts();
        PContact contact = NULL;
        cons_show("Awaiting subscription responses from:");
        while (contacts != NULL) {
            contact = (PContact) contacts->data;
            if (p_contact_pending_out(contact)) {
                cons_show("  %s", p_contact_barejid(contact));
            }
            contacts = g_slist_next(contacts);
        }
    } else {
        cons_show("No pending requests sent.");
    }
}

void
cons_show_room_list(GSList *rooms, const char * const conference_node)
{
    if ((rooms != NULL) && (g_slist_length(rooms) > 0)) {
        cons_show("Chat rooms at %s:", conference_node);
        while (rooms != NULL) {
            DiscoItem *room = rooms->data;
            win_print_time(console, '-');
            wprintw(console->win, "  %s", room->jid);
            if (room->name != NULL) {
                wprintw(console->win, ", (%s)", room->name);
            }
            wprintw(console->win, "\n");
            rooms = g_slist_next(rooms);
        }
    } else {
        cons_show("No chat rooms at %s", conference_node);
    }

    ui_console_dirty();
    _cons_alert();
}

void
cons_show_disco_info(const char *jid, GSList *identities, GSList *features)
{
    if (((identities != NULL) && (g_slist_length(identities) > 0)) ||
        ((features != NULL) && (g_slist_length(features) > 0))) {
        cons_show("");
        cons_show("Service disovery info for %s", jid);

        if (identities != NULL) {
            cons_show("  Identities");
        }
        while (identities != NULL) {
            DiscoIdentity *identity = identities->data;  // anme trpe, cat
            GString *identity_str = g_string_new("    ");
            if (identity->name != NULL) {
                identity_str = g_string_append(identity_str, strdup(identity->name));
                identity_str = g_string_append(identity_str, " ");
            }
            if (identity->type != NULL) {
                identity_str = g_string_append(identity_str, strdup(identity->type));
                identity_str = g_string_append(identity_str, " ");
            }
            if (identity->category != NULL) {
                identity_str = g_string_append(identity_str, strdup(identity->category));
            }
            cons_show(identity_str->str);
            g_string_free(identity_str, FALSE);
            identities = g_slist_next(identities);
        }

        if (features != NULL) {
            cons_show("  Features:");
        }
        while (features != NULL) {
            cons_show("    %s", features->data);
            features = g_slist_next(features);
        }

        ui_console_dirty();
        _cons_alert();
    }
}

void
cons_show_disco_items(GSList *items, const char * const jid)
{
    if ((items != NULL) && (g_slist_length(items) > 0)) {
        cons_show("");
        cons_show("Service discovery items for %s:", jid);
        while (items != NULL) {
            DiscoItem *item = items->data;
            win_print_time(console, '-');
            wprintw(console->win, "  %s", item->jid);
            if (item->name != NULL) {
                wprintw(console->win, ", (%s)", item->name);
            }
            wprintw(console->win, "\n");
            items = g_slist_next(items);
        }
    } else {
        cons_show("");
        cons_show("No service discovery items for %s", jid);
    }
    ui_console_dirty();
    _cons_alert();
}

void
cons_show_status(const char * const contact)
{
    PContact pcontact = roster_get_contact(contact);

    if (pcontact != NULL) {
        win_show_contact(console, pcontact);
    } else {
        cons_show("No such contact \"%s\" in roster.", contact);
    }
    ui_console_dirty();
    _cons_alert();
}

void
cons_show_room_invite(const char * const invitor, const char * const room,
    const char * const reason)
{
    cons_show("");
    cons_show("Chat room invite received:");
    cons_show("  From   : %s", invitor);
    cons_show("  Room   : %s", room);

    if (reason != NULL) {
        cons_show("  Message: %s", reason);
    }

    cons_show("Use /join or /decline");

    if (prefs_get_boolean(PREF_NOTIFY_INVITE)) {
        notify_invite(invitor, room, reason);
    }

    ui_console_dirty();
    _cons_alert();
}

void
cons_show_account_list(gchar **accounts)
{
    int size = g_strv_length(accounts);
    if (size > 0) {
        cons_show("Accounts:");
        int i = 0;
        for (i = 0; i < size; i++) {
            if ((jabber_get_connection_status() == JABBER_CONNECTED) &&
                    (g_strcmp0(jabber_get_account_name(), accounts[i]) == 0)) {
                resource_presence_t presence = accounts_get_last_presence(accounts[i]);
                win_print_time(console, '-');
                win_presence_colour_on(console, string_from_resource_presence(presence));
                wprintw(console->win, "%s\n", accounts[i]);
                win_presence_colour_off(console, string_from_resource_presence(presence));
            } else {
                cons_show(accounts[i]);
            }
        }
        cons_show("");
    } else {
        cons_show("No accounts created yet.");
        cons_show("");
    }

    ui_console_dirty();
    _cons_alert();
}

void
cons_show_account(ProfAccount *account)
{
    cons_show("");
    cons_show("Account %s:", account->name);
    if (account->enabled) {
        cons_show   ("enabled        : TRUE");
    } else {
        cons_show   ("enabled        : FALSE");
    }
    cons_show       ("jid            : %s", account->jid);
    if (account->resource != NULL) {
        cons_show   ("resource       : %s", account->resource);
    }
    if (account->server != NULL) {
        cons_show   ("server         : %s", account->server);
    }
    if (account->last_presence != NULL) {
        cons_show   ("Last presence  : %s", account->last_presence);
    }
    if (account->login_presence != NULL) {
        cons_show   ("Login presence : %s", account->login_presence);
    }
    cons_show       ("Priority       : chat:%d, online:%d, away:%d, xa:%d, dnd:%d",
        account->priority_chat, account->priority_online, account->priority_away,
        account->priority_xa, account->priority_dnd);

    if ((jabber_get_connection_status() == JABBER_CONNECTED) &&
            (g_strcmp0(jabber_get_account_name(), account->name) == 0)) {
        GList *resources = jabber_get_available_resources();
        GList *ordered_resources = NULL;

        WINDOW *win = console->win;
        if (resources != NULL) {
            win_print_time(console, '-');
            wprintw(win, "Resources:\n");

            // sort in order of availabiltiy
            while (resources != NULL) {
                Resource *resource = resources->data;
                ordered_resources = g_list_insert_sorted(ordered_resources,
                    resource, (GCompareFunc)resource_compare_availability);
                resources = g_list_next(resources);
            }
        }

        while (ordered_resources != NULL) {
            Resource *resource = ordered_resources->data;
            const char *resource_presence = string_from_resource_presence(resource->presence);
            win_print_time(console, '-');
            win_presence_colour_on(console, resource_presence);
            wprintw(win, "  %s (%d), %s", resource->name, resource->priority, resource_presence);
            if (resource->status != NULL) {
                wprintw(win, ", \"%s\"", resource->status);
            }
            wprintw(win, "\n");
            win_presence_colour_off(console, resource_presence);

            if (resource->caps_str != NULL) {
                Capabilities *caps = caps_get(resource->caps_str);
                if (caps != NULL) {
                    // show identity
                    if ((caps->category != NULL) || (caps->type != NULL) || (caps->name != NULL)) {
                        win_print_time(console, '-');
                        wprintw(win, "    Identity: ");
                        if (caps->name != NULL) {
                            wprintw(win, "%s", caps->name);
                            if ((caps->category != NULL) || (caps->type != NULL)) {
                                wprintw(win, " ");
                            }
                        }
                        if (caps->type != NULL) {
                            wprintw(win, "%s", caps->type);
                            if (caps->category != NULL) {
                                wprintw(win, " ");
                            }
                        }
                        if (caps->category != NULL) {
                            wprintw(win, "%s", caps->category);
                        }
                        wprintw(win, "\n");
                    }
                    if (caps->software != NULL) {
                        win_print_time(console, '-');
                        wprintw(win, "    Software: %s", caps->software);
                    }
                    if (caps->software_version != NULL) {
                        wprintw(win, ", %s", caps->software_version);
                    }
                    if ((caps->software != NULL) || (caps->software_version != NULL)) {
                        wprintw(win, "\n");
                    }
                    if (caps->os != NULL) {
                        win_print_time(console, '-');
                        wprintw(win, "    OS: %s", caps->os);
                    }
                    if (caps->os_version != NULL) {
                        wprintw(win, ", %s", caps->os_version);
                    }
                    if ((caps->os != NULL) || (caps->os_version != NULL)) {
                        wprintw(win, "\n");
                    }
                }
            }

            ordered_resources = g_list_next(ordered_resources);
        }
    }

    ui_console_dirty();
    _cons_alert();
}

void
cons_show_ui_prefs(void)
{
    cons_show("UI preferences:");
    cons_show("");

    gchar *theme = prefs_get_string(PREF_THEME);
    if (theme == NULL) {
        cons_show("Theme (/theme)               : default");
    } else {
        cons_show("Theme (/theme)               : %s", theme);
    }

    if (prefs_get_boolean(PREF_BEEP))
        cons_show("Terminal beep (/beep)        : ON");
    else
        cons_show("Terminal beep (/beep)        : OFF");

    if (prefs_get_boolean(PREF_FLASH))
        cons_show("Terminal flash (/flash)      : ON");
    else
        cons_show("Terminal flash (/flash)      : OFF");

    if (prefs_get_boolean(PREF_INTYPE))
        cons_show("Show typing (/intype)        : ON");
    else
        cons_show("Show typing (/intype)        : OFF");

    if (prefs_get_boolean(PREF_SPLASH))
        cons_show("Splash screen (/splash)      : ON");
    else
        cons_show("Splash screen (/splash)      : OFF");

    if (prefs_get_boolean(PREF_HISTORY))
        cons_show("Chat history (/history)      : ON");
    else
        cons_show("Chat history (/history)      : OFF");

    if (prefs_get_boolean(PREF_VERCHECK))
        cons_show("Version checking (/vercheck) : ON");
    else
        cons_show("Version checking (/vercheck) : OFF");

    if (prefs_get_boolean(PREF_MOUSE))
        cons_show("Mouse handling (/mouse)      : ON");
    else
        cons_show("Mouse handling (/mouse)      : OFF");

    if (prefs_get_boolean(PREF_STATUSES))
        cons_show("Status (/statuses)           : ON");
    else
        cons_show("Status (/statuses)           : OFF");

    ui_console_dirty();
    _cons_alert();
}

void
cons_show_desktop_prefs(void)
{
    cons_show("Desktop notification preferences:");
    cons_show("");

    if (prefs_get_boolean(PREF_NOTIFY_MESSAGE))
        cons_show("Messages (/notify message)       : ON");
    else
        cons_show("Messages (/notify message)       : OFF");

    if (prefs_get_boolean(PREF_NOTIFY_TYPING))
        cons_show("Composing (/notify typing)       : ON");
    else
        cons_show("Composing (/notify typing)       : OFF");

    if (prefs_get_boolean(PREF_NOTIFY_INVITE))
        cons_show("Room invites (/notify invite)    : ON");
    else
        cons_show("Room invites (/notify invite)    : OFF");

    gint remind_period = prefs_get_notify_remind();
    if (remind_period == 0) {
        cons_show("Reminder period (/notify remind) : OFF");
    } else if (remind_period == 1) {
        cons_show("Reminder period (/notify remind) : 1 second");
    } else {
        cons_show("Reminder period (/notify remind) : %d seconds", remind_period);
    }

    ui_console_dirty();
    _cons_alert();
}

void
cons_show_chat_prefs(void)
{
    cons_show("Chat preferences:");
    cons_show("");

    if (prefs_get_boolean(PREF_STATES))
        cons_show("Send chat states (/states) : ON");
    else
        cons_show("Send chat states (/states) : OFF");

    if (prefs_get_boolean(PREF_OUTTYPE))
        cons_show("Send composing (/outtype)  : ON");
    else
        cons_show("Send composing (/outtype)  : OFF");

    gint gone_time = prefs_get_gone();
    if (gone_time == 0) {
        cons_show("Leave conversation (/gone) : OFF");
    } else if (gone_time == 1) {
        cons_show("Leave conversation (/gone) : 1 minute");
    } else {
        cons_show("Leave conversation (/gone) : %d minutes", gone_time);
    }

    ui_console_dirty();
    _cons_alert();
}

void
cons_show_log_prefs(void)
{
    cons_show("Logging preferences:");
    cons_show("");

    cons_show("Max log size (/log maxsize) : %d bytes", prefs_get_max_log_size());

    if (prefs_get_boolean(PREF_CHLOG))
        cons_show("Chat logging (/chlog)       : ON");
    else
        cons_show("Chat logging (/chlog)       : OFF");

    if (prefs_get_boolean(PREF_GRLOG))
        cons_show("Groupchat logging (/grlog)  : ON");
    else
        cons_show("Groupchat logging (/grlog)  : OFF");

    ui_console_dirty();
    _cons_alert();
}

void
cons_show_presence_prefs(void)
{
    cons_show("Presence preferences:");
    cons_show("");

    if (strcmp(prefs_get_string(PREF_AUTOAWAY_MODE), "off") == 0) {
        cons_show("Autoaway (/autoaway mode)            : OFF");
    } else {
        cons_show("Autoaway (/autoaway mode)            : %s", prefs_get_string(PREF_AUTOAWAY_MODE));
    }

    cons_show("Autoaway minutes (/autoaway time)    : %d minutes", prefs_get_autoaway_time());

    if ((prefs_get_string(PREF_AUTOAWAY_MESSAGE) == NULL) ||
            (strcmp(prefs_get_string(PREF_AUTOAWAY_MESSAGE), "") == 0)) {
        cons_show("Autoaway message (/autoaway message) : OFF");
    } else {
        cons_show("Autoaway message (/autoaway message) : \"%s\"", prefs_get_string(PREF_AUTOAWAY_MESSAGE));
    }

    if (prefs_get_boolean(PREF_AUTOAWAY_CHECK)) {
        cons_show("Autoaway check (/autoaway check)     : ON");
    } else {
        cons_show("Autoaway check (/autoaway check)     : OFF");
    }

    ui_console_dirty();
    _cons_alert();
}

void
cons_show_connection_prefs(void)
{
    cons_show("Connection preferences:");
    cons_show("");

    gint reconnect_interval = prefs_get_reconnect();
    if (reconnect_interval == 0) {
        cons_show("Reconnect interval (/reconnect) : OFF");
    } else if (reconnect_interval == 1) {
        cons_show("Reconnect interval (/reconnect) : 1 second");
    } else {
        cons_show("Reconnect interval (/reconnect) : %d seconds", reconnect_interval);
    }

    gint autoping_interval = prefs_get_autoping();
    if (autoping_interval == 0) {
        cons_show("Autoping interval (/autoping)   : OFF");
    } else if (autoping_interval == 1) {
        cons_show("Autoping interval (/autoping)   : 1 second");
    } else {
        cons_show("Autoping interval (/autoping)   : %d seconds", autoping_interval);
    }

    ui_console_dirty();
    _cons_alert();
}

void
cons_show_themes(GSList *themes)
{
    cons_show("");

    if (themes == NULL) {
        cons_show("No available themes.");
    } else {
        cons_show("Available themes:");
        while (themes != NULL) {
            cons_show(themes->data);
            themes = g_slist_next(themes);
        }
    }

    ui_console_dirty();
    _cons_alert();
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

    ui_console_dirty();
    _cons_alert();
}

void
cons_help(void)
{
    cons_show("");
    cons_show("Choose a help option:");
    cons_show("");
    cons_show("/help list       - List all commands.");
    cons_show("/help basic      - Summary of basic usage commands.");
    cons_show("/help presence   - Summary of online status change commands.");
    cons_show("/help settings   - Summary of commands for changing Profanity settings.");
    cons_show("/help navigation - How to navigate around Profanity.");
    cons_show("/help [command]  - Detailed help on a specific command.");
    cons_show("");

    ui_console_dirty();
    _cons_alert();
}

void
cons_basic_help(void)
{
    cons_show("");
    cons_show("Basic Commands:");
    _cons_show_basic_help();

    ui_console_dirty();
    _cons_alert();
}

void
cons_settings_help(void)
{
    cons_show("");
    cons_show("Settings:");
    cons_show("");

    GSList *settings_helpers = cmd_get_settings_help();
    while (settings_helpers != NULL) {
        struct cmd_help_t *help = (struct cmd_help_t *)settings_helpers->data;
        cons_show("%-27s: %s", help->usage, help->short_help);
        settings_helpers = g_slist_next(settings_helpers);
    }

    cons_show("");

    ui_console_dirty();
    _cons_alert();
}

void
cons_presence_help(void)
{
    cons_show("");
    cons_show("Presence changes:");
    cons_show("");

    GSList *presence_helpers = cmd_get_presence_help();
    while (presence_helpers != NULL) {
        struct cmd_help_t *help = (struct cmd_help_t *)presence_helpers->data;
        cons_show("%-25s: %s", help->usage, help->short_help);
        presence_helpers = g_slist_next(presence_helpers);
    }

    cons_show("");

    ui_console_dirty();
    _cons_alert();
}

void
cons_navigation_help(void)
{
    cons_show("");
    cons_show("Navigation:");
    cons_show("");
    cons_show("Alt-1                    : This console window.");
    cons_show("Alt-2..Alt-0             : Chat windows.");
    cons_show("F1                       : This console window.");
    cons_show("F2..F10                  : Chat windows.");
    cons_show("UP, DOWN                 : Navigate input history.");
    cons_show("LEFT, RIGHT, HOME, END   : Edit current input.");
    cons_show("ESC                      : Clear current input.");
    cons_show("TAB                      : Autocomplete command/recipient/login.");
    cons_show("PAGE UP, PAGE DOWN       : Page the main window.");
    cons_show("");

    ui_console_dirty();
    _cons_alert();
}

void
cons_show_contacts(GSList *list)
{
    GSList *curr = list;

    while(curr) {
        PContact contact = curr->data;
        if (strcmp(p_contact_subscription(contact), "none") != 0) {
            win_show_contact(console, contact);
        }
        curr = g_slist_next(curr);
    }

    ui_console_dirty();
    _cons_alert();
}

static void
_cons_splash_logo(void)
{
    win_print_time(console, '-');
    wprintw(console->win, "Welcome to\n");

    win_print_time(console, '-');
    wattron(console->win, COLOUR_SPLASH);
    wprintw(console->win, "                   ___            _           \n");
    wattroff(console->win, COLOUR_SPLASH);

    win_print_time(console, '-');
    wattron(console->win, COLOUR_SPLASH);
    wprintw(console->win, "                  / __)          (_)_         \n");
    wattroff(console->win, COLOUR_SPLASH);

    win_print_time(console, '-');
    wattron(console->win, COLOUR_SPLASH);
    wprintw(console->win, " ____   ____ ___ | |__ ____ ____  _| |_ _   _ \n");
    wattroff(console->win, COLOUR_SPLASH);

    win_print_time(console, '-');
    wattron(console->win, COLOUR_SPLASH);
    wprintw(console->win, "|  _ \\ / ___) _ \\|  __) _  |  _ \\| |  _) | | |\n");
    wattroff(console->win, COLOUR_SPLASH);

    win_print_time(console, '-');
    wattron(console->win, COLOUR_SPLASH);
    wprintw(console->win, "| | | | |  | |_| | | ( ( | | | | | | |_| |_| |\n");
    wattroff(console->win, COLOUR_SPLASH);

    win_print_time(console, '-');
    wattron(console->win, COLOUR_SPLASH);
    wprintw(console->win, "| ||_/|_|   \\___/|_|  \\_||_|_| |_|_|\\___)__  |\n");
    wattroff(console->win, COLOUR_SPLASH);

    win_print_time(console, '-');
    wattron(console->win, COLOUR_SPLASH);
    wprintw(console->win, "|_|                                    (____/ \n");
    wattroff(console->win, COLOUR_SPLASH);

    win_print_time(console, '-');
    wprintw(console->win, "\n");
    win_print_time(console, '-');
    if (strcmp(PACKAGE_STATUS, "development") == 0) {
        wprintw(console->win, "Version %sdev\n", PACKAGE_VERSION);
    } else {
        wprintw(console->win, "Version %s\n", PACKAGE_VERSION);
    }
}

static void
_cons_show_basic_help(void)
{
    cons_show("");

    GSList *basic_helpers = cmd_get_basic_help();
    while (basic_helpers != NULL) {
        struct cmd_help_t *help = (struct cmd_help_t *)basic_helpers->data;
        cons_show("%-30s: %s", help->usage, help->short_help);
        basic_helpers = g_slist_next(basic_helpers);
    }

    cons_show("");
    ui_console_dirty();
    _cons_alert();
}

static void
_cons_alert(void)
{
    if (ui_current_win_type() != WIN_CONSOLE) {
        status_bar_new(0);
    }
}

