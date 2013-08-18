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
    VALUE exists = rb_eval_string("RubyTest.respond_to?(:prof_init)");
    if (TYPE(exists) == T_TRUE) {
        rb_funcall(module, rb_intern("prof_init"), 2, v_version, v_status);
    }
}

void
ruby_on_start_hook(ProfPlugin *plugin)
{
    VALUE module = (VALUE) plugin->module;
    VALUE exists = rb_eval_string("RubyTest.respond_to?(:prof_on_start)");
    if (TYPE(exists) == T_TRUE) {
        rb_funcall(module, rb_intern("prof_on_start"), 0);
    }
}

void
ruby_on_connect_hook(ProfPlugin *plugin)
{
    VALUE module = (VALUE) plugin->module;
    VALUE exists = rb_eval_string("RubyTest.respond_to?(:prof_on_connect)");
    if (TYPE(exists) == T_TRUE) {
        rb_funcall(module, rb_intern("prof_on_connect"), 0);
    }
}

void
ruby_on_message_received_hook(ProfPlugin *plugin, const char * const jid, const char * const message)
{
    VALUE v_jid = rb_str_new2(jid);
    VALUE v_message = rb_str_new2(message);

    VALUE module = (VALUE) plugin->module;
    VALUE exists = rb_eval_string("RubyTest.respond_to?(:prof_on_message_received)");
    if (TYPE(exists) == T_TRUE) {
        rb_funcall(module, rb_intern("prof_on_message_received"), 2, v_jid, v_message);
    }
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
