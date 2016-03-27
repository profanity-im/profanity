/*
 * c_plugins.h
 *
 * Copyright (C) 2012 - 2016 James Booth <boothj5@gmail.com>
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
 * along with Profanity.  If not, see <http://www.gnu.org/licenses/>.
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

#ifndef C_PLUGINS_H
#define C_PLUGINS_H

#include "plugins/plugins.h"

void c_env_init(void);

ProfPlugin* c_plugin_create(const char * const filename);
void c_plugin_destroy(ProfPlugin * plugin);
void c_shutdown(void);

void c_init_hook(ProfPlugin *plugin, const char * const version, const char * const status);
void c_on_start_hook(ProfPlugin *plugin);
void c_on_shutdown_hook(ProfPlugin *plugin);
void c_on_connect_hook(ProfPlugin *plugin, const char * const account_name, const char * const fulljid);
void c_on_disconnect_hook(ProfPlugin *plugin, const char * const account_name, const char * const fulljid);

char* c_pre_chat_message_display_hook(ProfPlugin *plugin, const char * const jid, const char *message);
void  c_post_chat_message_display_hook(ProfPlugin *plugin, const char * const jid, const char *message);
char* c_pre_chat_message_send_hook(ProfPlugin *plugin, const char * const jid, const char *message);
void  c_post_chat_message_send_hook(ProfPlugin *plugin, const char * const jid, const char *message);

char* c_pre_room_message_display_hook(ProfPlugin *plugin, const char * const room, const char * const nick, const char *message);
void  c_post_room_message_display_hook(ProfPlugin *plugin, const char * const room, const char * const nick, const char *message);
char* c_pre_room_message_send_hook(ProfPlugin *plugin, const char * const room, const char *message);
void  c_post_room_message_send_hook(ProfPlugin *plugin, const char * const room, const char *message);

char* c_pre_priv_message_display_hook(ProfPlugin *plugin, const char * const room, const char * const nick, const char *message);
void  c_post_priv_message_display_hook(ProfPlugin *plugin, const char * const room, const char * const nick, const char *message);
char* c_pre_priv_message_send_hook(ProfPlugin *plugin, const char * const room, const char * const nick, const char * const message);
void  c_post_priv_message_send_hook(ProfPlugin *plugin, const char * const room, const char * const nick, const char * const message);

char* c_on_message_stanza_send_hook(ProfPlugin *plugin, const char *const text);
gboolean c_on_message_stanza_receive_hook(ProfPlugin *plugin, const char *const text);

char* c_on_presence_stanza_send_hook(ProfPlugin *plugin, const char *const text);
gboolean c_on_presence_stanza_receive_hook(ProfPlugin *plugin, const char *const text);

char* c_on_iq_stanza_send_hook(ProfPlugin *plugin, const char *const text);
gboolean c_on_iq_stanza_receive_hook(ProfPlugin *plugin, const char *const text);

#endif
