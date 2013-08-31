/*
 * ruby_plugins.c
 *
 * Copyright (C) 2012, 2013 James Booth <boothj5@gmail.com>
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
 */

#include <ruby.h>

#include "config/preferences.h"
#include "plugins/api.h"
#include "plugins/callbacks.h"
#include "plugins/plugins.h"
#include "plugins/ruby_api.h"
#include "plugins/ruby_plugins.h"
#include "ui/ui.h"

static gboolean _method_exists(ProfPlugin *plugin, const char * const name);

void
ruby_env_init(void)
{
    ruby_init();
    ruby_init_loadpath();
    ruby_api_init();
    ruby_check_error();
}

ProfPlugin *
ruby_plugin_create(const char * const filename)
{
    GString *path = g_string_new("./plugins/");
    g_string_append(path, filename);
    rb_require(path->str);
    gchar *module_name = g_strndup(filename, strlen(filename) - 3);
    ruby_check_error();

    ProfPlugin *plugin = malloc(sizeof(ProfPlugin));
    plugin->name = module_name;
    plugin->lang = LANG_RUBY;
    plugin->module = (void *)rb_const_get(rb_cObject, rb_intern(module_name));
    plugin->init_func = ruby_init_hook;
    plugin->on_start_func = ruby_on_start_hook;
    plugin->on_connect_func = ruby_on_connect_hook;
    plugin->on_message_received_func = ruby_on_message_received_hook;
    return plugin;
}

void
ruby_init_hook(ProfPlugin *plugin, const char * const version, const char * const status)
{
    VALUE v_version = rb_str_new2(version);
    VALUE v_status = rb_str_new2(status);

    VALUE module = (VALUE) plugin->module;
    if (_method_exists(plugin, "prof_init")) {
        rb_funcall(module, rb_intern("prof_init"), 2, v_version, v_status);
    }
}

void
ruby_on_start_hook(ProfPlugin *plugin)
{
    VALUE module = (VALUE) plugin->module;
    if (_method_exists(plugin, "prof_on_start")) {
        rb_funcall(module, rb_intern("prof_on_start"), 0);
    }
}

void
ruby_on_connect_hook(ProfPlugin *plugin, const char * const account_name,
    const  char * const fulljid)
{
    VALUE v_account_name = rb_str_new2(account_name);
    VALUE v_fulljid = rb_str_new2(fulljid);
    VALUE module = (VALUE) plugin->module;
    if (_method_exists(plugin, "prof_on_connect")) {
        rb_funcall(module, rb_intern("prof_on_connect"), 2, v_account_name, v_fulljid);
    }
}

char *
ruby_on_message_received_hook(ProfPlugin *plugin, const char * const jid,
    const char *message)
{
    VALUE v_jid = rb_str_new2(jid);
    VALUE v_message = rb_str_new2(message);

    VALUE module = (VALUE) plugin->module;
    if (_method_exists(plugin, "prof_on_message_received")) {
        VALUE result = rb_funcall(module, rb_intern("prof_on_message_received"), 2, v_jid, v_message);
        if (TYPE(result) != T_NIL) {
            char *result_str = STR2CSTR(result);
            rb_gc_unregister_address(&result);
            return result_str;
        } else {
            return NULL;
        }
    }

    return NULL;
}

void
ruby_check_error(void)
{
}

void
ruby_shutdown(void)
{
    ruby_finalize();
}

static gboolean
_method_exists(ProfPlugin *plugin, const char * const name)
{
    gboolean result = FALSE;
    GString *respond = g_string_new(plugin->name);
    g_string_append(respond, ".respond_to?(:");
    g_string_append(respond, name);
    g_string_append(respond, ")");

    VALUE exists = rb_eval_string(respond->str);
    if (TYPE(exists) == T_TRUE) {
        result = TRUE;
    }

    g_string_free(respond, TRUE);

    return result;
}
