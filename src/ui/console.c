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

#include "common.h"
#include "config/preferences.h"
#include "contact_list.h"
#include "config/theme.h"
#include "ui/window.h"
#include "ui/ui.h"

#define CONS_WIN_TITLE "_cons"

static ProfWin* console;
static int dirty;

static void _cons_splash_logo(void);

ProfWin *
cons_create(void)
{
    int cols = getmaxx(stdscr);
    console = window_create(CONS_WIN_TITLE, cols, WIN_CONSOLE);
    dirty = FALSE;
    return console;
}

void
cons_refresh(void)
{
    int rows, cols;
    if (dirty == TRUE) {
        getmaxyx(stdscr, rows, cols);
        prefresh(console->win, console->y_pos, 0, 1, 0, rows-3, cols-1);
        dirty = FALSE;
    }
}

void
cons_show(const char * const msg, ...)
{
    va_list arg;
    va_start(arg, msg);
    GString *fmt_msg = g_string_new(NULL);
    g_string_vprintf(fmt_msg, msg, arg);
    window_show_time(console, '-');
    wprintw(console->win, "%s\n", fmt_msg->str);
    g_string_free(fmt_msg, TRUE);
    va_end(arg);

    dirty = TRUE;
}

void
cons_about(void)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    if (prefs_get_boolean(PREF_SPLASH)) {
        _cons_splash_logo();
    } else {
        window_show_time(console, '-');

        if (strcmp(PACKAGE_STATUS, "development") == 0) {
            wprintw(console->win, "Welcome to Profanity, version %sdev\n", PACKAGE_VERSION);
        } else {
            wprintw(console->win, "Welcome to Profanity, version %s\n", PACKAGE_VERSION);
        }
    }

    window_show_time(console, '-');
    wprintw(console->win, "Copyright (C) 2012, 2013 James Booth <%s>.\n", PACKAGE_BUGREPORT);
    window_show_time(console, '-');
    wprintw(console->win, "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n");
    window_show_time(console, '-');
    wprintw(console->win, "\n");
    window_show_time(console, '-');
    wprintw(console->win, "This is free software; you are free to change and redistribute it.\n");
    window_show_time(console, '-');
    wprintw(console->win, "There is NO WARRANTY, to the extent permitted by law.\n");
    window_show_time(console, '-');
    wprintw(console->win, "\n");
    window_show_time(console, '-');
    wprintw(console->win, "Type '/help' to show complete help.\n");
    window_show_time(console, '-');
    wprintw(console->win, "\n");

    if (prefs_get_boolean(PREF_VERCHECK)) {
        cons_check_version(FALSE);
    }

    prefresh(console->win, 0, 0, 1, 0, rows-3, cols-1);

    dirty = TRUE;
    if (!win_current_is_console()) {
        status_bar_new(0);
    }
}

void
cons_check_version(gboolean not_available_msg)
{
    char *latest_release = release_get_latest();

    if (latest_release != NULL) {
        gboolean relase_valid = g_regex_match_simple("^\\d+\\.\\d+\\.\\d+$", latest_release, 0, 0);

        if (relase_valid) {
            if (release_is_new(latest_release)) {
                window_show_time(console, '-');
                wprintw(console->win, "A new version of Profanity is available: %s", latest_release);
                window_show_time(console, '-');
                wprintw(console->win, "Check <http://www.profanity.im> for details.\n");
                free(latest_release);
                window_show_time(console, '-');
                wprintw(console->win, "\n");
            } else {
                if (not_available_msg) {
                    cons_show("No new version available.");
                    cons_show("");
                }
            }

            dirty = TRUE;
            if (!win_current_is_console()) {
                status_bar_new(0);
            }
        }
    }
}

void
cons_show_login_success(ProfAccount *account)
{
    window_show_time(console, '-');
    wprintw(console->win, "%s logged in successfully, ", account->jid);

    resource_presence_t presence = accounts_get_login_presence(account->name);
    const char *presence_str = string_from_resource_presence(presence);

    window_presence_colour_on(console, presence_str);
    wprintw(console->win, "%s", presence_str);
    window_presence_colour_off(console, presence_str);
    wprintw(console->win, " (priority %d)",
        accounts_get_priority_for_presence_type(account->name, presence));
    wprintw(console->win, ".\n");
}

