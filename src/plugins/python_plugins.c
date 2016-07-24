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

#include <Python.h>

#include "config.h"

#include "config/preferences.h"
#include "plugins/api.h"
#include "plugins/callbacks.h"
#include "plugins/plugins.h"
#include "plugins/python_api.h"
#include "plugins/python_plugins.h"
#include "ui/ui.h"

static PyThreadState *thread_state;
static GHashTable *loaded_modules;

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

static void
_unref_module(PyObject *module)
{
    Py_XDECREF(module);
}

void
python_env_init(void)
{
    loaded_modules = g_hash_table_new_full(g_str_hash, g_str_equal, free, (GDestroyNotify)_unref_module);

    python_init_prof();

    const char *ver = Py_GetVersion();
    cons_show("PYTHON: %s", ver);

    gchar *plugins_dir = plugins_get_dir();
    GString *path = g_string_new("import sys\n");
    g_string_append(path, "sys.path.append(\"");
    g_string_append(path, plugins_dir);
    g_string_append(path, "/\")\n");

    PyRun_SimpleString(path->str);
    python_check_error();

    g_string_free(path, TRUE);
    g_free(plugins_dir);

    allow_python_threads();
}

ProfPlugin*
python_plugin_create(const char *const filename)
{
    disable_python_threads();

    PyObject *p_module = g_hash_table_lookup(loaded_modules, filename);
    if (p_module) {
        p_module = PyImport_ReloadModule(p_module);
    } else {
        gchar *module_name = g_strndup(filename, strlen(filename) - 3);
        p_module = PyImport_ImportModule(module_name);
        if (p_module) {
            g_hash_table_insert(loaded_modules, strdup(filename), p_module);
        }
        g_free(module_name);
    }

    python_check_error();
    if (p_module) {
        ProfPlugin *plugin = malloc(sizeof(ProfPlugin));
        plugin->name = strdup(filename);
        plugin->lang = LANG_PYTHON;
        plugin->module = p_module;
        plugin->init_func = python_init_hook;
        plugin->on_start_func = python_on_start_hook;
        plugin->on_shutdown_func = python_on_shutdown_hook;
        plugin->on_unload_func = python_on_unload_hook;
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
        plugin->on_room_history_message = python_on_room_history_message_hook;
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
        plugin->on_chat_win_focus = python_on_chat_win_focus_hook;
        plugin->on_room_win_focus = python_on_room_win_focus_hook;

        allow_python_threads();
        return plugin;
    } else {
        allow_python_threads();
        return NULL;
    }
}

void
python_init_hook(ProfPlugin *plugin, const char *const version, const char *const status, const char *const account_name,
    const char *const fulljid)
{
    disable_python_threads();
    PyObject *p_args = Py_BuildValue("ssss", version, status, account_name, fulljid);
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
python_on_unload_hook(ProfPlugin *plugin)
{
    disable_python_threads();
    PyObject *p_function;

    PyObject *p_module = plugin->module;
    if (PyObject_HasAttrString(p_module, "prof_on_unload")) {
        p_function = PyObject_GetAttrString(p_module, "prof_on_unload");
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
            char *result_str = python_str_or_unicode_to_string(result);
            allow_python_threads();

            return result_str;
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
            char *result_str = python_str_or_unicode_to_string(result);
            allow_python_threads();

            return result_str;
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
            char *result_str = python_str_or_unicode_to_string(result);
            allow_python_threads();

            return result_str;
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
            char *result_str = python_str_or_unicode_to_string(result);
            allow_python_threads();

            return result_str;
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

void
python_on_room_history_message_hook(ProfPlugin *plugin, const char *const room, const char *const nick,
    const char *const message, const char *const timestamp)
{
    disable_python_threads();
    PyObject *p_args = Py_BuildValue("ssss", room, nick, message, timestamp);
    PyObject *p_function;

    PyObject *p_module = plugin->module;
    if (PyObject_HasAttrString(p_module, "prof_on_room_history_message")) {
        p_function = PyObject_GetAttrString(p_module, "prof_on_room_history_message");
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
            char *result_str = python_str_or_unicode_to_string(result);
            allow_python_threads();

            return result_str;
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
            char *result_str = python_str_or_unicode_to_string(result);
            allow_python_threads();

            return result_str;
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
            char *result_str = python_str_or_unicode_to_string(result);
            allow_python_threads();

            return result_str;
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
            if (PyObject_IsTrue(result)) {
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
            char *result_str = python_str_or_unicode_to_string(result);
            allow_python_threads();

            return result_str;
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
            if (PyObject_IsTrue(result)) {
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
            char *result_str = python_str_or_unicode_to_string(result);
            allow_python_threads();

            return result_str;
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
            if (PyObject_IsTrue(result)) {
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
python_on_chat_win_focus_hook(ProfPlugin *plugin, const char *const barejid)
{
    disable_python_threads();
    PyObject *p_args = Py_BuildValue("(s)", barejid);
    PyObject *p_function;

    PyObject *p_module = plugin->module;
    if (PyObject_HasAttrString(p_module, "prof_on_chat_win_focus")) {
        p_function = PyObject_GetAttrString(p_module, "prof_on_chat_win_focus");
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
python_on_room_win_focus_hook(ProfPlugin *plugin, const char *const roomjid)
{
    disable_python_threads();
    PyObject *p_args = Py_BuildValue("(s)", roomjid);
    PyObject *p_function;

    PyObject *p_module = plugin->module;
    if (PyObject_HasAttrString(p_module, "prof_on_room_win_focus")) {
        p_function = PyObject_GetAttrString(p_module, "prof_on_room_win_focus");
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
        PyRun_SimpleString("import sys\nsys.stdout.flush()");
        PyErr_Clear();
    }
}

void
python_plugin_destroy(ProfPlugin *plugin)
{
    disable_python_threads();
    callbacks_remove(plugin->name);
    free(plugin->name);
    free(plugin);
    allow_python_threads();
}

void
python_shutdown(void)
{
    disable_python_threads();
    g_hash_table_destroy(loaded_modules);
    Py_Finalize();
}
