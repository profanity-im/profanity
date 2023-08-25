/*
 * chatwin.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 * Copyright (C) 2019 - 2023 Michael Vetter <jubalh@iodoru.org>
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
#include "database.h"
#include "config/preferences.h"
#include "ui/ui.h"
#include "ui/window.h"
#include "ui/titlebar.h"
#include "plugins/plugins.h"
#ifdef HAVE_LIBOTR
#include "otr/otr.h"
#endif
#ifdef HAVE_LIBGPGME
#include "pgp/gpg.h"
#endif
#ifdef HAVE_OMEMO
#include "omemo/omemo.h"
#endif

static void _chatwin_history(ProfChatWin* chatwin, const char* const contact_barejid);
static void _chatwin_set_last_message(ProfChatWin* chatwin, const char* const id, const char* const message);

gboolean
_pgp_automatic_start(const char* const recipient)
{
    gboolean result = FALSE;
    char* account_name = session_get_account_name();
    ProfAccount* account = accounts_get_account(account_name);

    if (g_list_find_custom(account->pgp_enabled, recipient, (GCompareFunc)g_strcmp0)) {
        result = TRUE;
    }

    account_free(account);
    return result;
}

gboolean
_ox_automatic_start(const char* const recipient)
{
    gboolean result = FALSE;
    char* account_name = session_get_account_name();
    ProfAccount* account = accounts_get_account(account_name);

    if (g_list_find_custom(account->ox_enabled, recipient, (GCompareFunc)g_strcmp0)) {
        result = TRUE;
    }

    account_free(account);
    return result;
}

ProfChatWin*
chatwin_new(const char* const barejid)
{
    ProfWin* window = wins_new_chat(barejid);
    ProfChatWin* chatwin = (ProfChatWin*)window;

    if (!prefs_get_boolean(PREF_MAM) && (prefs_get_boolean(PREF_CHLOG) && prefs_get_boolean(PREF_HISTORY))) {
        _chatwin_history(chatwin, barejid);
    }

    // if the contact is offline, show a message
    PContact contact = roster_get_contact(barejid);
    if (contact) {
        if (strcmp(p_contact_presence(contact), "offline") == 0) {
            const char* const show = p_contact_presence(contact);
            const char* const status = p_contact_status(contact);
            win_show_status_string(window, barejid, show, status, NULL, "--", "offline");
        }
    }

    // We start a new OMEMO session if this contact has been configured accordingly.
    // However, if OTR is *also* configured, ask the user to choose between OMEMO and OTR.

    gboolean is_ox_secure = FALSE;
    gboolean is_otr_secure = FALSE;
    gboolean is_omemo_secure = FALSE;
#ifdef HAVE_LIBGPGME
    is_ox_secure = _ox_automatic_start(barejid);
#endif

#ifdef HAVE_OMEMO
    is_omemo_secure = omemo_automatic_start(barejid);
#endif

#ifdef HAVE_LIBOTR
    is_otr_secure = otr_is_secure(barejid);
#endif

    if (is_omemo_secure + is_otr_secure + _pgp_automatic_start(barejid) + is_ox_secure > 1) {
        win_println(window, THEME_DEFAULT, "!", "This chat could be either OMEMO, PGP, OX or OTR encrypted, but not more than one. "
                                                "Use '/omemo start', '/pgp start', '/ox start' or '/otr start' to select the encryption method.");
    } else if (is_omemo_secure) {
#ifdef HAVE_OMEMO
        // Start the OMEMO session
        omemo_start_session(barejid);
        chatwin->is_omemo = TRUE;
#endif
    } else if (_pgp_automatic_start(barejid)) {
        // Start the PGP session
        chatwin->pgp_send = TRUE;
    } else if (is_ox_secure) {
        // Start the OX session
        chatwin->is_ox = TRUE;
    }

    if (prefs_get_boolean(PREF_MAM)) {
        iq_mam_request(chatwin, NULL);
        win_print_loading_history(window);
    }

    return chatwin;
}

void
chatwin_receipt_received(ProfChatWin* chatwin, const char* const id)
{
    assert(chatwin != NULL);

    ProfWin* win = (ProfWin*)chatwin;
    win_mark_received(win, id);
}

#ifdef HAVE_LIBOTR
void
chatwin_otr_secured(ProfChatWin* chatwin, gboolean trusted)
{
    assert(chatwin != NULL);

    chatwin->is_otr = TRUE;
    chatwin->otr_is_trusted = trusted;

    ProfWin* window = (ProfWin*)chatwin;
    if (trusted) {
        win_println(window, THEME_OTR_STARTED_TRUSTED, "!", "OTR session started (trusted).");
    } else {
        win_println(window, THEME_OTR_STARTED_UNTRUSTED, "!", "OTR session started (untrusted).");
    }

    if (wins_is_current(window)) {
        title_bar_switch();
    } else {
        int num = wins_get_num(window);
        status_bar_new(num, WIN_CHAT, chatwin->barejid);

        int ui_index = num;
        if (ui_index == 10) {
            ui_index = 0;
        }
        cons_show("%s started an OTR session (%d).", chatwin->barejid, ui_index);
        cons_alert(window);
    }
}

void
chatwin_otr_unsecured(ProfChatWin* chatwin)
{
    assert(chatwin != NULL);

    chatwin->is_otr = FALSE;
    chatwin->otr_is_trusted = FALSE;

    ProfWin* window = (ProfWin*)chatwin;
    win_println(window, THEME_OTR_ENDED, "!", "OTR session ended.");
    if (wins_is_current(window)) {
        title_bar_switch();
    }
}

void
chatwin_otr_smp_event(ProfChatWin* chatwin, prof_otr_smp_event_t event, void* data)
{
    assert(chatwin != NULL);

    switch (event) {
    case PROF_OTR_SMP_INIT:
        win_println((ProfWin*)chatwin, THEME_DEFAULT, "!",
                    "%s wants to authenticate your identity, use '/otr secret <secret>'.", chatwin->barejid);
        break;
    case PROF_OTR_SMP_INIT_Q:
        win_println((ProfWin*)chatwin, THEME_DEFAULT, "!",
                    "%s wants to authenticate your identity with the following question:", chatwin->barejid);
        win_println((ProfWin*)chatwin, THEME_DEFAULT, "!", "  %s", (char*)data);
        win_println((ProfWin*)chatwin, THEME_DEFAULT, "!", "use '/otr answer <answer>'.");
        break;
    case PROF_OTR_SMP_SENDER_FAIL:
        win_println((ProfWin*)chatwin, THEME_DEFAULT, "!",
                    "Authentication failed, the secret you entered does not match the secret entered by %s.",
                    chatwin->barejid);
        break;
    case PROF_OTR_SMP_RECEIVER_FAIL:
        win_println((ProfWin*)chatwin, THEME_DEFAULT, "!",
                    "Authentication failed, the secret entered by %s does not match yours.", chatwin->barejid);
        break;
    case PROF_OTR_SMP_ABORT:
        win_println((ProfWin*)chatwin, THEME_DEFAULT, "!", "SMP session aborted.");
        break;
    case PROF_OTR_SMP_SUCCESS:
        win_println((ProfWin*)chatwin, THEME_DEFAULT, "!", "Authentication successful.");
        break;
    case PROF_OTR_SMP_SUCCESS_Q:
        win_println((ProfWin*)chatwin, THEME_DEFAULT, "!", "%s successfully authenticated you.", chatwin->barejid);
        break;
    case PROF_OTR_SMP_FAIL_Q:
        win_println((ProfWin*)chatwin, THEME_DEFAULT, "!", "%s failed to authenticate you.", chatwin->barejid);
        break;
    case PROF_OTR_SMP_AUTH:
        win_println((ProfWin*)chatwin, THEME_DEFAULT, "!", "Authenticating %s…", chatwin->barejid);
        break;
    case PROF_OTR_SMP_AUTH_WAIT:
        win_println((ProfWin*)chatwin, THEME_DEFAULT, "!", "Awaiting authentication from %s…", chatwin->barejid);
        break;
    default:
        break;
    }
}

void
chatwin_otr_trust(ProfChatWin* chatwin)
{
    assert(chatwin != NULL);

    chatwin->is_otr = TRUE;
    chatwin->otr_is_trusted = TRUE;

    ProfWin* window = (ProfWin*)chatwin;
    win_println(window, THEME_OTR_TRUSTED, "!", "OTR session trusted.");
    if (wins_is_current(window)) {
        title_bar_switch();
    }
}

void
chatwin_otr_untrust(ProfChatWin* chatwin)
{
    assert(chatwin != NULL);

    chatwin->is_otr = TRUE;
    chatwin->otr_is_trusted = FALSE;

    ProfWin* window = (ProfWin*)chatwin;
    win_println(window, THEME_OTR_UNTRUSTED, "!", "OTR session untrusted.");
    if (wins_is_current(window)) {
        title_bar_switch();
    }
}
#endif

void
chatwin_recipient_gone(ProfChatWin* chatwin)
{
    assert(chatwin != NULL);

    const char* display_usr = NULL;
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

    win_println((ProfWin*)chatwin, THEME_GONE, "!", "<- %s has left the conversation.", display_usr);
}

void
chatwin_incoming_msg(ProfChatWin* chatwin, ProfMessage* message, gboolean win_created)
{
    assert(chatwin != NULL);

    if (message->plain == NULL) {
        log_error("chatwin_incoming_msg: Message with no plain field from: %s", message->from_jid);
        return;
    }

    char* old_plain = message->plain;

    message->plain = plugins_pre_chat_message_display(message->from_jid->barejid, message->from_jid->resourcepart, message->plain);
    gboolean show_message = true;

    ProfWin* window = (ProfWin*)chatwin;
    int num = wins_get_num(window);

    auto_gchar gchar* display_name;
    auto_char char* mybarejid = connection_get_barejid();
    if (g_strcmp0(mybarejid, message->from_jid->barejid) == 0) {
        display_name = strdup("me");
    } else {
        display_name = roster_get_msg_display_name(message->from_jid->barejid, message->from_jid->resourcepart);
    }

#ifdef HAVE_LIBGPGME
    if (prefs_get_boolean(PREF_PGP_PUBKEY_AUTOIMPORT)) {
        if (p_gpg_is_public_key_format(message->plain)) {
            ProfPGPKey* key = p_gpg_import_pubkey(message->plain);
            if (key != NULL) {
                show_message = false;
                win_println(window, THEME_DEFAULT, "-", "Received and imported PGP key %s: \"%s\". To assign it to the correspondent using /pgp setkey %s %s", key->fp, key->name, display_name, key->id);
                p_gpg_free_key(key);
            } else {
                win_println(window, THEME_DEFAULT, "-", "Received PGP key, but couldn't import PGP key above.");
            }
        }
    }
#endif

    gboolean is_current = wins_is_current(window);
    gboolean notify = prefs_do_chat_notify(is_current) && !message->is_mam;

    // currently viewing chat window with sender
    if (wins_is_current(window)) {
        if (show_message) {
            win_print_incoming(window, display_name, message);
        }
        title_bar_set_typing(FALSE);
        status_bar_active(num, WIN_CHAT, chatwin->barejid);

        // not currently viewing chat window with sender
    } else {
        status_bar_new(num, WIN_CHAT, chatwin->barejid);

        if (!message->is_mam) {
            cons_show_incoming_message(display_name, num, chatwin->unread, window);

            if (prefs_get_boolean(PREF_FLASH)) {
                flash();
            }

            chatwin->unread++;
        }

        // TODO: so far we don't ask for MAM when incoming message occurs.
        // Need to figure out:
        // 1) only send IQ once
        // 2) sort incoming messages on timestamp
        // for now if experimental MAM is enabled we dont show no history from sql either

        // MUCPMs also get printed here. In their case we don't save any logs (because nick owners can change) and thus we shouldn't read logs
        // (and if we do we need to check the resourcepart)
        if (!prefs_get_boolean(PREF_MAM) && prefs_get_boolean(PREF_CHLOG) && prefs_get_boolean(PREF_HISTORY) && message->type == PROF_MSG_TYPE_CHAT) {
            _chatwin_history(chatwin, chatwin->barejid);
        }

        // show users status first, when receiving message via delayed delivery
        if (message->timestamp && win_created) {
            PContact pcontact = roster_get_contact(chatwin->barejid);
            if (pcontact) {
                win_show_contact(window, pcontact);
            }
        }

        win_insert_last_read_position_marker((ProfWin*)chatwin, chatwin->barejid);
        if (show_message) {
            win_print_incoming(window, display_name, message);
        }
    }

    if (!message->is_mam) {
        wins_add_urls_ac(window, message, FALSE);
        wins_add_quotes_ac(window, message->plain, FALSE);

        if (prefs_get_boolean(PREF_BEEP)) {
            beep();
        }
    }

    if (notify) {
        notify_message(display_name, num, message->plain);
    }

    plugins_post_chat_message_display(message->from_jid->barejid, message->from_jid->resourcepart, message->plain);

    free(message->plain);
    message->plain = old_plain;
}

void
chatwin_outgoing_msg(ProfChatWin* chatwin, const char* const message, char* id, prof_enc_t enc_mode,
                     gboolean request_receipt, const char* const replace_id)
{
    assert(chatwin != NULL);

    ProfWin* window = (ProfWin*)chatwin;
    wins_add_quotes_ac(window, message, FALSE);

    auto_char char* enc_char;
    if (chatwin->outgoing_char) {
        enc_char = chatwin->outgoing_char;
    } else if (enc_mode == PROF_MSG_ENC_OTR) {
        enc_char = prefs_get_otr_char();
    } else if (enc_mode == PROF_MSG_ENC_PGP) {
        enc_char = prefs_get_pgp_char();
    } else if (enc_mode == PROF_MSG_ENC_OMEMO) {
        enc_char = prefs_get_omemo_char();
    } else if (enc_mode == PROF_MSG_ENC_OX) {
        enc_char = prefs_get_ox_char();
    } else {
        enc_char = strdup("-");
    }

    if (request_receipt && id) {
        win_print_outgoing_with_receipt((ProfWin*)chatwin, enc_char, "me", message, id, replace_id);
    } else {
        win_print_outgoing((ProfWin*)chatwin, enc_char, id, replace_id, message);
    }

    // save last id and message for LMC
    if (id) {
        _chatwin_set_last_message(chatwin, id, message);
    }
}

void
chatwin_outgoing_carbon(ProfChatWin* chatwin, ProfMessage* message)
{
    assert(chatwin != NULL);

    auto_char char* enc_char;
    if (message->enc == PROF_MSG_ENC_PGP) {
        enc_char = prefs_get_pgp_char();
    } else if (message->enc == PROF_MSG_ENC_OMEMO) {
        enc_char = prefs_get_omemo_char();
    } else if (message->enc == PROF_MSG_ENC_OX) {
        enc_char = prefs_get_ox_char();
    } else {
        enc_char = strdup("-");
    }

    ProfWin* window = (ProfWin*)chatwin;

    win_print_outgoing(window, enc_char, message->id, message->replace_id, message->plain);
    int num = wins_get_num(window);
    status_bar_active(num, WIN_CHAT, chatwin->barejid);
}

void
chatwin_contact_online(ProfChatWin* chatwin, Resource* resource, GDateTime* last_activity)
{
    assert(chatwin != NULL);

    const char* show = string_from_resource_presence(resource->presence);
    PContact contact = roster_get_contact(chatwin->barejid);
    auto_char char* display_str = p_contact_create_display_string(contact, resource->name);

    win_show_status_string((ProfWin*)chatwin, display_str, show, resource->status, last_activity, "++", "online");
}

void
chatwin_contact_offline(ProfChatWin* chatwin, char* resource, char* status)
{
    assert(chatwin != NULL);

    PContact contact = roster_get_contact(chatwin->barejid);
    auto_char char* display_str = p_contact_create_display_string(contact, resource);

    win_show_status_string((ProfWin*)chatwin, display_str, "offline", status, NULL, "--", "offline");
}

char*
chatwin_get_string(ProfChatWin* chatwin)
{
    assert(chatwin != NULL);

    GString* res = g_string_new("Chat ");

    jabber_conn_status_t conn_status = connection_get_status();
    if (conn_status == JABBER_CONNECTED) {
        PContact contact = roster_get_contact(chatwin->barejid);
        if (contact == NULL) {
            g_string_append(res, chatwin->barejid);
        } else {
            const char* display_name = p_contact_name_or_jid(contact);
            g_string_append(res, display_name);
            g_string_append_printf(res, " - %s", p_contact_presence(contact));
        }
    } else {
        g_string_append(res, chatwin->barejid);
    }

    if (chatwin->unread > 0) {
        g_string_append_printf(res, ", %d unread", chatwin->unread);
    }

    return g_string_free(res, FALSE);
}

void
chatwin_set_enctext(ProfChatWin* chatwin, const char* const enctext)
{
    if (chatwin->enctext) {
        free(chatwin->enctext);
    }
    chatwin->enctext = strdup(enctext);
}

void
chatwin_unset_enctext(ProfChatWin* chatwin)
{
    if (chatwin->enctext) {
        free(chatwin->enctext);
        chatwin->enctext = NULL;
    }
}

void
chatwin_set_incoming_char(ProfChatWin* chatwin, const char* const ch)
{
    if (chatwin->incoming_char) {
        free(chatwin->incoming_char);
    }
    chatwin->incoming_char = strdup(ch);
}

void
chatwin_unset_incoming_char(ProfChatWin* chatwin)
{
    if (chatwin->incoming_char) {
        free(chatwin->incoming_char);
        chatwin->incoming_char = NULL;
    }
}

void
chatwin_set_outgoing_char(ProfChatWin* chatwin, const char* const ch)
{
    if (chatwin->outgoing_char) {
        free(chatwin->outgoing_char);
    }
    chatwin->outgoing_char = strdup(ch);
}

void
chatwin_unset_outgoing_char(ProfChatWin* chatwin)
{
    if (chatwin->outgoing_char) {
        free(chatwin->outgoing_char);
        chatwin->outgoing_char = NULL;
    }
}

static void
_chatwin_history(ProfChatWin* chatwin, const char* const contact_barejid)
{
    if (!chatwin->history_shown) {
        GSList* history = log_database_get_previous_chat(contact_barejid, NULL, NULL, FALSE, FALSE);
        GSList* curr = history;

        while (curr) {
            ProfMessage* msg = curr->data;
            char* msg_plain = msg->plain;
            msg->plain = plugins_pre_chat_message_display(msg->from_jid->barejid, msg->from_jid->resourcepart, msg->plain);
            // This is dirty workaround for memory leak. We reassign msg->plain above so have to free previous object
            // TODO: Make a better solution, for example, pass msg object to the function and it will replace msg->plain properly if needed.
            free(msg_plain);
            win_print_history((ProfWin*)chatwin, msg);
            curr = g_slist_next(curr);
        }
        chatwin->history_shown = TRUE;

        g_slist_free_full(history, (GDestroyNotify)message_free);
    }
}

// Print history starting from start_time to end_time if end_time is null the
// first entry's timestamp in the buffer is used. Flip true to prepend to buffer.
// Timestamps should be in iso8601
gboolean
chatwin_db_history(ProfChatWin* chatwin, const char* start_time, char* end_time, gboolean flip)
{
    if (!end_time) {
        end_time = buffer_size(((ProfWin*)chatwin)->layout->buffer) == 0 ? NULL : g_date_time_format_iso8601(buffer_get_entry(((ProfWin*)chatwin)->layout->buffer, 0)->time);
    }

    GSList* history = log_database_get_previous_chat(chatwin->barejid, start_time, end_time, !flip, flip);
    gboolean has_items = g_slist_length(history) != 0;
    GSList* curr = history;

    while (curr) {
        ProfMessage* msg = curr->data;
        char* msg_plain = msg->plain;
        msg->plain = plugins_pre_chat_message_display(msg->from_jid->barejid, msg->from_jid->resourcepart, msg->plain);
        // This is dirty workaround for memory leak. We reassign msg->plain above so have to free previous object
        // TODO: Make a better solution, for example, pass msg object to the function and it will replace msg->plain properly if needed.
        free(msg_plain);
        if (flip) {
            win_print_old_history((ProfWin*)chatwin, msg);
        } else {
            win_print_history((ProfWin*)chatwin, msg);
        }
        curr = g_slist_next(curr);
    }

    g_slist_free_full(history, (GDestroyNotify)message_free);
    win_redraw((ProfWin*)chatwin);

    return has_items;
}

static void
_chatwin_set_last_message(ProfChatWin* chatwin, const char* const id, const char* const message)
{
    free(chatwin->last_message);
    chatwin->last_message = strdup(message);

    free(chatwin->last_msg_id);
    chatwin->last_msg_id = strdup(id);
}