void
cons_show_wins(void)
{
    int i = 0;
    int count = 0;

    cons_show("");
    cons_show("Active windows:");
    window_show_time(console, '-');
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
                window_show_time(console, '-');

                switch (window->type)
                {
                    case WIN_CHAT:
                        wprintw(console->win, "%d: chat %s", i + 1, window->from);
                        PContact contact = contact_list_get_contact(window->from);

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
                        wprintw(console->win, "%d: private %s", i + 1, window->from);

                        if (window->unread > 0) {
                            wprintw(console->win, ", %d unread", window->unread);
                        }

                        break;

                    case WIN_MUC:
                        wprintw(console->win, "%d: room %s", i + 1, window->from);

                        if (window->unread > 0) {
                            wprintw(console->win, ", %d unread", window->unread);
                        }

                        break;

                    default:
                        break;
                }

                wprintw(console->win, "\n");
            }
        }
    }

    cons_show("");
    dirty = TRUE;
    if (!win_current_is_console()) {
        status_bar_new(0);
    }
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

    window_show_time(console, '-');
    wprintw(win, "\n");
    window_show_time(console, '-');
    window_presence_colour_on(console, presence);
    wprintw(win, "%s", barejid);
    if (name != NULL) {
        wprintw(win, " (%s)", name);
    }
    window_presence_colour_off(console, presence);
    wprintw(win, ":\n");

    if (sub != NULL) {
        window_show_time(console, '-');
        wprintw(win, "Subscription: %s\n", sub);
    }

    if (last_activity != NULL) {
        GDateTime *now = g_date_time_new_now_local();
        GTimeSpan span = g_date_time_difference(now, last_activity);

        window_show_time(console, '-');
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
        window_show_time(console, '-');
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
        window_show_time(console, '-');
        window_presence_colour_on(console, resource_presence);
        wprintw(win, "  %s (%d), %s", resource->name, resource->priority, resource_presence);
        if (resource->status != NULL) {
            wprintw(win, ", \"%s\"", resource->status);
        }
        wprintw(win, "\n");
        window_presence_colour_off(console, resource_presence);

        if (resource->caps_str != NULL) {
            Capabilities *caps = caps_get(resource->caps_str);
            if (caps != NULL) {
                // show identity
                if ((caps->category != NULL) || (caps->type != NULL) || (caps->name != NULL)) {
                    window_show_time(console, '-');
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
                    window_show_time(console, '-');
                    wprintw(win, "    Software: %s", caps->software);
                }
                if (caps->software_version != NULL) {
                    wprintw(win, ", %s", caps->software_version);
                }
                if ((caps->software != NULL) || (caps->software_version != NULL)) {
                    wprintw(win, "\n");
                }
                if (caps->os != NULL) {
                    window_show_time(console, '-');
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

    dirty = TRUE;
    if (!win_current_is_console()) {
        status_bar_new(0);
    }
}

void
cons_show_caps(const char * const contact, Resource *resource)
{
    WINDOW *win = console->win;
    cons_show("");
    const char *resource_presence = string_from_resource_presence(resource->presence);
    window_show_time(console, '-');
    window_presence_colour_on(console, resource_presence);
    wprintw(console->win, "%s", contact);
    window_presence_colour_off(console, resource_presence);
    wprintw(win, ":\n");

    if (resource->caps_str != NULL) {
        Capabilities *caps = caps_get(resource->caps_str);
        if (caps != NULL) {
            // show identity
            if ((caps->category != NULL) || (caps->type != NULL) || (caps->name != NULL)) {
                window_show_time(console, '-');
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
                window_show_time(console, '-');
                wprintw(win, "Software: %s", caps->software);
            }
            if (caps->software_version != NULL) {
                wprintw(win, ", %s", caps->software_version);
            }
            if ((caps->software != NULL) || (caps->software_version != NULL)) {
                wprintw(win, "\n");
            }
            if (caps->os != NULL) {
                window_show_time(console, '-');
                wprintw(win, "OS: %s", caps->os);
            }
            if (caps->os_version != NULL) {
                wprintw(win, ", %s", caps->os_version);
            }
            if ((caps->os != NULL) || (caps->os_version != NULL)) {
                wprintw(win, "\n");
            }

            if (caps->features != NULL) {
                window_show_time(console, '-');
                wprintw(win, "Features:\n");
                GSList *feature = caps->features;
                while (feature != NULL) {
                    window_show_time(console, '-');
                    wprintw(win, "  %s\n", feature->data);
                    feature = g_slist_next(feature);
                }
            }
        }
    }

    dirty = TRUE;
    if (!win_current_is_console()) {
        status_bar_new(0);
    }
}

void
cons_show_software_version(const char * const jid, const char * const  presence,
    const char * const name, const char * const version, const char * const os)
{
    if ((name != NULL) || (version != NULL) || (os != NULL)) {
        cons_show("");
        window_show_time(console, '-');
        window_presence_colour_on(console, presence);
        wprintw(console->win, "%s", jid);
        window_presence_colour_off(console, presence);
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

    dirty = TRUE;
    if (!win_current_is_console()) {
        status_bar_new(0);
    }
}

void
cons_show_room_list(GSList *rooms, const char * const conference_node)
{
    if ((rooms != NULL) && (g_slist_length(rooms) > 0)) {
        cons_show("Chat rooms at %s:", conference_node);
        while (rooms != NULL) {
            DiscoItem *room = rooms->data;
            window_show_time(console, '-');
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

    dirty = TRUE;
    if (!win_current_is_console()) {
        status_bar_new(0);
    }
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

        dirty = TRUE;
        if (!win_current_is_console()) {
            status_bar_new(0);
        }
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
            window_show_time(console, '-');
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
    dirty = TRUE;
    if (!win_current_is_console()) {
        status_bar_new(0);
    }
}

static void
_cons_splash_logo(void)
{
    window_show_time(console, '-');
    wprintw(console->win, "Welcome to\n");

    window_show_time(console, '-');
    wattron(console->win, COLOUR_SPLASH);
    wprintw(console->win, "                   ___            _           \n");
    wattroff(console->win, COLOUR_SPLASH);

    window_show_time(console, '-');
    wattron(console->win, COLOUR_SPLASH);
    wprintw(console->win, "                  / __)          (_)_         \n");
    wattroff(console->win, COLOUR_SPLASH);

    window_show_time(console, '-');
    wattron(console->win, COLOUR_SPLASH);
    wprintw(console->win, " ____   ____ ___ | |__ ____ ____  _| |_ _   _ \n");
    wattroff(console->win, COLOUR_SPLASH);

    window_show_time(console, '-');
    wattron(console->win, COLOUR_SPLASH);
    wprintw(console->win, "|  _ \\ / ___) _ \\|  __) _  |  _ \\| |  _) | | |\n");
    wattroff(console->win, COLOUR_SPLASH);

    window_show_time(console, '-');
    wattron(console->win, COLOUR_SPLASH);
    wprintw(console->win, "| | | | |  | |_| | | ( ( | | | | | | |_| |_| |\n");
    wattroff(console->win, COLOUR_SPLASH);

    window_show_time(console, '-');
    wattron(console->win, COLOUR_SPLASH);
    wprintw(console->win, "| ||_/|_|   \\___/|_|  \\_||_|_| |_|_|\\___)__  |\n");
    wattroff(console->win, COLOUR_SPLASH);

    window_show_time(console, '-');
    wattron(console->win, COLOUR_SPLASH);
    wprintw(console->win, "|_|                                    (____/ \n");
    wattroff(console->win, COLOUR_SPLASH);

    window_show_time(console, '-');
    wprintw(console->win, "\n");
    window_show_time(console, '-');
    if (strcmp(PACKAGE_STATUS, "development") == 0) {
        wprintw(console->win, "Version %sdev\n", PACKAGE_VERSION);
    } else {
        wprintw(console->win, "Version %s\n", PACKAGE_VERSION);
    }
}


