/*
 * Copyright (c) 2014, Christopher Jackson
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

#if PY_MAJOR_VERSION >= 3
   #define PyInt_FromSize_t(x) PyLong_FromSize_t(x)
#endif

#define CHECK(cond, ...) if (LZ4F_isError(cond)) { printf("%s%s", "Error => ", LZ4F_getErrorName(cond)); goto _output_error; }

static int LZ4S_GetBlockSize_FromBlockId (int id) { return (1 << (8 + (2 * id))); }

/* Compression methods */

static PyObject *py_lz4f_createCompCtx(PyObject *self, PyObject *args) {
    PyObject *result;
    LZ4F_compressionContext_t cCtx;
    size_t err;

    (void)self;
    (void)args;

    err = LZ4F_createCompressionContext(&cCtx, LZ4F_VERSION);
    CHECK(err, "Allocation failed (error %i)", (int)err);
    result = PyCapsule_New(cCtx, NULL, NULL);

    return result;
_output_error:
    return Py_None;
}

static PyObject *py_lz4f_freeCompCtx(PyObject *self, PyObject *args) {
    PyObject *py_cCtx;
    LZ4F_compressionContext_t cCtx;

    (void)self;
    if (!PyArg_ParseTuple(args, "O", &py_cCtx)) {
        return NULL;
    }

    cCtx = (LZ4F_compressionContext_t)PyCapsule_GetPointer(py_cCtx, NULL);
    LZ4F_freeCompressionContext(cCtx);

    return Py_None;
}

static PyObject *py_lz4f_compressFrame(PyObject *self, PyObject *args) {
    PyObject *result;
    const char* source;
    char* dest;
    int src_size;
    size_t dest_size;
    size_t final_size;
    size_t ssrc_size;

    (void)self;
    if (!PyArg_ParseTuple(args, "s#", &source, &src_size)) {
        return NULL;
    }

    ssrc_size = (size_t)src_size;
    dest_size = LZ4F_compressFrameBound(ssrc_size, NULL);
    dest = (char*)malloc(dest_size);

    final_size = LZ4F_compressFrame(dest, dest_size, source, ssrc_size, NULL);
    result = PyBytes_FromStringAndSize(dest, final_size);

    free(dest);

    return result;
}


static PyObject *py_lz4f_makePrefs(PyObject *self, PyObject *args, PyObject *keywds) {
    LZ4F_frameInfo_t frameInfo;
    LZ4F_preferences_t* prefs;
    PyObject *result = PyDict_New();
    static char *kwlist[] = {"blockSizeID", "blockMode", "chkFlag"
                             "autoFlush"};
    unsigned int blkID=7;
    unsigned int blkMode=1;
    unsigned int chkSumFlag=0;
//    unsigned int compLevel=0; //For future expansion
    unsigned int autoFlush=0;

    (void)self;
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "|IIII", kwlist, &blkID,
                                     &blkMode, &chkSumFlag, &autoFlush)) {
        return NULL;
    }

    //fprintf(stdout, "blkID: %u As self.", blkID);
    prefs = calloc(1,sizeof(LZ4F_preferences_t));
    frameInfo = (LZ4F_frameInfo_t){blkID, blkMode, chkSumFlag, 0, 0, {0,0}};
    prefs->frameInfo = frameInfo;
    prefs->autoFlush = autoFlush;
    result = PyCapsule_New(prefs, NULL, NULL);

    return result;
//_output_error:
//    return Py_None;
}

static PyObject *py_lz4f_compressBegin(PyObject *self, PyObject *args) {
    char* dest;
    LZ4F_compressionContext_t cCtx;
    LZ4F_preferences_t prefs = {{7, 0, 0, 0, 0, {0}}, 0, 0, {0}};
    LZ4F_preferences_t* prefsPtr = &prefs;
    PyObject *result;
    PyObject *py_cCtx;
    PyObject *py_prefsPtr = Py_None;
    size_t dest_size;
    size_t final_size;

    (void)self;
    if (!PyArg_ParseTuple(args, "O|O", &py_cCtx, &py_prefsPtr)) {
        return NULL;
    }

    cCtx = (LZ4F_compressionContext_t)PyCapsule_GetPointer(py_cCtx, NULL);
    dest_size = 19;
    dest = (char*)malloc(dest_size);
    if (py_prefsPtr != Py_None) {
        prefsPtr = (LZ4F_preferences_t*)PyCapsule_GetPointer(py_prefsPtr, NULL);
    }

    final_size = LZ4F_compressBegin(cCtx, dest, dest_size, prefsPtr);
    CHECK(final_size);
    result = PyBytes_FromStringAndSize(dest, final_size);

    free(dest);

    return result;
_output_error:
    return Py_None;
}

