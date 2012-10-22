/*
 * windows.c
 *
 * Copyright (C) 2012 James Booth <boothj5@gmail.com>
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

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <glib.h>
#ifdef HAVE_LIBNOTIFY
#include <libnotify/notify.h>
#endif
#ifdef HAVE_NCURSES_H
#include <ncurses.h>
#endif
#ifdef HAVE_NCURSES_NCURSES_H
#include <ncurses/ncurses.h>
#endif

#include "chat_log.h"
#include "command.h"
#include "contact.h"
#include "contact_list.h"
#include "log.h"
#include "preferences.h"
#include "ui.h"

#define CONS_WIN_TITLE "_cons"
#define PAD_SIZE 1000
#define NUM_WINS 10

// holds console at index 0 and chat wins 1 through to 9
static struct prof_win _wins[NUM_WINS];

// the window currently being displayed
static int _curr_prof_win = 0;

// shortcut pointer to console window
static WINDOW * _cons_win = NULL;

// current window state
static int dirty;

// max columns for main windows, never resize below
static int max_cols = 0;

static void _create_windows(void);
static void _cons_splash_logo(void);
static void _cons_show_basic_help(void);
static int _find_prof_win_index(const char * const contact);
static int _new_prof_win(const char * const contact);
static void _current_window_refresh(void);
static void _win_switch_if_active(const int i);
static void _win_show_time(WINDOW *win);
static void _win_show_user(WINDOW *win, const char * const user, const int colour);
static void _win_show_message(WINDOW *win, const char * const message);
static void _win_show_error_msg(WINDOW *win, const char * const message);
static void _show_status_string(WINDOW *win, const char * const from,
    const char * const show, const char * const status, const char * const pre,
    const char * const default_show);
static void _cons_show_typing(const char * const short_from);
static void _cons_show_incoming_message(const char * const short_from,
    const int win_index);
static void _win_handle_switch(const int * const ch);
static void _win_handle_page(const int * const ch);
static void _win_resize_all(void);
static gint _win_get_unread(void);
static void _win_show_history(WINDOW *win, int win_index,
    const char * const contact);

#ifdef HAVE_LIBNOTIFY
static void _win_notify(const char * const message, int timeout,
    const char * const category);
static void _win_notify_remind(gint unread);
static void _win_notify_message(char * short_from);
static void _win_notify_typing(char * short_from);
#endif

void
gui_init(void)
{
    log_info("Initialising UI");
    initscr();
    cbreak();
    keypad(stdscr, TRUE);

    if (has_colors()) {
        use_default_colors();
        start_color();

        init_pair(1, prefs_get_text(), prefs_get_bkgnd());
        init_pair(2, prefs_get_online(), prefs_get_bkgnd());
        init_pair(3, prefs_get_text(), prefs_get_bar());
        init_pair(4, prefs_get_bar_draw(), prefs_get_bar());
        init_pair(5, prefs_get_offline(), prefs_get_bkgnd());
        init_pair(6, prefs_get_err(), prefs_get_bkgnd());
        init_pair(7, prefs_get_inc(), prefs_get_bkgnd());
        init_pair(8, prefs_get_bar_text(), prefs_get_bar());

        //statuses
        init_pair(9, prefs_get_away(), prefs_get_bkgnd());
        init_pair(10, prefs_get_chat(), prefs_get_bkgnd());
        init_pair(11, prefs_get_dnd(), prefs_get_bkgnd());
        init_pair(12, prefs_get_xa(), prefs_get_bkgnd());
    }

    refresh();

    create_title_bar();
    create_status_bar();
    create_input_window();
    _create_windows();

    dirty = TRUE;
}

void
gui_refresh(void)
{
    title_bar_refresh();
    status_bar_refresh();

    if (dirty) {
        _current_window_refresh();
        dirty = FALSE;
    }

    inp_put_back();
}

void
gui_close(void)
{
#ifdef HAVE_LIBNOTIFY
    if (notify_is_initted()) {
        notify_uninit();
    }
#endif
    endwin();
}

void
gui_resize(const int ch, const char * const input, const int size)
{
    log_info("Resizing UI");
    title_bar_resize();
    status_bar_resize();
    _win_resize_all();
    inp_win_resize(input, size);
    dirty = TRUE;
}

int
win_close_win(void)
{
    if (win_in_chat()) {
        // reset the chat win to unused
        strcpy(_wins[_curr_prof_win].from, "");
        wclear(_wins[_curr_prof_win].win);
        _wins[_curr_prof_win].history_shown = 0;

        // set it as inactive in the status bar
        status_bar_inactive(_curr_prof_win);

        // go back to console window
        _curr_prof_win = 0;
        title_bar_title();

        dirty = TRUE;

        // success
        return 1;
    } else {
        // didn't close anything
        return 0;
    }
}

int
win_in_chat(void)
{
    return ((_curr_prof_win != 0) &&
        (strcmp(_wins[_curr_prof_win].from, "") != 0));
}

char *
win_get_recipient(void)
{
    struct prof_win current = _wins[_curr_prof_win];
    char *recipient = (char *) malloc(sizeof(char) * (strlen(current.from) + 1));
    strcpy(recipient, current.from);
    return recipient;
}

void
win_show_typing(const char * const from)
{
    char from_cpy[strlen(from) + 1];
    strcpy(from_cpy, from);
    char *short_from = strtok(from_cpy, "/");

    int win_index = _find_prof_win_index(short_from);

    // no chat window for user
    if (win_index == NUM_WINS) {
        _cons_show_typing(short_from);

    // have chat window but not currently in it
    } else if (win_index != _curr_prof_win) {
        _cons_show_typing(short_from);
        dirty = TRUE;

    // in chat window with user
    } else {
        title_bar_set_typing(TRUE);
        title_bar_draw();

        status_bar_active(win_index);
        dirty = TRUE;
   }

#ifdef HAVE_LIBNOTIFY
    if (prefs_get_notify())
        _win_notify_typing(short_from);
#endif
}

void
win_remind(void)
{
#ifdef HAVE_LIBNOTIFY
    gint unread = _win_get_unread();
    if (unread > 0) {
        _win_notify_remind(unread);
    }
#endif
}

void
win_show_incomming_msg(const char * const from, const char * const message)
{
    char from_cpy[strlen(from) + 1];
    strcpy(from_cpy, from);
    char *short_from = strtok(from_cpy, "/");

    int win_index = _find_prof_win_index(short_from);
    if (win_index == NUM_WINS)
        win_index = _new_prof_win(short_from);

    WINDOW *win = _wins[win_index].win;

    // currently viewing chat window with sender
    if (win_index == _curr_prof_win) {
        _win_show_time(win);
        _win_show_user(win, short_from, 1);
        _win_show_message(win, message);
        title_bar_set_typing(FALSE);
        title_bar_draw();
        status_bar_active(win_index);
        dirty = TRUE;

    // not currently viewing chat window with sender
    } else {
        status_bar_new(win_index);
        _cons_show_incoming_message(short_from, win_index);
        if (prefs_get_flash())
            flash();

        _wins[win_index].unread++;
        if (prefs_get_chlog() && prefs_get_history()) {
            _win_show_history(win, win_index, short_from);
        }

        _win_show_time(win);
        _win_show_user(win, short_from, 1);
        _win_show_message(win, message);
    }

    if (prefs_get_beep())
        beep();
#ifdef HAVE_LIBNOTIFY
    if (prefs_get_notify())
        _win_notify_message(short_from);
#endif
}

void
win_show_error_msg(const char * const from, const char *err_msg)
{
    int win_index;
    WINDOW *win;

    if (from == NULL || err_msg == NULL)
        return;

    win_index = _find_prof_win_index(from);
    // chat window exists
    if (win_index < NUM_WINS) {
        win = _wins[win_index].win;
        _win_show_time(win);
        _win_show_error_msg(win, err_msg);
        if (win_index == _curr_prof_win) {
            dirty = TRUE;
        }
    }
}

#ifdef HAVE_LIBNOTIFY
static void
_win_notify(const char * const message, int timeout,
    const char * const category)
{
    gboolean notify_initted = notify_is_initted();

    if (!notify_initted) {
        notify_initted = notify_init("Profanity");
    }

    if (notify_initted) {
        NotifyNotification *notification;
        notification = notify_notification_new("Profanity", message, NULL);
        notify_notification_set_timeout(notification, timeout);
        notify_notification_set_category(notification, category);
        notify_notification_set_urgency(notification, NOTIFY_URGENCY_NORMAL);

        GError *error = NULL;
        gboolean notify_success = notify_notification_show(notification, &error);

        if (!notify_success) {
            log_error("Error sending desktop notification:");
            log_error("  -> Message : %s", message);
            log_error("  -> Error   : %s", error->message);
        }
    } else {
        log_error("Libnotify initialisation error.");
    }
}

static void
_win_notify_remind(gint unread)
{
    char message[20];
    if (unread == 1) {
        sprintf(message, "1 unread message");
    } else {
        sprintf(message, "%d unread messages", unread);
    }

    _win_notify(message, 5000, "Incoming message");
}

static void
_win_notify_message(char * short_from)
{
    char message[strlen(short_from) + 1 + 10];
    sprintf(message, "%s: message.", short_from);

    _win_notify(message, 10000, "Incoming message");
}

static void
_win_notify_typing(char * short_from)
{
    char message[strlen(short_from) + 1 + 11];
    sprintf(message, "%s: typing...", short_from);

    _win_notify(message, 10000, "Incoming message");
}
#endif

void
win_show_outgoing_msg(const char * const from, const char * const to,
    const char * const message)
{
    // if the contact is offline, show a message
    PContact contact = contact_list_get_contact(to);
    int win_index = _find_prof_win_index(to);
    WINDOW *win = NULL;

    // create new window
    if (win_index == NUM_WINS) {
        win_index = _new_prof_win(to);
        win = _wins[win_index].win;

        if (prefs_get_chlog() && prefs_get_history()) {
            _win_show_history(win, win_index, to);
        }

        if (contact != NULL) {
            if (strcmp(p_contact_show(contact), "offline") == 0) {
                const char const *show = p_contact_show(contact);
                const char const *status = p_contact_status(contact);
                _show_status_string(win, to, show, status, "--", "offline");
            }
        }

    // use existing window
    } else {
        win = _wins[win_index].win;
    }

    _win_show_time(win);
    _win_show_user(win, from, 0);
    _win_show_message(win, message);
    _win_switch_if_active(win_index);
}

void
win_show(const char * const msg)
{
    WINDOW *win = _wins[_curr_prof_win].win;
    _win_show_time(win);
    wprintw(win, "%s\n", msg);

    dirty = TRUE;
}

void
win_bad_show(const char * const msg)
{
    WINDOW *win = _wins[_curr_prof_win].win;
    _win_show_time(win);
    wattron(win, COLOUR_ERR);
    wprintw(win, "%s\n", msg);
    wattroff(win, COLOUR_ERR);

    dirty = TRUE;
}

void
win_contact_online(const char * const from, const char * const show,
    const char * const status)
{
    _show_status_string(_cons_win, from, show, status, "++", "online");

    int win_index = _find_prof_win_index(from);
    if (win_index != NUM_WINS) {
        WINDOW *win = _wins[win_index].win;
        _show_status_string(win, from, show, status, "++", "online");
    }

    if (win_index == _curr_prof_win)
        dirty = TRUE;
}

void
win_contact_offline(const char * const from, const char * const show,
    const char * const status)
{
    _show_status_string(_cons_win, from, show, status, "--", "offline");

    int win_index = _find_prof_win_index(from);
    if (win_index != NUM_WINS) {
        WINDOW *win = _wins[win_index].win;
        _show_status_string(win, from, show, status, "--", "offline");
    }

    if (win_index == _curr_prof_win)
        dirty = TRUE;
}

void
win_disconnected(void)
{
    int i;
    // show message in all active chats
    for (i = 1; i < NUM_WINS; i++) {
        if (strcmp(_wins[i].from, "") != 0) {
            WINDOW *win = _wins[_curr_prof_win].win;
            _win_show_time(win);
            wattron(win, COLOUR_ERR);
            wprintw(win, "%s\n", "Lost connection.");
            wattroff(win, COLOUR_ERR);

            // if current win, set dirty
            if (i == _curr_prof_win) {
                dirty = TRUE;
            }
        }
    }
}

void
cons_prefs(void)
{
    cons_show("");
    cons_show("Current preferences:");
    cons_show("");

    if (prefs_get_beep())
        cons_show("Terminal beep           : ON");
    else
        cons_show("Terminal beep           : OFF");

    if (prefs_get_flash())
        cons_show("Terminal flash          : ON");
    else
        cons_show("Terminal flash          : OFF");

    if (prefs_get_notify())
        cons_show("Message notifications   : ON");
    else
        cons_show("Message notifications   : OFF");

    if (prefs_get_typing())
        cons_show("Typing notifications    : ON");
    else
        cons_show("Typing notifications    : OFF");

    if (prefs_get_showsplash())
        cons_show("Splash screen           : ON");
    else
        cons_show("Splash screen           : OFF");

    if (prefs_get_chlog())
        cons_show("Chat logging            : ON");
    else
        cons_show("Chat logging            : OFF");

    if (prefs_get_history())
        cons_show("Chat history            : ON");
    else
        cons_show("Chat history            : OFF");

    gint remind_period = prefs_get_remind();
    if (remind_period == 0) {
        cons_show("Message reminder period : OFF");
    } else if (remind_period == 1) {
        cons_show("Message reminder period : 1 second");
    } else {
        cons_show("Message reminder period : %d seconds", remind_period);
    }

    cons_show("");

    if (_curr_prof_win == 0)
        dirty = TRUE;
}

static void
_cons_show_basic_help(void)
{
    cons_show("");

    GSList *basic_helpers = cmd_get_basic_help();
    while (basic_helpers != NULL) {
        struct cmd_help_t *help = (struct cmd_help_t *)basic_helpers->data;
        cons_show("%-25s: %s", help->usage, help->short_help);
        basic_helpers = g_slist_next(basic_helpers);
    }

    cons_show("");
}

void
cons_help(void)
{
    cons_show("");
    cons_show("Choose an area you need help with:");
    cons_show("");
    cons_show("/help basic      - Basic commands, for connecting, chatting etc.");
    cons_show("/help status     - How to change your status.");
    cons_show("/help settings   - Commands for configuring Profanity.");
    cons_show("/help navigation - How to navigate around Profanity.");
    cons_show("");

    if (_curr_prof_win == 0)
        dirty = TRUE;
}

void
cons_basic_help(void)
{
    cons_show("");
    cons_show("Basic Commands:");
    _cons_show_basic_help();

    if (_curr_prof_win == 0)
        dirty = TRUE;
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
        cons_show("%-25s: %s", help->usage, help->short_help);
        settings_helpers = g_slist_next(settings_helpers);
    }

    cons_show("");

    if (_curr_prof_win == 0)
        dirty = TRUE;
}

void
cons_status_help(void)
{
    cons_show("");
    cons_show("Status changes:");
    cons_show("");

    GSList *status_helpers = cmd_get_status_help();
    while (status_helpers != NULL) {
        struct cmd_help_t *help = (struct cmd_help_t *)status_helpers->data;
        cons_show("%-25s: %s", help->usage, help->short_help);
        status_helpers = g_slist_next(status_helpers);
    }

    cons_show("");

    if (_curr_prof_win == 0)
        dirty = TRUE;
}

void
cons_navigation_help(void)
{
    cons_show("");
    cons_show("Navigation:");
    cons_show("");
    cons_show("F1                       : This console window.");
    cons_show("F2-F10                   : Chat windows.");
    cons_show("UP, DOWN                 : Navigate input history.");
    cons_show("LEFT, RIGHT, HOME, END   : Edit current input.");
    cons_show("ESC                      : Clear current input.");
    cons_show("TAB                      : Autocomplete command/recipient/login");
    cons_show("PAGE UP, PAGE DOWN       : Page the main window.");
    cons_show("");

    if (_curr_prof_win == 0)
        dirty = TRUE;
}

void
cons_show_contacts(GSList *list)
{
    GSList *curr = list;

    while(curr) {
        PContact contact = curr->data;
        const char *show = p_contact_show(contact);

        _win_show_time(_cons_win);

        if (strcmp(show, "online") == 0) {
            wattron(_cons_win, COLOUR_ONLINE);
        } else if (strcmp(show, "away") == 0) {
            wattron(_cons_win, COLOUR_AWAY);
        } else if (strcmp(show, "chat") == 0) {
            wattron(_cons_win, COLOUR_CHAT);
        } else if (strcmp(show, "dnd") == 0) {
            wattron(_cons_win, COLOUR_DND);
        } else if (strcmp(show, "xa") == 0) {
            wattron(_cons_win, COLOUR_XA);
        } else {
            wattron(_cons_win, COLOUR_OFFLINE);
        }

        wprintw(_cons_win, "%s", p_contact_name(contact));
        wprintw(_cons_win, " is %s", show);

        if (p_contact_status(contact))
            wprintw(_cons_win, ", \"%s\"", p_contact_status(contact));

        wprintw(_cons_win, "\n");

        if (strcmp(show, "online") == 0) {
            wattroff(_cons_win, COLOUR_ONLINE);
        } else if (strcmp(show, "away") == 0) {
            wattroff(_cons_win, COLOUR_AWAY);
        } else if (strcmp(show, "chat") == 0) {
            wattroff(_cons_win, COLOUR_CHAT);
        } else if (strcmp(show, "dnd") == 0) {
            wattroff(_cons_win, COLOUR_DND);
        } else if (strcmp(show, "xa") == 0) {
            wattroff(_cons_win, COLOUR_XA);
        } else {
            wattroff(_cons_win, COLOUR_OFFLINE);
        }

        curr = g_slist_next(curr);
    }
}

void
cons_bad_show(const char * const msg)
{
    _win_show_time(_cons_win);
    wattron(_cons_win, COLOUR_ERR);
    wprintw(_cons_win, "%s\n", msg);
    wattroff(_cons_win, COLOUR_ERR);

    if (_curr_prof_win == 0)
        dirty = TRUE;
}

void
cons_show(const char * const msg, ...)
{
    va_list arg;
    va_start(arg, msg);
    GString *fmt_msg = g_string_new(NULL);
    g_string_vprintf(fmt_msg, msg, arg);
    _win_show_time(_cons_win);
    wprintw(_cons_win, "%s\n", fmt_msg->str);
    g_string_free(fmt_msg, TRUE);
    va_end(arg);

    if (_curr_prof_win == 0)
        dirty = TRUE;
}

void
cons_bad_command(const char * const cmd)
{
    _win_show_time(_cons_win);
    wprintw(_cons_win, "Unknown command: %s\n", cmd);

    if (_curr_prof_win == 0)
        dirty = TRUE;
}

void
win_handle_special_keys(const int * const ch)
{
    _win_handle_switch(ch);
    _win_handle_page(ch);
}

void
win_page_off(void)
{
    int rows = getmaxy(stdscr);
    _wins[_curr_prof_win].paged = 0;

    int y = getcury(_wins[_curr_prof_win].win);

    int size = rows - 3;

    _wins[_curr_prof_win].y_pos = y - (size - 1);
    if (_wins[_curr_prof_win].y_pos < 0)
        _wins[_curr_prof_win].y_pos = 0;

    dirty = TRUE;
}

static void
_create_windows(void)
{
    int cols = getmaxx(stdscr);
    max_cols = cols;

    // create the console window in 0
    struct prof_win cons;
    strcpy(cons.from, CONS_WIN_TITLE);
    cons.win = newpad(PAD_SIZE, cols);
    wbkgd(cons.win, COLOUR_TEXT);
    cons.y_pos = 0;
    cons.paged = 0;
    cons.unread = 0;
    cons.history_shown = 0;
    scrollok(cons.win, TRUE);

    _wins[0] = cons;

    cons_about();

    // create the chat windows
    int i;
    for (i = 1; i < NUM_WINS; i++) {
        struct prof_win chat;
        strcpy(chat.from, "");
        chat.win = newpad(PAD_SIZE, cols);
        wbkgd(chat.win, COLOUR_TEXT);
        chat.y_pos = 0;
        chat.paged = 0;
        chat.unread = 0;
        chat.history_shown = 0;
        scrollok(chat.win, TRUE);
        _wins[i] = chat;
    }
}

void
cons_about(void)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    _cons_win = _wins[0].win;

    if (prefs_get_showsplash()) {
        _cons_splash_logo();
    } else {
        _win_show_time(_cons_win);
        wprintw(_cons_win, "Welcome to Profanity, version %s\n", PACKAGE_VERSION);
    }

    _win_show_time(_cons_win);
    wprintw(_cons_win, "Copyright (C) 2012 James Booth <%s>.\n", PACKAGE_BUGREPORT);
    _win_show_time(_cons_win);
    wprintw(_cons_win, "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n");
    _win_show_time(_cons_win);
    wprintw(_cons_win, "\n");
    _win_show_time(_cons_win);
    wprintw(_cons_win, "This is free software; you are free to change and redistribute it.\n");
    _win_show_time(_cons_win);
    wprintw(_cons_win, "There is NO WARRANTY, to the extent permitted by law.\n");
    _win_show_time(_cons_win);
    wprintw(_cons_win, "\n");
    _win_show_time(_cons_win);
    wprintw(_cons_win, "Type '/help' to show complete help.\n");
    _win_show_time(_cons_win);
    wprintw(_cons_win, "\n");

    prefresh(_cons_win, 0, 0, 1, 0, rows-3, cols-1);

    dirty = TRUE;
}

static void
_cons_splash_logo(void)
{
    _win_show_time(_cons_win);
    wprintw(_cons_win, "Welcome to\n");


    _win_show_time(_cons_win);
    wattron(_cons_win, COLOUR_OFFLINE);
    wprintw(_cons_win, "                   ___            _           \n");
    wattroff(_cons_win, COLOUR_OFFLINE);

    _win_show_time(_cons_win);
    wattron(_cons_win, COLOUR_OFFLINE);
    wprintw(_cons_win, "                  / __)          (_)_         \n");
    wattroff(_cons_win, COLOUR_OFFLINE);

    _win_show_time(_cons_win);
    wattron(_cons_win, COLOUR_OFFLINE);
    wprintw(_cons_win, " ____   ____ ___ | |__ ____ ____  _| |_ _   _ \n");
    wattroff(_cons_win, COLOUR_OFFLINE);

    _win_show_time(_cons_win);
    wattron(_cons_win, COLOUR_OFFLINE);
    wprintw(_cons_win, "|  _ \\ / ___) _ \\|  __) _  |  _ \\| |  _) | | |\n");
    wattroff(_cons_win, COLOUR_OFFLINE);

    _win_show_time(_cons_win);
    wattron(_cons_win, COLOUR_OFFLINE);
    wprintw(_cons_win, "| | | | |  | |_| | | ( ( | | | | | | |_| |_| |\n");
    wattroff(_cons_win, COLOUR_OFFLINE);

    _win_show_time(_cons_win);
    wattron(_cons_win, COLOUR_OFFLINE);
    wprintw(_cons_win, "| ||_/|_|   \\___/|_|  \\_||_|_| |_|_|\\___)__  |\n");
    wattroff(_cons_win, COLOUR_OFFLINE);

    _win_show_time(_cons_win);
    wattron(_cons_win, COLOUR_OFFLINE);
    wprintw(_cons_win, "|_|                                    (____/ \n");
    wattroff(_cons_win, COLOUR_OFFLINE);

    _win_show_time(_cons_win);
    wprintw(_cons_win, "\n");
    _win_show_time(_cons_win);
    wprintw(_cons_win, "Version %s\n", PACKAGE_VERSION);
}

static int
_find_prof_win_index(const char * const contact)
{
    // find the chat window for recipient
    int i;
    for (i = 1; i < NUM_WINS; i++)
        if (strcmp(_wins[i].from, contact) == 0)
            break;

    return i;
}

static int
_new_prof_win(const char * const contact)
{
    int i;
    // find the first unused one
    for (i = 1; i < NUM_WINS; i++)
        if (strcmp(_wins[i].from, "") == 0)
            break;

    // set it up
    strcpy(_wins[i].from, contact);
    wclear(_wins[i].win);

    return i;
}

static void
_win_switch_if_active(const int i)
{
    win_page_off();
    if (strcmp(_wins[i].from, "") != 0) {
        _curr_prof_win = i;
        win_page_off();

        _wins[i].unread = 0;

        if (i == 0) {
            title_bar_title();
        } else {
            title_bar_set_recipient(_wins[i].from);
            title_bar_draw();;
            status_bar_active(i);
        }
    }

    dirty = TRUE;
}

static void
_win_show_time(WINDOW *win)
{
    GDateTime *time = g_date_time_new_now_local();
    gchar *date_fmt = g_date_time_format(time, "%H:%M:%S");
    wprintw(win, "%s - ", date_fmt);
    g_date_time_unref(time);
    g_free(date_fmt);
}

static void
_win_show_user(WINDOW *win, const char * const user, const int colour)
{
    if (colour)
        wattron(win, COLOUR_ONLINE);
    else
        wattron(win, COLOUR_INC);
    wprintw(win, "%s: ", user);
    if (colour)
        wattroff(win, COLOUR_ONLINE);
    else
        wattroff(win, COLOUR_INC);
}

static void
_win_show_message(WINDOW *win, const char * const message)
{
    wprintw(win, "%s\n", message);
}

static void
_win_show_error_msg(WINDOW *win, const char * const message)
{
    wattron(win, COLOUR_ERR);
    wprintw(win, "%s\n", message);
    wattroff(win, COLOUR_ERR);
}

static void
_current_window_refresh(void)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    WINDOW *current = _wins[_curr_prof_win].win;
    prefresh(current, _wins[_curr_prof_win].y_pos, 0, 1, 0, rows-3, cols-1);
}

void
_win_resize_all(void)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    // only make the pads bigger, to avoid data loss on cropping
    if (cols > max_cols) {
        max_cols = cols;

        int i;
        for (i = 0; i < NUM_WINS; i++) {
            wresize(_wins[i].win, PAD_SIZE, cols);
        }
    }

    WINDOW *current = _wins[_curr_prof_win].win;
    prefresh(current, _wins[_curr_prof_win].y_pos, 0, 1, 0, rows-3, cols-1);
}

static void
_show_status_string(WINDOW *win, const char * const from,
    const char * const show, const char * const status, const char * const pre,
    const char * const default_show)
{
    _win_show_time(win);

    if (show != NULL) {
        if (strcmp(show, "away") == 0) {
            wattron(win, COLOUR_AWAY);
        } else if (strcmp(show, "chat") == 0) {
            wattron(win, COLOUR_CHAT);
        } else if (strcmp(show, "dnd") == 0) {
            wattron(win, COLOUR_DND);
        } else if (strcmp(show, "xa") == 0) {
            wattron(win, COLOUR_XA);
        } else if (strcmp(show, "online") == 0) {
            wattron(win, COLOUR_ONLINE);
        } else {
            wattron(win, COLOUR_OFFLINE);
        }
    } else if (strcmp(default_show, "online") == 0) {
        wattron(win, COLOUR_ONLINE);
    } else {
        wattron(win, COLOUR_OFFLINE);
    }

    wprintw(win, "%s %s", pre, from);

    if (show != NULL)
        wprintw(win, " is %s", show);
    else
        wprintw(win, " is %s", default_show);

    if (status != NULL)
        wprintw(win, ", \"%s\"", status);

    wprintw(win, "\n");

    if (show != NULL) {
        if (strcmp(show, "away") == 0) {
            wattroff(win, COLOUR_AWAY);
        } else if (strcmp(show, "chat") == 0) {
            wattroff(win, COLOUR_CHAT);
        } else if (strcmp(show, "dnd") == 0) {
            wattroff(win, COLOUR_DND);
        } else if (strcmp(show, "xa") == 0) {
            wattroff(win, COLOUR_XA);
        } else if (strcmp(show, "online") == 0) {
            wattroff(win, COLOUR_ONLINE);
        } else {
            wattroff(win, COLOUR_OFFLINE);
        }
    } else if (strcmp(default_show, "online") == 0) {
        wattroff(win, COLOUR_ONLINE);
    } else {
        wattroff(win, COLOUR_OFFLINE);
    }
}

static void
_cons_show_typing(const char * const short_from)
{
    _win_show_time(_cons_win);
    wattron(_cons_win, COLOUR_INC);
    wprintw(_cons_win, "!! %s is typing a message...\n", short_from);
    wattroff(_cons_win, COLOUR_INC);
}

static void
_cons_show_incoming_message(const char * const short_from, const int win_index)
{
    _win_show_time(_cons_win);
    wattron(_cons_win, COLOUR_INC);
    wprintw(_cons_win, "<< incoming from %s (%d)\n", short_from, win_index + 1);
    wattroff(_cons_win, COLOUR_INC);
}

static void
_win_handle_switch(const int * const ch)
{
    if (*ch == KEY_F(1)) {
        _win_switch_if_active(0);
    } else if (*ch == KEY_F(2)) {
        _win_switch_if_active(1);
    } else if (*ch == KEY_F(3)) {
        _win_switch_if_active(2);
    } else if (*ch == KEY_F(4)) {
        _win_switch_if_active(3);
    } else if (*ch == KEY_F(5)) {
        _win_switch_if_active(4);
    } else if (*ch == KEY_F(6)) {
        _win_switch_if_active(5);
    } else if (*ch == KEY_F(7)) {
        _win_switch_if_active(6);
    } else if (*ch == KEY_F(8)) {
        _win_switch_if_active(7);
    } else if (*ch == KEY_F(9)) {
        _win_switch_if_active(8);
    } else if (*ch == KEY_F(10)) {
        _win_switch_if_active(9);
    }
}

static void
_win_handle_page(const int * const ch)
{
    int rows = getmaxy(stdscr);
    int y = getcury(_wins[_curr_prof_win].win);

    int page_space = rows - 4;
    int *page_start = &_wins[_curr_prof_win].y_pos;

    // page up
    if (*ch == KEY_PPAGE) {
        *page_start -= page_space;

        // went past beginning, show first page
        if (*page_start < 0)
            *page_start = 0;

        _wins[_curr_prof_win].paged = 1;
        dirty = TRUE;

    // page down
    } else if (*ch == KEY_NPAGE) {
        *page_start += page_space;

        // only got half a screen, show full screen
        if ((y - (*page_start)) < page_space)
            *page_start = y - page_space;

        // went past end, show full screen
        else if (*page_start >= y)
            *page_start = y - page_space;

        _wins[_curr_prof_win].paged = 1;
        dirty = TRUE;
    }
}

static gint
_win_get_unread(void)
{
    int i;
    gint result = 0;
    for (i = 0; i < NUM_WINS; i++) {
        result += _wins[i].unread;
    }
    return result;
}

static void
_win_show_history(WINDOW *win, int win_index, const char * const contact)
{
    if (!_wins[win_index].history_shown) {
        GSList *history = NULL;
        history = chat_log_get_previous(jabber_get_jid(), contact, history);
        while (history != NULL) {
            wprintw(win, "%s\n", history->data);
            history = g_slist_next(history);
        }
        _wins[win_index].history_shown = 1;

        g_slist_free_full(history, free);
    }
}

