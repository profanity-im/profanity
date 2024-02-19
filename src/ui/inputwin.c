/*
 * inputwin.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
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

#define _XOPEN_SOURCE_EXTENDED
#include "config.h"

#include <assert.h>
#include <stdio.h>
#include <sys/select.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <sys/time.h>
#include <errno.h>
#include <pthread.h>

#include <readline/readline.h>
#include <readline/history.h>

#ifdef HAVE_NCURSESW_NCURSES_H
#include <ncursesw/ncurses.h>
#elif HAVE_NCURSES_H
#include <ncurses.h>
#elif HAVE_CURSES_H
#include <curses.h>
#endif

#include "profanity.h"
#include "log.h"
#include "common.h"
#include "command/cmd_ac.h"
#include "config/files.h"
#include "config/accounts.h"
#include "config/preferences.h"
#include "config/theme.h"
#include "ui/ui.h"
#include "ui/screen.h"
#include "ui/statusbar.h"
#include "ui/inputwin.h"
#include "ui/window.h"
#include "ui/window_list.h"
#include "xmpp/xmpp.h"
#include "xmpp/muc.h"
#include "xmpp/roster_list.h"
#include "xmpp/chat_state.h"
#include "tools/editor.h"

static WINDOW* inp_win;
static int pad_start = 0;

static struct timeval p_rl_timeout;
/* Timeout in ms. Shows how long select() may block. */
static gint inp_timeout = 0;
static gint no_input_count = 0;

static FILE* discard;
static fd_set fds;
static int r;
static char* inp_line = NULL;
static gboolean get_password = FALSE;

static void _inp_win_update_virtual(void);
static int _inp_edited(const wint_t ch);
static void _inp_win_handle_scroll(void);
static int _inp_offset_to_col(char* str, int offset);
static void _inp_write(char* line, int offset);
static void _inp_redisplay(void);

static void _inp_rl_addfuncs(void);
static int _inp_rl_getc(FILE* stream);
static void _inp_rl_linehandler(char* line);
static int _inp_rl_tab_handler(int count, int key);
static int _inp_rl_shift_tab_handler(int count, int key);
static int _inp_rl_win_clear_handler(int count, int key);
static int _inp_rl_win_close_handler(int count, int key);
static int _inp_rl_win_1_handler(int count, int key);
static int _inp_rl_win_2_handler(int count, int key);
static int _inp_rl_win_3_handler(int count, int key);
static int _inp_rl_win_4_handler(int count, int key);
static int _inp_rl_win_5_handler(int count, int key);
static int _inp_rl_win_6_handler(int count, int key);
static int _inp_rl_win_7_handler(int count, int key);
static int _inp_rl_win_8_handler(int count, int key);
static int _inp_rl_win_9_handler(int count, int key);
static int _inp_rl_win_0_handler(int count, int key);
static int _inp_rl_win_11_handler(int count, int key);
static int _inp_rl_win_12_handler(int count, int key);
static int _inp_rl_win_13_handler(int count, int key);
static int _inp_rl_win_14_handler(int count, int key);
static int _inp_rl_win_15_handler(int count, int key);
static int _inp_rl_win_16_handler(int count, int key);
static int _inp_rl_win_17_handler(int count, int key);
static int _inp_rl_win_18_handler(int count, int key);
static int _inp_rl_win_19_handler(int count, int key);
static int _inp_rl_win_20_handler(int count, int key);
static int _inp_rl_win_prev_handler(int count, int key);
static int _inp_rl_win_next_handler(int count, int key);
static int _inp_rl_win_next_unread_handler(int count, int key);
static int _inp_rl_win_attention_handler(int count, int key);
static int _inp_rl_win_attention_next_handler(int count, int key);
static int _inp_rl_win_pageup_handler(int count, int key);
static int _inp_rl_win_pagedown_handler(int count, int key);
static int _inp_rl_subwin_pageup_handler(int count, int key);
static int _inp_rl_subwin_pagedown_handler(int count, int key);
static int _inp_rl_startup_hook(void);
static int _inp_rl_down_arrow_handler(int count, int key);
static int _inp_rl_scroll_handler(int count, int key);
static int _inp_rl_send_to_editor(int count, int key);
static int _inp_rl_print_newline_symbol(int count, int key);

