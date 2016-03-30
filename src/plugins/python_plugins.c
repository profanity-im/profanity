/*
 * python_plugins.c
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

    PySys_SetPath(path->str);
    g_string_free(path, TRUE);

    // add site packages paths
    PyRun_SimpleString(
        "import site\n"
        "import sys\n"
        "from distutils.sysconfig import get_python_lib\n"
        "sys.path.append(get_python_lib())\n"
        "for dir in site.getsitepackages():\n"
        "   sys.path.append(dir)\n"
    );

    allow_python_threads();
}

ProfPlugin*
python_plugin_create(const char *const filename)
{
    disable_python_threads();
    gchar *module_name = g_strndup(filename, strlen(filename) - 3);
    PyObject *p_module = PyImport_ImportModule(module_name);
    python_check_error();
    if (p_module) {
        ProfPlugin *plugin = malloc(sizeof(ProfPlugin));
        plugin->name = strdup(module_name);
        plugin->lang = LANG_PYTHON;
        plugin->module = p_module;
        plugin->init_func = python_init_hook;
        plugin->on_start_func = python_on_start_hook;
        plugin->on_shutdown_func = python_on_shutdown_hook;
        plugin->on_connect_func = python_on_connect_hook;
        plugin->on_disconnect_func = python_on_disconnect_hook;
        plugin->pre_chat_message_display = python_pre_chat_message_display_hook;
        plugin->post_chat_message_display = python_post_chat_message_display_hook;
        plugin->pre_chat_message_send = python_pre_chat_message_send_hook;
        plugin->post_chat_message_send = python_post_chat_message_send_hook;
        plugin->pre_room_message_display = python_pre_room_message_display_hook;
        plugin->post_room_message_display = python_post_room_message_display_hook;
        plugin->pre_room_message_send = python_pre_room_message_send_hook;
        plugin->post_room_message_send = python_post_room_message_send_hook;
        plugin->pre_priv_message_display = python_pre_priv_message_display_hook;
        plugin->post_priv_message_display = python_post_priv_message_display_hook;
        plugin->pre_priv_message_send = python_pre_priv_message_send_hook;
        plugin->post_priv_message_send = python_post_priv_message_send_hook;
        plugin->on_message_stanza_send = python_on_message_stanza_send_hook;
        plugin->on_message_stanza_receive = python_on_message_stanza_receive_hook;
        plugin->on_presence_stanza_send = python_on_presence_stanza_send_hook;
        plugin->on_presence_stanza_receive = python_on_presence_stanza_receive_hook;
        plugin->on_iq_stanza_send = python_on_iq_stanza_send_hook;
        plugin->on_iq_stanza_receive = python_on_iq_stanza_receive_hook;
        plugin->on_contact_offline = python_on_contact_offline_hook;
        plugin->on_contact_presence = python_on_contact_presence_hook;
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
python_init_hook(ProfPlugin *plugin, const char *const version, const char *const status)
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
python_on_connect_hook(ProfPlugin *plugin, const char *const account_name, const char *const fulljid)
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
python_on_disconnect_hook(ProfPlugin *plugin, const char *const account_name, const char *const fulljid)
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

char*
python_pre_chat_message_display_hook(ProfPlugin *plugin, const char *const jid, const char *message)
{
    disable_python_threads();
    PyObject *p_args = Py_BuildValue("ss", jid, message);
    PyObject *p_function;

    PyObject *p_module = plugin->module;
    if (PyObject_HasAttrString(p_module, "prof_pre_chat_message_display")) {
        p_function = PyObject_GetAttrString(p_module, "prof_pre_chat_message_display");
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
python_post_chat_message_display_hook(ProfPlugin *plugin, const char *const jid, const char *message)
{
    disable_python_threads();
    PyObject *p_args = Py_BuildValue("ss", jid, message);
    PyObject *p_function;

    PyObject *p_module = plugin->module;
    if (PyObject_HasAttrString(p_module, "prof_post_chat_message_display")) {
        p_function = PyObject_GetAttrString(p_module, "prof_post_chat_message_display");
        python_check_error();
        if (p_function && PyCallable_Check(p_function)) {
            PyObject_CallObject(p_function, p_args);
            python_check_error();
            Py_XDECREF(p_function);
        }
    }

    allow_python_threads();
}

char*
python_pre_chat_message_send_hook(ProfPlugin *plugin, const char * const jid, const char *message)
{
    disable_python_threads();
    PyObject *p_args = Py_BuildValue("ss", jid, message);
    PyObject *p_function;

    PyObject *p_module = plugin->module;
    if (PyObject_HasAttrString(p_module, "prof_pre_chat_message_send")) {
        p_function = PyObject_GetAttrString(p_module, "prof_pre_chat_message_send");
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
python_post_chat_message_send_hook(ProfPlugin *plugin, const char *const jid, const char *message)
{
    disable_python_threads();
    PyObject *p_args = Py_BuildValue("ss", jid, message);
    PyObject *p_function;

    PyObject *p_module = plugin->module;
    if (PyObject_HasAttrString(p_module, "prof_post_chat_message_send")) {
        p_function = PyObject_GetAttrString(p_module, "prof_post_chat_message_send");
        python_check_error();
        if (p_function && PyCallable_Check(p_function)) {
            PyObject_CallObject(p_function, p_args);
            python_check_error();
            Py_XDECREF(p_function);
        }
    }

    allow_python_threads();
}

char*
python_pre_room_message_display_hook(ProfPlugin *plugin, const char * const room, const char * const nick, const char *message)
{
    disable_python_threads();
    PyObject *p_args = Py_BuildValue("sss", room, nick, message);
    PyObject *p_function;

    PyObject *p_module = plugin->module;
    if (PyObject_HasAttrString(p_module, "prof_pre_room_message_display")) {
        p_function = PyObject_GetAttrString(p_module, "prof_pre_room_message_display");
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
python_post_room_message_display_hook(ProfPlugin *plugin, const char *const room, const char *const nick,
    const char *message)
{
    disable_python_threads();
    PyObject *p_args = Py_BuildValue("sss", room, nick, message);
    PyObject *p_function;

    PyObject *p_module = plugin->module;
    if (PyObject_HasAttrString(p_module, "prof_post_room_message_display")) {
        p_function = PyObject_GetAttrString(p_module, "prof_post_room_message_display");
        python_check_error();
        if (p_function && PyCallable_Check(p_function)) {
            PyObject_CallObject(p_function, p_args);
            python_check_error();
            Py_XDECREF(p_function);
        }
    }

    allow_python_threads();
}

char*
python_pre_room_message_send_hook(ProfPlugin *plugin, const char *const room, const char *message)
{
    disable_python_threads();
    PyObject *p_args = Py_BuildValue("ss", room, message);
    PyObject *p_function;

    PyObject *p_module = plugin->module;
    if (PyObject_HasAttrString(p_module, "prof_pre_room_message_send")) {
        p_function = PyObject_GetAttrString(p_module, "prof_pre_room_message_send");
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
python_post_room_message_send_hook(ProfPlugin *plugin, const char *const room, const char *message)
{
    disable_python_threads();
    PyObject *p_args = Py_BuildValue("ss", room, message);
    PyObject *p_function;

    PyObject *p_module = plugin->module;
    if (PyObject_HasAttrString(p_module, "prof_post_room_message_send")) {
        p_function = PyObject_GetAttrString(p_module, "prof_post_room_message_send");
        python_check_error();
        if (p_function && PyCallable_Check(p_function)) {
            PyObject_CallObject(p_function, p_args);
            python_check_error();
            Py_XDECREF(p_function);
        }
    }

    allow_python_threads();
}

char*
python_pre_priv_message_display_hook(ProfPlugin *plugin, const char *const room, const char *const nick,
    const char *message)
{
    disable_python_threads();
    PyObject *p_args = Py_BuildValue("sss", room, nick, message);
    PyObject *p_function;

    PyObject *p_module = plugin->module;
    if (PyObject_HasAttrString(p_module, "prof_pre_priv_message_display")) {
        p_function = PyObject_GetAttrString(p_module, "prof_pre_priv_message_display");
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
python_post_priv_message_display_hook(ProfPlugin *plugin, const char *const room, const char *const nick,
    const char *message)
{
    disable_python_threads();
    PyObject *p_args = Py_BuildValue("sss", room, nick, message);
    PyObject *p_function;

    PyObject *p_module = plugin->module;
    if (PyObject_HasAttrString(p_module, "prof_post_priv_message_display")) {
        p_function = PyObject_GetAttrString(p_module, "prof_post_priv_message_display");
        python_check_error();
        if (p_function && PyCallable_Check(p_function)) {
            PyObject_CallObject(p_function, p_args);
            python_check_error();
            Py_XDECREF(p_function);
        }
    }

    allow_python_threads();
}

char*
python_pre_priv_message_send_hook(ProfPlugin *plugin, const char *const room, const char *const nick,
    const char *const message)
{
    disable_python_threads();
    PyObject *p_args = Py_BuildValue("sss", room, nick, message);
    PyObject *p_function;

    PyObject *p_module = plugin->module;
    if (PyObject_HasAttrString(p_module, "prof_pre_priv_message_send")) {
        p_function = PyObject_GetAttrString(p_module, "prof_pre_priv_message_send");
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
python_post_priv_message_send_hook(ProfPlugin *plugin, const char *const room, const char *const nick,
    const char *const message)
{
    disable_python_threads();
    PyObject *p_args = Py_BuildValue("sss", room, nick, message);
    PyObject *p_function;

    PyObject *p_module = plugin->module;
    if (PyObject_HasAttrString(p_module, "prof_post_priv_message_send")) {
        p_function = PyObject_GetAttrString(p_module, "prof_post_priv_message_send");
        python_check_error();
        if (p_function && PyCallable_Check(p_function)) {
            PyObject_CallObject(p_function, p_args);
            python_check_error();
            Py_XDECREF(p_function);
        }
    }

    allow_python_threads();
}

char*
python_on_message_stanza_send_hook(ProfPlugin *plugin, const char *const text)
{
    disable_python_threads();
    PyObject *p_args = Py_BuildValue("(s)", text);
    PyObject *p_function;

    PyObject *p_module = plugin->module;
    if (PyObject_HasAttrString(p_module, "prof_on_message_stanza_send")) {
        p_function = PyObject_GetAttrString(p_module, "prof_on_message_stanza_send");
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

gboolean
python_on_message_stanza_receive_hook(ProfPlugin *plugin, const char *const text)
{
    disable_python_threads();
    PyObject *p_args = Py_BuildValue("(s)", text);
    PyObject *p_function;

    PyObject *p_module = plugin->module;
    if (PyObject_HasAttrString(p_module, "prof_on_message_stanza_receive")) {
        p_function = PyObject_GetAttrString(p_module, "prof_on_message_stanza_receive");
        python_check_error();
        if (p_function && PyCallable_Check(p_function)) {
            PyObject *result = PyObject_CallObject(p_function, p_args);
            python_check_error();
            Py_XDECREF(p_function);
            if (PyBool_Check(result)) {
                allow_python_threads();
                return TRUE;
            } else {
                allow_python_threads();
                return FALSE;
            }
        }
    }

    allow_python_threads();
    return TRUE;
}

char*
python_on_presence_stanza_send_hook(ProfPlugin *plugin, const char *const text)
{
    disable_python_threads();
    PyObject *p_args = Py_BuildValue("(s)", text);
    PyObject *p_function;

    PyObject *p_module = plugin->module;
    if (PyObject_HasAttrString(p_module, "prof_on_presence_stanza_send")) {
        p_function = PyObject_GetAttrString(p_module, "prof_on_presence_stanza_send");
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

gboolean
python_on_presence_stanza_receive_hook(ProfPlugin *plugin, const char *const text)
{
    disable_python_threads();
    PyObject *p_args = Py_BuildValue("(s)", text);
    PyObject *p_function;

    PyObject *p_module = plugin->module;
    if (PyObject_HasAttrString(p_module, "prof_on_presence_stanza_receive")) {
        p_function = PyObject_GetAttrString(p_module, "prof_on_presence_stanza_receive");
        python_check_error();
        if (p_function && PyCallable_Check(p_function)) {
            PyObject *result = PyObject_CallObject(p_function, p_args);
            python_check_error();
            Py_XDECREF(p_function);
            if (PyBool_Check(result)) {
                allow_python_threads();
                return TRUE;
            } else {
                allow_python_threads();
                return FALSE;
            }
        }
    }

    allow_python_threads();
    return TRUE;
}

char*
python_on_iq_stanza_send_hook(ProfPlugin *plugin, const char *const text)
{
    disable_python_threads();
    PyObject *p_args = Py_BuildValue("(s)", text);
    PyObject *p_function;

    PyObject *p_module = plugin->module;
    if (PyObject_HasAttrString(p_module, "prof_on_iq_stanza_send")) {
        p_function = PyObject_GetAttrString(p_module, "prof_on_iq_stanza_send");
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

gboolean
python_on_iq_stanza_receive_hook(ProfPlugin *plugin, const char *const text)
{
    disable_python_threads();
    PyObject *p_args = Py_BuildValue("(s)", text);
    PyObject *p_function;

    PyObject *p_module = plugin->module;
    if (PyObject_HasAttrString(p_module, "prof_on_iq_stanza_receive")) {
        p_function = PyObject_GetAttrString(p_module, "prof_on_iq_stanza_receive");
        python_check_error();
        if (p_function && PyCallable_Check(p_function)) {
            PyObject *result = PyObject_CallObject(p_function, p_args);
            python_check_error();
            Py_XDECREF(p_function);
            if (PyBool_Check(result)) {
                allow_python_threads();
                return TRUE;
            } else {
                allow_python_threads();
                return FALSE;
            }
        }
    }

    allow_python_threads();
    return TRUE;
}

void
python_on_contact_offline_hook(ProfPlugin *plugin, const char *const barejid, const char *const resource,
    const char *const status)
{
    disable_python_threads();
    PyObject *p_args = Py_BuildValue("sss", barejid, resource, status);
    PyObject *p_function;

    PyObject *p_module = plugin->module;
    if (PyObject_HasAttrString(p_module, "prof_on_contact_offline")) {
        p_function = PyObject_GetAttrString(p_module, "prof_on_contact_offline");
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
python_on_contact_presence_hook(ProfPlugin *plugin, const char *const barejid, const char *const resource,
    const char *const presence, const char *const status, const int priority)
{
    disable_python_threads();
    PyObject *p_args = Py_BuildValue("ssssi", barejid, resource, presence, status, priority);
    PyObject *p_function;

    PyObject *p_module = plugin->module;
    if (PyObject_HasAttrString(p_module, "prof_on_contact_presence")) {
        p_function = PyObject_GetAttrString(p_module, "prof_on_contact_presence");
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
