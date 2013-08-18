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

static PyObject*
ruby_api_cons_alert(PyObject *self, PyObject *args)
{
    api_cons_alert();
    return Py_BuildValue("");
}

static VALUE
ruby_api_cons_show(VALUE self, const char * const message)
{
    if (message != NULL) {
        api_cons_show(message);
    }
    return self;
}

static PyObject*
ruby_api_register_command(PyObject *self, PyObject *args)
{
    const char *command_name = NULL;
    int min_args = 0;
    int max_args = 0;
    const char *usage = NULL;
    const char *short_help = NULL;
    const char *long_help = NULL;
    PyObject *p_callback = NULL;

    if (!PyArg_ParseTuple(args, "siisssO", &command_name, &min_args, &max_args,
            &usage, &short_help, &long_help, &p_callback)) {
        return NULL;
    }

    if (p_callback && PyCallable_Check(p_callback)) {
        api_register_command(command_name, min_args, max_args, usage,
            short_help, long_help, p_callback, ruby_command_callback);
    }

    return Py_BuildValue("");
}

static PyObject *
ruby_api_register_timed(PyObject *self, PyObject *args)
{
    PyObject *p_callback = NULL;
    int interval_seconds = 0;

    if (!PyArg_ParseTuple(args, "Oi", &p_callback, &interval_seconds)) {
        return NULL;
    }

    if (p_callback && PyCallable_Check(p_callback)) {
        api_register_timed(p_callback, interval_seconds, ruby_timed_callback);
    }

    return Py_BuildValue("");
}

static PyObject*
ruby_api_notify(PyObject *self, PyObject *args)
{
    const char *message = NULL;
    const char *category = NULL;
    int timeout_ms = 5000;

    if (!PyArg_ParseTuple(args, "sis", &message, &timeout_ms, &category)) {
        return NULL;
    }

    api_notify(message, category, timeout_ms);

    return Py_BuildValue("");
}

static PyObject*
ruby_api_send_line(PyObject *self, PyObject *args)
{
    char *line = NULL;
    if (!PyArg_ParseTuple(args, "s", &line)) {
        return NULL;
    }

    api_send_line(line);

    return Py_BuildValue("");
}

static PyObject *
ruby_api_get_current_recipient(PyObject *self, PyObject *args)
{
    char *recipient = api_get_current_recipient();
    if (recipient != NULL) {
        return Py_BuildValue("s", recipient);
    } else {
        return Py_BuildValue("");
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
    rb_define_module_function(prof_module, "cons_show", RUBY_METHOD_FUNC(ruby_api_cons_show), 1);
}
