/*
 * python_plugins.c
 *
 * Copyright (C) 2012 - 2014 James Booth <boothj5@gmail.com>
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

static PyThreadState *thread_state;

void
allow_python_threads()
{
    thread_state = PyEval_SaveThread();
}

void
disable_python_threads()
{
    PyEval_RestoreThread(thread_state);
}

void
python_env_init(void)
{
    Py_Initialize();
    PyEval_InitThreads();
    python_api_init();
    GString *path = g_string_new(Py_GetPath());
    g_string_append(path, ":");
    gchar *plugins_dir = plugins_get_dir();
    g_string_append(path, plugins_dir);
    g_string_append(path, "/");
    g_free(plugins_dir);
    g_string_append(path, ":");
    g_string_append(path, PROF_PYTHON_SITE_PATH);
    PySys_SetPath(path->str);
    g_string_free(path, TRUE);
    allow_python_threads();
}

ProfPlugin *
python_plugin_create(const char * const filename)
{
    disable_python_threads();
    gchar *module_name = g_strndup(filename, strlen(filename) - 3);
    PyObject *p_module = PyImport_ImportModule(module_name);
    python_check_error();
    if (p_module != NULL) {
        ProfPlugin *plugin = malloc(sizeof(ProfPlugin));
        plugin->name = strdup(module_name);
        plugin->lang = LANG_PYTHON;
        plugin->module = p_module;
        plugin->init_func = python_init_hook;
        plugin->on_start_func = python_on_start_hook;
        plugin->on_connect_func = python_on_connect_hook;
        plugin->on_disconnect_func = python_on_disconnect_hook;
        plugin->before_message_displayed_func = python_before_message_displayed_hook;
        plugin->on_message_received_func = python_on_message_received_hook;
        plugin->on_room_message_received_func = python_on_room_message_received_hook;
        plugin->on_private_message_received_func = python_on_private_message_received_hook;
        plugin->on_message_send_func = python_on_message_send_hook;
        plugin->on_private_message_send_func = python_on_private_message_send_hook;
        plugin->on_room_message_send_func = python_on_room_message_send_hook;
        plugin->on_shutdown_func = python_on_shutdown_hook;
        g_free(module_name);

        allow_python_threads();
        return plugin;
    } else {
        g_free(module_name);
        allow_python_threads();
        return NULL;
    }
}

void
python_init_hook(ProfPlugin *plugin, const char * const version, const char * const status)
{
    disable_python_threads();
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
    allow_python_threads();
}

void
python_on_start_hook(ProfPlugin *plugin)
{
    disable_python_threads();
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
    allow_python_threads();
}

void
python_on_connect_hook(ProfPlugin *plugin, const char * const account_name,
    const char * const fulljid)
{
    disable_python_threads();
    PyObject *p_args = Py_BuildValue("ss", account_name, fulljid);
    PyObject *p_function;

    PyObject *p_module = plugin->module;
    if (PyObject_HasAttrString(p_module, "prof_on_connect")) {
        p_function = PyObject_GetAttrString(p_module, "prof_on_connect");
        python_check_error();
        if (p_function && PyCallable_Check(p_function)) {
            PyObject_CallObject(p_function, p_args);
            python_check_error();
            Py_XDECREF(p_function);
        }
    }
    allow_python_threads();
}

void
python_on_disconnect_hook(ProfPlugin *plugin, const char * const account_name,
    const char * const fulljid)
{
    disable_python_threads();
    PyObject *p_args = Py_BuildValue("ss", account_name, fulljid);
    PyObject *p_function;

    PyObject *p_module = plugin->module;
    if (PyObject_HasAttrString(p_module, "prof_on_disconnect")) {
        p_function = PyObject_GetAttrString(p_module, "prof_on_disconnect");
        python_check_error();
        if (p_function && PyCallable_Check(p_function)) {
            PyObject_CallObject(p_function, p_args);
            python_check_error();
            Py_XDECREF(p_function);
        }
    }
    allow_python_threads();
}

char *
python_before_message_displayed_hook(ProfPlugin *plugin, const char *message)
{
    disable_python_threads();
    PyObject *p_args = Py_BuildValue("(s)", message);
    PyObject *p_function;

    PyObject *p_module = plugin->module;
    if (PyObject_HasAttrString(p_module, "prof_before_message_displayed")) {
        p_function = PyObject_GetAttrString(p_module, "prof_before_message_displayed");
        python_check_error();
        if (p_function && PyCallable_Check(p_function)) {
            PyObject *result = PyObject_CallObject(p_function, p_args);
            python_check_error();
            Py_XDECREF(p_function);
            if (PyUnicode_Check(result)) {
                char *result_str = strdup(PyString_AsString(PyUnicode_AsUTF8String(result)));
                Py_XDECREF(result);
                allow_python_threads();
                return result_str;
            } else if (result != Py_None) {
                char *result_str = strdup(PyString_AsString(result));
                Py_XDECREF(result);
                allow_python_threads();
                return result_str;
            } else {
                allow_python_threads();
                return NULL;
            }
        }
    }
    allow_python_threads();
    return NULL;
}

char *
python_on_message_received_hook(ProfPlugin *plugin, const char * const jid,
    const char *message)
{
    disable_python_threads();
    PyObject *p_args = Py_BuildValue("ss", jid, message);
    PyObject *p_function;

    PyObject *p_module = plugin->module;
    if (PyObject_HasAttrString(p_module, "prof_on_message_received")) {
        p_function = PyObject_GetAttrString(p_module, "prof_on_message_received");
        python_check_error();
        if (p_function && PyCallable_Check(p_function)) {
            PyObject *result = PyObject_CallObject(p_function, p_args);
            python_check_error();
            Py_XDECREF(p_function);
            if (PyUnicode_Check(result)) {
                char *result_str = strdup(PyString_AsString(PyUnicode_AsUTF8String(result)));
                Py_XDECREF(result);
                allow_python_threads();
                return result_str;
            } else if (result != Py_None) {
                char *result_str = strdup(PyString_AsString(result));
                Py_XDECREF(result);
                allow_python_threads();
                return result_str;
            } else {
                allow_python_threads();
                return NULL;
            }
        }
    }

    allow_python_threads();
    return NULL;
}

char *
python_on_private_message_received_hook(ProfPlugin *plugin, const char * const room,
    const char * const nick, const char *message)
{
    disable_python_threads();
    PyObject *p_args = Py_BuildValue("sss", room, nick, message);
    PyObject *p_function;

    PyObject *p_module = plugin->module;
    if (PyObject_HasAttrString(p_module, "prof_on_private_message_received")) {
        p_function = PyObject_GetAttrString(p_module, "prof_on_private_message_received");
        python_check_error();
        if (p_function && PyCallable_Check(p_function)) {
            PyObject *result = PyObject_CallObject(p_function, p_args);
            python_check_error();
            Py_XDECREF(p_function);
            if (PyUnicode_Check(result)) {
                char *result_str = strdup(PyString_AsString(PyUnicode_AsUTF8String(result)));
                Py_XDECREF(result);
                allow_python_threads();
                return result_str;
            } else if (result != Py_None) {
                char *result_str = strdup(PyString_AsString(result));
                Py_XDECREF(result);
                allow_python_threads();
                return result_str;
            } else {
                allow_python_threads();
                return NULL;
            }
        }
    }

    allow_python_threads();
    return NULL;
}

char *
python_on_room_message_received_hook(ProfPlugin *plugin, const char * const room,
    const char * const nick, const char *message)
{
    disable_python_threads();
    PyObject *p_args = Py_BuildValue("sss", room, nick, message);
    PyObject *p_function;

    PyObject *p_module = plugin->module;
    if (PyObject_HasAttrString(p_module, "prof_on_room_message_received")) {
        p_function = PyObject_GetAttrString(p_module, "prof_on_room_message_received");
        python_check_error();
        if (p_function && PyCallable_Check(p_function)) {
            PyObject *result = PyObject_CallObject(p_function, p_args);
            python_check_error();
            Py_XDECREF(p_function);
            if (PyUnicode_Check(result)) {
                char *result_str = strdup(PyString_AsString(PyUnicode_AsUTF8String(result)));
                Py_XDECREF(result);
                allow_python_threads();
                return result_str;
            } else if (result != Py_None) {
                char *result_str = strdup(PyString_AsString(result));
                Py_XDECREF(result);
                allow_python_threads();
                return result_str;
            } else {
                allow_python_threads();
                return NULL;
            }
        }
    }

    allow_python_threads();
    return NULL;
}

char *
python_on_message_send_hook(ProfPlugin *plugin, const char * const jid,
    const char *message)
{
    disable_python_threads();
    PyObject *p_args = Py_BuildValue("ss", jid, message);
    PyObject *p_function;

    PyObject *p_module = plugin->module;
    if (PyObject_HasAttrString(p_module, "prof_on_message_send")) {
        p_function = PyObject_GetAttrString(p_module, "prof_on_message_send");
        python_check_error();
        if (p_function && PyCallable_Check(p_function)) {
            PyObject *result = PyObject_CallObject(p_function, p_args);
            python_check_error();
            Py_XDECREF(p_function);
            if (PyUnicode_Check(result)) {
                char *result_str = strdup(PyString_AsString(PyUnicode_AsUTF8String(result)));
                Py_XDECREF(result);
                allow_python_threads();
                return result_str;
            } else if (result != Py_None) {
                char *result_str = strdup(PyString_AsString(result));
                Py_XDECREF(result);
                allow_python_threads();
                return result_str;
            } else {
                allow_python_threads();
                return NULL;
            }
        }
    }

    allow_python_threads();
    return NULL;
}

char *
python_on_private_message_send_hook(ProfPlugin *plugin, const char * const room,
    const char * const nick, const char *message)
{
    disable_python_threads();
    PyObject *p_args = Py_BuildValue("sss", room, nick, message);
    PyObject *p_function;

    PyObject *p_module = plugin->module;
    if (PyObject_HasAttrString(p_module, "prof_on_private_message_send")) {
        p_function = PyObject_GetAttrString(p_module, "prof_on_private_message_send");
        python_check_error();
        if (p_function && PyCallable_Check(p_function)) {
            PyObject *result = PyObject_CallObject(p_function, p_args);
            python_check_error();
            Py_XDECREF(p_function);
            if (PyUnicode_Check(result)) {
                char *result_str = strdup(PyString_AsString(PyUnicode_AsUTF8String(result)));
                Py_XDECREF(result);
                allow_python_threads();
                return result_str;
            } else if (result != Py_None) {
                char *result_str = strdup(PyString_AsString(result));
                Py_XDECREF(result);
                allow_python_threads();
                return result_str;
            } else {
                allow_python_threads();
                return NULL;
            }
        }
    }

    allow_python_threads();
    return NULL;
}

char *
python_on_room_message_send_hook(ProfPlugin *plugin, const char * const room,
    const char *message)
{
    disable_python_threads();
    PyObject *p_args = Py_BuildValue("ss", room, message);
    PyObject *p_function;

    PyObject *p_module = plugin->module;
    if (PyObject_HasAttrString(p_module, "prof_on_room_message_send")) {
        p_function = PyObject_GetAttrString(p_module, "prof_on_room_message_send");
        python_check_error();
        if (p_function && PyCallable_Check(p_function)) {
            PyObject *result = PyObject_CallObject(p_function, p_args);
            python_check_error();
            Py_XDECREF(p_function);
            if (PyUnicode_Check(result)) {
                char *result_str = strdup(PyString_AsString(PyUnicode_AsUTF8String(result)));
                Py_XDECREF(result);
                allow_python_threads();
                return result_str;
            } else if (result != Py_None) {
                char *result_str = strdup(PyString_AsString(result));
                Py_XDECREF(result);
                allow_python_threads();
                return result_str;
            } else {
                allow_python_threads();
                return NULL;
            }
        }
    }

    allow_python_threads();
    return NULL;
}

void
python_on_shutdown_hook(ProfPlugin *plugin)
{
    disable_python_threads();
    PyObject *p_function;

    PyObject *p_module = plugin->module;
    if (PyObject_HasAttrString(p_module, "prof_on_shutdown")) {
        p_function = PyObject_GetAttrString(p_module, "prof_on_shutdown");
        python_check_error();
        if (p_function && PyCallable_Check(p_function)) {
            PyObject_CallObject(p_function, NULL);
            python_check_error();
            Py_XDECREF(p_function);
        }
    }
    allow_python_threads();
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
python_plugin_destroy(ProfPlugin *plugin)
{
    disable_python_threads();
    free(plugin->name);
    Py_XDECREF(plugin->module);
    free(plugin);
    allow_python_threads();
}

void
python_shutdown(void)
{
    disable_python_threads();
    Py_Finalize();
}