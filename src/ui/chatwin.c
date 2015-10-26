/*
 * chatwin.c
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

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "chat_session.h"
#include "window_list.h"
#include "roster_list.h"
#include "log.h"
#include "config/preferences.h"
#include "ui/ui.h"
#include "ui/window.h"
#include "ui/titlebar.h"

static void _win_show_history(ProfChatWin *chatwin, const char *const contact);

void
ui_message_receipt(ProfChatWin *chatwin, const char *const id)
{
    assert(chatwin != NULL);

    ProfWin *win = (ProfWin*) chatwin;
    win_mark_received(win, id);
}

void
ui_gone_secure(ProfChatWin *chatwin, gboolean trusted)
{
    assert(chatwin != NULL);

    chatwin->is_otr = TRUE;
    chatwin->otr_is_trusted = trusted;

    ProfWin *window = (ProfWin*) chatwin;
    if (trusted) {
        win_print(window, '!', 0, NULL, 0, THEME_OTR_STARTED_TRUSTED, "", "OTR session started (trusted).");
    } else {
        win_print(window, '!', 0, NULL, 0, THEME_OTR_STARTED_UNTRUSTED, "", "OTR session started (untrusted).");
    }

    if (wins_is_current(window)) {
         title_bar_switch();
    } else {
        int num = wins_get_num(window);
        status_bar_new(num);

        int ui_index = num;
        if (ui_index == 10) {
            ui_index = 0;
        }
        cons_show("%s started an OTR session (%d).", chatwin->barejid, ui_index);
        cons_alert();
    }
}

void
ui_gone_insecure(ProfChatWin *chatwin)
{
    assert(chatwin != NULL);

    chatwin->is_otr = FALSE;
    chatwin->otr_is_trusted = FALSE;

    ProfWin *window = (ProfWin*)chatwin;
    win_print(window, '!', 0, NULL, 0, THEME_OTR_ENDED, "", "OTR session ended.");
    if (wins_is_current(window)) {
        title_bar_switch();
    }
}

void
ui_smp_recipient_initiated(ProfChatWin *chatwin)
{
    assert(chatwin != NULL);

    win_vprint((ProfWin*)chatwin, '!', 0, NULL, 0, 0, "", "%s wants to authenticate your identity, use '/otr secret <secret>'.", chatwin->barejid);
}

void
ui_smp_recipient_initiated_q(const char *const barejid, const char *question)
{
    ProfChatWin *chatwin = wins_get_chat(barejid);
    if (chatwin) {
        win_vprint((ProfWin*)chatwin, '!', 0, NULL, 0, 0, "", "%s wants to authenticate your identity with the following question:", barejid);
        win_vprint((ProfWin*)chatwin, '!', 0, NULL, 0, 0, "", "  %s", question);
        win_print((ProfWin*)chatwin, '!', 0, NULL, 0, 0, "", "use '/otr answer <answer>'.");
    }
}

void
ui_smp_unsuccessful_sender(const char *const barejid)
{
    ProfChatWin *chatwin = wins_get_chat(barejid);
    if (chatwin) {
        win_vprint((ProfWin*)chatwin, '!', 0, NULL, 0, 0, "", "Authentication failed, the secret you entered does not match the secret entered by %s.", barejid);
    }
}

void
ui_smp_unsuccessful_receiver(const char *const barejid)
{
    ProfChatWin *chatwin = wins_get_chat(barejid);
    if (chatwin) {
        win_vprint((ProfWin*)chatwin, '!', 0, NULL, 0, 0, "", "Authentication failed, the secret entered by %s does not match yours.", barejid);
    }
}

void
ui_smp_aborted(const char *const barejid)
{
    ProfChatWin *chatwin = wins_get_chat(barejid);
    if (chatwin) {
        win_print((ProfWin*)chatwin, '!', 0, NULL, 0, 0, "", "SMP session aborted.");
    }
}

void
ui_smp_successful(const char *const barejid)
{
    ProfChatWin *chatwin = wins_get_chat(barejid);
    if (chatwin) {
        win_print((ProfWin*)chatwin, '!', 0, NULL, 0, 0, "", "Authentication successful.");
    }
}

void
ui_smp_answer_success(const char *const barejid)
{
    ProfChatWin *chatwin = wins_get_chat(barejid);
    if (chatwin) {
        win_vprint((ProfWin*)chatwin, '!', 0, NULL, 0, 0, "", "%s successfully authenticated you.", barejid);
    }
}

void
ui_smp_answer_failure(const char *const barejid)
{
    ProfChatWin *chatwin = wins_get_chat(barejid);
    if (chatwin) {
        win_vprint((ProfWin*)chatwin, '!', 0, NULL, 0, 0, "", "%s failed to authenticate you.", barejid);
    }
}

void
ui_otr_authenticating(const char *const barejid)
{
    ProfChatWin *chatwin = wins_get_chat(barejid);
    if (chatwin) {
        win_vprint((ProfWin*)chatwin, '!', 0, NULL, 0, 0, "", "Authenticating %s...", barejid);
    }
}

void
ui_otr_authetication_waiting(const char *const barejid)
{
    ProfChatWin *chatwin = wins_get_chat(barejid);
    if (chatwin) {
        win_vprint((ProfWin*)chatwin, '!', 0, NULL, 0, 0, "", "Awaiting authentication from %s...", barejid);
    }
}

void
ui_handle_otr_error(const char *const barejid, const char *const message)
{
    ProfChatWin *chatwin = wins_get_chat(barejid);
    if (chatwin) {
        win_print((ProfWin*)chatwin, '!', 0, NULL, 0, THEME_ERROR, "", message);
    } else {
        cons_show_error("%s - %s", barejid, message);
    }
}

void
ui_trust(const char *const barejid)
{
    ProfChatWin *chatwin = wins_get_chat(barejid);
    if (chatwin) {
        chatwin->is_otr = TRUE;
        chatwin->otr_is_trusted = TRUE;

        ProfWin *window = (ProfWin*)chatwin;
        win_print(window, '!', 0, NULL, 0, THEME_OTR_TRUSTED, "", "OTR session trusted.");
        if (wins_is_current(window)) {
            title_bar_switch();
        }
    }
}

void
ui_untrust(const char *const barejid)
{
    ProfChatWin *chatwin = wins_get_chat(barejid);
    if (chatwin) {
        chatwin->is_otr = TRUE;
        chatwin->otr_is_trusted = FALSE;

        ProfWin *window = (ProfWin*)chatwin;
        win_print(window, '!', 0, NULL, 0, THEME_OTR_UNTRUSTED, "", "OTR session untrusted.");
        if (wins_is_current(window)) {
            title_bar_switch();
        }
    }
}

void
ui_recipient_gone(const char *const barejid, const char *const resource)
{
    if (barejid == NULL)
        return;
    if (resource == NULL)
        return;

    gboolean show_message = TRUE;

    ProfChatWin *chatwin = wins_get_chat(barejid);
    if (chatwin) {
        ChatSession *session = chat_session_get(barejid);
        if (session && g_strcmp0(session->resource, resource) != 0) {
            show_message = FALSE;
        }
        if (show_message) {
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

            win_vprint((ProfWin*)chatwin, '!', 0, NULL, 0, THEME_GONE, "", "<- %s has left the conversation.", display_usr);
        }
    }
}

ProfChatWin*
ui_new_chat_win(const char *const barejid)
{
    ProfWin *window = wins_new_chat(barejid);
    ProfChatWin *chatwin = (ProfChatWin *)window;

    if (prefs_get_boolean(PREF_CHLOG) && prefs_get_boolean(PREF_HISTORY)) {
        _win_show_history(chatwin, barejid);
    }

    // if the contact is offline, show a message
    PContact contact = roster_get_contact(barejid);
    if (contact) {
        if (strcmp(p_contact_presence(contact), "offline") == 0) {
            const char * const show = p_contact_presence(contact);
            const char * const status = p_contact_status(contact);
            win_show_status_string(window, barejid, show, status, NULL, "--", "offline");
        }
    }

    return chatwin;
}

void
ui_incoming_msg(ProfChatWin *chatwin, const char *const resource, const char *const message, GDateTime *timestamp, gboolean win_created, prof_enc_t enc_mode)
{
    ProfWin *window = (ProfWin*)chatwin;
    int num = wins_get_num(window);

    char *display_name = roster_get_msg_display_name(chatwin->barejid, resource);

    // currently viewing chat window with sender
    if (wins_is_current(window)) {
        win_print_incoming_message(window, timestamp, display_name, message, enc_mode);
        title_bar_set_typing(FALSE);
        status_bar_active(num);

    // not currently viewing chat window with sender
    } else {
        status_bar_new(num);
        cons_show_incoming_message(display_name, num);

        if (prefs_get_boolean(PREF_FLASH)) {
            flash();
        }

        chatwin->unread++;
        if (prefs_get_boolean(PREF_CHLOG) && prefs_get_boolean(PREF_HISTORY)) {
            _win_show_history(chatwin, chatwin->barejid);
        }

        // show users status first, when receiving message via delayed delivery
        if (timestamp && win_created) {
            PContact pcontact = roster_get_contact(chatwin->barejid);
            if (pcontact) {
                win_show_contact(window, pcontact);
            }
        }

        win_print_incoming_message(window, timestamp, display_name, message, enc_mode);
    }

    if (prefs_get_boolean(PREF_BEEP)) {
        beep();
    }

    if (prefs_get_boolean(PREF_NOTIFY_MESSAGE)) {
        notify_message(window, display_name, message);
    }

    free(display_name);
}

void
ui_outgoing_chat_msg(ProfChatWin *chatwin, const char *const message, char *id, prof_enc_t enc_mode)
{
    char enc_char = '-';
    if (enc_mode == PROF_MSG_OTR) {
        enc_char = prefs_get_otr_char();
    } else if (enc_mode == PROF_MSG_PGP) {
        enc_char = prefs_get_pgp_char();
    }

    if (prefs_get_boolean(PREF_RECEIPTS_REQUEST) && id) {
        win_print_with_receipt((ProfWin*)chatwin, enc_char, 0, NULL, 0, THEME_TEXT_ME, "me", message, id);
    } else {
        win_print((ProfWin*)chatwin, enc_char, 0, NULL, 0, THEME_TEXT_ME, "me", message);
    }
}

void
ui_outgoing_chat_msg_carbon(const char *const barejid, const char *const message)
{
    ProfChatWin *chatwin = wins_get_chat(barejid);

    // create new window
    if (!chatwin) {
        chatwin = ui_new_chat_win(barejid);
    }

    chat_state_active(chatwin->state);

    win_print((ProfWin*)chatwin, '-', 0, NULL, 0, THEME_TEXT_ME, "me", message);

    int num = wins_get_num((ProfWin*)chatwin);
    status_bar_active(num);
}

void
ui_chat_win_contact_online(PContact contact, Resource *resource, GDateTime *last_activity)
{
    const char *show = string_from_resource_presence(resource->presence);
    char *display_str = p_contact_create_display_string(contact, resource->name);
    const char *barejid = p_contact_barejid(contact);

    ProfWin *window = (ProfWin*)wins_get_chat(barejid);
    if (window) {
        win_show_status_string(window, display_str, show, resource->status,
            last_activity, "++", "online");

    }

    free(display_str);
}

void
ui_chat_win_contact_offline(PContact contact, char *resource, char *status)
{
    char *display_str = p_contact_create_display_string(contact, resource);
    const char *barejid = p_contact_barejid(contact);

    ProfWin *window = (ProfWin*)wins_get_chat(barejid);
    if (window) {
        win_show_status_string(window, display_str, "offline", status, NULL, "--",
            "offline");
    }

    free(display_str);
}

static void
_win_show_history(ProfChatWin *chatwin, const char *const contact)
{
    if (!chatwin->history_shown) {
        Jid *jid = jid_create(jabber_get_fulljid());
        GSList *history = chat_log_get_previous(jid->barejid, contact);
        jid_destroy(jid);
        GSList *curr = history;
        while (curr) {
            char *line = curr->data;
            // entry
            if (line[2] == ':') {
                char hh[3]; memcpy(hh, &line[0], 2); hh[2] = '\0'; int ihh = atoi(hh);
                char mm[3]; memcpy(mm, &line[3], 2); mm[2] = '\0'; int imm = atoi(mm);
                char ss[3]; memcpy(ss, &line[6], 2); ss[2] = '\0'; int iss = atoi(ss);
                GDateTime *timestamp = g_date_time_new_local(2000, 1, 1, ihh, imm, iss);
                win_print((ProfWin*)chatwin, '-', 0, timestamp, NO_COLOUR_DATE, 0, "", curr->data+11);
                g_date_time_unref(timestamp);
            // header
            } else {
                win_print((ProfWin*)chatwin, '-', 0, NULL, 0, 0, "", curr->data);
            }
            curr = g_slist_next(curr);
        }
        chatwin->history_shown = TRUE;

        g_slist_free_full(history, free);
    }
}
