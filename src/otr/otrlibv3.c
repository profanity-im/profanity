/*
 * otrlibv3.c
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

#include "otr/otr.h"
#include "otr/otrlib.h"
#include "ui/ui.h"
#include "ui/window_list.h"

OtrlPolicy
otrlib_policy(void)
{
    return OTRL_POLICY_ALLOW_V1 | OTRL_POLICY_ALLOW_V2 ;
}

void
otrlib_init_timer(void)
{
}

void
otrlib_poll(void)
{
}

char*
otrlib_start_query(void)
{
    return "?OTR?v2? This user has requested an Off-the-Record private conversation. However, you do not have a plugin to support that. See http://otr.cypherpunks.ca/ for more information.";
}

static int
cb_display_otr_message(void *opdata, const char *accountname,
    const char *protocol, const char *username, const char *msg)
{
    cons_show_error("%s", msg);
    return 0;
}

void
otrlib_init_ops(OtrlMessageAppOps *ops)
{
    ops->display_otr_message = cb_display_otr_message;
}

ConnContext*
otrlib_context_find(OtrlUserState user_state, const char *const recipient, char *jid)
{
    return otrl_context_find(user_state, recipient, jid, "xmpp", 0, NULL, NULL, NULL);
}

void
otrlib_end_session(OtrlUserState user_state, const char *const recipient, char *jid, OtrlMessageAppOps *ops)
{
    ConnContext *context = otrl_context_find(user_state, recipient, jid, "xmpp",
        0, NULL, NULL, NULL);

    if (context) {
        otrl_message_disconnect(user_state, ops, NULL, jid, "xmpp", recipient);
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
        message,
        0,
        newmessage,
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
        NULL);
}

void
otrlib_handle_tlvs(OtrlUserState user_state, OtrlMessageAppOps *ops, ConnContext *context, OtrlTLV *tlvs, GHashTable *smp_initiators)
{
    NextExpectedSMP nextMsg = context->smstate->nextExpected;
    OtrlTLV *tlv = otrl_tlv_find(tlvs, OTRL_TLV_SMP1);
    if (tlv) {
        if (nextMsg != OTRL_SMP_EXPECT1) {
            otrl_message_abort_smp(user_state, ops, NULL, context);
        } else {
            ProfChatWin *chatwin = wins_get_chat(context->username);
            if (chatwin) {
                chatwin_otr_smp_event(chatwin, PROF_OTR_SMP_INIT, NULL);
            }
            g_hash_table_insert(smp_initiators, strdup(context->username), strdup(context->username));
        }
    }
    tlv = otrl_tlv_find(tlvs, OTRL_TLV_SMP1Q);
    if (tlv) {
        if (nextMsg != OTRL_SMP_EXPECT1) {
            otrl_message_abort_smp(user_state, ops, NULL, context);
        } else {
            ProfChatWin *chatwin = wins_get_chat(context->username);
            if (chatwin) {
                char *question = (char *)tlv->data;
                char *eoq = memchr(question, '\0', tlv->len);
                if (eoq) {
                    chatwin_otr_smp_event(chatwin, PROF_OTR_SMP_INIT_Q, question);
                }
            }
        }
    }
    tlv = otrl_tlv_find(tlvs, OTRL_TLV_SMP2);
    if (tlv) {
        if (nextMsg != OTRL_SMP_EXPECT2) {
            otrl_message_abort_smp(user_state, ops, NULL, context);
        } else {
            context->smstate->nextExpected = OTRL_SMP_EXPECT4;
        }
    }
    tlv = otrl_tlv_find(tlvs, OTRL_TLV_SMP3);
    if (tlv) {
        if (nextMsg != OTRL_SMP_EXPECT3) {
            otrl_message_abort_smp(user_state, ops, NULL, context);
        } else {
            context->smstate->nextExpected = OTRL_SMP_EXPECT1;
            ProfChatWin *chatwin = wins_get_chat(context->username);
            if (chatwin) {
                if (context->smstate->received_question == 0) {
                    if (context->active_fingerprint->trust && (context->active_fingerprint->trust[0] != '\0')) {
                        chatwin_otr_smp_event(chatwin, PROF_OTR_SMP_SUCCESS, NULL);
                        chatwin_otr_trust(chatwin);
                    } else {
                        chatwin_otr_smp_event(chatwin, PROF_OTR_SMP_SENDER_FAIL, NULL);
                        chatwin_otr_untrust(chatwin);
                    }
                } else {
                    if (context->smstate->sm_prog_state == OTRL_SMP_PROG_SUCCEEDED) {
                        chatwin_otr_smp_event(chatwin, PROF_OTR_SMP_SUCCESS_Q, NULL);
                    } else {
                        chatwin_otr_smp_event(chatwin, PROF_OTR_SMP_FAIL_Q, NULL);
                    }
                }
            }
        }
    }
    tlv = otrl_tlv_find(tlvs, OTRL_TLV_SMP4);
    if (tlv) {
        if (nextMsg != OTRL_SMP_EXPECT4) {
            otrl_message_abort_smp(user_state, ops, NULL, context);
        } else {
            context->smstate->nextExpected = OTRL_SMP_EXPECT1;
            ProfChatWin *chatwin = wins_get_chat(context->username);
            if (chatwin) {
                if (context->active_fingerprint->trust && (context->active_fingerprint->trust[0] != '\0')) {
                    chatwin_otr_smp_event(chatwin, PROF_OTR_SMP_SUCCESS, NULL);
                    chatwin_otr_trust(chatwin);
                } else {
                    chatwin_otr_smp_event(chatwin, PROF_OTR_SMP_RECEIVER_FAIL, NULL);
                    chatwin_otr_untrust(chatwin);
                }
            }
        }
    }
    tlv = otrl_tlv_find(tlvs, OTRL_TLV_SMP_ABORT);
    if (tlv) {
        context->smstate->nextExpected = OTRL_SMP_EXPECT1;
        ProfChatWin *chatwin = wins_get_chat(context->username);
        if (chatwin) {
            chatwin_otr_smp_event(chatwin, PROF_OTR_SMP_ABORT, NULL);
            chatwin_otr_untrust(chatwin);
        }
        otr_untrust(context->username);
    }
}