void
create_input_window(void)
{
    /* MB_CUR_MAX is evaluated at runtime depending on the current
     * locale, therefore we check that our own version is big enough
     * and bail out if it isn't.
     */
    assert(MB_CUR_MAX <= PROF_MB_CUR_MAX);
#ifdef NCURSES_REENTRANT
    set_escdelay(25);
#else
    ESCDELAY = 25;
#endif
    discard = fopen("/dev/null", "a");
    rl_outstream = discard;
    rl_readline_name = "profanity";
    _inp_rl_addfuncs();
    rl_getc_function = _inp_rl_getc;
    rl_redisplay_function = _inp_redisplay;
    rl_startup_hook = _inp_rl_startup_hook;
    rl_callback_handler_install(NULL, _inp_rl_linehandler);

    inp_win = newpad(1, INP_WIN_MAX);
    wbkgd(inp_win, theme_attrs(THEME_INPUT_TEXT));
    ;
    keypad(inp_win, TRUE);
    wmove(inp_win, 0, 0);

    _inp_win_update_virtual();
}

char*
inp_readline(void)
{
    free(inp_line);
    inp_line = NULL;
    p_rl_timeout.tv_sec = inp_timeout / 1000;
    p_rl_timeout.tv_usec = inp_timeout % 1000 * 1000;
    FD_ZERO(&fds);
    FD_SET(fileno(rl_instream), &fds);
    errno = 0;
    pthread_mutex_unlock(&lock);
    r = select(FD_SETSIZE, &fds, NULL, NULL, &p_rl_timeout);
    pthread_mutex_lock(&lock);
    if (r < 0) {
        if (errno != EINTR) {
            const char* err_msg = strerror(errno);
            log_error("Readline failed: %s", err_msg);
        }
        return NULL;
    }

    if (FD_ISSET(fileno(rl_instream), &fds)) {
        rl_callback_read_char();

        if (rl_line_buffer && rl_line_buffer[0] != '/' && rl_line_buffer[0] != '\0' && rl_line_buffer[0] != '\n') {
            chat_state_activity();
        }

        ui_reset_idle_time();
        inp_nonblocking(TRUE);
    } else {
        inp_nonblocking(FALSE);
        chat_state_idle();
    }

    if (inp_line) {
        if (!get_password && prefs_get_boolean(PREF_SLASH_GUARD)) {
            // ignore quoted messages
            if (strlen(inp_line) > 1 && inp_line[0] != '>') {
                char* res = (char*)memchr(inp_line + 1, '/', 3);
                if (res) {
                    cons_show("Your text contains a slash in the first 4 characters");
                    return NULL;
                }
            }
        }
        return strdup(inp_line);
    } else {
        return NULL;
    }
}

void
inp_win_resize(void)
{
    int col = getcurx(inp_win);
    int wcols = getmaxx(stdscr);

    // if lost cursor off screen, move contents to show it
    if (col >= pad_start + wcols) {
        pad_start = col - (wcols / 2);
        if (pad_start < 0) {
            pad_start = 0;
        }
    }

    wbkgd(inp_win, theme_attrs(THEME_INPUT_TEXT));

    _inp_win_update_virtual();
}

void
inp_nonblocking(gboolean reset)
{
    gint inpblock = prefs_get_inpblock();
    if (!prefs_get_boolean(PREF_INPBLOCK_DYNAMIC)) {
        inp_timeout = inpblock;
        return;
    }

    if (reset) {
        inp_timeout = 0;
        no_input_count = 0;
    }

    if (inp_timeout < inpblock) {
        no_input_count++;

        if (no_input_count % 10 == 0) {
            inp_timeout += no_input_count;

            if (inp_timeout > inpblock) {
                inp_timeout = inpblock;
            }
        }
    }
}

void
inp_close(void)
{
    rl_callback_handler_remove();
    fclose(discard);
}

char*
inp_get_line(void)
{
    werase(inp_win);
    wmove(inp_win, 0, 0);
    _inp_win_update_virtual();
    doupdate();
    char* line = NULL;
    while (!line) {
        line = inp_readline();
        ui_update();
    }
    status_bar_clear_prompt();
    return line;
}

