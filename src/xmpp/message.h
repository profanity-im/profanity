/*
 * message.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef XMPP_MESSAGE_H
#define XMPP_MESSAGE_H

#include "xmpp/xmpp.h"

typedef int (*ProfMessageCallback)(xmpp_stanza_t* const stanza, void* const userdata);
typedef void (*ProfMessageFreeCallback)(void* userdata);

ProfMessage* message_init(void);
void message_free(ProfMessage* message);
void message_handlers_init(void);
void message_handlers_clear(void);
void message_pubsub_event_handler_add(const char* const node, ProfMessageCallback func, ProfMessageFreeCallback free_func, void* userdata);

#endif
