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

void
api_init(void)
{
    PyObject *pName, *pModule, *pFunc;
    PyObject *pValue;

    Py_Initialize();
    pName = PyString_FromString("profplugin");
    pModule = PyImport_Import(pName);
    Py_DECREF(pName);

    if (pModule != NULL) {

        pFunc = PyObject_GetAttrString(pModule, "pluginname");

        if (pFunc == NULL) {
            cons_show("NULL pfunc");
        }

        if (pFunc && PyCallable_Check(pFunc)) {
            pValue = PyObject_CallObject(pFunc, NULL);
            if (pValue != NULL) {
                cons_show("Plugin loaded");
                cons_show("Result of call: %s", PyString_AsString(pValue));
                Py_DECREF(pValue);
            }
            else {
                Py_DECREF(pFunc);
                Py_DECREF(pModule);
                cons_show("Error loading plugin");
                return;
            }
        }
        else {
            if (PyErr_Occurred())
                cons_show("PyErr occurred");
            cons_show("Could not find function");
        }
        Py_XDECREF(pFunc);
        Py_DECREF(pModule);
    }
    else {
        cons_show("Failed to load plugin");
        return ;
    }
    Py_Finalize();
    return;    
}

void
api_prof_cons_show(const char *message)
{
    if (message != NULL) {
        cons_show("%s", message);
    }
}