char*
inp_get_password(void)
{
    werase(inp_win);
    wmove(inp_win, 0, 0);
    _inp_win_update_virtual();
    doupdate();
    char* password = NULL;
    get_password = TRUE;
    while (!password) {
        password = inp_readline();
        ui_update();
    }
    get_password = FALSE;
    status_bar_clear_prompt();
    return password;
}

void
inp_put_back(void)
{
    _inp_win_update_virtual();
}

static void
_inp_win_update_virtual(void)
{
    int wcols = getmaxx(stdscr);
    int row = screen_inputwin_row();
    if (inp_win != NULL) {
        pnoutrefresh(inp_win, 0, pad_start, row, 0, row, wcols - 1);
    }
}

static void
_inp_write(char* line, int offset)
{
    int x;
    int y __attribute__((unused));
    int col = _inp_offset_to_col(line, offset);
    werase(inp_win);

    waddstr(inp_win, rl_display_prompt);
    getyx(inp_win, y, x);
    col += x;

    for (size_t i = 0; line[i] != '\0'; i++) {
        char* c = &line[i];
        char retc[PROF_MB_CUR_MAX] = { 0 };

        size_t ch_len = mbrlen(c, MB_CUR_MAX, NULL);
        if ((ch_len == (size_t)-2) || (ch_len == (size_t)-1)) {
            waddch(inp_win, ' ');
            continue;
        }

        if (line[i] == '\n') {
            c = retc;
            ch_len = wctomb(retc, L'\u23ce'); /* return symbol */
            if (ch_len == -1) {               /* not representable */
                retc[0] = '\\';
                ch_len = 1;
            }
        } else {
            i += ch_len - 1;
        }

        waddnstr(inp_win, c, ch_len);
    }

    wmove(inp_win, 0, col);
    _inp_win_handle_scroll();

    _inp_win_update_virtual();
    doupdate();
}

static int
_inp_edited(const wint_t ch)
{
    // Use own state to be thread-safe with possible pthreads. C standard
    // guarantees that initial value of the state will be zeroed buffer.
    static mbstate_t mbstate;

    // backspace
    if (ch == 127) {
        return 1;
    }

    // ctrl-w
    if (ch == 23) {
        return 1;
    }

    // enter
    if (ch == 13) {
        return 1;
    }

    // printable
    char bytes[MB_CUR_MAX + 1];
    size_t utf_len = wcrtomb(bytes, ch, &mbstate);
    if (utf_len == (size_t)-1) {
        return 0;
    }
    bytes[utf_len] = '\0';
    gunichar unichar = g_utf8_get_char(bytes);

    return g_unichar_isprint(unichar);
}

static int
_inp_offset_to_col(char* str, int offset)
{
    int i = 0;
    int col = 0;

    while (i < offset && str[i] != '\0') {
        gunichar uni = g_utf8_get_char(&str[i]);
        size_t ch_len = mbrlen(&str[i], MB_CUR_MAX, NULL);
        if ((ch_len == (size_t)-2) || (ch_len == (size_t)-1)) {
            i++;
            continue;
        }
        i += ch_len;
        col++;
        if (g_unichar_iswide(uni)) {
            col++;
        }
    }

    return col;
}

static void
_inp_win_handle_scroll(void)
{
    int col = getcurx(inp_win);
    int wcols = getmaxx(stdscr);

    if (col == 0) {
        pad_start = 0;
    } else if (col >= pad_start + (wcols - 1)) {
        pad_start = col - (wcols / 2);
        if (pad_start < 0) {
            pad_start = 0;
        }
    } else if (col <= pad_start) {
        pad_start = pad_start - (wcols / 2);
        if (pad_start < 0) {
            pad_start = 0;
        }
    }
}

