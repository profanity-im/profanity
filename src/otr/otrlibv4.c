/*
 * otrlibv4.c
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

#include <libotr/proto.h>
#include <libotr/privkey.h>
#include <libotr/message.h>

#include "log.h"
#include "otr/otr.h"
#include "otr/otrlib.h"
#include "ui/ui.h"
#include "ui/window_list.h"

static GTimer *timer;
static unsigned int current_interval;

OtrlPolicy
otrlib_policy(void)
{
    return OTRL_POLICY_ALLOW_V1 | OTRL_POLICY_ALLOW_V2;
}

void
otrlib_init_timer(void)
{
    OtrlUserState user_state = otr_userstate();
    timer = g_timer_new();
    current_interval = otrl_message_poll_get_default_interval(user_state);
}

void
otrlib_poll(void)
{
    gdouble elapsed = g_timer_elapsed(timer, NULL);

    if (current_interval != 0 && elapsed > current_interval) {
        OtrlUserState user_state = otr_userstate();
        OtrlMessageAppOps *ops = otr_messageops();
        otrl_message_poll(user_state, ops, NULL);
        g_timer_start(timer);
    }
}

char*
otrlib_start_query(void)
{
    return "?OTR?v2? This user has requested an Off-the-Record private conversation. However, you do not have a plugin to support that. See http://otr.cypherpunks.ca/ for more information.";
}

static const char*
cb_otr_error_message(void *opdata, ConnContext *context, OtrlErrorCode err_code)
{
    switch(err_code)
    {
        case OTRL_ERRCODE_ENCRYPTION_ERROR:
            return strdup("OTR Error: occurred while encrypting a message");
        case OTRL_ERRCODE_MSG_NOT_IN_PRIVATE:
            return strdup("OTR Error: Sent encrypted message to somebody who is not in a mutual OTR session");
        case OTRL_ERRCODE_MSG_UNREADABLE:
            return strdup("OTR Error: sent an unreadable encrypted message");
        case OTRL_ERRCODE_MSG_MALFORMED:
            return strdup("OTR Error: message sent is malformed");
        default:
            return strdup("OTR Error: unknown");
    }
}

static void
cb_otr_error_message_free(void *opdata, const char *err_msg)
{
    free((char *)err_msg);
}

static void
cb_timer_control(void *opdata, unsigned int interval)
{
    current_interval = interval;
}

static void
cb_handle_msg_event(void *opdata, OtrlMessageEvent msg_event,
    ConnContext *context, const char *message,
    gcry_error_t err)
{
    GString *err_msg;
    switch (msg_event)
    {
        case OTRL_MSGEVENT_ENCRYPTION_REQUIRED:
            ui_handle_otr_error(context->username, "OTR: Policy requires encryption, but attempting to send an unencrypted message.");
            break;
        case OTRL_MSGEVENT_ENCRYPTION_ERROR:
             ui_handle_otr_error(context->username, "OTR: Error occurred while encrypting a message, message not sent.");
            break;
        case OTRL_MSGEVENT_CONNECTION_ENDED:
            ui_handle_otr_error(context->username, "OTR: Message not sent because contact has ended the private conversation.");
            break;
        case OTRL_MSGEVENT_SETUP_ERROR:
            ui_handle_otr_error(context->username, "OTR: A private conversation could not be set up.");
            break;
        case OTRL_MSGEVENT_MSG_REFLECTED:
            ui_handle_otr_error(context->username, "OTR: Received our own OTR message.");
            break;
        case OTRL_MSGEVENT_MSG_RESENT:
            ui_handle_otr_error(context->username, "OTR: The previous message was resent.");
            break;
        case OTRL_MSGEVENT_RCVDMSG_NOT_IN_PRIVATE:
            ui_handle_otr_error(context->username, "OTR: Received an encrypted message but no private connection established.");
            break;
        case OTRL_MSGEVENT_RCVDMSG_UNREADABLE:
            ui_handle_otr_error(context->username, "OTR: Cannot read the received message.");
            break;
        case OTRL_MSGEVENT_RCVDMSG_MALFORMED:
            ui_handle_otr_error(context->username, "OTR: The message received contains malformed data.");
            break;
        case OTRL_MSGEVENT_RCVDMSG_GENERAL_ERR:
            err_msg = g_string_new("OTR: Received error: ");
            g_string_append(err_msg, message);
            g_string_append(err_msg, ".");
            ui_handle_otr_error(context->username, err_msg->str);
            g_string_free(err_msg, TRUE);
            break;
        case OTRL_MSGEVENT_RCVDMSG_UNENCRYPTED:
            err_msg = g_string_new("OTR: Received an unencrypted message: ");
            g_string_append(err_msg, message);
            ui_handle_otr_error(context->username, err_msg->str);
            g_string_free(err_msg, TRUE);
            break;
        case OTRL_MSGEVENT_RCVDMSG_UNRECOGNIZED:
            ui_handle_otr_error(context->username, "OTR: Cannot recognize the type of message received.");
            break;
        case OTRL_MSGEVENT_RCVDMSG_FOR_OTHER_INSTANCE:
            ui_handle_otr_error(context->username, "OTR: Received and discarded a message intended for another instance.");
            break;
        default:
            break;
    }
}

static void
cb_handle_smp_event(void *opdata, OtrlSMPEvent smp_event,
    ConnContext *context, unsigned short progress_percent,
    char *question)
{
    NextExpectedSMP nextMsg = context->smstate->nextExpected;
    OtrlUserState user_state = otr_userstate();
    OtrlMessageAppOps *ops = otr_messageops();
    GHashTable *smp_initiators = otr_smpinitators();

    ProfChatWin *chatwin = wins_get_chat(context->username);

    switch(smp_event)
    {
        case OTRL_SMPEVENT_ASK_FOR_SECRET:
            if (chatwin) {
                chatwin_otr_smp_event(chatwin, PROF_OTR_SMP_INIT, NULL);
            }
            g_hash_table_insert(smp_initiators, strdup(context->username), strdup(context->username));
            break;

        case OTRL_SMPEVENT_ASK_FOR_ANSWER:
            if (chatwin) {
                chatwin_otr_smp_event(chatwin, PROF_OTR_SMP_INIT_Q, question);
            }
            break;

        case OTRL_SMPEVENT_SUCCESS:
            if (chatwin) {
                if (context->smstate->received_question == 0) {
                    chatwin_otr_smp_event(chatwin, PROF_OTR_SMP_SUCCESS, NULL);
                    chatwin_otr_trust(chatwin);
                } else {
                    chatwin_otr_smp_event(chatwin, PROF_OTR_SMP_SUCCESS_Q, NULL);
                }
            }
            break;

        case OTRL_SMPEVENT_FAILURE:
            if (chatwin) {
                if (context->smstate->received_question == 0) {
                    if (nextMsg == OTRL_SMP_EXPECT3) {
                        chatwin_otr_smp_event(chatwin, PROF_OTR_SMP_SENDER_FAIL, NULL);
                    } else if (nextMsg == OTRL_SMP_EXPECT4) {
                        chatwin_otr_smp_event(chatwin, PROF_OTR_SMP_RECEIVER_FAIL, NULL);
                    }
                    chatwin_otr_untrust(chatwin);
                } else {
                    chatwin_otr_smp_event(chatwin, PROF_OTR_SMP_FAIL_Q, NULL);
                }
            }
            break;

        case OTRL_SMPEVENT_ERROR:
            otrl_message_abort_smp(user_state, ops, NULL, context);
            break;

        case OTRL_SMPEVENT_CHEATED:
            otrl_message_abort_smp(user_state, ops, NULL, context);
            break;

        case OTRL_SMPEVENT_ABORT:
            if (chatwin) {
                chatwin_otr_smp_event(chatwin, PROF_OTR_SMP_ABORT, NULL);
                chatwin_otr_untrust(chatwin);
            }
            break;

        case OTRL_SMPEVENT_IN_PROGRESS:
            break;

        default:
            break;
    }
}

void
otrlib_init_ops(OtrlMessageAppOps *ops)
{
    ops->otr_error_message = cb_otr_error_message;
    ops->otr_error_message_free = cb_otr_error_message_free;
    ops->handle_msg_event = cb_handle_msg_event;
    ops->handle_smp_event = cb_handle_smp_event;
    ops->timer_control = cb_timer_control;
}

ConnContext*
otrlib_context_find(OtrlUserState user_state, const char *const recipient, char *jid)
{
    return otrl_context_find(user_state, recipient, jid, "xmpp", OTRL_INSTAG_MASTER, 0, NULL, NULL, NULL);
}

void
otrlib_end_session(OtrlUserState user_state, const char *const recipient, char *jid, OtrlMessageAppOps *ops)
{
    ConnContext *context = otrl_context_find(user_state, recipient, jid, "xmpp",
        OTRL_INSTAG_MASTER, 0, NULL, NULL, NULL);

    if (context) {
        otrl_message_disconnect(user_state, ops, NULL, jid, "xmpp", recipient, 0);
    }
}

gcry_error_t
otrlib_encrypt_message(OtrlUserState user_state, OtrlMessageAppOps *ops, char *jid, const char *const to,
    const char *const message, char **newmessage)
{
    gcry_error_t err;

    err = otrl_message_sending(
        user_state,
        ops,
        NULL,
        jid,
        "xmpp",
        to,
        OTRL_INSTAG_MASTER,
        message,
        0,
        newmessage,
        OTRL_FRAGMENT_SEND_SKIP,
        NULL,
        NULL,
        NULL);

    return err;
}

int
otrlib_decrypt_message(OtrlUserState user_state, OtrlMessageAppOps *ops, char *jid, const char *const from,
    const char *const message, char **decrypted, OtrlTLV **tlvs)
{
    return otrl_message_receiving(
        user_state,
        ops,
        NULL,
        jid,
        "xmpp",
        from,
        message,
        decrypted,
        tlvs,
        NULL,
        NULL,
        NULL);
}

void
otrlib_handle_tlvs(OtrlUserState user_state, OtrlMessageAppOps *ops, ConnContext *context, OtrlTLV *tlvs, GHashTable *smp_initiators)
{
}
