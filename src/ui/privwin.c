/*
 * privwin.c
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
#include <glib.h>
#include <stdlib.h>

#include "ui/win_types.h"
#include "ui/window.h"
#include "ui/titlebar.h"
#include "window_list.h"
#include "config/preferences.h"


void
privwin_incoming_msg(ProfPrivateWin *privatewin, const char *const message, GDateTime *timestamp)
{
    assert(privatewin != NULL);

    ProfWin *window = (ProfWin*) privatewin;
    int num = wins_get_num(window);

    Jid *jidp = jid_create(privatewin->fulljid);
    if (jidp == NULL) {
        return;
    }

    gboolean is_current = wins_is_current(window);
    gboolean notify = prefs_do_chat_notify(is_current, message);

    // currently viewing chat window with sender
    if (wins_is_current(window)) {
        win_print_incoming_message(window, timestamp, jidp->resourcepart, message, PROF_MSG_PLAIN);
        title_bar_set_typing(FALSE);
        status_bar_active(num);

    // not currently viewing chat window with sender
    } else {
        privatewin->unread++;
        if (notify) {
            privatewin->notify = TRUE;
        }
        status_bar_new(num);
        cons_show_incoming_private_message(jidp->resourcepart, jidp->barejid, num);
        win_print_incoming_message(window, timestamp, jidp->resourcepart, message, PROF_MSG_PLAIN);

        if (prefs_get_boolean(PREF_FLASH)) {
            flash();
        }
    }

    if (prefs_get_boolean(PREF_BEEP)) {
        beep();
    }

    if (!notify) {
        jid_destroy(jidp);
        return;
    }

    int ui_index = num;
    if (ui_index == 10) {
        ui_index = 0;
    }

    if (prefs_get_boolean(PREF_NOTIFY_CHAT_TEXT)) {
        notify_message(jidp->resourcepart, ui_index, message);
    } else {
        notify_message(jidp->resourcepart, ui_index, NULL);
    }

    jid_destroy(jidp);
}

void
privwin_outgoing_msg(ProfPrivateWin *privwin, const char *const message)
{
    assert(privwin != NULL);

    win_print((ProfWin*)privwin, '-', 0, NULL, 0, THEME_TEXT_ME, "me", message);
}

char*
privwin_get_string(ProfPrivateWin *privwin)
{
    assert(privwin != NULL);

    GString *res = g_string_new("Private ");
    g_string_append(res, privwin->fulljid);

    if (privwin->unread > 0) {
        g_string_append_printf(res, ", %d unread", privwin->unread);
    }

    char *resstr = res->str;
    g_string_free(res, FALSE);

    return resstr;
}