static void
_inp_rl_addfuncs(void)
{
    rl_add_funmap_entry("prof_win_1", _inp_rl_win_1_handler);
    rl_add_funmap_entry("prof_win_2", _inp_rl_win_2_handler);
    rl_add_funmap_entry("prof_win_3", _inp_rl_win_3_handler);
    rl_add_funmap_entry("prof_win_4", _inp_rl_win_4_handler);
    rl_add_funmap_entry("prof_win_5", _inp_rl_win_5_handler);
    rl_add_funmap_entry("prof_win_6", _inp_rl_win_6_handler);
    rl_add_funmap_entry("prof_win_7", _inp_rl_win_7_handler);
    rl_add_funmap_entry("prof_win_8", _inp_rl_win_8_handler);
    rl_add_funmap_entry("prof_win_9", _inp_rl_win_9_handler);
    rl_add_funmap_entry("prof_win_0", _inp_rl_win_0_handler);
    rl_add_funmap_entry("prof_win_11", _inp_rl_win_11_handler);
    rl_add_funmap_entry("prof_win_12", _inp_rl_win_12_handler);
    rl_add_funmap_entry("prof_win_13", _inp_rl_win_13_handler);
    rl_add_funmap_entry("prof_win_14", _inp_rl_win_14_handler);
    rl_add_funmap_entry("prof_win_15", _inp_rl_win_15_handler);
    rl_add_funmap_entry("prof_win_16", _inp_rl_win_16_handler);
    rl_add_funmap_entry("prof_win_17", _inp_rl_win_17_handler);
    rl_add_funmap_entry("prof_win_18", _inp_rl_win_18_handler);
    rl_add_funmap_entry("prof_win_19", _inp_rl_win_19_handler);
    rl_add_funmap_entry("prof_win_20", _inp_rl_win_20_handler);
    rl_add_funmap_entry("prof_win_prev", _inp_rl_win_prev_handler);
    rl_add_funmap_entry("prof_win_next", _inp_rl_win_next_handler);
    rl_add_funmap_entry("prof_win_next_unread", _inp_rl_win_next_unread_handler);
    rl_add_funmap_entry("prof_win_set_attention", _inp_rl_win_attention_handler);
    rl_add_funmap_entry("prof_win_attention_next", _inp_rl_win_attention_next_handler);
    rl_add_funmap_entry("prof_win_pageup", _inp_rl_win_pageup_handler);
    rl_add_funmap_entry("prof_win_pagedown", _inp_rl_win_pagedown_handler);
    rl_add_funmap_entry("prof_subwin_pageup", _inp_rl_subwin_pageup_handler);
    rl_add_funmap_entry("prof_subwin_pagedown", _inp_rl_subwin_pagedown_handler);
    rl_add_funmap_entry("prof_complete_next", _inp_rl_tab_handler);
    rl_add_funmap_entry("prof_complete_prev", _inp_rl_shift_tab_handler);
    rl_add_funmap_entry("prof_win_clear", _inp_rl_win_clear_handler);
    rl_add_funmap_entry("prof_win_close", _inp_rl_win_close_handler);
    rl_add_funmap_entry("prof_send_to_editor", _inp_rl_send_to_editor);
    rl_add_funmap_entry("prof_cut_to_history", _inp_rl_down_arrow_handler);
    rl_add_funmap_entry("prof_print_newline_symbol", _inp_rl_print_newline_symbol);
}

// Readline callbacks

