/*
 * blocking.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef XMPP_BLOCKING_H
#define XMPP_BLOCKING_H

void blocking_request(void);
int blocked_set_handler(xmpp_stanza_t* stanza);

#endif