static PyObject *py_lz4f_compressUpdate(PyObject *self, PyObject *args) {
    const char* source;
    char* dest;
    int src_size;
    LZ4F_compressionContext_t cCtx;
    PyObject *result;
    PyObject *py_cCtx;
    size_t dest_size;
    size_t final_size;
    size_t ssrc_size;

    (void)self;
    if (!PyArg_ParseTuple(args, "s#O", &source, &src_size, &py_cCtx)) {
        return NULL;
    }

    cCtx = (LZ4F_compressionContext_t)PyCapsule_GetPointer(py_cCtx, NULL);
    ssrc_size = (size_t)src_size;
    dest_size = LZ4F_compressBound(ssrc_size, (LZ4F_preferences_t *)cCtx);
    dest = (char*)malloc(dest_size);

    final_size = LZ4F_compressUpdate(cCtx, dest, dest_size, source, ssrc_size, NULL);
    CHECK(final_size);
    result = PyBytes_FromStringAndSize(dest, final_size);

    free(dest);

    return result;
_output_error:
    return Py_None;
}

static PyObject *py_lz4f_compressEnd(PyObject *self, PyObject *args) {
    char* dest;
    LZ4F_compressionContext_t cCtx;
    PyObject *result;
    PyObject *py_cCtx;
    size_t dest_size;
    size_t final_size;

    (void)self;
    if (!PyArg_ParseTuple(args, "O", &py_cCtx)) {
        return NULL;
    }

    cCtx = (LZ4F_compressionContext_t)PyCapsule_GetPointer(py_cCtx, NULL);
    dest_size = LZ4F_compressBound(0, (LZ4F_preferences_t *)cCtx);
    dest = (char*)malloc(dest_size);

    final_size = LZ4F_compressEnd(cCtx, dest, dest_size, NULL);
    CHECK(final_size);
    result = PyBytes_FromStringAndSize(dest, final_size);

    free(dest);

    return result;
_output_error:
    return Py_None;
}


/* Decompression methods */
static PyObject *py_lz4f_createDecompCtx(PyObject *self, PyObject *args) {
    PyObject *result;
    LZ4F_decompressionContext_t dCtx;
    size_t err;

    (void)self;
    (void)args;

    err = LZ4F_createDecompressionContext(&dCtx, LZ4F_VERSION);
    CHECK(LZ4F_isError(err), "Allocation failed (error %i)", (int)err);
    result = PyCapsule_New(dCtx, NULL, NULL);

    return result;
_output_error:
    return Py_None;
}

static PyObject *py_lz4f_freeDecompCtx(PyObject *self, PyObject *args) {
    PyObject *py_dCtx;
    LZ4F_decompressionContext_t dCtx;

    (void)self;
    if (!PyArg_ParseTuple(args, "O", &py_dCtx)) {
        return NULL;
    }

    dCtx = (LZ4F_decompressionContext_t)PyCapsule_GetPointer(py_dCtx, NULL);
    LZ4F_freeDecompressionContext(dCtx);

    return Py_None;
}

static PyObject *py_lz4f_getFrameInfo(PyObject *self, PyObject *args) {
    const char *source;
    int src_size;
    LZ4F_decompressionContext_t dCtx;
    LZ4F_frameInfo_t frameInfo;
    PyObject *blkSize;
    PyObject *blkMode;
    PyObject *contChkFlag;
    PyObject *py_dCtx;
    PyObject *result = PyDict_New();
    size_t ssrc_size;
    size_t err;

    (void)self;
    if (!PyArg_ParseTuple(args, "s#O", &source, &src_size, &py_dCtx)) {
        return NULL;
    }

    dCtx = (LZ4F_decompressionContext_t)PyCapsule_GetPointer(py_dCtx, NULL);
    ssrc_size = (size_t)src_size;

    err = LZ4F_getFrameInfo(dCtx, &frameInfo, (unsigned char*)source, &ssrc_size);
    CHECK(LZ4F_isError(err), "Failed getting frameInfo. (error %i)", (int)err);

    blkSize = PyInt_FromSize_t(frameInfo.blockSizeID);
    blkMode = PyInt_FromSize_t(frameInfo.blockMode);
    contChkFlag = PyInt_FromSize_t(frameInfo.contentChecksumFlag);
    PyDict_SetItemString(result, "blkSize", blkSize);
    PyDict_SetItemString(result, "blkMode", blkMode);
    PyDict_SetItemString(result, "chkFlag", contChkFlag);


    return result;
_output_error:
    return Py_None;
}

