/*
 * message.h
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

#ifndef XMPP_MESSAGE_H
#define XMPP_MESSAGE_H

#include "xmpp/xmpp.h"

typedef enum {
    PROF_MSG_ENC_PLAIN,
    PROF_MSG_ENC_OTR,
    PROF_MSG_ENC_PGP,
    PROF_MSG_ENC_OMEMO
} prof_enc_t;

typedef struct {
   Jid *jid;
   char *id;
   /* The raw body from xmpp message, either plaintext or OTR encrypted text */
   char *body;
   /* The encrypted message as for PGP */
   char *encrypted;
   /* The message that will be printed on screen and logs */
   char *plain;
   GDateTime *timestamp;
   prof_enc_t enc;
   gboolean trusted;
} prof_message_t;

typedef int(*ProfMessageCallback)(xmpp_stanza_t *const stanza, void *const userdata);
typedef void(*ProfMessageFreeCallback)(void *userdata);

prof_message_t *message_init(void);
void message_free(prof_message_t *message);
void message_handlers_init(void);
void message_handlers_clear(void);
void message_pubsub_event_handler_add(const char *const node, ProfMessageCallback func, ProfMessageFreeCallback free_func, void *userdata);

#endif
