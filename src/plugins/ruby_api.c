/*
 * ruby_api.c
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

#include <Python.h>
#include <ruby.h>

#include <glib.h>

#include "plugins/api.h"
#include "plugins/ruby_api.h"
#include "plugins/callbacks.h"

static VALUE
ruby_api_cons_alert(VALUE self)
{
    api_cons_alert();
    return Qnil;
}

static VALUE
ruby_api_cons_show(VALUE self, VALUE v_message)
{
    char *message = STR2CSTR(v_message);

    if (message != NULL) {
        api_cons_show(message);
    }
    return self;
}

static VALUE
ruby_api_register_command(VALUE self, VALUE v_command_name, VALUE v_min_args,
    VALUE v_max_args, VALUE v_usage, VALUE v_short_help, VALUE v_long_help,
    VALUE v_callback)
{
    const char *command_name = STR2CSTR(v_command_name);
    int min_args = NUM2INT(v_min_args);
    int max_args = NUM2INT(v_max_args);
    const char *usage = STR2CSTR(v_usage);
    const char *short_help = STR2CSTR(v_short_help);
    const char *long_help = STR2CSTR(v_long_help);

    api_register_command(command_name, min_args, max_args, usage,
        short_help, long_help, (void *)v_callback, ruby_command_callback);

    return Qnil;
}

static VALUE
ruby_api_register_timed(VALUE self, VALUE v_callback, VALUE v_interval_seconds)
{
    int interval_seconds = NUM2INT(v_interval_seconds);

    api_register_timed((void*)v_callback, interval_seconds, ruby_timed_callback);

    return Qnil;
}

static VALUE
ruby_api_notify(VALUE self, VALUE v_message, VALUE v_timeout_ms, VALUE v_category)
{
    const char *message = STR2CSTR(v_message);
    const char *category = STR2CSTR(v_category);
    int timeout_ms = NUM2INT(v_timeout_ms);

    api_notify(message, category, timeout_ms);

    return Qnil;
}

static VALUE
ruby_api_send_line(VALUE self, VALUE v_line)
{
    char *line = STR2CSTR(v_line);

    api_send_line(line);

    return Qnil;
}

static VALUE
ruby_api_get_current_recipient(VALUE self)
{
    char *recipient = api_get_current_recipient();
    if (recipient != NULL) {
        return rb_str_new2(recipient);
    } else {
        return Qnil;
    }
}

void
ruby_command_callback(PluginCommand *command, gchar **args)
{
    PyObject *p_args = NULL;
    int num_args = g_strv_length(args);
    if (num_args == 0) {
        if (command->max_args == 1) {
            p_args = Py_BuildValue("(O)", Py_BuildValue(""));
            PyObject_CallObject(command->callback, p_args);
            Py_XDECREF(p_args);
        } else {
            PyObject_CallObject(command->callback, p_args);
        }
    } else if (num_args == 1) {
        p_args = Py_BuildValue("(s)", args[0]);
        PyObject_CallObject(command->callback, p_args);
        Py_XDECREF(p_args);
    } else if (num_args == 2) {
        p_args = Py_BuildValue("ss", args[0], args[1]);
        PyObject_CallObject(command->callback, p_args);
        Py_XDECREF(p_args);
    } else if (num_args == 3) {
        p_args = Py_BuildValue("sss", args[0], args[1], args[2]);
        PyObject_CallObject(command->callback, p_args);
        Py_XDECREF(p_args);
    } else if (num_args == 4) {
        p_args = Py_BuildValue("ssss", args[0], args[1], args[2], args[3]);
        PyObject_CallObject(command->callback, p_args);
        Py_XDECREF(p_args);
    } else if (num_args == 5) {
        p_args = Py_BuildValue("sssss", args[0], args[1], args[2], args[3], args[4]);
        PyObject_CallObject(command->callback, p_args);
        Py_XDECREF(p_args);
    }

    if (PyErr_Occurred()) {
        PyErr_Print();
        PyErr_Clear();
    }
}

void
ruby_timed_callback(PluginTimedFunction *timed_function)
{
    PyObject_CallObject(timed_function->callback, NULL);
}

static VALUE prof_module;

void
ruby_api_init(void)
{
    prof_module = rb_define_module("prof");
    rb_define_module_function(prof_module, "cons_alert", RUBY_METHOD_FUNC(ruby_api_cons_alert), 0);
    rb_define_module_function(prof_module, "cons_show", RUBY_METHOD_FUNC(ruby_api_cons_show), 1);
    rb_define_module_function(prof_module, "register_command", RUBY_METHOD_FUNC(ruby_api_register_command), 7);
    rb_define_module_function(prof_module, "register_timed", RUBY_METHOD_FUNC(ruby_api_register_timed), 2);
    rb_define_module_function(prof_module, "send_line", RUBY_METHOD_FUNC(ruby_api_send_line), 1);
    rb_define_module_function(prof_module, "notify", RUBY_METHOD_FUNC(ruby_api_notify), 3);
    rb_define_module_function(prof_module, "get_current_recipient", RUBY_METHOD_FUNC(ruby_api_get_current_recipient), 0);
}
