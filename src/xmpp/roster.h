/*
 * roster.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef XMPP_ROSTER_H
#define XMPP_ROSTER_H

void roster_request(void);
void roster_set_handler(xmpp_stanza_t* const stanza);
void roster_result_handler(xmpp_stanza_t* const stanza);
GSList* roster_get_groups_from_item(xmpp_stanza_t* const item);

#endif