static int
_inp_rl_startup_hook(void)
{
    rl_bind_keyseq("\\e1", _inp_rl_win_1_handler);
    rl_bind_keyseq("\\e2", _inp_rl_win_2_handler);
    rl_bind_keyseq("\\e3", _inp_rl_win_3_handler);
    rl_bind_keyseq("\\e4", _inp_rl_win_4_handler);
    rl_bind_keyseq("\\e5", _inp_rl_win_5_handler);
    rl_bind_keyseq("\\e6", _inp_rl_win_6_handler);
    rl_bind_keyseq("\\e7", _inp_rl_win_7_handler);
    rl_bind_keyseq("\\e8", _inp_rl_win_8_handler);
    rl_bind_keyseq("\\e9", _inp_rl_win_9_handler);
    rl_bind_keyseq("\\e0", _inp_rl_win_0_handler);
    rl_bind_keyseq("\\eq", _inp_rl_win_11_handler);
    rl_bind_keyseq("\\ew", _inp_rl_win_12_handler);
    rl_bind_keyseq("\\ee", _inp_rl_win_13_handler);
    rl_bind_keyseq("\\er", _inp_rl_win_14_handler);
    rl_bind_keyseq("\\et", _inp_rl_win_15_handler);
    rl_bind_keyseq("\\ey", _inp_rl_win_16_handler);
    rl_bind_keyseq("\\eu", _inp_rl_win_17_handler);
    rl_bind_keyseq("\\ei", _inp_rl_win_18_handler);
    rl_bind_keyseq("\\eo", _inp_rl_win_19_handler);
    rl_bind_keyseq("\\ep", _inp_rl_win_20_handler);

    rl_bind_keyseq("\\eOP", _inp_rl_win_1_handler);
    rl_bind_keyseq("\\eOQ", _inp_rl_win_2_handler);
    rl_bind_keyseq("\\eOR", _inp_rl_win_3_handler);
    rl_bind_keyseq("\\eOS", _inp_rl_win_4_handler);
    rl_bind_keyseq("\\e[15~", _inp_rl_win_5_handler);
    rl_bind_keyseq("\\e[17~", _inp_rl_win_6_handler);
    rl_bind_keyseq("\\e[18~", _inp_rl_win_7_handler);
    rl_bind_keyseq("\\e[19~", _inp_rl_win_8_handler);
    rl_bind_keyseq("\\e[20~", _inp_rl_win_9_handler);
    rl_bind_keyseq("\\e[21~", _inp_rl_win_0_handler);

    rl_bind_keyseq("\\e[1;9D", _inp_rl_win_prev_handler);
    rl_bind_keyseq("\\e[1;3D", _inp_rl_win_prev_handler);
    rl_bind_keyseq("\\e\\e[D", _inp_rl_win_prev_handler);
    rl_bind_keyseq("\\e\\eOD", _inp_rl_win_prev_handler);

    rl_bind_keyseq("\\e[1;9C", _inp_rl_win_next_handler);
    rl_bind_keyseq("\\e[1;3C", _inp_rl_win_next_handler);
    rl_bind_keyseq("\\e\\e[C", _inp_rl_win_next_handler);
    rl_bind_keyseq("\\e\\eOC", _inp_rl_win_next_handler);

    rl_bind_keyseq("\\ea", _inp_rl_win_next_unread_handler);
    rl_bind_keyseq("\\ev", _inp_rl_win_attention_handler);
    rl_bind_keyseq("\\em", _inp_rl_win_attention_next_handler);
    rl_bind_keyseq("\\ec", _inp_rl_send_to_editor);

    rl_bind_keyseq("\\e\\e[5~", _inp_rl_subwin_pageup_handler);
    rl_bind_keyseq("\\e[5;3~", _inp_rl_subwin_pageup_handler);
    rl_bind_keyseq("\\e\\eOy", _inp_rl_subwin_pageup_handler);

    rl_bind_keyseq("\\e\\e[6~", _inp_rl_subwin_pagedown_handler);
    rl_bind_keyseq("\\e[6;3~", _inp_rl_subwin_pagedown_handler);
    rl_bind_keyseq("\\e\\eOs", _inp_rl_subwin_pagedown_handler);

    rl_bind_keyseq("\\e[5~", _inp_rl_win_pageup_handler);
    rl_bind_keyseq("\\eOy", _inp_rl_win_pageup_handler);
    rl_bind_keyseq("\\e[6~", _inp_rl_win_pagedown_handler);
    rl_bind_keyseq("\\eOs", _inp_rl_win_pagedown_handler);

    rl_bind_key('\t', _inp_rl_tab_handler);
    rl_bind_keyseq("\\e[Z", _inp_rl_shift_tab_handler);

    rl_bind_keyseq("\\e[1;3A", _inp_rl_scroll_handler);     // alt + scroll/arrow up
    rl_bind_keyseq("\\e[1;3B", _inp_rl_scroll_handler);     // alt + scroll/arrow down
    rl_bind_keyseq("\\e[1;5B", _inp_rl_down_arrow_handler); // ctrl+arrow down
    rl_bind_keyseq("\\eOb", _inp_rl_down_arrow_handler);

    rl_bind_keyseq("\\e\\C-\r", _inp_rl_print_newline_symbol); // alt+enter

    // unbind unwanted mappings
    rl_bind_keyseq("\\e=", NULL);

    // disable readline completion
    rl_variable_bind("disable-completion", "on");

    // check for and load ~/.config/profanity/inputrc
    auto_gchar gchar* inputrc = files_get_inputrc_file();
    if (inputrc) {
        rl_read_init_file(inputrc);
    }

    return 0;
}

