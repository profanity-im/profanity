/*
 * presence.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef XMPP_PRESENCE_H
#define XMPP_PRESENCE_H

void presence_handlers_init(void);
void presence_sub_requests_init(void);
void presence_sub_requests_destroy(void);
void presence_clear_sub_requests(void);

#endif
