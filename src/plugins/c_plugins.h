/*
 * c_plugins.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef PLUGINS_C_PLUGINS_H
#define PLUGINS_C_PLUGINS_H

#include "plugins/plugins.h"

void c_env_init(void);

ProfPlugin* c_plugin_create(const char* const filename);
void c_plugin_destroy(ProfPlugin* plugin);
void c_shutdown(void);

void c_init_hook(ProfPlugin* plugin, const char* const version, const char* const status, const char* const account_name,
                 const char* const fulljid);

gboolean c_contains_hook(ProfPlugin* plugin, const char* const hook);

void c_on_start_hook(ProfPlugin* plugin);
void c_on_shutdown_hook(ProfPlugin* plugin);
void c_on_unload_hook(ProfPlugin* plugin);
void c_on_connect_hook(ProfPlugin* plugin, const char* const account_name, const char* const fulljid);
void c_on_disconnect_hook(ProfPlugin* plugin, const char* const account_name, const char* const fulljid);

char* c_pre_chat_message_display_hook(ProfPlugin* plugin, const char* const barejid, const char* const resource, const char* message);
void c_post_chat_message_display_hook(ProfPlugin* plugin, const char* const barejid, const char* const resource, const char* message);
char* c_pre_chat_message_send_hook(ProfPlugin* plugin, const char* const barejid, const char* message);
void c_post_chat_message_send_hook(ProfPlugin* plugin, const char* const barejid, const char* message);

char* c_pre_room_message_display_hook(ProfPlugin* plugin, const char* const barejid, const char* const nick,
                                      const char* message);
void c_post_room_message_display_hook(ProfPlugin* plugin, const char* const barejid, const char* const nick,
                                      const char* message);
char* c_pre_room_message_send_hook(ProfPlugin* plugin, const char* const barejid, const char* message);
void c_post_room_message_send_hook(ProfPlugin* plugin, const char* const barejid, const char* message);
void c_on_room_history_message_hook(ProfPlugin* plugin, const char* const barejid, const char* const nick,
                                    const char* const message, const char* const timestamp);

char* c_pre_priv_message_display_hook(ProfPlugin* plugin, const char* const barejid, const char* const nick,
                                      const char* message);
void c_post_priv_message_display_hook(ProfPlugin* plugin, const char* const barejid, const char* const nick,
                                      const char* message);
char* c_pre_priv_message_send_hook(ProfPlugin* plugin, const char* const barejid, const char* const nick,
                                   const char* const message);
void c_post_priv_message_send_hook(ProfPlugin* plugin, const char* const barejid, const char* const nick,
                                   const char* const message);

char* c_on_message_stanza_send_hook(ProfPlugin* plugin, const char* const text);
gboolean c_on_message_stanza_receive_hook(ProfPlugin* plugin, const char* const text);

char* c_on_presence_stanza_send_hook(ProfPlugin* plugin, const char* const text);
gboolean c_on_presence_stanza_receive_hook(ProfPlugin* plugin, const char* const text);

char* c_on_iq_stanza_send_hook(ProfPlugin* plugin, const char* const text);
gboolean c_on_iq_stanza_receive_hook(ProfPlugin* plugin, const char* const text);

void c_on_contact_offline_hook(ProfPlugin* plugin, const char* const barejid, const char* const resource,
                               const char* const status);
void c_on_contact_presence_hook(ProfPlugin* plugin, const char* const barejid, const char* const resource,
                                const char* const presence, const char* const status, const int priority);

void c_on_chat_win_focus_hook(ProfPlugin* plugin, const char* const barejid);
void c_on_room_win_focus_hook(ProfPlugin* plugin, const char* const barejid);

#endif
