/*
 * omemo.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2019 Paul Fariello <paul@fariello.eu>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#include <glib.h>

#include "xmpp/iq.h"

void omemo_devicelist_subscribe(void);
void omemo_devicelist_configure_and_request(void);
void omemo_devicelist_publish(GList* device_list);
void omemo_devicelist_request(const char* const jid);
void omemo_bundle_publish(gboolean first);
void omemo_bundle_request(const char* const jid, uint32_t device_id, ProfIqCallback func, ProfIqFreeCallback free_func, void* userdata);
int omemo_start_device_session_handle_bundle(xmpp_stanza_t* const stanza, void* const userdata);

char* omemo_receive_message(xmpp_stanza_t* const stanza, gboolean* trusted, omemo_error_t* error) __attribute__((nonnull(2, 3)));
