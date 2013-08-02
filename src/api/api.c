/*
 * api.c
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

#include "ui/ui.h"

static GSList* _get_module_names(void);

// API

static PyObject*
api_cons_show(PyObject *self, PyObject *args)
{
    const char *message = NULL;
    if (!PyArg_ParseTuple(args, "s", &message)) {
        return NULL;
    }
    cons_show("%s", message);
    return Py_BuildValue("");
}

static PyMethodDef apiMethods[] = {
    { "cons_show", api_cons_show, METH_VARARGS, "Print a line to the console." },
    { NULL, NULL, 0, NULL }
};

void
api_init(void)
{
    PyObject *p_module, *p_prof_init, *p_args, *p_prof_on_start;

    GSList *module_names = _get_module_names();

    Py_Initialize();
    Py_InitModule("prof", apiMethods);

    // TODO change to use XDG spec
    PySys_SetPath("$PYTHONPATH:./plugins/");

    if (module_names != NULL) {
        cons_show("Loading plugins...");

        GSList *module_name = module_names;

        while (module_name != NULL) {
            cons_show("Loading plugin: %s", module_name->data);

            p_module = PyImport_ImportModule(module_name->data);

            if (p_module != NULL) {
                cons_show("LOADED");
                p_prof_init = PyObject_GetAttrString(p_module, "prof_init");
                if (p_prof_init && PyCallable_Check(p_prof_init)) {
                    p_args = Py_BuildValue("ss", PACKAGE_VERSION, PACKAGE_STATUS);
                    PyObject_CallObject(p_prof_init, p_args);
                    Py_XDECREF(p_args);
                    Py_XDECREF(p_prof_init);
                }

                p_prof_on_start = PyObject_GetAttrString(p_module, "prof_on_start");
                if (p_prof_on_start && PyCallable_Check(p_prof_on_start)) {
                    PyObject_CallObject(p_prof_on_start, NULL);
                    Py_XDECREF(p_prof_on_start);
                }
                Py_XDECREF(p_module);
            } else {
                cons_show("p_module NULL");
                if (PyErr_Occurred()) {
                    PyErr_Print();
                }
            }

            module_name = g_slist_next(module_name);
        }
    }
    Py_Finalize();
    return;
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