static void
_inp_rl_linehandler(char* line)
{
    inp_line = line;
    if (!line || !*line || get_password) {
        return;
    }
    HISTORY_STATE* history = history_get_history_state();
    HIST_ENTRY* last = history->length > 0 ? history->entries[history->length - 1] : NULL;
    if (last == NULL || strcmp(last->line, line) != 0) {
        add_history(line);
    }
    free(history);
}

static gboolean shift_tab = FALSE;

static int
_inp_rl_getc(FILE* stream)
{
    int ch = rl_getc(stream);

    // 27, 91, 90 = Shift tab
    if (ch == 27) {
        shift_tab = TRUE;
        return ch;
    }
    if (shift_tab && ch == 91) {
        return ch;
    }
    if (shift_tab && ch == 90) {
        return ch;
    }

    shift_tab = FALSE;

    if (_inp_edited(ch)) {
        ProfWin* window = wins_get_current();
        cmd_ac_reset(window);

        if ((window->type == WIN_CHAT || window->type == WIN_MUC || window->type == WIN_PRIVATE) && window->quotes_ac != NULL) {
            autocomplete_reset(window->quotes_ac);
        }
    }
    return ch;
}

static void
_inp_redisplay(void)
{
    if (!get_password) {
        _inp_write(rl_line_buffer, rl_point);
    }
}

static int
_inp_rl_win_clear_handler(int count, int key)
{
    ProfWin* win = wins_get_current();
    win_clear(win);
    return 0;
}

static int
_inp_rl_win_close_handler(int count, int key)
{
    ProfWin* win = wins_get_current();
    gchar* args = 0;
    cmd_close(win, 0, &args);
    return 0;
}

static int
_inp_rl_tab_com_handler(int count, int key, gboolean previous)
{
    if (rl_point != rl_end || !rl_line_buffer) {
        return 0;
    }

    if (strncmp(rl_line_buffer, "/", 1) == 0) {
        ProfWin* window = wins_get_current();
        auto_char char* result = cmd_ac_complete(window, rl_line_buffer, previous);
        if (result) {
            rl_replace_line(result, 1);
            rl_point = rl_end;
            return 0;
        }
    }

    if (strncmp(rl_line_buffer, ">", 1) == 0) {
        ProfWin* window = wins_get_current();
        auto_char char* result = win_quote_autocomplete(window, rl_line_buffer, previous);
        if (result) {
            rl_replace_line(result, 1);
            rl_point = rl_end;
            return 0;
        }
    }

    ProfWin* current = wins_get_current();
    if (current->type == WIN_MUC) {
        auto_char char* result = muc_autocomplete(current, rl_line_buffer, previous);
        if (result) {
            rl_replace_line(result, 1);
            rl_point = rl_end;
        }
    }

    return 0;
}

static int
_inp_rl_tab_handler(int count, int key)
{
    return _inp_rl_tab_com_handler(count, key, FALSE);
}

static int
_inp_rl_shift_tab_handler(int count, int key)
{
    return _inp_rl_tab_com_handler(count, key, TRUE);
}

static void
_go_to_win(int i)
{
    ProfWin* window = wins_get_by_num(i);
    if (window) {
        ui_focus_win(window);
    }
}

static int
_inp_rl_win_1_handler(int count, int key)
{
    _go_to_win(1);
    return 0;
}

static int
_inp_rl_win_2_handler(int count, int key)
{
    _go_to_win(2);
    return 0;
}

static int
_inp_rl_win_3_handler(int count, int key)
{
    _go_to_win(3);
    return 0;
}

static int
_inp_rl_win_4_handler(int count, int key)
{
    _go_to_win(4);
    return 0;
}

static int
_inp_rl_win_5_handler(int count, int key)
{
    _go_to_win(5);
    return 0;
}

static int
_inp_rl_win_6_handler(int count, int key)
{
    _go_to_win(6);
    return 0;
}

static int
_inp_rl_win_7_handler(int count, int key)
{
    _go_to_win(7);
    return 0;
}

static int
_inp_rl_win_8_handler(int count, int key)
{
    _go_to_win(8);
    return 0;
}

static int
_inp_rl_win_9_handler(int count, int key)
{
    _go_to_win(9);
    return 0;
}

