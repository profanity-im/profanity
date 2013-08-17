/*
 * python_api.c
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

#include <glib.h>

#include "plugins/api.h"
#include "profanity.h"
#include "ui/notifier.h"
#include "ui/ui.h"

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
        return NULL;
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
        return NULL;
    }

    if (p_callback && PyCallable_Check(p_callback)) {
        api_register_command(command_name, min_args, max_args, usage,
            short_help, long_help, p_callback);
    }

    return Py_BuildValue("");
}

static PyObject *
python_api_register_timed(PyObject *self, PyObject *args)
{
    PyObject *p_callback = NULL;
    int interval_seconds = 0;

    if (!PyArg_ParseTuple(args, "Oi", &p_callback, &interval_seconds)) {
        return NULL;
    }

    if (p_callback && PyCallable_Check(p_callback)) {
        api_register_timed(p_callback, interval_seconds);
    }

    return Py_BuildValue("");
}

static PyObject*
python_api_notify(PyObject *self, PyObject *args)
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
python_api_send_line(PyObject *self, PyObject *args)
{
    char *line = NULL;
    if (!PyArg_ParseTuple(args, "s", &line)) {
        return NULL;
    }

    api_send_line(line);

    return Py_BuildValue("");
}

static PyObject *
python_api_get_current_recipient(PyObject *self, PyObject *args)
{
    char *recipient = api_get_current_recipient();
    if (recipient != NULL) {
        return Py_BuildValue("s", recipient);
    } else {
        return Py_BuildValue("");
    }
}

static PyMethodDef apiMethods[] = {
    { "cons_alert", python_api_cons_alert, METH_NOARGS, "Highlight the console window in the status bar." },
    { "cons_show", python_api_cons_show, METH_VARARGS, "Print a line to the console." },
    { "register_command", python_api_register_command, METH_VARARGS, "Register a command." },
    { "register_timed", python_api_register_timed, METH_VARARGS, "Register a timed function." },
    { "send_line", python_api_send_line, METH_VARARGS, "Send a line of input." },
    { "notify", python_api_notify, METH_VARARGS, "Send desktop notification." },
    { "get_current_recipient", python_api_get_current_recipient, METH_VARARGS, "Return the jid of the recipient of the current window." },
    { NULL, NULL, 0, NULL }
};

void
python_api_init(void)
{
    Py_InitModule("prof", apiMethods);
}
