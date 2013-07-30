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
    PyObject *pName, *pModule, *pProfInit, *pProfOnStart, *pArgs;

    Py_Initialize();
    PySys_SetPath("$PYTHONPATH:./plugins/");
    Py_InitModule("prof", apiMethods);
    pName = PyString_FromString("helloworld");
    pModule = PyImport_Import(pName);
    Py_DECREF(pName);

    if (pModule != NULL) {
        pProfInit = PyObject_GetAttrString(pModule, "prof_init");
        if (pProfInit && PyCallable_Check(pProfInit)) {
            pArgs = Py_BuildValue("ss", PACKAGE_VERSION, PACKAGE_STATUS);
            PyObject_CallObject(pProfInit, pArgs);
            Py_XDECREF(pArgs);
        }
        Py_XDECREF(pProfInit);

        pProfOnStart = PyObject_GetAttrString(pModule, "prof_on_start");
        if (pProfOnStart && PyCallable_Check(pProfOnStart)) {
            PyObject_CallObject(pProfOnStart, NULL);
        }
        Py_XDECREF(pProfOnStart);

        Py_DECREF(pModule);
    }
    else {
        cons_show("Failed to load plugin");
        return ;
    }
    Py_Finalize();
    return;
}