static int
_inp_rl_win_0_handler(int count, int key)
{
    _go_to_win(0);
    return 0;
}

static int
_inp_rl_win_11_handler(int count, int key)
{
    _go_to_win(11);
    return 0;
}

static int
_inp_rl_win_12_handler(int count, int key)
{
    _go_to_win(12);
    return 0;
}

static int
_inp_rl_win_13_handler(int count, int key)
{
    _go_to_win(13);
    return 0;
}

static int
_inp_rl_win_14_handler(int count, int key)
{
    _go_to_win(14);
    return 0;
}

static int
_inp_rl_win_15_handler(int count, int key)
{
    _go_to_win(15);
    return 0;
}

static int
_inp_rl_win_16_handler(int count, int key)
{
    _go_to_win(16);
    return 0;
}

static int
_inp_rl_win_17_handler(int count, int key)
{
    _go_to_win(17);
    return 0;
}

static int
_inp_rl_win_18_handler(int count, int key)
{
    _go_to_win(18);
    return 0;
}

static int
_inp_rl_win_19_handler(int count, int key)
{
    _go_to_win(19);
    return 0;
}

static int
_inp_rl_win_20_handler(int count, int key)
{
    _go_to_win(20);
    return 0;
}

static int
_inp_rl_win_prev_handler(int count, int key)
{
    ProfWin* window = wins_get_previous();
    if (window) {
        ui_focus_win(window);
    }
    return 0;
}

static int
_inp_rl_win_next_handler(int count, int key)
{
    ProfWin* window = wins_get_next();
    if (window) {
        ui_focus_win(window);
    }
    return 0;
}

static int
_inp_rl_win_next_unread_handler(int count, int key)
{
    ProfWin* window = wins_get_next_unread();
    if (window) {
        ui_focus_win(window);
    }
    return 0;
}

static int
_inp_rl_win_attention_handler(int count, int key)
{
    ProfWin* current = wins_get_current();
    if (current) {
        gboolean attention = win_toggle_attention(current);
        if (attention) {
            win_println(current, THEME_DEFAULT, "!", "Attention flag has been activated");
        } else {
            win_println(current, THEME_DEFAULT, "!", "Attention flag has been deactivated");
        }
        win_redraw(current);
    }
    return 0;
}

static int
_inp_rl_win_attention_next_handler(int count, int key)
{
    ProfWin* window = wins_get_next_attention();
    if (window) {
        ui_focus_win(window);
    }
    return 0;
}

static int
_inp_rl_win_pageup_handler(int count, int key)
{
    ProfWin* current = wins_get_current();
    win_page_up(current, 0);
    return 0;
}

static int
_inp_rl_win_pagedown_handler(int count, int key)
{
    ProfWin* current = wins_get_current();
    win_page_down(current, 0);
    return 0;
}

static int
_inp_rl_subwin_pageup_handler(int count, int key)
{
    ProfWin* current = wins_get_current();
    win_sub_page_up(current);
    return 0;
}

static int
_inp_rl_subwin_pagedown_handler(int count, int key)
{
    ProfWin* current = wins_get_current();
    win_sub_page_down(current);
    return 0;
}

static int
_inp_rl_scroll_handler(int count, int key)
{
    ProfWin* window = wins_get_current();

    if (key == 'B') {
        // mouse wheel down
        win_page_down(window, 4);
    } else if (key == 'A') {
        // mouse wheel up
        win_page_up(window, 4);
    }
    return 0;
}

static int
_inp_rl_down_arrow_handler(int count, int key)
{
    add_history(rl_line_buffer);
    using_history();
    rl_replace_line("", 0);
    rl_redisplay();
    return 0;
}

static int
_inp_rl_send_to_editor(int count, int key)
{
    if (!rl_line_buffer) {
        return 0;
    }

    auto_gchar gchar* message = NULL;

    if (get_message_from_editor(rl_line_buffer, &message)) {
        return 0;
    }

    rl_replace_line(message, 0);
    ui_resize();
    rl_point = rl_end;
    rl_forced_update_display();

    return 0;
}

static int
_inp_rl_print_newline_symbol(int count, int key)
{
    rl_insert_text("\n");
    return 0;
}
