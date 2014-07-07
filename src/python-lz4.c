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
#include <math.h>
#include "lz4.h"
#include "lz4hc.h"
#include "python-lz4.h"

#define MAX(a, b)               ((a) > (b) ? (a) : (b))

typedef int (*compressor)(const char *source, char *dest, int isize);

static inline void store_le32(char *c, uint32_t x) {
    c[0] = x & 0xff;
    c[1] = (x >> 8) & 0xff;
    c[2] = (x >> 16) & 0xff;
    c[3] = (x >> 24) & 0xff;
}

static inline uint32_t load_le32(const char *c) {
    const uint8_t *d = (const uint8_t *)c;
    return d[0] | (d[1] << 8) | (d[2] << 16) | (d[3] << 24);
}

static const int hdr_size = sizeof(uint32_t);

static PyObject *compress_with(compressor compress, PyObject *self, PyObject *args) {
    PyObject *result;
    const char *source;
    int source_size;
    char *dest;
    int dest_size;

    if (!PyArg_ParseTuple(args, "s#", &source, &source_size))
        return NULL;

    dest_size = hdr_size + LZ4_compressBound(source_size);
    result = PyBytes_FromStringAndSize(NULL, dest_size);
    if (result == NULL) {
        return NULL;
    }
    dest = PyBytes_AS_STRING(result);
    store_le32(dest, source_size);
    if (source_size > 0) {
        int osize = compress(source, dest + hdr_size, source_size);
        int actual_size = hdr_size + osize;
        /* Resizes are expensive; tolerate some slop to avoid. */
        if (actual_size < (dest_size / 4) * 3) {
            _PyBytes_Resize(&result, actual_size);
        } else {
            Py_SIZE(result) = actual_size;
        }
    }
    return result;
}

static PyObject *py_lz4_compress(PyObject *self, PyObject *args) {
    return compress_with(LZ4_compress, self, args);
}

static PyObject *py_lz4_compressHC(PyObject *self, PyObject *args) {
    return compress_with(LZ4_compressHC, self, args);
}

static PyObject *py_lz4_uncompress(PyObject *self, PyObject *args) {
    PyObject *result;
    const char *source;
    int source_size;
    uint32_t dest_size;

    if (!PyArg_ParseTuple(args, "s#", &source, &source_size)) {
        return NULL;
    }

    if (source_size < hdr_size) {
        PyErr_SetString(PyExc_ValueError, "input too short");
        return NULL;
    }
    dest_size = load_le32(source);
    if (dest_size > INT_MAX) {
        PyErr_Format(PyExc_ValueError, "invalid size in header: 0x%x", dest_size);
        return NULL;
    }
    result = PyBytes_FromStringAndSize(NULL, dest_size);
    if (result != NULL && dest_size > 0) {
        char *dest = PyBytes_AS_STRING(result);
        int osize = LZ4_decompress_safe(source + hdr_size, dest, source_size - hdr_size, dest_size);
        if (osize < 0) {
            PyErr_Format(PyExc_ValueError, "corrupt input at byte %d", -osize);
            Py_CLEAR(result);
        }
    }

    return result;
}

static PyMethodDef Lz4Methods[] = {
    {"LZ4_compress",  py_lz4_compress, METH_VARARGS, COMPRESS_DOCSTRING},
    {"LZ4_uncompress",  py_lz4_uncompress, METH_VARARGS, UNCOMPRESS_DOCSTRING},
    {"compress",  py_lz4_compress, METH_VARARGS, COMPRESS_DOCSTRING},
    {"compressHC",  py_lz4_compressHC, METH_VARARGS, COMPRESSHC_DOCSTRING},
    {"uncompress",  py_lz4_uncompress, METH_VARARGS, UNCOMPRESS_DOCSTRING},
    {"decompress",  py_lz4_uncompress, METH_VARARGS, UNCOMPRESS_DOCSTRING},
    {"dumps",  py_lz4_compress, METH_VARARGS, COMPRESS_DOCSTRING},
    {"loads",  py_lz4_uncompress, METH_VARARGS, UNCOMPRESS_DOCSTRING},
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

    st->error = PyErr_NewException("lz4.Error", NULL, NULL);
    if (st->error == NULL) {
        Py_DECREF(module);
        INITERROR;
    }

#if PY_MAJOR_VERSION >= 3
    return module;
#endif
}
