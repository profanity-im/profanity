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

// API

static PyObject*
api_cons_show(PyObject *self, PyObject *args)
{
    const char *message = NULL;
    if (!PyArg_ParseTuple(args, "s", &message)) {
        return NULL;
    }
    cons_show("%s", message);
    return NULL;
}

static PyMethodDef apiMethods[] = {
    { "cons_show", api_cons_show, METH_VARARGS, "Print a line to the console." },
    { NULL, NULL, 0, NULL }
};

void
api_init(void)
{
    PyObject *pName, *pModule, *pFunc;

    Py_Initialize();
    Py_InitModule("prof", apiMethods);
    pName = PyString_FromString("helloworld");
    pModule = PyImport_Import(pName);
    Py_DECREF(pName);

    if (pModule != NULL) {

        pFunc = PyObject_GetAttrString(pModule, "prof_on_start");

        if (pFunc == NULL) {
            cons_show("NULL pfunc");
        }

        if (pFunc && PyCallable_Check(pFunc)) {
            PyObject_CallObject(pFunc, NULL);
        }
        else {
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

