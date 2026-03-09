/*
 * iq.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef XMPP_IQ_H
#define XMPP_IQ_H

typedef int (*ProfIqCallback)(xmpp_stanza_t* const stanza, void* const userdata);
typedef void (*ProfIqFreeCallback)(void* userdata);

void iq_handlers_init(void);
void iq_feature_retrieval_complete_handler(void);
void iq_send_stanza(xmpp_stanza_t* const stanza);
void iq_id_handler_add(const char* const id, ProfIqCallback func, ProfIqFreeCallback free_func, void* userdata);
void iq_disco_info_request_onconnect(const char* jid);
void iq_disco_items_request_onconnect(const char* jid);
void iq_send_caps_request(const char* const to, const char* const id, const char* const node, const char* const ver);
void iq_send_caps_request_for_jid(const char* const to, const char* const id, const char* const node,
                                  const char* const ver);
void iq_send_caps_request_legacy(const char* const to, const char* const id, const char* const node,
                                 const char* const ver);

#endif
