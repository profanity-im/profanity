/*
 * chatwin.c
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

#include "config.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "xmpp/chat_session.h"
#include "window_list.h"
#include "xmpp/roster_list.h"
#include "log.h"
#include "config/preferences.h"
#include "ui/ui.h"
#include "ui/window.h"
#include "ui/titlebar.h"
#include "plugins/plugins.h"
#ifdef HAVE_LIBOTR
#include "otr/otr.h"
#endif

static void _chatwin_history(ProfChatWin *chatwin, const char *const contact);

ProfChatWin*
chatwin_new(const char *const barejid)
{
    ProfWin *window = wins_new_chat(barejid);
    ProfChatWin *chatwin = (ProfChatWin *)window;

    if (prefs_get_boolean(PREF_CHLOG) && prefs_get_boolean(PREF_HISTORY)) {
        _chatwin_history(chatwin, barejid);
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
chatwin_receipt_received(ProfChatWin *chatwin, const char *const id)
{
    assert(chatwin != NULL);

    ProfWin *win = (ProfWin*) chatwin;
    win_mark_received(win, id);
}

#ifdef HAVE_LIBOTR
void
chatwin_otr_secured(ProfChatWin *chatwin, gboolean trusted)
{
    assert(chatwin != NULL);

    chatwin->is_otr = TRUE;
    chatwin->otr_is_trusted = trusted;

    ProfWin *window = (ProfWin*) chatwin;
    if (trusted) {
        win_println(window, THEME_OTR_STARTED_TRUSTED, '!', "OTR session started (trusted).");
    } else {
        win_println(window, THEME_OTR_STARTED_UNTRUSTED, '!', "OTR session started (untrusted).");
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
chatwin_otr_unsecured(ProfChatWin *chatwin)
{
    assert(chatwin != NULL);

    chatwin->is_otr = FALSE;
    chatwin->otr_is_trusted = FALSE;

    ProfWin *window = (ProfWin*)chatwin;
    win_println(window, THEME_OTR_ENDED, '!', "OTR session ended.");
    if (wins_is_current(window)) {
        title_bar_switch();
    }
}

void
chatwin_otr_smp_event(ProfChatWin *chatwin, prof_otr_smp_event_t event, void *data)
{
    assert(chatwin != NULL);

    switch (event) {
        case PROF_OTR_SMP_INIT:
            win_println((ProfWin*)chatwin, THEME_DEFAULT, '!',
                "%s wants to authenticate your identity, use '/otr secret <secret>'.", chatwin->barejid);
            break;
        case PROF_OTR_SMP_INIT_Q:
            win_println((ProfWin*)chatwin, THEME_DEFAULT, '!',
                "%s wants to authenticate your identity with the following question:", chatwin->barejid);
            win_println((ProfWin*)chatwin, THEME_DEFAULT, '!', "  %s", (char*)data);
            win_println((ProfWin*)chatwin, THEME_DEFAULT, '!', "use '/otr answer <answer>'.");
            break;
        case PROF_OTR_SMP_SENDER_FAIL:
            win_println((ProfWin*)chatwin, THEME_DEFAULT, '!',
                "Authentication failed, the secret you entered does not match the secret entered by %s.",
                chatwin->barejid);
            break;
        case PROF_OTR_SMP_RECEIVER_FAIL:
            win_println((ProfWin*)chatwin, THEME_DEFAULT, '!',
                "Authentication failed, the secret entered by %s does not match yours.", chatwin->barejid);
            break;
        case PROF_OTR_SMP_ABORT:
            win_println((ProfWin*)chatwin, THEME_DEFAULT, '!', "SMP session aborted.");
            break;
        case PROF_OTR_SMP_SUCCESS:
            win_println((ProfWin*)chatwin, THEME_DEFAULT, '!', "Authentication successful.");
            break;
        case PROF_OTR_SMP_SUCCESS_Q:
            win_println((ProfWin*)chatwin, THEME_DEFAULT, '!', "%s successfully authenticated you.", chatwin->barejid);
            break;
        case PROF_OTR_SMP_FAIL_Q:
            win_println((ProfWin*)chatwin, THEME_DEFAULT, '!', "%s failed to authenticate you.", chatwin->barejid);
            break;
        case PROF_OTR_SMP_AUTH:
            win_println((ProfWin*)chatwin, THEME_DEFAULT, '!', "Authenticating %s...", chatwin->barejid);
            break;
        case PROF_OTR_SMP_AUTH_WAIT:
            win_println((ProfWin*)chatwin, THEME_DEFAULT, '!', "Awaiting authentication from %s...", chatwin->barejid);
            break;
        default:
            break;
    }
}

void
chatwin_otr_trust(ProfChatWin *chatwin)
{
    assert(chatwin != NULL);

    chatwin->is_otr = TRUE;
    chatwin->otr_is_trusted = TRUE;

    ProfWin *window = (ProfWin*)chatwin;
    win_println(window, THEME_OTR_TRUSTED, '!', "OTR session trusted.");
    if (wins_is_current(window)) {
        title_bar_switch();
    }
}

void
chatwin_otr_untrust(ProfChatWin *chatwin)
{
    assert(chatwin != NULL);

    chatwin->is_otr = TRUE;
    chatwin->otr_is_trusted = FALSE;

    ProfWin *window = (ProfWin*)chatwin;
    win_println(window, THEME_OTR_UNTRUSTED, '!', "OTR session untrusted.");
    if (wins_is_current(window)) {
        title_bar_switch();
    }
}
#endif

void
chatwin_recipient_gone(ProfChatWin *chatwin)
{
    assert(chatwin != NULL);

    const char *display_usr = NULL;
    PContact contact = roster_get_contact(chatwin->barejid);
    if (contact) {
        if (p_contact_name(contact)) {
            display_usr = p_contact_name(contact);
        } else {
            display_usr = chatwin->barejid;
        }
    } else {
        display_usr = chatwin->barejid;
    }

    win_println((ProfWin*)chatwin, THEME_GONE, '!', "<- %s has left the conversation.", display_usr);
}

void
chatwin_incoming_msg(ProfChatWin *chatwin, const char *const resource, const char *const message, GDateTime *timestamp, gboolean win_created, prof_enc_t enc_mode)
{
    assert(chatwin != NULL);

    char *plugin_message = plugins_pre_chat_message_display(chatwin->barejid, resource, message);

    ProfWin *window = (ProfWin*)chatwin;
    int num = wins_get_num(window);

    char *display_name = roster_get_msg_display_name(chatwin->barejid, resource);

    gboolean is_current = wins_is_current(window);
    gboolean notify = prefs_do_chat_notify(is_current);

    // currently viewing chat window with sender
    if (wins_is_current(window)) {
        win_print_incoming(window, timestamp, display_name, plugin_message, enc_mode);
        title_bar_set_typing(FALSE);
        status_bar_active(num);

    // not currently viewing chat window with sender
    } else {
        status_bar_new(num);
        cons_show_incoming_message(display_name, num, chatwin->unread);

        if (prefs_get_boolean(PREF_FLASH)) {
            flash();
        }

        chatwin->unread++;

        if (prefs_get_boolean(PREF_CHLOG) && prefs_get_boolean(PREF_HISTORY)) {
            _chatwin_history(chatwin, chatwin->barejid);
        }

        // show users status first, when receiving message via delayed delivery
        if (timestamp && win_created) {
            PContact pcontact = roster_get_contact(chatwin->barejid);
            if (pcontact) {
                win_show_contact(window, pcontact);
            }
        }

        win_print_incoming(window, timestamp, display_name, plugin_message, enc_mode);
    }

    if (prefs_get_boolean(PREF_BEEP)) {
        beep();
    }

    if (notify) {
        notify_message(display_name, num, plugin_message);
    }

    free(display_name);

    plugins_post_chat_message_display(chatwin->barejid, resource, plugin_message);

    free(plugin_message);
}

void
chatwin_outgoing_msg(ProfChatWin *chatwin, const char *const message, char *id, prof_enc_t enc_mode,
    gboolean request_receipt)
{
    assert(chatwin != NULL);

    char enc_char = '-';
    if (chatwin->outgoing_char) {
        enc_char = chatwin->outgoing_char[0];
    } else if (enc_mode == PROF_MSG_OTR) {
        enc_char = prefs_get_otr_char();
    } else if (enc_mode == PROF_MSG_PGP) {
        enc_char = prefs_get_pgp_char();
    }

    if (request_receipt && id) {
        win_print_with_receipt((ProfWin*)chatwin, enc_char, "me", message, id);
    } else {
        win_print_outgoing((ProfWin*)chatwin, enc_char, "%s", message);
    }
}

void
chatwin_outgoing_carbon(ProfChatWin *chatwin, const char *const message, prof_enc_t enc_mode)
{
    assert(chatwin != NULL);

    char enc_char = '-';
    if (enc_mode == PROF_MSG_PGP) {
        enc_char = prefs_get_pgp_char();
    }

    win_print_outgoing((ProfWin*)chatwin, enc_char, "%s", message);
    int num = wins_get_num((ProfWin*)chatwin);
    status_bar_active(num);
}

void
chatwin_contact_online(ProfChatWin *chatwin, Resource *resource, GDateTime *last_activity)
{
    assert(chatwin != NULL);

    const char *show = string_from_resource_presence(resource->presence);
    PContact contact = roster_get_contact(chatwin->barejid);
    char *display_str = p_contact_create_display_string(contact, resource->name);

    win_show_status_string((ProfWin*)chatwin, display_str, show, resource->status, last_activity, "++", "online");

    free(display_str);
}

void
chatwin_contact_offline(ProfChatWin *chatwin, char *resource, char *status)
{
    assert(chatwin != NULL);

    PContact contact = roster_get_contact(chatwin->barejid);
    char *display_str = p_contact_create_display_string(contact, resource);

    win_show_status_string((ProfWin*)chatwin, display_str, "offline", status, NULL, "--", "offline");

    free(display_str);
}

char*
chatwin_get_string(ProfChatWin *chatwin)
{
    assert(chatwin != NULL);

    GString *res = g_string_new("Chat ");

    jabber_conn_status_t conn_status = connection_get_status();
    if (conn_status == JABBER_CONNECTED) {
        PContact contact = roster_get_contact(chatwin->barejid);
        if (contact == NULL) {
            g_string_append(res, chatwin->barejid);
        } else {
            const char *display_name = p_contact_name_or_jid(contact);
            g_string_append(res, display_name);
            g_string_append_printf(res, " - %s", p_contact_presence(contact));
        }
    } else {
        g_string_append(res, chatwin->barejid);
    }

    if (chatwin->unread > 0) {
        g_string_append_printf(res, ", %d unread", chatwin->unread);
    }

    char *resstr = res->str;
    g_string_free(res, FALSE);

    return resstr;
}

void
chatwin_set_enctext(ProfChatWin *chatwin, const char *const enctext)
{
    if (chatwin->enctext) {
        free(chatwin->enctext);
    }
    chatwin->enctext = strdup(enctext);
}

void
chatwin_unset_enctext(ProfChatWin *chatwin)
{
    if (chatwin->enctext) {
        free(chatwin->enctext);
        chatwin->enctext = NULL;
    }
}

void
chatwin_set_incoming_char(ProfChatWin *chatwin, const char *const ch)
{
    if (chatwin->incoming_char) {
        free(chatwin->incoming_char);
    }
    chatwin->incoming_char = strdup(ch);
}

void
chatwin_unset_incoming_char(ProfChatWin *chatwin)
{
    if (chatwin->incoming_char) {
        free(chatwin->incoming_char);
        chatwin->incoming_char = NULL;
    }
}

void
chatwin_set_outgoing_char(ProfChatWin *chatwin, const char *const ch)
{
    if (chatwin->outgoing_char) {
        free(chatwin->outgoing_char);
    }
    chatwin->outgoing_char = strdup(ch);
}

void
chatwin_unset_outgoing_char(ProfChatWin *chatwin)
{
    if (chatwin->outgoing_char) {
        free(chatwin->outgoing_char);
        chatwin->outgoing_char = NULL;
    }
}

static void
_chatwin_history(ProfChatWin *chatwin, const char *const contact)
{
    if (!chatwin->history_shown) {
        Jid *jid = jid_create(connection_get_fulljid());
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
                win_print_history((ProfWin*)chatwin, timestamp, "%s", curr->data+11);
                g_date_time_unref(timestamp);
            // header
            } else {
                win_println((ProfWin*)chatwin, THEME_DEFAULT, '-', "%s", curr->data);
            }
            curr = g_slist_next(curr);
        }
        chatwin->history_shown = TRUE;

        g_slist_free_full(history, free);
    }
}
