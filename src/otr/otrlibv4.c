/*
 * otrlibv4.c
 *
 * Copyright (C) 2012 - 2014 James Booth <boothj5@gmail.com>
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

#include <libotr/proto.h>
#include <libotr/privkey.h>
#include <libotr/message.h>

#include "ui/ui.h"
#include "log.h"
#include "otr/otr.h"
#include "otr/otrlib.h"

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

char *
otrlib_start_query(void)
{
    return "?OTR?v2? This user has requested an Off-the-Record private conversation. However, you do not have a plugin to support that. See http://otr.cypherpunks.ca/ for more information.";
}

static const char*
cb_otr_error_message(void *opdata, ConnContext *context,
    OtrlErrorCode err_code)
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
    if (err != 0) {
        if (message != NULL) {
            cons_show_error("%s", message);
        } else {
            cons_show_error("OTR error event with no message.");
        }
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

    switch(smp_event)
    {
        case OTRL_SMPEVENT_ASK_FOR_SECRET:
            ui_smp_recipient_initiated(context->username);
            g_hash_table_insert(smp_initiators, strdup(context->username), strdup(context->username));
            break;

        case OTRL_SMPEVENT_ASK_FOR_ANSWER:
            ui_smp_recipient_initiated_q(context->username, question);
            break;

        case OTRL_SMPEVENT_SUCCESS:
            if (context->smstate->received_question == 0) {
                ui_smp_successful(context->username);
                ui_trust(context->username);
            } else {
                ui_smp_answer_success(context->username);
            }
            break;

        case OTRL_SMPEVENT_FAILURE:
            if (context->smstate->received_question == 0) {
                if (nextMsg == OTRL_SMP_EXPECT3) {
                    ui_smp_unsuccessful_sender(context->username);
                } else if (nextMsg == OTRL_SMP_EXPECT4) {
                    ui_smp_unsuccessful_receiver(context->username);
                }
                ui_untrust(context->username);
            } else {
                ui_smp_answer_failure(context->username);
            }
            break;

        case OTRL_SMPEVENT_ERROR:
            otrl_message_abort_smp(user_state, ops, NULL, context);
            break;

        case OTRL_SMPEVENT_CHEATED:
            otrl_message_abort_smp(user_state, ops, NULL, context);
            break;

        case OTRL_SMPEVENT_ABORT:
            ui_smp_aborted(context->username);
            ui_untrust(context->username);
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

ConnContext *
otrlib_context_find(OtrlUserState user_state, const char * const recipient, char *jid)
{
    return otrl_context_find(user_state, recipient, jid, "xmpp", OTRL_INSTAG_MASTER, 0, NULL, NULL, NULL);
}

void
otrlib_end_session(OtrlUserState user_state, const char * const recipient, char *jid, OtrlMessageAppOps *ops)
{
    ConnContext *context = otrl_context_find(user_state, recipient, jid, "xmpp",
        OTRL_INSTAG_MASTER, 0, NULL, NULL, NULL);

    if (context != NULL) {
        otrl_message_disconnect(user_state, ops, NULL, jid, "xmpp", recipient, 0);
    }
}

gcry_error_t
otrlib_encrypt_message(OtrlUserState user_state, OtrlMessageAppOps *ops, char *jid, const char * const to,
    const char * const message, char **newmessage)
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
otrlib_decrypt_message(OtrlUserState user_state, OtrlMessageAppOps *ops, char *jid, const char * const from,
    const char * const message, char **decrypted, OtrlTLV **tlvs)
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
