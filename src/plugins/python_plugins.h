/*
 * python_plugins.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef PLUGINS_PYTHON_PLUGINS_H
#define PLUGINS_PYTHON_PLUGINS_H

#include "plugins/plugins.h"

ProfPlugin* python_plugin_create(const char* const filename);
void python_plugin_destroy(ProfPlugin* plugin);
void python_check_error(void);
void allow_python_threads();
void disable_python_threads();

const char* python_get_version_string(void);
gchar* python_get_version_number(void);

void python_init_hook(ProfPlugin* plugin, const char* const version, const char* const status,
                      const char* const account_name, const char* const fulljid);

gboolean python_contains_hook(ProfPlugin* plugin, const char* const hook);

void python_on_start_hook(ProfPlugin* plugin);
void python_on_shutdown_hook(ProfPlugin* plugin);
void python_on_unload_hook(ProfPlugin* plugin);
void python_on_connect_hook(ProfPlugin* plugin, const char* const account_name, const char* const fulljid);
void python_on_disconnect_hook(ProfPlugin* plugin, const char* const account_name, const char* const fulljid);

char* python_pre_chat_message_display_hook(ProfPlugin* plugin, const char* const barejid, const char* const resource, const char* message);
void python_post_chat_message_display_hook(ProfPlugin* plugin, const char* const barejid, const char* const resource, const char* message);
char* python_pre_chat_message_send_hook(ProfPlugin* plugin, const char* const barejid, const char* message);
void python_post_chat_message_send_hook(ProfPlugin* plugin, const char* const barejid, const char* message);

char* python_pre_room_message_display_hook(ProfPlugin* plugin, const char* const barejid, const char* const nick,
                                           const char* message);
void python_post_room_message_display_hook(ProfPlugin* plugin, const char* const barejid, const char* const nick,
                                           const char* message);
char* python_pre_room_message_send_hook(ProfPlugin* plugin, const char* const barejid, const char* message);
void python_post_room_message_send_hook(ProfPlugin* plugin, const char* const barejid, const char* message);
void python_on_room_history_message_hook(ProfPlugin* plugin, const char* const barejid, const char* const nick,
                                         const char* const message, const char* const timestamp);

char* python_pre_priv_message_display_hook(ProfPlugin* plugin, const char* const barejid, const char* const nick,
                                           const char* message);
void python_post_priv_message_display_hook(ProfPlugin* plugin, const char* const barejid, const char* const nick,
                                           const char* message);
char* python_pre_priv_message_send_hook(ProfPlugin* plugin, const char* const barejid, const char* const nick,
                                        const char* const message);
void python_post_priv_message_send_hook(ProfPlugin* plugin, const char* const barejid, const char* const nick,
                                        const char* const message);

char* python_on_message_stanza_send_hook(ProfPlugin* plugin, const char* const text);
gboolean python_on_message_stanza_receive_hook(ProfPlugin* plugin, const char* const text);
char* python_on_presence_stanza_send_hook(ProfPlugin* plugin, const char* const text);
gboolean python_on_presence_stanza_receive_hook(ProfPlugin* plugin, const char* const text);
char* python_on_iq_stanza_send_hook(ProfPlugin* plugin, const char* const text);
gboolean python_on_iq_stanza_receive_hook(ProfPlugin* plugin, const char* const text);

void python_on_contact_offline_hook(ProfPlugin* plugin, const char* const barejid, const char* const resource,
                                    const char* const status);
void python_on_contact_presence_hook(ProfPlugin* plugin, const char* const barejid, const char* const resource,
                                     const char* const presence, const char* const status, const int priority);

void python_on_chat_win_focus_hook(ProfPlugin* plugin, const char* const barejid);
void python_on_room_win_focus_hook(ProfPlugin* plugin, const char* const barejid);

#endif
