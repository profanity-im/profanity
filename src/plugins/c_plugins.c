/*
 * c_plugins.c
 *
 * Copyright (C) 2012 - 2017 James Booth <boothj5@gmail.com>
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

#include <dlfcn.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <glib.h>

#include "log.h"
#include "config/preferences.h"
#include "config/files.h"
#include "plugins/api.h"
#include "plugins/callbacks.h"
#include "plugins/disco.h"
#include "plugins/plugins.h"
#include "plugins/c_plugins.h"
#include "plugins/c_api.h"
#include "ui/ui.h"

void
c_env_init(void)
{
    c_api_init();
}

ProfPlugin*
c_plugin_create(const char *const filename)
{
    ProfPlugin *plugin;
    void *handle = NULL;

    char *plugins_dir = files_get_data_path(DIR_PLUGINS);
    GString *path = g_string_new(plugins_dir);
    free(plugins_dir);
    g_string_append(path, "/");
    g_string_append(path, filename);

    handle = dlopen (path->str, RTLD_NOW | RTLD_GLOBAL);

    if (!handle) {
        log_warning("dlopen failed to open `%s', %s", filename, dlerror ());
        g_string_free(path, TRUE);
        return NULL;
    }

    plugin = malloc(sizeof(ProfPlugin));
    plugin->name = strdup(filename);
    plugin->lang = LANG_C;
    plugin->module = handle;
    plugin->init_func = c_init_hook;
    plugin->contains_hook = c_contains_hook;
    plugin->on_start_func = c_on_start_hook;
    plugin->on_shutdown_func = c_on_shutdown_hook;
    plugin->on_unload_func = c_on_unload_hook;
    plugin->on_connect_func = c_on_connect_hook;
    plugin->on_disconnect_func = c_on_disconnect_hook;
    plugin->pre_chat_message_display = c_pre_chat_message_display_hook;
    plugin->post_chat_message_display = c_post_chat_message_display_hook;
    plugin->pre_chat_message_send = c_pre_chat_message_send_hook;
    plugin->post_chat_message_send = c_post_chat_message_send_hook;
    plugin->pre_room_message_display = c_pre_room_message_display_hook;
    plugin->post_room_message_display = c_post_room_message_display_hook;
    plugin->pre_room_message_send = c_pre_room_message_send_hook;
    plugin->post_room_message_send = c_post_room_message_send_hook;
    plugin->on_room_history_message = c_on_room_history_message_hook;
    plugin->pre_priv_message_display = c_pre_priv_message_display_hook;
    plugin->post_priv_message_display = c_post_priv_message_display_hook;
    plugin->pre_priv_message_send = c_pre_priv_message_send_hook;
    plugin->post_priv_message_send = c_post_priv_message_send_hook;
    plugin->on_message_stanza_send = c_on_message_stanza_send_hook;
    plugin->on_message_stanza_receive = c_on_message_stanza_receive_hook;
    plugin->on_presence_stanza_send = c_on_presence_stanza_send_hook;
    plugin->on_presence_stanza_receive = c_on_presence_stanza_receive_hook;
    plugin->on_iq_stanza_send = c_on_iq_stanza_send_hook;
    plugin->on_iq_stanza_receive = c_on_iq_stanza_receive_hook;
    plugin->on_contact_offline = c_on_contact_offline_hook;
    plugin->on_contact_presence = c_on_contact_presence_hook;
    plugin->on_chat_win_focus = c_on_chat_win_focus_hook;
    plugin->on_room_win_focus = c_on_room_win_focus_hook;

    g_string_free(path, TRUE);

    return plugin;
}

void
c_init_hook(ProfPlugin *plugin, const char *const version, const char *const status, const char *const account_name,
    const char *const fulljid)
{
    void *f = NULL;
    void (*func)(const char *const __version, const char *const __status, const char *const __account_name,
        const char *const __fulljid);

    assert(plugin && plugin->module);

    if (NULL == (f = dlsym(plugin->module, "prof_init"))) {
        log_warning ("warning: %s does not have init function", plugin->name);
        return;
    }

    func = (void (*)(const char *const, const char *const, const char *const, const char *const))f;

    // FIXME maybe we want to make it boolean to see if it succeeded or not?
    func(version, status, account_name, fulljid);
}

gboolean
c_contains_hook(ProfPlugin *plugin, const char *const hook)
{
    if (dlsym(plugin->module, hook)) {
        return TRUE;
    } else {
        return FALSE;
    }
}

void
c_on_start_hook(ProfPlugin *plugin)
{
    void *f = NULL;
    void (*func)(void);
    assert(plugin && plugin->module);

    if (NULL == (f = dlsym(plugin->module, "prof_on_start")))
        return;

    func = (void (*)(void))f;
    func();
}

void
c_on_shutdown_hook(ProfPlugin *plugin)
{
    void *f = NULL;
    void (*func)(void);
    assert(plugin && plugin->module);

    if (NULL == (f = dlsym(plugin->module, "prof_on_shutdown")))
        return;

    func = (void (*)(void))f;
    func();
}

void
c_on_unload_hook(ProfPlugin *plugin)
{
    void *f = NULL;
    void (*func)(void);
    assert(plugin && plugin->module);

    if (NULL == (f = dlsym(plugin->module, "prof_on_unload")))
        return;

    func = (void (*)(void))f;
    func();
}

void
c_on_connect_hook(ProfPlugin *plugin, const char *const account_name, const char *const fulljid)
{
    void *f = NULL;
    void (*func)(const char *const __account_name, const char *const __fulljid);
    assert(plugin && plugin->module);

    if (NULL == (f = dlsym(plugin->module, "prof_on_connect")))
        return;

    func = (void (*)(const char *const, const char *const))f;
    func(account_name, fulljid);
}

void
c_on_disconnect_hook(ProfPlugin *plugin, const char *const account_name, const char *const fulljid)
{
    void *f = NULL;
    void (*func)(const char *const __account_name, const char *const __fulljid);
    assert(plugin && plugin->module);

    if (NULL == (f = dlsym(plugin->module, "prof_on_disconnect")))
        return;

    func = (void (*)(const char *const, const char *const))f;
    func(account_name, fulljid);
}

char*
c_pre_chat_message_display_hook(ProfPlugin *plugin, const char *const barejid, const char *const resource, const char *message)
{
    void *f = NULL;
    char* (*func)(const char *const __barejid, const char *const __resource, const char *__message);
    assert(plugin && plugin->module);

    if (NULL == (f = dlsym(plugin->module, "prof_pre_chat_message_display")))
        return NULL;

    func = (char* (*)(const char *const, const char *const, const char *))f;
    return func(barejid, resource, message);
}

void
c_post_chat_message_display_hook(ProfPlugin *plugin, const char *const barejid, const char *const resource, const char *message)
{
    void *f = NULL;
    void (*func)(const char *const __barejid, const char *const __resource, const char *__message);
    assert(plugin && plugin->module);

    if (NULL == (f = dlsym(plugin->module, "prof_post_chat_message_display")))
        return;

    func = (void (*)(const char *const, const char *const, const char *))f;
    func(barejid, resource, message);
}

char*
c_pre_chat_message_send_hook(ProfPlugin *plugin, const char *const barejid, const char *message)
{
    void *f = NULL;
    char* (*func)(const char *const __barejid, const char *__message);
    assert(plugin && plugin->module);

    if (NULL == (f = dlsym(plugin->module, "prof_pre_chat_message_send")))
        return NULL;

    func = (char* (*)(const char *const, const char *))f;
    return func(barejid, message);
}

void
c_post_chat_message_send_hook(ProfPlugin *plugin, const char *const barejid, const char *message)
{
    void *f = NULL;
    void (*func)(const char *const __barejid, const char *__message);
    assert(plugin && plugin->module);

    if (NULL == (f = dlsym(plugin->module, "prof_post_chat_message_send")))
        return;

    func = (void (*)(const char *const, const char *))f;
    func(barejid, message);
}

char*
c_pre_room_message_display_hook(ProfPlugin *plugin, const char *const barejid, const char *const nick, const char *message)
{
    void *f = NULL;
    char* (*func)(const char *const __barejid, const char *const __nick, const char *__message);
    assert(plugin && plugin->module);

    if (NULL == (f = dlsym(plugin->module, "prof_pre_room_message_display")))
        return NULL;

    func = (char* (*)(const char *const, const char *const, const char *))f;
    return func(barejid, nick, message);
}

void
c_post_room_message_display_hook(ProfPlugin *plugin, const char *const barejid, const char *const nick,
    const char *message)
{
    void *f = NULL;
    void (*func)(const char *const __barejid, const char *const __nick, const char *__message);
    assert(plugin && plugin->module);

    if (NULL == (f = dlsym(plugin->module, "prof_post_room_message_display")))
        return;

    func = (void (*)(const char *const, const char *const, const char *))f;
    func(barejid, nick, message);
}

char*
c_pre_room_message_send_hook(ProfPlugin *plugin, const char *const barejid, const char *message)
{
    void *f = NULL;
    char* (*func)(const char *const __barejid, const char *__message);
    assert(plugin && plugin->module);

    if (NULL == (f = dlsym(plugin->module, "prof_pre_room_message_send")))
        return NULL;

    func = (char* (*)(const char *const, const char *))f;
    return func(barejid, message);
}

void
c_post_room_message_send_hook(ProfPlugin *plugin, const char *const barejid, const char *message)
{
    void *f = NULL;
    void (*func)(const char *const __barejid, const char *__message);
    assert(plugin && plugin->module);

    if (NULL == (f = dlsym(plugin->module, "prof_post_room_message_send")))
        return;

    func = (void (*)(const char *const, const char *))f;
    func(barejid, message);
}

void
c_on_room_history_message_hook(ProfPlugin *plugin, const char *const barejid, const char *const nick,
    const char *const message, const char *const timestamp)
{
    void *f = NULL;
    void (*func)(const char *const __barejid, const char *const __nick, const char *const __message,
        const char *const __timestamp);
    assert(plugin && plugin->module);

    if (NULL == (f = dlsym(plugin->module, "prof_on_room_history_message")))
        return;

    func = (void (*)(const char *const, const char *const, const char *const, const char *const))f;
    func(barejid, nick, message, timestamp);
}

char*
c_pre_priv_message_display_hook(ProfPlugin *plugin, const char *const barejid, const char *const nick, const char *message)
{
    void *f = NULL;
    char* (*func)(const char *const __barejid, const char *const __nick, const char *__message);
    assert(plugin && plugin->module);

    if (NULL == (f = dlsym(plugin->module, "prof_pre_priv_message_display")))
        return NULL;

    func = (char* (*)(const char *const, const char *const, const char *))f;
    return func(barejid, nick, message);
}

void
c_post_priv_message_display_hook(ProfPlugin *plugin, const char *const barejid, const char *const nick,
    const char *message)
{
    void *f = NULL;
    void (*func)(const char *const __barejid, const char *const __nick, const char *__message);
    assert(plugin && plugin->module);

    if (NULL == (f = dlsym(plugin->module, "prof_post_priv_message_display")))
        return;

    func = (void (*)(const char *const, const char *const, const char *))f;
    func(barejid, nick, message);
}

char*
c_pre_priv_message_send_hook(ProfPlugin *plugin, const char *const barejid, const char *const nick, const char *message)
{
    void *f = NULL;
    char* (*func)(const char *const __barejid, const char *const __nick, const char *__message);
    assert(plugin && plugin->module);

    if (NULL == (f = dlsym(plugin->module, "prof_pre_priv_message_send")))
        return NULL;

    func = (char* (*)(const char *const, const char *const, const char *))f;
    return func(barejid, nick, message);
}

void
c_post_priv_message_send_hook(ProfPlugin *plugin, const char *const barejid, const char *const nick, const char *message)
{
    void *f = NULL;
    void (*func)(const char *const __barejid, const char *const __nick, const char *__message);
    assert(plugin && plugin->module);

    if (NULL == (f = dlsym(plugin->module, "prof_post_priv_message_send")))
        return;

    func = (void (*)(const char *const, const char *const, const char *))f;
    func(barejid, nick, message);
}

char*
c_on_message_stanza_send_hook(ProfPlugin *plugin, const char *const text)
{
    void *f = NULL;
    char* (*func)(const char *const __text);
    assert(plugin && plugin->module);

    if (NULL == (f = dlsym(plugin->module, "prof_on_message_stanza_send")))
        return NULL;

    func = (char* (*)(const char *const))f;
    return func(text);
}

gboolean
c_on_message_stanza_receive_hook(ProfPlugin *plugin, const char *const text)
{
    void *f = NULL;
    int (*func)(const char *const __text);
    assert(plugin && plugin->module);

    if (NULL == (f = dlsym(plugin->module, "prof_on_message_stanza_receive")))
        return TRUE;

    func = (int (*)(const char *const))f;
    return func(text);
}

char*
c_on_presence_stanza_send_hook(ProfPlugin *plugin, const char *const text)
{
    void *f = NULL;
    char* (*func)(const char *const __text);
    assert(plugin && plugin->module);

    if (NULL == (f = dlsym(plugin->module, "prof_on_presence_stanza_send")))
        return NULL;

    func = (char* (*)(const char *const))f;
    return func(text);
}

gboolean
c_on_presence_stanza_receive_hook(ProfPlugin *plugin, const char *const text)
{
    void *f = NULL;
    int (*func)(const char *const __text);
    assert(plugin && plugin->module);

    if (NULL == (f = dlsym(plugin->module, "prof_on_presence_stanza_receive")))
        return TRUE;

    func = (int (*)(const char *const))f;
    return func(text);
}

char*
c_on_iq_stanza_send_hook(ProfPlugin *plugin, const char *const text)
{
    void *f = NULL;
    char* (*func)(const char *const __text);
    assert(plugin && plugin->module);

    if (NULL == (f = dlsym(plugin->module, "prof_on_iq_stanza_send")))
        return NULL;

    func = (char* (*)(const char *const))f;
    return func(text);
}

gboolean
c_on_iq_stanza_receive_hook(ProfPlugin *plugin, const char *const text)
{
    void *f = NULL;
    int (*func)(const char *const __text);
    assert(plugin && plugin->module);

    if (NULL == (f = dlsym(plugin->module, "prof_on_iq_stanza_receive")))
        return TRUE;

    func = (int (*)(const char *const))f;
    return func(text);
}

void
c_on_contact_offline_hook(ProfPlugin *plugin, const char *const barejid, const char *const resource,
    const char *const status)
{
    void *f = NULL;
    void (*func)(const char *const __barejid, const char *const __resource, const char *const __status);
    assert(plugin && plugin->module);

    if (NULL == (f = dlsym(plugin->module, "prof_on_contact_offline")))
        return;

    func = (void (*)(const char *const, const char *const, const char *const))f;
    func(barejid, resource, status);
}

void
c_on_contact_presence_hook(ProfPlugin *plugin, const char *const barejid, const char *const resource,
    const char *const presence, const char *const status, const int priority)
{
    void *f = NULL;
    void (*func)(const char *const __barejid, const char *const __resource, const char *const __presence,
        const char *const __status, const int __priority);
    assert(plugin && plugin->module);

    if (NULL == (f = dlsym(plugin->module, "prof_on_contact_presence")))
        return;

    func = (void (*)(const char *const, const char *const, const char *const, const char *const, const int))f;
    func(barejid, resource, presence, status, priority);
}

void
c_on_chat_win_focus_hook(ProfPlugin *plugin, const char *const barejid)
{
    void *f = NULL;
    void (*func)(const char *const __barejid);
    assert(plugin && plugin->module);

    if (NULL == (f = dlsym(plugin->module, "prof_on_chat_win_focus")))
        return;

    func = (void (*)(const char *const))f;
    func(barejid);
}

void
c_on_room_win_focus_hook(ProfPlugin *plugin, const char *const barejid)
{
    void *f = NULL;
    void (*func)(const char *const __barejid);
    assert(plugin && plugin->module);

    if (NULL == (f = dlsym(plugin->module, "prof_on_room_win_focus")))
        return;

    func = (void (*)(const char *const))f;
    func(barejid);
}

void
c_plugin_destroy(ProfPlugin *plugin)
{
    assert (plugin && plugin->module);

    callbacks_remove(plugin->name);

    disco_remove_features(plugin->name);

    if (dlclose (plugin->module)) {
        log_warning ("dlclose failed to close `%s' with `%s'", plugin->name, dlerror ());
    }

    free(plugin->name);
    free(plugin);
}

void
c_shutdown(void)
{

}
