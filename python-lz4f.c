/*
 * Copyright (c) 2012-2014, Christopher Jackson
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
 * 3. Neither the name of Christopher Jackson nor the names of its contributors 
 *    may be used to endorse or promote products derived from this software 
 *    without specific prior written permission.
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
#include "lz4frame.h"
#include "python-lz4f.h"
#include "structmember.h"

//#define MAX(a, b)               ((a) > (b) ? (a) : (b))

static int LZ4S_GetBlockSize_FromBlockId (int id) { return (1 << (8 + (2 * id))); }

/* Lz4sd start */
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
    {NULL}
};

static PyTypeObject Lz4sd_t_Type = {
    PyObject_HEAD_INIT(NULL)
    0/*ob_size*/,     "lz4.Lz4sd_t",      sizeof(Lz4sd_t),      0/*tp_itemsize*/,
    (destructor)Lz4sd_t_dealloc, /*tp_dealloc*/
    0/*tp_print*/,     0/*tp_getattr*/,   0/*tp_setattr*/,      0/*tp_compare*/,
    0/*tp_repr*/,      0/*tp_as_number*/, 0/*tp_as_sequence*/,  0/*tp_as_mapping*/,
    0/*tp_hash */,     0/*tp_call*/,      0/*tp_str*/,          0/*tp_getattro*/,
    0/*tp_setattro*/,  0/*tp_as_buffer*/, Py_TPFLAGS_DEFAULT,   "Lz4sd_t for decode_continue",
    0/* tp_traverse*/, 0/* tp_clear */,   0/* tp_richcompare*/, 0/* tp_weaklistoffset */,
    0/* tp_iter */,    0/* tp_iternext*/, 0/* tp_methods */,    Lz4sd_t_members,
    0/* tp_getset */,  0/* tp_base */,    0/* tp_dict */,       0/* tp_descr_get */,
    0/* tp_descr_set*/,0/* tp_dictoffst*/,0/* tp_init */,       0/* tp_alloc */,
    Lz4sd_t_new,       0/* tp_free */,    0/* tp_is_gc*/,       0/* tp_bases*/,
    0/* tp_mro*/,      0/* tp_cache*/,    0/* tp_subclasses*/,  0/* tp_weaklis*/,
    0/* tp_del*/,      2
};

/* Decompression methods */ 
static PyObject *py_lz4_createDecompressionContext(PyObject *self, PyObject *args) {
    PyObject *result;
    LZ4F_decompressionContext_t dCtx;
    size_t err;

    (void)self;
    (void)args;
   
    err = LZ4F_createDecompressionContext(&dCtx, LZ4F_VERSION);
    CHECK(LZ4F_isError(err), "Allocation failed (error %i)", (int)err);
    result = PyCObject_FromVoidPtr(dCtx, NULL/*LZ4F_freeDecompressionContext*/);

    return result;
_output_error:
    return Py_None; 
}

static PyObject *py_lz4f_getFrameInfo(PyObject *self, PyObject *args) {
    PyObject *result;
    PyObject *py_dCtx;
    LZ4F_decompressionContext_t dCtx;
    LZ4F_frameInfo_t frameInfoHold = { 0 };
    LZ4F_frameInfo_t *frameInfo;
    const char *source;
    size_t source_size;
    size_t err;

    (void)self;
    if (!PyArg_ParseTuple(args, "s#O", &source, &source_size, &py_dCtx)) {
        return NULL;
    }
    
    frameInfo = &frameInfoHold;
    dCtx = (LZ4F_decompressionContext_t)PyCObject_AsVoidPtr(py_dCtx);

    err = LZ4F_getFrameInfo(dCtx, frameInfo, (unsigned char*)source, &source_size);
    CHECK(LZ4F_isError(err), "Failed getting frameInfo. (error %i)", (int)err);

    frameInfoHold = *frameInfo;
    //py_dCtx = PyCObject_FromVoidPtr(dCtx, NULL/*LZ4F_freeDecompressionContext*/);
    result = PyLong_FromSize_t(frameInfoHold.blockSizeID);

    return result;
_output_error:
    return Py_None; 
}

static PyObject *pass_lz4f_decompress(PyObject *self, PyObject *args, PyObject *keywds) {
    PyObject *result = Py_None;
    PyObject *py_dCtx;
    LZ4F_decompressionContext_t dCtx;
    const char *source;
    size_t source_size;
    size_t dest_size;
    size_t err;
    unsigned int blkID=7;
    static char *kwlist[] = {"source", "dCtx", "blkID"};

    (void)self;
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "s#O|i", kwlist, &source, 
                                     &source_size, &py_dCtx, &blkID)) {
        return NULL;
    }

    dest_size = LZ4S_GetBlockSize_FromBlockId(blkID);
    dCtx = (LZ4F_decompressionContext_t)PyCObject_AsVoidPtr(py_dCtx);
    
    if (/*result != NULL && */dest_size > 0) {
        char* dest = (char*)malloc(dest_size);
        err = LZ4F_decompress(dCtx, dest, &dest_size, source, &source_size, NULL);
        //CHECK(LZ4F_isError(err), "Failed getting frameInfo. (error %i)", (int)err);
        fprintf(stdout, "Dest_size: %i  Error Code:%i \n", dest_size, err);
        result = PyBytes_FromStringAndSize(dest, dest_size);
        free(dest);
        /*if (osize < 0) {
            PyErr_Format(PyExc_ValueError, "corrupt input at byte %d", -osize);
            Py_CLEAR(result);
        }*/
    }

    return result;
_output_error:
    return Py_None; 
}

static PyObject *pass_lz4_decompress_continue(PyObject *self, PyObject *args, PyObject *keywds) {
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
    /*if (dest_size > INT_MAX) {
        PyErr_Format(PyExc_ValueError, "invalid size in header: 0x%x", dest_size);
        return NULL;
    }*/
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
    {"decompress_continue",  (PyCFunction)pass_lz4_decompress_continue, METH_VARARGS | METH_KEYWORDS, UNCOMPRESS_DOCSTRING},
    {"decompressFrame",  (PyCFunction)pass_lz4f_decompress, METH_VARARGS | METH_KEYWORDS, UNCOMPRESS_DOCSTRING},
    {"getFrameInfo", py_lz4f_getFrameInfo, METH_VARARGS, NULL},
    {"createDecompContext", py_lz4_createDecompressionContext, METH_VARARGS, NULL},
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
        "lz4f",
        NULL,
        sizeof(struct module_state),
        Lz4fMethods,
        NULL,
        myextension_traverse,
        myextension_clear,
        NULL
};

#define INITERROR return NULL
PyObject *PyInit_lz4f(void)

#else
#define INITERROR return
void initlz4f(void)

#endif
{
#if PY_MAJOR_VERSION >= 3
    PyObject *module = PyModule_Create(&moduledef);
#else
    PyObject *module = Py_InitModule("lz4f", Lz4Methods);
#endif
    struct module_state *state = NULL;

    if (module == NULL) {
        INITERROR;
    }
    state = GETSTATE(module);

    if (PyType_Ready(&Lz4sd_t_Type) < 0)
        return;

    state->error = PyErr_NewException("lz4.Error", NULL, NULL);
    if (state->error == NULL) {
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
