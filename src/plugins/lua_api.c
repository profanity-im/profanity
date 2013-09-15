/*
 * lua_api.c
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

#include <lua.h>

#include <glib.h>

#include "plugins/api.h"
#include "plugins/lua_api.h"
#include "plugins/callbacks.h"

static int
lua_api_cons_alert(lua_State *L)
{
    api_cons_alert();
    return 0;
}

static int
lua_api_cons_show(lua_State *L)
{
    const char *message = lua_tostring(L, 1);
    api_cons_show(message);
    return 0;
}

static int
lua_api_register_command(lua_State *L)
{
/*
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
*/
    return 0;
}

static int
lua_api_register_timed(lua_State *L)
{
/*
    PyObject *p_callback = NULL;
    int interval_seconds = 0;

    if (!PyArg_ParseTuple(args, "Oi", &p_callback, &interval_seconds)) {
        return Py_BuildValue("");
    }

    if (p_callback && PyCallable_Check(p_callback)) {
        api_register_timed(p_callback, interval_seconds, python_timed_callback);
    }

    return Py_BuildValue("");
*/
    return 0;
}

static int
lua_api_notify(lua_State *L)
{
/*
    const char *message = NULL;
    const char *category = NULL;
    int timeout_ms = 5000;

    if (!PyArg_ParseTuple(args, "sis", &message, &timeout_ms, &category)) {
        return Py_BuildValue("");
    }

    api_notify(message, category, timeout_ms);

    return Py_BuildValue("");
*/
    return 0;
}

static int
lua_api_send_line(lua_State *L)
{
/*
    char *line = NULL;
    if (!PyArg_ParseTuple(args, "s", &line)) {
        return Py_BuildValue("");
    }

    api_send_line(line);

    return Py_BuildValue("");
*/
    return 0;
}

static int
lua_api_get_current_recipient(lua_State *L)
{
/*
    char *recipient = api_get_current_recipient();
    if (recipient != NULL) {
        return Py_BuildValue("s", recipient);
    } else {
        return Py_BuildValue("");
    }
*/
    return 0;
}

static int
lua_api_log_debug(lua_State *L)
{
    const char *message = lua_tostring(L, 1);
    api_log_debug(message);
    return 0;
}

static int
lua_api_log_info(lua_State *L)
{
    const char *message = lua_tostring(L, 1);
    api_log_info(message);
    return 0;
}

static int
lua_api_log_warning(lua_State *L)
{
    const char *message = lua_tostring(L, 1);
    api_log_warning(message);
    return 0;
}

static int
lua_api_log_error(lua_State *L)
{
    const char *message = lua_tostring(L, 1);
    api_log_error(message);
    return 0;
}

static int
lua_api_win_exists(lua_State *L)
{
/*
    char *tag = NULL;
    if (!PyArg_ParseTuple(args, "s", &tag)) {
        return Py_BuildValue("");
    }

    if (api_win_exists(tag)) {
        return Py_BuildValue("i", 1);
    } else {
        return Py_BuildValue("i", 0);
    }
*/
    return 0;
}

static int
lua_api_win_create(lua_State *L)
{
/*
    char *tag = NULL;
    PyObject *p_callback = NULL;

    if (!PyArg_ParseTuple(args, "sO", &tag, &p_callback)) {
        return Py_BuildValue("");
    }

    if (p_callback && PyCallable_Check(p_callback)) {
        api_win_create(tag, p_callback, python_window_callback);
    }

    return Py_BuildValue("");
*/
    return 0;
}

static int
lua_api_win_focus(lua_State *L)
{
/*
    char *tag = NULL;

    if (!PyArg_ParseTuple(args, "s", &tag)) {
        return Py_BuildValue("");
    }

    api_win_focus(tag);
    return Py_BuildValue("");
*/
    return 0;
}

static int
lua_api_win_process_line(lua_State *L)
{
/*
    char *tag = NULL;
    char *line = NULL;

    if (!PyArg_ParseTuple(args, "ss", &tag, &line)) {
        return Py_BuildValue("");
    }

    api_win_process_line(tag, line);
    return Py_BuildValue("");
*/
    return 0;
}

static int
lua_api_win_show(lua_State *L)
{
/*
    char *tag = NULL;
    char *line = NULL;

    if (!PyArg_ParseTuple(args, "ss", &tag, &line)) {
        return Py_BuildValue("");
    }

    api_win_show(tag, line);
    return Py_BuildValue("");
*/
    return 0;
}

void
lua_command_callback(PluginCommand *command, gchar **args)
{
/*
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
*/
}

void
lua_timed_callback(PluginTimedFunction *timed_function)
{
/*
    PyObject_CallObject(timed_function->callback, NULL);
*/
}

void
lua_window_callback(PluginWindowCallback *window_callback, char *tag, char *line)
{
/*
    PyObject *p_args = NULL;
    p_args = Py_BuildValue("ss", tag, line);
    PyObject_CallObject(window_callback->callback, p_args);
    Py_XDECREF(p_args);

    if (PyErr_Occurred()) {
        PyErr_Print();
        PyErr_Clear();
    }
*/
}
void
lua_api_init(lua_State *L)
{
    lua_pushcfunction(L, lua_api_cons_alert);
    lua_setglobal(L, "prof_cons_alert");
    lua_pushcfunction(L, lua_api_cons_show);
    lua_setglobal(L, "prof_cons_show");
    lua_pushcfunction(L, lua_api_register_command);
    lua_setglobal(L, "prof_register_command");
    lua_pushcfunction(L, lua_api_register_timed);
    lua_setglobal(L, "prof_register_timed");
    lua_pushcfunction(L, lua_api_send_line);
    lua_setglobal(L, "prof_send_line");
    lua_pushcfunction(L, lua_api_notify);
    lua_setglobal(L, "prof_notify");
    lua_pushcfunction(L, lua_api_get_current_recipient);
    lua_setglobal(L, "prof_get_current_recipient");
    lua_pushcfunction(L, lua_api_log_debug);
    lua_setglobal(L, "prof_log_debug");
    lua_pushcfunction(L, lua_api_log_info);
    lua_setglobal(L, "prof_log_info");
    lua_pushcfunction(L, lua_api_log_warning);
    lua_setglobal(L, "prof_log_warning");
    lua_pushcfunction(L, lua_api_log_error);
    lua_setglobal(L, "prof_log_error");
    lua_pushcfunction(L, lua_api_win_exists);
    lua_setglobal(L, "prof_win_exists");
    lua_pushcfunction(L, lua_api_win_create);
    lua_setglobal(L, "prof_win_create");
    lua_pushcfunction(L, lua_api_win_focus);
    lua_setglobal(L, "prof_win_focus");
    lua_pushcfunction(L, lua_api_win_show);
    lua_setglobal(L, "prof_win_show");
    lua_pushcfunction(L, lua_api_win_process_line);
    lua_setglobal(L, "prof_win_process_line");
}
