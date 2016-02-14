/*
 * python_api.c
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

#include <Python.h>

#include <glib.h>

#include "plugins/api.h"
#include "plugins/python_api.h"
#include "plugins/python_plugins.h"
#include "plugins/callbacks.h"
#include "plugins/autocompleters.h"

static PyObject*
python_api_cons_alert(PyObject *self, PyObject *args)
{
    api_cons_alert();
    return Py_BuildValue("");
}

static PyObject*
python_api_cons_show(PyObject *self, PyObject *args)
{
    const char *message = NULL;
    if (!PyArg_ParseTuple(args, "s", &message)) {
        return Py_BuildValue("");
    }
    api_cons_show(message);
    return Py_BuildValue("");
}

static PyObject*
python_api_register_command(PyObject *self, PyObject *args)
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
        return Py_BuildValue("");
    }

    if (p_callback && PyCallable_Check(p_callback)) {
        api_register_command(command_name, min_args, max_args, usage,
            short_help, long_help, p_callback, python_command_callback);
    }

    return Py_BuildValue("");
}

static PyObject *
python_api_register_timed(PyObject *self, PyObject *args)
{
    PyObject *p_callback = NULL;
    int interval_seconds = 0;

    if (!PyArg_ParseTuple(args, "Oi", &p_callback, &interval_seconds)) {
        return Py_BuildValue("");
    }

    if (p_callback && PyCallable_Check(p_callback)) {
        api_register_timed(p_callback, interval_seconds, python_timed_callback);
    }

    return Py_BuildValue("");
}

static PyObject *
python_api_register_ac(PyObject *self, PyObject *args)
{
    const char *key = NULL;
    PyObject *items = NULL;

    if (!PyArg_ParseTuple(args, "sO", &key, &items)) {
        return Py_BuildValue("");
    }

    Py_ssize_t len = PyList_Size(items);
    char *c_items[len];

    Py_ssize_t i = 0;
    for (i = 0; i < len; i++) {
        PyObject *item = PyList_GetItem(items, i);
        char *c_item = PyString_AsString(item);
        c_items[i] = c_item;
    }
    c_items[len] = NULL;

    autocompleters_add(key, c_items);
    return Py_BuildValue("");
}

static PyObject*
python_api_notify(PyObject *self, PyObject *args)
{
    const char *message = NULL;
    const char *category = NULL;
    int timeout_ms = 5000;

    if (!PyArg_ParseTuple(args, "sis", &message, &timeout_ms, &category)) {
        return Py_BuildValue("");
    }

    api_notify(message, category, timeout_ms);

    return Py_BuildValue("");
}

static PyObject*
python_api_send_line(PyObject *self, PyObject *args)
{
    char *line = NULL;
    if (!PyArg_ParseTuple(args, "s", &line)) {
        return Py_BuildValue("");
    }

    api_send_line(line);

    return Py_BuildValue("");
}

static PyObject *
python_api_get_current_recipient(PyObject *self, PyObject *args)
{
    char *recipient = api_get_current_recipient();
    if (recipient) {
        return Py_BuildValue("s", recipient);
    } else {
        return Py_BuildValue("");
    }
}

static PyObject *
python_api_get_current_muc(PyObject *self, PyObject *args)
{
    char *room = api_get_current_muc();
    if (room) {
        return Py_BuildValue("s", room);
    } else {
        return Py_BuildValue("");
    }
}

static PyObject *
python_api_log_debug(PyObject *self, PyObject *args)
{
    const char *message = NULL;
    if (!PyArg_ParseTuple(args, "s", &message)) {
        return Py_BuildValue("");
    }
    api_log_debug(message);
    return Py_BuildValue("");
}

static PyObject *
python_api_log_info(PyObject *self, PyObject *args)
{
    const char *message = NULL;
    if (!PyArg_ParseTuple(args, "s", &message)) {
        return Py_BuildValue("");
    }
    api_log_info(message);
    return Py_BuildValue("");
}

static PyObject *
python_api_log_warning(PyObject *self, PyObject *args)
{
    const char *message = NULL;
    if (!PyArg_ParseTuple(args, "s", &message)) {
        return Py_BuildValue("");
    }
    api_log_warning(message);
    return Py_BuildValue("");
}

static PyObject *
python_api_log_error(PyObject *self, PyObject *args)
{
    const char *message = NULL;
    if (!PyArg_ParseTuple(args, "s", &message)) {
        return Py_BuildValue("");
    }
    api_log_error(message);
    return Py_BuildValue("");
}

static PyObject *
python_api_win_exists(PyObject *self, PyObject *args)
{
    char *tag = NULL;
    if (!PyArg_ParseTuple(args, "s", &tag)) {
        return Py_BuildValue("");
    }

    if (api_win_exists(tag)) {
        return Py_BuildValue("i", 1);
    } else {
        return Py_BuildValue("i", 0);
    }
}

static PyObject *
python_api_win_create(PyObject *self, PyObject *args)
{
    char *tag = NULL;
    PyObject *p_callback = NULL;

    if (!PyArg_ParseTuple(args, "sO", &tag, &p_callback)) {
        return Py_BuildValue("");
    }

    if (p_callback && PyCallable_Check(p_callback)) {
        api_win_create(tag, p_callback, python_window_callback);
    }

    return Py_BuildValue("");
}

static PyObject *
python_api_win_focus(PyObject *self, PyObject *args)
{
    char *tag = NULL;

    if (!PyArg_ParseTuple(args, "s", &tag)) {
        return Py_BuildValue("");
    }

    api_win_focus(tag);
    return Py_BuildValue("");
}

static PyObject *
python_api_win_show(PyObject *self, PyObject *args)
{
    char *tag = NULL;
    char *line = NULL;

    if (!PyArg_ParseTuple(args, "ss", &tag, &line)) {
        return Py_BuildValue("");
    }

    api_win_show(tag, line);
    return Py_BuildValue("");
}

static PyObject *
python_api_win_show_green(PyObject *self, PyObject *args)
{
    char *tag = NULL;
    char *line = NULL;

    if (!PyArg_ParseTuple(args, "ss", &tag, &line)) {
        return Py_BuildValue("");
    }

    api_win_show_green(tag, line);
    return Py_BuildValue("");
}

static PyObject *
python_api_win_show_red(PyObject *self, PyObject *args)
{
    char *tag = NULL;
    char *line = NULL;

    if (!PyArg_ParseTuple(args, "ss", &tag, &line)) {
        return Py_BuildValue("");
    }

    api_win_show_red(tag, line);
    return Py_BuildValue("");
}

static PyObject *
python_api_win_show_cyan(PyObject *self, PyObject *args)
{
    char *tag = NULL;
    char *line = NULL;

    if (!PyArg_ParseTuple(args, "ss", &tag, &line)) {
        return Py_BuildValue("");
    }

    api_win_show_cyan(tag, line);
    return Py_BuildValue("");
}

static PyObject *
python_api_win_show_yellow(PyObject *self, PyObject *args)
{
    char *tag = NULL;
    char *line = NULL;

    if (!PyArg_ParseTuple(args, "ss", &tag, &line)) {
        return Py_BuildValue("");
    }

    api_win_show_yellow(tag, line);
    return Py_BuildValue("");
}

void
python_command_callback(PluginCommand *command, gchar **args)
{
    disable_python_threads();
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
    allow_python_threads();
}

void
python_timed_callback(PluginTimedFunction *timed_function)
{
    disable_python_threads();
    PyObject_CallObject(timed_function->callback, NULL);
    allow_python_threads();
}

void
python_window_callback(PluginWindowCallback *window_callback, char *tag, char *line)
{
    disable_python_threads();
    PyObject *p_args = NULL;
    p_args = Py_BuildValue("ss", tag, line);
    PyObject_CallObject(window_callback->callback, p_args);
    Py_XDECREF(p_args);

    if (PyErr_Occurred()) {
        PyErr_Print();
        PyErr_Clear();
    }
    allow_python_threads();
}

static PyMethodDef apiMethods[] = {
    { "cons_alert", python_api_cons_alert, METH_NOARGS, "Highlight the console window in the status bar." },
    { "cons_show", python_api_cons_show, METH_VARARGS, "Print a line to the console." },
    { "register_command", python_api_register_command, METH_VARARGS, "Register a command." },
    { "register_timed", python_api_register_timed, METH_VARARGS, "Register a timed function." },
    { "register_ac", python_api_register_ac, METH_VARARGS, "Register an autocompleter." },
    { "send_line", python_api_send_line, METH_VARARGS, "Send a line of input." },
    { "notify", python_api_notify, METH_VARARGS, "Send desktop notification." },
    { "get_current_recipient", python_api_get_current_recipient, METH_VARARGS, "Return the jid of the recipient of the current window." },
    { "get_current_muc", python_api_get_current_muc, METH_VARARGS, "Return the jid of the room of the current window." },
    { "log_debug", python_api_log_debug, METH_VARARGS, "Log a debug message" },
    { "log_info", python_api_log_info, METH_VARARGS, "Log an info message" },
    { "log_warning", python_api_log_warning, METH_VARARGS, "Log a warning message" },
    { "log_error", python_api_log_error, METH_VARARGS, "Log an error message" },
    { "win_exists", python_api_win_exists, METH_VARARGS, "Determine whether a window exists." },
    { "win_create", python_api_win_create, METH_VARARGS, "Create a new window." },
    { "win_focus", python_api_win_focus, METH_VARARGS, "Focus a window." },
    { "win_show", python_api_win_show, METH_VARARGS, "Show text in the window." },
    { "win_show_green", python_api_win_show_green, METH_VARARGS, "Show green text in the window." },
    { "win_show_red", python_api_win_show_red, METH_VARARGS, "Show red text in the window." },
    { "win_show_cyan", python_api_win_show_cyan, METH_VARARGS, "Show cyan text in the window." },
    { "win_show_yellow", python_api_win_show_yellow, METH_VARARGS, "Show yellow text in the window." },
    { NULL, NULL, 0, NULL }
};

void
python_api_init(void)
{
    Py_InitModule("prof", apiMethods);
}
