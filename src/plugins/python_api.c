/*
 * python_api.c
 *
 * Copyright (C) 2012 - 2017 James Booth <boothj5@gmail.com>
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

#include "config.h"

#include <Python.h>
#include <frameobject.h>

#include <stdlib.h>

#include <glib.h>

#include "log.h"
#include "plugins/api.h"
#include "plugins/python_api.h"
#include "plugins/python_plugins.h"
#include "plugins/callbacks.h"
#include "plugins/autocompleters.h"

static char* _python_plugin_name(void);

static PyObject*
python_api_cons_alert(PyObject *self, PyObject *args)
{
    allow_python_threads();
    api_cons_alert();
    disable_python_threads();

    Py_RETURN_NONE;
}

static PyObject*
python_api_cons_show(PyObject *self, PyObject *args)
{
    PyObject* message = NULL;
    if (!PyArg_ParseTuple(args, "O", &message)) {
        Py_RETURN_NONE;
    }

    char *message_str = python_str_or_unicode_to_string(message);

    allow_python_threads();
    api_cons_show(message_str);
    free(message_str);
    disable_python_threads();

    Py_RETURN_NONE;
}

static PyObject*
python_api_cons_show_themed(PyObject *self, PyObject *args)
{
    PyObject *group = NULL;
    PyObject *key = NULL;
    PyObject *def = NULL;
    PyObject *message = NULL;
    if (!PyArg_ParseTuple(args, "OOOO", &group, &key, &def, &message)) {
        Py_RETURN_NONE;
    }

    char *group_str = python_str_or_unicode_to_string(group);
    char *key_str = python_str_or_unicode_to_string(key);
    char *def_str = python_str_or_unicode_to_string(def);
    char *message_str = python_str_or_unicode_to_string(message);

    allow_python_threads();
    api_cons_show_themed(group_str, key_str, def_str, message_str);
    free(group_str);
    free(key_str);
    free(def_str);
    free(message_str);
    disable_python_threads();

    Py_RETURN_NONE;
}

static PyObject*
python_api_cons_bad_cmd_usage(PyObject *self, PyObject *args)
{
    PyObject *cmd = NULL;
    if (!PyArg_ParseTuple(args, "O", &cmd)) {
        Py_RETURN_NONE;
    }

    char *cmd_str = python_str_or_unicode_to_string(cmd);

    allow_python_threads();
    api_cons_bad_cmd_usage(cmd_str);
    free(cmd_str);
    disable_python_threads();

    Py_RETURN_NONE;
}

static PyObject*
python_api_register_command(PyObject *self, PyObject *args)
{
    PyObject *command_name = NULL;
    int min_args = 0;
    int max_args = 0;
    PyObject *synopsis = NULL;
    PyObject *description = NULL;
    PyObject *arguments = NULL;
    PyObject *examples = NULL;
    PyObject *p_callback = NULL;

    if (!PyArg_ParseTuple(args, "OiiOOOOO", &command_name, &min_args, &max_args,
            &synopsis, &description, &arguments, &examples, &p_callback)) {
        Py_RETURN_NONE;
    }

    char *command_name_str = python_str_or_unicode_to_string(command_name);
    char *description_str = python_str_or_unicode_to_string(description);

    char *plugin_name = _python_plugin_name();
    log_debug("Register command %s for %s", command_name_str, plugin_name);

    if (p_callback && PyCallable_Check(p_callback)) {
        Py_ssize_t len = PyList_Size(synopsis);
        char *c_synopsis[len == 0 ? 0 : len+1];
        Py_ssize_t i = 0;
        for (i = 0; i < len; i++) {
            PyObject *item = PyList_GetItem(synopsis, i);
            char *c_item = python_str_or_unicode_to_string(item);
            c_synopsis[i] = c_item;
        }
        c_synopsis[len] = NULL;

        Py_ssize_t args_len = PyList_Size(arguments);
        char *c_arguments[args_len == 0 ? 0 : args_len+1][2];
        i = 0;
        for (i = 0; i < args_len; i++) {
            PyObject *item = PyList_GetItem(arguments, i);
            Py_ssize_t len2 = PyList_Size(item);
            if (len2 != 2) {
                Py_RETURN_NONE;
            }
            PyObject *arg = PyList_GetItem(item, 0);
            char *c_arg = python_str_or_unicode_to_string(arg);
            c_arguments[i][0] = c_arg;

            PyObject *desc = PyList_GetItem(item, 1);
            char *c_desc = python_str_or_unicode_to_string(desc);
            c_arguments[i][1] = c_desc;
        }

        c_arguments[args_len][0] = NULL;
        c_arguments[args_len][1] = NULL;

        len = PyList_Size(examples);
        char *c_examples[len == 0 ? 0 : len+1];
        i = 0;
        for (i = 0; i < len; i++) {
            PyObject *item = PyList_GetItem(examples, i);
            char *c_item = python_str_or_unicode_to_string(item);
            c_examples[i] = c_item;
        }
        c_examples[len] = NULL;

        allow_python_threads();
        api_register_command(plugin_name, command_name_str, min_args, max_args, c_synopsis,
            description_str, c_arguments, c_examples, p_callback, python_command_callback, NULL);
        free(command_name_str);
        free(description_str);
        i = 0;
        while (c_synopsis[i] != NULL) {
            free(c_synopsis[i++]);
        }
        i = 0;
        while (c_arguments[i] != NULL && c_arguments[i][0] != NULL) {
            free(c_arguments[i][0]);
            free(c_arguments[i][1]);
            i++;
        }
        i = 0;
        while (c_examples[i] != NULL) {
            free(c_examples[i++]);
        }
        disable_python_threads();
    }

    free(plugin_name);

    Py_RETURN_NONE;
}

static PyObject *
python_api_register_timed(PyObject *self, PyObject *args)
{
    PyObject *p_callback = NULL;
    int interval_seconds = 0;

    if (!PyArg_ParseTuple(args, "Oi", &p_callback, &interval_seconds)) {
        Py_RETURN_NONE;
    }

    char *plugin_name = _python_plugin_name();
    log_debug("Register timed for %s", plugin_name);

    if (p_callback && PyCallable_Check(p_callback)) {
        allow_python_threads();
        api_register_timed(plugin_name, p_callback, interval_seconds, python_timed_callback, NULL);
        disable_python_threads();
    }

    free(plugin_name);

    Py_RETURN_NONE;
}

static PyObject *
python_api_completer_add(PyObject *self, PyObject *args)
{
    PyObject *key = NULL;
    PyObject *items = NULL;

    if (!PyArg_ParseTuple(args, "OO", &key, &items)) {
        Py_RETURN_NONE;
    }

    char *key_str = python_str_or_unicode_to_string(key);

    char *plugin_name = _python_plugin_name();
    log_debug("Autocomplete add %s for %s", key_str, plugin_name);

    Py_ssize_t len = PyList_Size(items);
    char *c_items[len];

    Py_ssize_t i = 0;
    for (i = 0; i < len; i++) {
        PyObject *item = PyList_GetItem(items, i);
        char *c_item = python_str_or_unicode_to_string(item);
        c_items[i] = c_item;
    }
    c_items[len] = NULL;

    allow_python_threads();
    api_completer_add(plugin_name, key_str, c_items);
    free(key_str);
    i = 0;
    while (c_items[i] != NULL) {
        free(c_items[i++]);
    }
    disable_python_threads();

    free(plugin_name);

    Py_RETURN_NONE;
}

static PyObject *
python_api_completer_remove(PyObject *self, PyObject *args)
{
    PyObject *key = NULL;
    PyObject *items = NULL;

    if (!PyArg_ParseTuple(args, "OO", &key, &items)) {
        Py_RETURN_NONE;
    }

    char *key_str = python_str_or_unicode_to_string(key);

    char *plugin_name = _python_plugin_name();
    log_debug("Autocomplete remove %s for %s", key_str, plugin_name);

    Py_ssize_t len = PyList_Size(items);
    char *c_items[len];

    Py_ssize_t i = 0;
    for (i = 0; i < len; i++) {
        PyObject *item = PyList_GetItem(items, i);
        char *c_item = python_str_or_unicode_to_string(item);
        c_items[i] = c_item;
    }
    c_items[len] = NULL;

    allow_python_threads();
    api_completer_remove(plugin_name, key_str, c_items);
    free(key_str);
    disable_python_threads();

    free(plugin_name);

    Py_RETURN_NONE;
}

static PyObject *
python_api_completer_clear(PyObject *self, PyObject *args)
{
    PyObject *key = NULL;

    if (!PyArg_ParseTuple(args, "O", &key)) {
        Py_RETURN_NONE;
    }

    char *key_str = python_str_or_unicode_to_string(key);

    char *plugin_name = _python_plugin_name();
    log_debug("Autocomplete clear %s for %s", key_str, plugin_name);

    allow_python_threads();
    api_completer_clear(plugin_name, key_str);
    free(key_str);
    disable_python_threads();

    free(plugin_name);

    Py_RETURN_NONE;
}

static PyObject*
python_api_filepath_completer_add(PyObject *self, PyObject *args)
{
    PyObject *prefix = NULL;

    if (!PyArg_ParseTuple(args, "O", &prefix)) {
        Py_RETURN_NONE;
    }

    char *prefix_str = python_str_or_unicode_to_string(prefix);

    char *plugin_name = _python_plugin_name();
    log_debug("Filepath autocomplete added '%s' for %s", prefix_str, plugin_name);

    allow_python_threads();
    api_filepath_completer_add(plugin_name, prefix_str);
    free(prefix_str);
    disable_python_threads();

    free(plugin_name);

    Py_RETURN_NONE;
}

static PyObject*
python_api_notify(PyObject *self, PyObject *args)
{
    PyObject *message = NULL;
    PyObject *category = NULL;
    int timeout_ms = 5000;

    if (!PyArg_ParseTuple(args, "OiO", &message, &timeout_ms, &category)) {
        Py_RETURN_NONE;
    }

    char *message_str = python_str_or_unicode_to_string(message);
    char *category_str = python_str_or_unicode_to_string(category);

    allow_python_threads();
    api_notify(message_str, category_str, timeout_ms);
    free(message_str);
    free(category_str);
    disable_python_threads();

    Py_RETURN_NONE;
}

static PyObject*
python_api_send_line(PyObject *self, PyObject *args)
{
    PyObject *line = NULL;
    if (!PyArg_ParseTuple(args, "O", &line)) {
        Py_RETURN_NONE;
    }

    char *line_str = python_str_or_unicode_to_string(line);

    allow_python_threads();
    api_send_line(line_str);
    free(line_str);
    disable_python_threads();

    Py_RETURN_NONE;
}

static PyObject *
python_api_get_current_recipient(PyObject *self, PyObject *args)
{
    allow_python_threads();
    char *recipient = api_get_current_recipient();
    disable_python_threads();
    if (recipient) {
        return Py_BuildValue("s", recipient);
    } else {
        Py_RETURN_NONE;
    }
}

static PyObject *
python_api_get_current_muc(PyObject *self, PyObject *args)
{
    allow_python_threads();
    char *room = api_get_current_muc();
    disable_python_threads();
    if (room) {
        return Py_BuildValue("s", room);
    } else {
        Py_RETURN_NONE;
    }
}

static PyObject *
python_api_get_current_nick(PyObject *self, PyObject *args)
{
    allow_python_threads();
    char *nick = api_get_current_nick();
    disable_python_threads();
    if (nick) {
        return Py_BuildValue("s", nick);
    } else {
        Py_RETURN_NONE;
    }
}

static PyObject*
python_api_get_current_occupants(PyObject *self, PyObject *args)
{
    allow_python_threads();
    char **occupants = api_get_current_occupants();
    disable_python_threads();
    PyObject *result = PyList_New(0);
    if (occupants) {
        int len = g_strv_length(occupants);
        int i = 0;
        for (i = 0; i < len; i++) {
            PyList_Append(result, Py_BuildValue("s", occupants[i]));
        }
        return result;
    } else {
        return result;
    }
}

static PyObject*
python_api_current_win_is_console(PyObject *self, PyObject *args)
{
    allow_python_threads();
    int res = api_current_win_is_console();
    disable_python_threads();
    if (res) {
        return Py_BuildValue("O", Py_True);
    } else {
        return Py_BuildValue("O", Py_False);
    }
}

static PyObject*
python_api_get_room_nick(PyObject *self, PyObject *args)
{
    PyObject *barejid = NULL;
    if (!PyArg_ParseTuple(args, "O", &barejid)) {
        Py_RETURN_NONE;
    }

    char *barejid_str = python_str_or_unicode_to_string(barejid);

    allow_python_threads();
    char *nick = api_get_room_nick(barejid_str);
    free(barejid_str);
    disable_python_threads();
    if (nick) {
        return Py_BuildValue("s", nick);
    } else {
        Py_RETURN_NONE;
    }
}

static PyObject *
python_api_log_debug(PyObject *self, PyObject *args)
{
    PyObject *message = NULL;
    if (!PyArg_ParseTuple(args, "O", &message)) {
        Py_RETURN_NONE;
    }

    char *message_str = python_str_or_unicode_to_string(message);

    allow_python_threads();
    api_log_debug(message_str);
    free(message_str);
    disable_python_threads();

    Py_RETURN_NONE;
}

static PyObject *
python_api_log_info(PyObject *self, PyObject *args)
{
    PyObject *message = NULL;
    if (!PyArg_ParseTuple(args, "O", &message)) {
        Py_RETURN_NONE;
    }

    char *message_str = python_str_or_unicode_to_string(message);

    allow_python_threads();
    api_log_info(message_str);
    free(message_str);
    disable_python_threads();

    Py_RETURN_NONE;
}

static PyObject *
python_api_log_warning(PyObject *self, PyObject *args)
{
    PyObject *message = NULL;
    if (!PyArg_ParseTuple(args, "O", &message)) {
        Py_RETURN_NONE;
    }

    char *message_str = python_str_or_unicode_to_string(message);

    allow_python_threads();
    api_log_warning(message_str);
    free(message_str);
    disable_python_threads();

    Py_RETURN_NONE;
}

static PyObject *
python_api_log_error(PyObject *self, PyObject *args)
{
    PyObject *message = NULL;
    if (!PyArg_ParseTuple(args, "O", &message)) {
        Py_RETURN_NONE;
    }

    char *message_str = python_str_or_unicode_to_string(message);

    allow_python_threads();
    api_log_error(message_str);
    free(message_str);
    disable_python_threads();

    Py_RETURN_NONE;
}

static PyObject *
python_api_win_exists(PyObject *self, PyObject *args)
{
    PyObject *tag = NULL;
    if (!PyArg_ParseTuple(args, "O", &tag)) {
        Py_RETURN_NONE;
    }

    char *tag_str = python_str_or_unicode_to_string(tag);

    allow_python_threads();
    gboolean exists = api_win_exists(tag_str);
    free(tag_str);
    disable_python_threads();

    if (exists) {
        return Py_BuildValue("O", Py_True);
    } else {
        return Py_BuildValue("O", Py_False);
    }
}

static PyObject *
python_api_win_create(PyObject *self, PyObject *args)
{
    PyObject *tag = NULL;
    PyObject *p_callback = NULL;

    if (!PyArg_ParseTuple(args, "OO", &tag, &p_callback)) {
        Py_RETURN_NONE;
    }

    char *tag_str = python_str_or_unicode_to_string(tag);

    char *plugin_name = _python_plugin_name();

    if (p_callback && PyCallable_Check(p_callback)) {
        allow_python_threads();
        api_win_create(plugin_name, tag_str, p_callback, python_window_callback, NULL);
        free(tag_str);
        disable_python_threads();
    }

    free(plugin_name);

    Py_RETURN_NONE;
}

static PyObject *
python_api_win_focus(PyObject *self, PyObject *args)
{
    PyObject *tag = NULL;

    if (!PyArg_ParseTuple(args, "O", &tag)) {
        Py_RETURN_NONE;
    }

    char *tag_str = python_str_or_unicode_to_string(tag);

    allow_python_threads();
    api_win_focus(tag_str);
    free(tag_str);
    disable_python_threads();

    Py_RETURN_NONE;
}

static PyObject *
python_api_win_show(PyObject *self, PyObject *args)
{
    PyObject *tag = NULL;
    PyObject *line = NULL;

    if (!PyArg_ParseTuple(args, "OO", &tag, &line)) {
        Py_RETURN_NONE;
    }

    char *tag_str = python_str_or_unicode_to_string(tag);
    char *line_str = python_str_or_unicode_to_string(line);

    allow_python_threads();
    api_win_show(tag_str, line_str);
    free(tag_str);
    free(line_str);
    disable_python_threads();

    Py_RETURN_NONE;
}

static PyObject *
python_api_win_show_themed(PyObject *self, PyObject *args)
{
    PyObject *tag = NULL;
    PyObject *group = NULL;
    PyObject *key = NULL;
    PyObject *def = NULL;
    PyObject *line = NULL;

    if (!PyArg_ParseTuple(args, "OOOOO", &tag, &group, &key, &def, &line)) {
        python_check_error();
        Py_RETURN_NONE;
    }

    char *tag_str = python_str_or_unicode_to_string(tag);
    char *group_str = python_str_or_unicode_to_string(group);
    char *key_str = python_str_or_unicode_to_string(key);
    char *def_str = python_str_or_unicode_to_string(def);
    char *line_str = python_str_or_unicode_to_string(line);

    allow_python_threads();
    api_win_show_themed(tag_str, group_str, key_str, def_str, line_str);
    free(tag_str);
    free(group_str);
    free(key_str);
    free(def_str);
    free(line_str);
    disable_python_threads();

    Py_RETURN_NONE;
}

static PyObject*
python_api_send_stanza(PyObject *self, PyObject *args)
{
    PyObject *stanza = NULL;
    if (!PyArg_ParseTuple(args, "O", &stanza)) {
        return Py_BuildValue("O", Py_False);
    }

    char *stanza_str = python_str_or_unicode_to_string(stanza);

    allow_python_threads();
    int res = api_send_stanza(stanza_str);
    free(stanza_str);
    disable_python_threads();
    if (res) {
        return Py_BuildValue("O", Py_True);
    } else {
        return Py_BuildValue("O", Py_False);
    }
}

static PyObject*
python_api_settings_boolean_get(PyObject *self, PyObject *args)
{
    PyObject *group = NULL;
    PyObject *key = NULL;
    PyObject *defobj = NULL;

    if (!PyArg_ParseTuple(args, "OOO!", &group, &key, &PyBool_Type, &defobj)) {
        Py_RETURN_NONE;
    }

    char *group_str = python_str_or_unicode_to_string(group);
    char *key_str = python_str_or_unicode_to_string(key);
    int def = PyObject_IsTrue(defobj);

    allow_python_threads();
    int res = api_settings_boolean_get(group_str, key_str, def);
    free(group_str);
    free(key_str);
    disable_python_threads();

    if (res) {
        return Py_BuildValue("O", Py_True);
    } else {
        return Py_BuildValue("O", Py_False);
    }
}

static PyObject*
python_api_settings_boolean_set(PyObject *self, PyObject *args)
{
    PyObject *group = NULL;
    PyObject *key = NULL;
    PyObject *valobj = NULL;

    if (!PyArg_ParseTuple(args, "OOO!", &group, &key, &PyBool_Type, &valobj)) {
        Py_RETURN_NONE;
    }

    char *group_str = python_str_or_unicode_to_string(group);
    char *key_str = python_str_or_unicode_to_string(key);
    int val = PyObject_IsTrue(valobj);

    allow_python_threads();
    api_settings_boolean_set(group_str, key_str, val);
    free(group_str);
    free(key_str);
    disable_python_threads();

    Py_RETURN_NONE;
}

static PyObject*
python_api_settings_string_get(PyObject *self, PyObject *args)
{
    PyObject *group = NULL;
    PyObject *key = NULL;
    PyObject *def = NULL;

    if (!PyArg_ParseTuple(args, "OOO", &group, &key, &def)) {
        Py_RETURN_NONE;
    }

    char *group_str = python_str_or_unicode_to_string(group);
    char *key_str = python_str_or_unicode_to_string(key);
    char *def_str = python_str_or_unicode_to_string(def);

    allow_python_threads();
    char *res = api_settings_string_get(group_str, key_str, def_str);
    free(group_str);
    free(key_str);
    free(def_str);
    disable_python_threads();

    if (res) {
        PyObject *pyres = Py_BuildValue("s", res);
        free(res);
        return pyres;
    } else {
        Py_RETURN_NONE;
    }
}

static PyObject*
python_api_settings_string_set(PyObject *self, PyObject *args)
{
    PyObject *group = NULL;
    PyObject *key = NULL;
    PyObject *val = NULL;

    if (!PyArg_ParseTuple(args, "OOO", &group, &key, &val)) {
        Py_RETURN_NONE;
    }

    char *group_str = python_str_or_unicode_to_string(group);
    char *key_str = python_str_or_unicode_to_string(key);
    char *val_str = python_str_or_unicode_to_string(val);

    allow_python_threads();
    api_settings_string_set(group_str, key_str, val_str);
    free(group_str);
    free(key_str);
    free(val_str);
    disable_python_threads();

    Py_RETURN_NONE;
}

static PyObject*
python_api_settings_int_get(PyObject *self, PyObject *args)
{
    PyObject *group = NULL;
    PyObject *key = NULL;
    int def = 0;

    if (!PyArg_ParseTuple(args, "OOi", &group, &key, &def)) {
        Py_RETURN_NONE;
    }

    char *group_str = python_str_or_unicode_to_string(group);
    char *key_str = python_str_or_unicode_to_string(key);

    allow_python_threads();
    int res = api_settings_int_get(group_str, key_str, def);
    free(group_str);
    free(key_str);
    disable_python_threads();

    return Py_BuildValue("i", res);
}

static PyObject*
python_api_settings_int_set(PyObject *self, PyObject *args)
{
    PyObject *group = NULL;
    PyObject *key = NULL;
    int val = 0;

    if (!PyArg_ParseTuple(args, "OOi", &group, &key, &val)) {
        Py_RETURN_NONE;
    }

    char *group_str = python_str_or_unicode_to_string(group);
    char *key_str = python_str_or_unicode_to_string(key);

    allow_python_threads();
    api_settings_int_set(group_str, key_str, val);
    free(group_str);
    free(key_str);
    disable_python_threads();

    Py_RETURN_NONE;
}

static PyObject*
python_api_settings_string_list_get(PyObject *self, PyObject *args)
{
    PyObject *group = NULL;
    PyObject *key = NULL;

    if (!PyArg_ParseTuple(args, "OO", &group, &key)) {
        Py_RETURN_NONE;
    }

    char *group_str = python_str_or_unicode_to_string(group);
    char *key_str = python_str_or_unicode_to_string(key);

    allow_python_threads();
    char** c_list = api_settings_string_list_get(group_str, key_str);
    free(group_str);
    free(key_str);
    disable_python_threads();

    if (!c_list) {
        Py_RETURN_NONE;
    }


    int len = g_strv_length(c_list);
    PyObject *py_list = PyList_New(0);
    int i = 0;
    for (i = 0; i < len; i++) {
        PyObject *py_curr = Py_BuildValue("s", c_list[i]);
        int res = PyList_Append(py_list, py_curr);
        if (res != 0) {
            g_strfreev(c_list);
            Py_RETURN_NONE;
        }
    }

    g_strfreev(c_list);

    return Py_BuildValue("O", py_list);
}

static PyObject*
python_api_settings_string_list_add(PyObject *self, PyObject *args)
{
    PyObject *group = NULL;
    PyObject *key = NULL;
    PyObject *val = NULL;

    if (!PyArg_ParseTuple(args, "OOO", &group, &key, &val)) {
        Py_RETURN_NONE;
    }

    char *group_str = python_str_or_unicode_to_string(group);
    char *key_str = python_str_or_unicode_to_string(key);
    char *val_str = python_str_or_unicode_to_string(val);

    allow_python_threads();
    api_settings_string_list_add(group_str, key_str, val_str);
    free(group_str);
    free(key_str);
    free(val_str);
    disable_python_threads();

    Py_RETURN_NONE;
}

static PyObject*
python_api_settings_string_list_remove(PyObject *self, PyObject *args)
{
    PyObject *group = NULL;
    PyObject *key = NULL;
    PyObject *val = NULL;

    if (!PyArg_ParseTuple(args, "OOO", &group, &key, &val)) {
        Py_RETURN_NONE;
    }

    char *group_str = python_str_or_unicode_to_string(group);
    char *key_str = python_str_or_unicode_to_string(key);
    char *val_str = python_str_or_unicode_to_string(val);

    allow_python_threads();
    int res = api_settings_string_list_remove(group_str, key_str, val_str);
    free(group_str);
    free(key_str);
    free(val_str);
    disable_python_threads();

    if (res) {
        return Py_BuildValue("O", Py_True);
    } else {
        return Py_BuildValue("O", Py_False);
    }
}

static PyObject*
python_api_settings_string_list_clear(PyObject *self, PyObject *args)
{
    PyObject *group = NULL;
    PyObject *key = NULL;

    if (!PyArg_ParseTuple(args, "OO", &group, &key)) {
        return Py_BuildValue("O", Py_False);
    }

    char *group_str = python_str_or_unicode_to_string(group);
    char *key_str = python_str_or_unicode_to_string(key);

    allow_python_threads();
    int res = api_settings_string_list_clear(group_str, key_str);
    free(group_str);
    free(key_str);
    disable_python_threads();

    if (res) {
        return Py_BuildValue("O", Py_True);
    } else {
        return Py_BuildValue("O", Py_False);
    }
}

static PyObject*
python_api_incoming_message(PyObject *self, PyObject *args)
{
    PyObject *barejid = NULL;
    PyObject *resource = NULL;
    PyObject *message = NULL;

    if (!PyArg_ParseTuple(args, "OOO", &barejid, &resource, &message)) {
        Py_RETURN_NONE;
    }

    char *barejid_str = python_str_or_unicode_to_string(barejid);
    char *resource_str = python_str_or_unicode_to_string(resource);
    char *message_str = python_str_or_unicode_to_string(message);

    allow_python_threads();
    api_incoming_message(barejid_str, resource_str, message_str);
    free(barejid_str);
    free(resource_str);
    free(message_str);
    disable_python_threads();

    Py_RETURN_NONE;
}

static PyObject*
python_api_disco_add_feature(PyObject *self, PyObject *args)
{
    PyObject *feature = NULL;
    if (!PyArg_ParseTuple(args, "O", &feature)) {
        Py_RETURN_NONE;
    }

    char *feature_str = python_str_or_unicode_to_string(feature);
    char *plugin_name = _python_plugin_name();

    allow_python_threads();
    api_disco_add_feature(plugin_name, feature_str);
    free(feature_str);
    disable_python_threads();

    free(plugin_name);

    Py_RETURN_NONE;
}

static PyObject*
python_api_encryption_reset(PyObject *self, PyObject *args)
{
    PyObject *barejid = NULL;
    if (!PyArg_ParseTuple(args, "O", &barejid)) {
        Py_RETURN_NONE;
    }

    char *barejid_str = python_str_or_unicode_to_string(barejid);

    allow_python_threads();
    api_encryption_reset(barejid_str);
    free(barejid_str);
    disable_python_threads();

    Py_RETURN_NONE;
}

static PyObject*
python_api_chat_set_titlebar_enctext(PyObject *self, PyObject *args)
{
    PyObject *barejid = NULL;
    PyObject *enctext = NULL;
    if (!PyArg_ParseTuple(args, "OO", &barejid, &enctext)) {
        Py_RETURN_NONE;
    }

    char *barejid_str = python_str_or_unicode_to_string(barejid);
    char *enctext_str = python_str_or_unicode_to_string(enctext);

    allow_python_threads();
    int res = api_chat_set_titlebar_enctext(barejid_str, enctext_str);
    free(barejid_str);
    free(enctext_str);
    disable_python_threads();

    if (res) {
        return Py_BuildValue("O", Py_True);
    } else {
        return Py_BuildValue("O", Py_False);
    }
}

static PyObject*
python_api_chat_unset_titlebar_enctext(PyObject *self, PyObject *args)
{
    PyObject *barejid = NULL;
    if (!PyArg_ParseTuple(args, "O", &barejid)) {
        Py_RETURN_NONE;
    }

    char *barejid_str = python_str_or_unicode_to_string(barejid);

    allow_python_threads();
    int res = api_chat_unset_titlebar_enctext(barejid_str);
    free(barejid_str);
    disable_python_threads();

    if (res) {
        return Py_BuildValue("O", Py_True);
    } else {
        return Py_BuildValue("O", Py_False);
    }
}

static PyObject*
python_api_chat_set_incoming_char(PyObject *self, PyObject *args)
{
    PyObject *barejid = NULL;
    PyObject *ch = NULL;
    if (!PyArg_ParseTuple(args, "OO", &barejid, &ch)) {
        Py_RETURN_NONE;
    }

    char *barejid_str = python_str_or_unicode_to_string(barejid);
    char *ch_str = python_str_or_unicode_to_string(ch);

    allow_python_threads();
    int res = api_chat_set_incoming_char(barejid_str, ch_str);
    free(barejid_str);
    free(ch_str);
    disable_python_threads();

    if (res) {
        return Py_BuildValue("O", Py_True);
    } else {
        return Py_BuildValue("O", Py_False);
    }
}

static PyObject*
python_api_chat_unset_incoming_char(PyObject *self, PyObject *args)
{
    PyObject *barejid = NULL;
    if (!PyArg_ParseTuple(args, "O", &barejid)) {
        Py_RETURN_NONE;
    }

    char *barejid_str = python_str_or_unicode_to_string(barejid);

    allow_python_threads();
    int res = api_chat_unset_incoming_char(barejid_str);
    free(barejid_str);
    disable_python_threads();

    if (res) {
        return Py_BuildValue("O", Py_True);
    } else {
        return Py_BuildValue("O", Py_False);
    }
}

static PyObject*
python_api_chat_set_outgoing_char(PyObject *self, PyObject *args)
{
    PyObject *barejid = NULL;
    PyObject *ch = NULL;
    if (!PyArg_ParseTuple(args, "OO", &barejid, &ch)) {
        Py_RETURN_NONE;
    }

    char *barejid_str = python_str_or_unicode_to_string(barejid);
    char *ch_str = python_str_or_unicode_to_string(ch);

    allow_python_threads();
    int res = api_chat_set_outgoing_char(barejid_str, ch_str);
    free(barejid_str);
    free(ch_str);
    disable_python_threads();

    if (res) {
        return Py_BuildValue("O", Py_True);
    } else {
        return Py_BuildValue("O", Py_False);
    }
}

static PyObject*
python_api_chat_unset_outgoing_char(PyObject *self, PyObject *args)
{
    PyObject *barejid = NULL;
    if (!PyArg_ParseTuple(args, "O", &barejid)) {
        Py_RETURN_NONE;
    }

    char *barejid_str = python_str_or_unicode_to_string(barejid);

    allow_python_threads();
    int res = api_chat_unset_outgoing_char(barejid_str);
    free(barejid_str);
    disable_python_threads();

    if (res) {
        return Py_BuildValue("O", Py_True);
    } else {
        return Py_BuildValue("O", Py_False);
    }
}

static PyObject*
python_api_room_set_titlebar_enctext(PyObject *self, PyObject *args)
{
    PyObject *roomjid = NULL;
    PyObject *enctext = NULL;
    if (!PyArg_ParseTuple(args, "OO", &roomjid, &enctext)) {
        Py_RETURN_NONE;
    }

    char *roomjid_str = python_str_or_unicode_to_string(roomjid);
    char *enctext_str = python_str_or_unicode_to_string(enctext);

    allow_python_threads();
    int res = api_room_set_titlebar_enctext(roomjid_str, enctext_str);
    free(roomjid_str);
    free(enctext_str);
    disable_python_threads();

    if (res) {
        return Py_BuildValue("O", Py_True);
    } else {
        return Py_BuildValue("O", Py_False);
    }
}

static PyObject*
python_api_room_unset_titlebar_enctext(PyObject *self, PyObject *args)
{
    PyObject *roomjid = NULL;
    if (!PyArg_ParseTuple(args, "O", &roomjid)) {
        Py_RETURN_NONE;
    }

    char *roomjid_str = python_str_or_unicode_to_string(roomjid);

    allow_python_threads();
    int res = api_room_unset_titlebar_enctext(roomjid_str);
    free(roomjid_str);
    disable_python_threads();

    if (res) {
        return Py_BuildValue("O", Py_True);
    } else {
        return Py_BuildValue("O", Py_False);
    }
}

static PyObject*
python_api_room_set_message_char(PyObject *self, PyObject *args)
{
    PyObject *roomjid = NULL;
    PyObject *ch = NULL;
    if (!PyArg_ParseTuple(args, "OO", &roomjid, &ch)) {
        Py_RETURN_NONE;
    }

    char *roomjid_str = python_str_or_unicode_to_string(roomjid);
    char *ch_str = python_str_or_unicode_to_string(ch);

    allow_python_threads();
    int res = api_room_set_message_char(roomjid_str, ch_str);
    free(roomjid_str);
    free(ch_str);
    disable_python_threads();

    if (res) {
        return Py_BuildValue("O", Py_True);
    } else {
        return Py_BuildValue("O", Py_False);
    }
}

static PyObject*
python_api_room_unset_message_char(PyObject *self, PyObject *args)
{
    PyObject *roomjid = NULL;
    if (!PyArg_ParseTuple(args, "O", &roomjid)) {
        Py_RETURN_NONE;
    }

    char *roomjid_str = python_str_or_unicode_to_string(roomjid);

    allow_python_threads();
    int res = api_room_unset_message_char(roomjid_str);
    free(roomjid_str);
    disable_python_threads();

    if (res) {
        return Py_BuildValue("O", Py_True);
    } else {
        return Py_BuildValue("O", Py_False);
    }
}

static PyObject*
python_api_chat_show(PyObject *self, PyObject *args)
{
    PyObject *barejid = NULL;
    PyObject *message = NULL;
    if (!PyArg_ParseTuple(args, "OO", &barejid, &message)) {
        Py_RETURN_NONE;
    }

    char *barejid_str = python_str_or_unicode_to_string(barejid);
    char *message_str = python_str_or_unicode_to_string(message);

    allow_python_threads();
    int res = api_chat_show(barejid_str, message_str);
    free(barejid_str);
    free(message_str);
    disable_python_threads();

    if (res) {
        return Py_BuildValue("O", Py_True);
    } else {
        return Py_BuildValue("O", Py_False);
    }
}

static PyObject*
python_api_chat_show_themed(PyObject *self, PyObject *args)
{
    PyObject *barejid = NULL;
    PyObject *group = NULL;
    PyObject *key = NULL;
    PyObject *def = NULL;
    PyObject *ch = NULL;
    PyObject *message = NULL;
    if (!PyArg_ParseTuple(args, "OOOOOO", &barejid, &group, &key, &def, &ch, &message)) {
        Py_RETURN_NONE;
    }

    char *barejid_str = python_str_or_unicode_to_string(barejid);
    char *group_str = python_str_or_unicode_to_string(group);
    char *key_str = python_str_or_unicode_to_string(key);
    char *def_str = python_str_or_unicode_to_string(def);
    char *ch_str = python_str_or_unicode_to_string(ch);
    char *message_str = python_str_or_unicode_to_string(message);

    allow_python_threads();
    int res = api_chat_show_themed(barejid_str, group_str, key_str, def_str, ch_str, message_str);
    free(barejid_str);
    free(group_str);
    free(key_str);
    free(def_str);
    free(ch_str);
    free(message_str);
    disable_python_threads();

    if (res) {
        return Py_BuildValue("O", Py_True);
    } else {
        return Py_BuildValue("O", Py_False);
    }
}

static PyObject*
python_api_room_show(PyObject *self, PyObject *args)
{
    PyObject *roomjid = NULL;
    PyObject *message = NULL;
    if (!PyArg_ParseTuple(args, "OO", &roomjid, &message)) {
        Py_RETURN_NONE;
    }

    char *roomjid_str = python_str_or_unicode_to_string(roomjid);
    char *message_str = python_str_or_unicode_to_string(message);

    allow_python_threads();
    int res = api_room_show(roomjid_str, message_str);
    free(roomjid_str);
    free(message_str);
    disable_python_threads();

    if (res) {
        return Py_BuildValue("O", Py_True);
    } else {
        return Py_BuildValue("O", Py_False);
    }
}

static PyObject*
python_api_room_show_themed(PyObject *self, PyObject *args)
{
    PyObject *roomjid = NULL;
    PyObject *group = NULL;
    PyObject *key = NULL;
    PyObject *def = NULL;
    PyObject *ch = NULL;
    PyObject *message = NULL;
    if (!PyArg_ParseTuple(args, "OOOOOO", &roomjid, &group, &key, &def, &ch, &message)) {
        Py_RETURN_NONE;
    }

    char *roomjid_str = python_str_or_unicode_to_string(roomjid);
    char *group_str = python_str_or_unicode_to_string(group);
    char *key_str = python_str_or_unicode_to_string(key);
    char *def_str = python_str_or_unicode_to_string(def);
    char *ch_str = python_str_or_unicode_to_string(ch);
    char *message_str = python_str_or_unicode_to_string(message);

    allow_python_threads();
    int res = api_room_show_themed(roomjid_str, group_str, key_str, def_str, ch_str, message_str);
    free(roomjid_str);
    free(group_str);
    free(key_str);
    free(def_str);
    free(ch_str);
    free(message_str);
    disable_python_threads();

    if (res) {
        return Py_BuildValue("O", Py_True);
    } else {
        return Py_BuildValue("O", Py_False);
    }
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
    { "cons_show_themed", python_api_cons_show_themed, METH_VARARGS, "Print a themed line to the console" },
    { "cons_bad_cmd_usage", python_api_cons_bad_cmd_usage, METH_VARARGS, "Show invalid command message in console" },
    { "register_command", python_api_register_command, METH_VARARGS, "Register a command." },
    { "register_timed", python_api_register_timed, METH_VARARGS, "Register a timed function." },
    { "completer_add", python_api_completer_add, METH_VARARGS, "Add items to an autocompleter." },
    { "completer_remove", python_api_completer_remove, METH_VARARGS, "Remove items from an autocompleter." },
    { "completer_clear", python_api_completer_clear, METH_VARARGS, "Remove all items from an autocompleter." },
    { "filepath_completer_add", python_api_filepath_completer_add, METH_VARARGS, "Add filepath autocompleter" },
    { "send_line", python_api_send_line, METH_VARARGS, "Send a line of input." },
    { "notify", python_api_notify, METH_VARARGS, "Send desktop notification." },
    { "get_current_recipient", python_api_get_current_recipient, METH_VARARGS, "Return the jid of the recipient of the current window." },
    { "get_current_muc", python_api_get_current_muc, METH_VARARGS, "Return the jid of the room of the current window." },
    { "get_current_nick", python_api_get_current_nick, METH_VARARGS, "Return nickname in current room." },
    { "get_current_occupants", python_api_get_current_occupants, METH_VARARGS, "Return list of occupants in current room." },
    { "current_win_is_console", python_api_current_win_is_console, METH_VARARGS, "Returns whether the current window is the console." },
    { "get_room_nick", python_api_get_room_nick, METH_VARARGS, "Return the nickname used in the specified room, or None if not in the room." },
    { "log_debug", python_api_log_debug, METH_VARARGS, "Log a debug message" },
    { "log_info", python_api_log_info, METH_VARARGS, "Log an info message" },
    { "log_warning", python_api_log_warning, METH_VARARGS, "Log a warning message" },
    { "log_error", python_api_log_error, METH_VARARGS, "Log an error message" },
    { "win_exists", python_api_win_exists, METH_VARARGS, "Determine whether a window exists." },
    { "win_create", python_api_win_create, METH_VARARGS, "Create a new window." },
    { "win_focus", python_api_win_focus, METH_VARARGS, "Focus a window." },
    { "win_show", python_api_win_show, METH_VARARGS, "Show text in the window." },
    { "win_show_themed", python_api_win_show_themed, METH_VARARGS, "Show themed text in the window." },
    { "send_stanza", python_api_send_stanza, METH_VARARGS, "Send an XMPP stanza." },
    { "settings_boolean_get", python_api_settings_boolean_get, METH_VARARGS, "Get a boolean setting." },
    { "settings_boolean_set", python_api_settings_boolean_set, METH_VARARGS, "Set a boolean setting." },
    { "settings_string_get", python_api_settings_string_get, METH_VARARGS, "Get a string setting." },
    { "settings_string_set", python_api_settings_string_set, METH_VARARGS, "Set a string setting." },
    { "settings_int_get", python_api_settings_int_get, METH_VARARGS, "Get a integer setting." },
    { "settings_int_set", python_api_settings_int_set, METH_VARARGS, "Set a integer setting." },
    { "settings_string_list_get", python_api_settings_string_list_get, METH_VARARGS, "Get a string list setting." },
    { "settings_string_list_add", python_api_settings_string_list_add, METH_VARARGS, "Add item to string list setting." },
    { "settings_string_list_remove", python_api_settings_string_list_remove, METH_VARARGS, "Remove item from string list setting." },
    { "settings_string_list_clear", python_api_settings_string_list_clear, METH_VARARGS, "Remove all items from string list setting." },
    { "incoming_message", python_api_incoming_message, METH_VARARGS, "Show an incoming message." },
    { "disco_add_feature", python_api_disco_add_feature, METH_VARARGS, "Add a feature to disco info response." },
    { "encryption_reset", python_api_encryption_reset, METH_VARARGS, "End encrypted chat session with barejid, if one exists" },
    { "chat_set_titlebar_enctext", python_api_chat_set_titlebar_enctext, METH_VARARGS, "Set the encryption status in the title bar for the specified contact" },
    { "chat_unset_titlebar_enctext", python_api_chat_unset_titlebar_enctext, METH_VARARGS, "Reset the encryption status in the title bar for the specified recipient" },
    { "chat_set_incoming_char", python_api_chat_set_incoming_char, METH_VARARGS, "Set the incoming message prefix character for specified contact" },
    { "chat_unset_incoming_char", python_api_chat_unset_incoming_char, METH_VARARGS, "Reset the incoming message prefix character for specified contact" },
    { "chat_set_outgoing_char", python_api_chat_set_outgoing_char, METH_VARARGS, "Set the outgoing message prefix character for specified contact" },
    { "chat_unset_outgoing_char", python_api_chat_unset_outgoing_char, METH_VARARGS, "Reset the outgoing message prefix character for specified contact" },
    { "room_set_titlebar_enctext", python_api_room_set_titlebar_enctext, METH_VARARGS, "Set the encryption status in the title bar for the specified room" },
    { "room_unset_titlebar_enctext", python_api_room_unset_titlebar_enctext, METH_VARARGS, "Reset the encryption status in the title bar for the specified room" },
    { "room_set_message_char", python_api_room_set_message_char, METH_VARARGS, "Set the message prefix character for specified room" },
    { "room_unset_message_char", python_api_room_unset_message_char, METH_VARARGS, "Reset the message prefix character for specified room" },
    { "chat_show", python_api_chat_show, METH_VARARGS, "Print a line in a chat window" },
    { "chat_show_themed", python_api_chat_show_themed, METH_VARARGS, "Print a themed line in a chat window" },
    { "room_show", python_api_room_show, METH_VARARGS, "Print a line in a chat room window" },
    { "room_show_themed", python_api_room_show_themed, METH_VARARGS, "Print a themed line in a chat room window" },
    { NULL, NULL, 0, NULL }
};

#if PY_MAJOR_VERSION >= 3
static struct PyModuleDef profModule =
{
    PyModuleDef_HEAD_INIT,
    "prof",
    "",
    -1,
    apiMethods
};
#endif

PyMODINIT_FUNC
python_api_init(void)
{
#if PY_MAJOR_VERSION >= 3
    PyObject *result = PyModule_Create(&profModule);
    if (!result) {
        log_debug("Failed to initialise prof module");
    } else {
        log_debug("Initialised prof module");
    }
    return result;
#else
    Py_InitModule("prof", apiMethods);
#endif
}

void
python_init_prof(void)
{
#if PY_MAJOR_VERSION >= 3
    PyImport_AppendInittab("prof", python_api_init);
    Py_Initialize();
    PyEval_InitThreads();
#else
    Py_Initialize();
    PyEval_InitThreads();
    python_api_init();
#endif
}

static char*
_python_plugin_name(void)
{
    PyThreadState *ts = PyThreadState_Get();
    PyFrameObject *frame = ts->frame;
    char* filename = python_str_or_unicode_to_string(frame->f_code->co_filename);
    gchar **split = g_strsplit(filename, "/", 0);
    free(filename);
    char *plugin_name = strdup(split[g_strv_length(split)-1]);
    g_strfreev(split);

    return plugin_name;
}

char*
python_str_or_unicode_to_string(void *obj)
{
    if (!obj) {
        return NULL;
    }
    PyObject *pyobj = (PyObject*)obj;
    if (pyobj == Py_None) {
        return NULL;
    }

#if PY_MAJOR_VERSION >= 3
    if (PyUnicode_Check(pyobj)) {
        PyObject *utf8_str = PyUnicode_AsUTF8String(pyobj);
        char *result = strdup(PyBytes_AS_STRING(utf8_str));
        Py_XDECREF(utf8_str);
        return result;
    } else {
        return strdup(PyBytes_AS_STRING(pyobj));
    }
#else
    if (PyUnicode_Check(pyobj)) {
        PyObject *utf8_str = PyUnicode_AsUTF8String(pyobj);
        char *result = strdup(PyString_AsString(utf8_str));
        Py_XDECREF(utf8_str);
        return result;
    } else {
        return strdup(PyString_AsString(pyobj));
    }
#endif
}