static PyObject *py_lz4f_disableChecksum(PyObject *self, PyObject *args) {
    PyObject *py_dCtx;
    LZ4F_decompressionContext_t dCtx;

    (void)self;
    if (!PyArg_ParseTuple(args, "O", &py_dCtx)) {
        return NULL;
    }

    dCtx = (LZ4F_decompressionContext_t)PyCapsule_GetPointer(py_dCtx, NULL);
    LZ4F_disableChecksum(dCtx);

    return Py_None;
}

static PyObject *py_lz4f_decompress(PyObject *self, PyObject *args, PyObject *keywds) {
    const char* source;
    char* dest;
    LZ4F_decompressionContext_t dCtx;
    int src_size;
    PyObject *decomp;
    PyObject *next;
    PyObject *py_dCtx;
    PyObject *result = PyDict_New();
    size_t ssrc_size;
    size_t dest_size;
    size_t err;
    static char *kwlist[] = {"source", "dCtx", "blkSizeID"};
    unsigned int blkID=7;

    (void)self;
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "s#O|i", kwlist, &source,
                                     &src_size, &py_dCtx, &blkID)) {
        return NULL;
    }

    dest_size = LZ4S_GetBlockSize_FromBlockId(blkID);
    dCtx = (LZ4F_decompressionContext_t)PyCapsule_GetPointer(py_dCtx, NULL);
    ssrc_size = (size_t)src_size;

    dest = (char*)malloc(dest_size);
    err = LZ4F_decompress(dCtx, dest, &dest_size, source, &ssrc_size, NULL);
    CHECK(LZ4F_isError(err), "Failed getting frameInfo. (error %i)", (int)err);
    //fprintf(stdout, "Dest_size: %zu  Error Code:%zu \n", dest_size, err);

    decomp = PyBytes_FromStringAndSize(dest, dest_size);
    next = PyInt_FromSize_t(err);
    PyDict_SetItemString(result, "decomp", decomp);
    PyDict_SetItemString(result, "next", next);

    Py_XDECREF(decomp);
    Py_XDECREF(next);
    free(dest);

    return result;
_output_error:
    return Py_None;
}

static PyMethodDef Lz4fMethods[] = {
    {"createCompContext",   py_lz4f_createCompCtx,   METH_VARARGS, CCCTX_DOCSTRING},
    {"compressFrame",       py_lz4f_compressFrame,   METH_VARARGS, COMPF_DOCSTRING},
    {"makePrefs",  (PyCFunction)py_lz4f_makePrefs,   METH_VARARGS | METH_KEYWORDS, MKPFS_DOCSTRING},
    {"compressBegin",       py_lz4f_compressBegin,   METH_VARARGS, COMPB_DOCSTRING},
    {"compressUpdate",      py_lz4f_compressUpdate,  METH_VARARGS, COMPU_DOCSTRING},
    {"compressEnd",         py_lz4f_compressEnd,     METH_VARARGS, COMPE_DOCSTRING},
    {"freeCompContext",     py_lz4f_freeCompCtx,     METH_VARARGS, FCCTX_DOCSTRING},
    {"createDecompContext", py_lz4f_createDecompCtx, METH_VARARGS, CDCTX_DOCSTRING},
    {"freeDecompContext",   py_lz4f_freeDecompCtx,   METH_VARARGS, FDCTX_DOCSTRING},
    {"getFrameInfo",        py_lz4f_getFrameInfo,    METH_VARARGS, GETFI_DOCSTRING},
    {"decompressFrame",  (PyCFunction)py_lz4f_decompress, METH_VARARGS | METH_KEYWORDS, DCOMP_DOCSTRING},
    {"disableChecksum",     py_lz4f_disableChecksum, METH_VARARGS, DCHKS_DOCSTRING},
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
    PyObject *module = Py_InitModule("lz4f", Lz4fMethods);
#endif
    struct module_state *state = NULL;

    if (module == NULL) {
        INITERROR;
    }
    state = GETSTATE(module);

    state->error = PyErr_NewException("lz4.Error", NULL, NULL);
    if (state->error == NULL) {
        Py_DECREF(module);
        INITERROR;
    }

    PyModule_AddStringConstant(module, "VERSION", VERSION);
    PyModule_AddStringConstant(module, "__version__", VERSION);
    PyModule_AddStringConstant(module, "LZ4_VERSION", LZ4_VERSION);

#if PY_MAJOR_VERSION >= 3
    return module;
#endif
}
