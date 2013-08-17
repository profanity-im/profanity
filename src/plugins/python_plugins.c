/*
 * python_plugins.c
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

#include "config/preferences.h"
#include "plugins/api.h"
#include "plugins/callbacks.h"
#include "plugins/plugins.h"
#include "plugins/python_api.h"
#include "plugins/python_plugins.h"
#include "ui/ui.h"

void
python_init(void)
{
    Py_Initialize();
    python_check_error();
    python_api_init();
    python_check_error();
    // TODO change to use XDG spec
    GString *path = g_string_new(Py_GetPath());
    g_string_append(path, ":./plugins/");
    PySys_SetPath(path->str);
    python_check_error();
    g_string_free(path, TRUE);
}

ProfPlugin *
python_plugin_create(const char * const filename)
{
    gchar *module_name = g_strndup(filename, strlen(filename) - 3);
    PyObject *p_module = PyImport_ImportModule(module_name);
    python_check_error();
    if (p_module != NULL) {
        ProfPlugin *plugin = malloc(sizeof(ProfPlugin));
        plugin->name = module_name;
        plugin->lang = PYTHON;
        plugin->module = p_module;
        plugin->init_func = python_init_hook;
        plugin->on_start_func = python_on_start_hook;
        plugin->on_connect_func = python_on_connect_hook;
        plugin->on_message_received_func = python_on_message_received_hook;
        g_free(module_name);
        return plugin;
    } else {
        g_free(module_name);
        return NULL;
    }
}

void
python_init_hook(ProfPlugin *plugin, const char * const version, const char * const status)
{
    PyObject *p_args = Py_BuildValue("ss", version, status);
    PyObject *p_function;

    PyObject *p_module = plugin->module;
    if (PyObject_HasAttrString(p_module, "prof_init")) {
        p_function = PyObject_GetAttrString(p_module, "prof_init");
        python_check_error();
        if (p_function && PyCallable_Check(p_function)) {
            PyObject_CallObject(p_function, p_args);
            python_check_error();
            Py_XDECREF(p_function);
        }
    }
}

void
python_on_start_hook(ProfPlugin *plugin)
{
    PyObject *p_function;

    PyObject *p_module = plugin->module;
    if (PyObject_HasAttrString(p_module, "prof_on_start")) {
        p_function = PyObject_GetAttrString(p_module, "prof_on_start");
        python_check_error();
        if (p_function && PyCallable_Check(p_function)) {
            PyObject_CallObject(p_function, NULL);
            python_check_error();
            Py_XDECREF(p_function);
        }
    }
}

void
python_on_connect_hook(ProfPlugin *plugin)
{
    PyObject *p_function;

    PyObject *p_module = plugin->module;
    if (PyObject_HasAttrString(p_module, "prof_on_connect")) {
        p_function = PyObject_GetAttrString(p_module, "prof_on_connect");
        python_check_error();
        if (p_function && PyCallable_Check(p_function)) {
            PyObject_CallObject(p_function, NULL);
            python_check_error();
            Py_XDECREF(p_function);
        }
    }
}

void
python_on_message_received_hook(ProfPlugin *plugin, const char * const jid, const char * const message)
{
    PyObject *p_args = Py_BuildValue("ss", jid, message);
    PyObject *p_function;

    PyObject *p_module = plugin->module;
    if (PyObject_HasAttrString(p_module, "prof_on_message_received")) {
        p_function = PyObject_GetAttrString(p_module, "prof_on_message_received");
        python_check_error();
        if (p_function && PyCallable_Check(p_function)) {
            PyObject_CallObject(p_function, p_args);
            python_check_error();
            Py_XDECREF(p_function);
        }
    }
}

void
python_check_error(void)
{
    if (PyErr_Occurred()) {
        PyErr_Print();
        PyErr_Clear();
    }
}

void
python_shutdown(void)
{
    Py_Finalize();
}
