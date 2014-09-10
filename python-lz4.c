/*
 * Copyright (c) 2012-2013, Steeve Morin
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of Steeve Morin nor the names of its contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <Python.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "lz4.h"
#include "lz4hc.h"
#include "python-lz4.h"
#include "structmember.h"

#define MAX(a, b)               ((a) > (b) ? (a) : (b))
#define throwWarn(msg)    PyErr_WarnEx(PyExc_UserWarning, msg, 1)

typedef int (*compressor)(const char *source, char *dest, int isize);

static int LZ4S_GetBlockSize_FromBlockId (int id) { return (1 << (8 + (2 * id))); }

static const int hdr_size = sizeof(uint32_t);

typedef struct {
    PyObject_HEAD
    LZ4_streamDecode_t lz4sd;
} Lz4sd_t;

static void Lz4sd_t_dealloc(Lz4sd_t* self)
{
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *Lz4sd_t_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    Lz4sd_t *self;
    (void) args;
    (void) kwds;

    self = (Lz4sd_t *)type->tp_alloc(type, 0);
    memset(&self->lz4sd, 0, sizeof(self->lz4sd));

    return (PyObject *)self;
}

static PyMemberDef Lz4sd_t_members[] = {
    {"lz4_streamDecode_t", T_UINT, offsetof(Lz4sd_t, lz4sd), 0, ""},
    {NULL}  /* Sentinel */
};

static PyTypeObject Lz4sd_t_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "lz4.Lz4sd_t",             /*tp_name*/
    sizeof(Lz4sd_t),           /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)Lz4sd_t_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,        /*tp_flags*/
    "Lz4sd_t for decode_continue",           /* tp_doc */
    0,		                   /* tp_traverse */
    0,		                   /* tp_clear */
    0,		                   /* tp_richcompare */
    0,		                   /* tp_weaklistoffset */
    0,		                   /* tp_iter */
    0,		                   /* tp_iternext */
    0,                         /* tp_methods */
    Lz4sd_t_members,           /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,                         /* tp_init */
    0,                         /* tp_alloc */
    Lz4sd_t_new,               /* tp_new */
    0,                         /* tp_free */ 
    0,                         /* tp_is_gc*/
    0,                         /* tp_bases*/
    0,                         /* tp_mro*/
    0,                         /* tp_cache*/
    0,                         /* tp_subclasses*/
    0,                         /* tp_weaklis*/
    0,                         /* tp_del*/
    2
};

static PyObject *py_lz4_decompress_continue(PyObject *self, PyObject *args, PyObject *keywds) {
    PyObject *result;
    PyObject *lz4sd_t;
    LZ4_streamDecode_t temp;
    Lz4sd_t *temp2;
    const char *source;
    int source_size;
    uint32_t dest_size;
    int blkID=7;
    static char *kwlist[] = {"source", "lz4sd", "blkID"};

    (void)self;
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "s#O|i", kwlist, &source, 
                                     &source_size, &lz4sd_t, &blkID)) {
        return NULL;
    }

    dest_size = LZ4S_GetBlockSize_FromBlockId(blkID);
    if (dest_size > INT_MAX) {
        PyErr_Format(PyExc_ValueError, "invalid size in header: 0x%x", dest_size);
        return NULL;
    }
    temp2=(Lz4sd_t *)lz4sd_t;
    temp = temp2->lz4sd;
    if (/*result != NULL && */dest_size > 0) {
        char* dest = (char*)malloc(dest_size);
        int osize = LZ4_decompress_safe_continue(&temp, source, dest, source_size, dest_size);
        result = PyBytes_FromStringAndSize(dest, osize);
        free(dest);
        if (osize < 0) {
            PyErr_Format(PyExc_ValueError, "corrupt input at byte %d", -osize);
            Py_CLEAR(result);
        }
    }
    else { return Py_None; }

    return result;
}

static PyMethodDef Lz4Methods[] = {
    {"decompress_continue",  (PyCFunction)py_lz4_decompress_continue, METH_VARARGS | METH_KEYWORDS, UNCOMPRESS_DOCSTRING},
    {NULL, NULL, 0, NULL}
};



struct module_state {
    PyObject *error;
};

#if PY_MAJOR_VERSION >= 3
#define GETSTATE(m) ((struct module_state*)PyModule_GetState(m))
#else
#define GETSTATE(m) (&_state)
static struct module_state _state;
#endif

#if PY_MAJOR_VERSION >= 3

static int myextension_traverse(PyObject *m, visitproc visit, void *arg) {
    Py_VISIT(GETSTATE(m)->error);
    return 0;
}

static int myextension_clear(PyObject *m) {
    Py_CLEAR(GETSTATE(m)->error);
    return 0;
}


static struct PyModuleDef moduledef = {
        PyModuleDef_HEAD_INIT,
        "lz4",
        NULL,
        sizeof(struct module_state),
        Lz4Methods,
        NULL,
        myextension_traverse,
        myextension_clear,
        NULL
};

#define INITERROR return NULL
PyObject *PyInit_lz4(void)

#else
#define INITERROR return
void initlz4(void)

#endif
{
#if PY_MAJOR_VERSION >= 3
    PyObject *module = PyModule_Create(&moduledef);
#else
    PyObject *module = Py_InitModule("lz4", Lz4Methods);
#endif
    struct module_state *st = NULL;

    if (module == NULL) {
        INITERROR;
    }
    st = GETSTATE(module);

    if (PyType_Ready(&Lz4sd_t_Type) < 0)
        return;

    st->error = PyErr_NewException("lz4.Error", NULL, NULL);
    if (st->error == NULL) {
        Py_DECREF(module);
        INITERROR;
    }

    PyModule_AddStringConstant(module, "VERSION", VERSION);
    PyModule_AddStringConstant(module, "__version__", VERSION);
    PyModule_AddStringConstant(module, "LZ4_VERSION", LZ4_VERSION);
    PyModule_AddObject(module, "Lz4sd_t", (PyObject *)&Lz4sd_t_Type);

#if PY_MAJOR_VERSION >= 3
    return module;
#endif
}
