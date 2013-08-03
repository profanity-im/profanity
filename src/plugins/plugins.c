/*
 * plugins.c
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

#include "plugins/api.h"
#include "plugins/command.h"
#include "plugins/plugins.h"
#include "ui/ui.h"

static GSList* _get_module_names(void);
static void _init(void);
static void _on_start(void);
static void _run_plugins(const char * const function, PyObject *p_args);

static GSList* plugins;
void
plugins_init(void)
{
    plugins = NULL;
    PyObject *p_module;

    GSList *module_names = _get_module_names();

    Py_Initialize();
    api_init();

    // TODO change to use XDG spec
    GString *path = g_string_new(Py_GetPath());
    g_string_append(path, ":./plugins/");
    PySys_SetPath(path->str);
    g_string_free(path, TRUE);

    if (module_names != NULL) {
        cons_show("Loading plugins...");

        GSList *module_name = module_names;

        while (module_name != NULL) {
            p_module = PyImport_ImportModule(module_name->data);
            if (p_module != NULL) {
                cons_show("Loaded plugin: %s", module_name->data);
                plugins = g_slist_append(plugins, p_module);
            }

            module_name = g_slist_next(module_name);
        }

        cons_show("");

        _init();
        _on_start();
        if (PyErr_Occurred()) {
            PyErr_Print();
            PyErr_Clear();
        }
    }
    return;
}

void
plugins_shutdown(void)
{
    Py_Finalize();
}

void
plugins_on_connect(void)
{
    _run_plugins("prof_on_connect", NULL);
}

static GSList *
_get_module_names(void)
{
    GSList *result = NULL;

    // TODO change to use XDG
    GDir *plugins_dir = g_dir_open("./plugins", 0, NULL);

    if (plugins_dir != NULL) {
        const gchar *file = g_dir_read_name(plugins_dir);
        while (file != NULL) {
            if (g_str_has_suffix(file, ".py")) {
                gchar *module_name = g_strndup(file, strlen(file) - 3);
                result = g_slist_append(result, module_name);
            }
            file = g_dir_read_name(plugins_dir);
        }
        g_dir_close(plugins_dir);
        return result;
    } else {
        return NULL;
    }
}

static void
_init(void)
{
    PyObject *p_args = Py_BuildValue("ss", PACKAGE_VERSION, PACKAGE_STATUS);
    _run_plugins("prof_init", p_args);
    Py_XDECREF(p_args);
}

static void
_on_start(void)
{
    _run_plugins("prof_on_start", NULL);
}

static void
_run_plugins(const char * const function, PyObject *p_args)
{
    GSList *plugin = plugins;
    PyObject *p_function;

    while (plugin != NULL) {
        PyObject *p_module = plugin->data;
        if (PyObject_HasAttrString(p_module, function)) {
            p_function = PyObject_GetAttrString(p_module, function);
            if (p_function && PyCallable_Check(p_function)) {
                PyObject_CallObject(p_function, p_args);
                Py_XDECREF(p_function);
            }
        }

        plugin = g_slist_next(plugin);
   }
}
