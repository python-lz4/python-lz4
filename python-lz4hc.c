/*
 * Copyright (c) 2015, Jerry Ryle
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
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
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
 *
 */
#include "py3c.h" // This must be included before any standard headers

#include "app_types.h"
#include <math.h>
#include <stdlib.h>

#include "lz4/lz4.h"
#include "lz4/lz4hc.h"

struct compression_stream {
   LZ4_streamHC_t *stream;
   int block_size;
   char *input_buffer[2];
   char *compressed_buffer;
   int compressed_buffer_max_size;
   int input_buffer_index;
};

static PyObject *compress_hc(PyObject *self, PyObject *args, PyObject *keywds);
static PyObject *create_hc_stream(PyObject *self, PyObject *args, PyObject *keywds);
static PyObject *free_hc_stream(PyObject *self, PyObject *args, PyObject *keywds);
static PyObject *compress_hc_continue(PyObject *self, PyObject *args, PyObject *keywds);

static PyMethodDef module_methods[] = {
    {"compress_hc",             (PyCFunction)compress_hc,           METH_VARARGS|METH_KEYWORDS, "Compresses source string of bytes with a compression level and returns compressed data as a string of bytes."},
    {"create_hc_stream",        (PyCFunction)create_hc_stream,      METH_VARARGS|METH_KEYWORDS, "Creates and returns an HC compression stream object."},
    {"free_hc_stream",          (PyCFunction)free_hc_stream,        METH_VARARGS|METH_KEYWORDS, "Frees an HC compression stream object."},
    {"compress_hc_continue",    (PyCFunction)compress_hc_continue,  METH_VARARGS|METH_KEYWORDS, "Compresses a block in streaming mode with high compression."},
    {NULL, NULL, 0, NULL} /* Sentinel */
};

static PyObject *compress_hc(PyObject *self, PyObject *args, PyObject *keywds)
{
    (void)self;

    const char *source;
    int source_size;
    int compression_level;

    static char *kwlist[] = {"source", "compression_level", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "s#i", kwlist,
        &source, &source_size,
        &compression_level)) {
        return NULL;
    }

    if (source_size > LZ4_MAX_INPUT_SIZE) {
        PyErr_Format(PyExc_ValueError, "input data is %d bytes, which is larger than the maximum supported size of %d bytes", source_size, LZ4_MAX_INPUT_SIZE);
        return NULL;
    }

    int dest_size = LZ4_compressBound(source_size);
    PyObject *py_dest = PyBytes_FromStringAndSize(NULL, dest_size);
    if (py_dest == NULL) {
        return NULL;
    }
    char *dest = PyBytes_AS_STRING(py_dest);
    if (source_size > 0) {
        int compressed_size = LZ4_compress_HC(source, dest, source_size, dest_size, compression_level);
        if (compressed_size <= 0) {
            Py_DECREF(py_dest);
            PyErr_Format(PyExc_RuntimeError, "LZ4_compress_HC failed with code: %d", compressed_size);
            return NULL;
        }
        /* The actual compressed size might be less than we allocated
           (we allocated using a worst case guess). If the actual size is
           less than 75% of what we allocated, then it's worth performing an
           expensive resize operation to reclaim some space. */
        if (compressed_size < (dest_size / 4) * 3) {
            _PyBytes_Resize(&py_dest, compressed_size);
        } else {
            Py_SIZE(py_dest) = compressed_size;
        }
    }
    return py_dest;
}

static PyObject *create_hc_stream(PyObject *self, PyObject *args, PyObject *keywds)
{
    (void)self;

    int block_size = 0;
    int compression_level = 9;

    static char *kwlist[] = {"block_size", "compression_level", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "i|i", kwlist,
        &block_size,
        &compression_level)) {
        return NULL;
    }

    if (block_size > LZ4_MAX_INPUT_SIZE) {
        PyErr_Format(PyExc_ValueError, "block size is %d bytes, which is larger than the maximum supported size of %d bytes", block_size, LZ4_MAX_INPUT_SIZE);
        return NULL;
    }
    if (block_size <= 0) {
        PyErr_Format(PyExc_ValueError, "block size is %d bytes, which is invalid", block_size);
        return NULL;
    }

    struct compression_stream *stream = (struct compression_stream *)PyMem_Malloc(sizeof(struct compression_stream));
    if (!stream) {
        return PyErr_NoMemory();
    }

    stream->block_size = block_size;
    stream->input_buffer_index = 0;
    stream->input_buffer[0] = (char *)PyMem_Malloc(block_size);
    stream->input_buffer[1] = (char *)PyMem_Malloc(block_size);
    stream->compressed_buffer_max_size = LZ4_COMPRESSBOUND(block_size);
    stream->compressed_buffer = (char *)PyMem_Malloc(stream->compressed_buffer_max_size);
    stream->stream = LZ4_createStreamHC();

    if (!stream->input_buffer[0] ||
            !stream->input_buffer[1] ||
            !stream->compressed_buffer ||
            !stream->stream) {
        PyMem_Free(stream->input_buffer[0]);
        PyMem_Free(stream->input_buffer[1]);
        PyMem_Free(stream->compressed_buffer);
        if (stream->stream) {
            LZ4_freeStreamHC(stream->stream);
        }
        PyMem_Free(stream);
        return PyErr_NoMemory();
    }

    LZ4_resetStreamHC(stream->stream, compression_level);

    return PyCapsule_New(stream, NULL, NULL);
}

static PyObject *free_hc_stream(PyObject *self, PyObject *args, PyObject *keywds)
{
    (void)self;

    PyObject *py_stream = NULL;

    static char *kwlist[] = {"stream", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "O", kwlist,
        &py_stream)) {
        return NULL;
    }

    struct compression_stream *stream = (struct compression_stream *)PyCapsule_GetPointer(py_stream, NULL);
    if (!stream) {
        PyErr_Format(PyExc_ValueError, "No stream supplied");
        return NULL;
    }

    LZ4_freeStreamHC(stream->stream);
    PyMem_Free(stream->compressed_buffer);
    PyMem_Free(stream->input_buffer[1]);
    PyMem_Free(stream->input_buffer[0]);
    PyMem_Free(stream);

    Py_RETURN_NONE;
}

static PyObject *compress_hc_continue(PyObject *self, PyObject *args, PyObject *keywds)
{
    (void)self;

    PyObject *py_stream = NULL;
    const char *source = NULL;
    int source_size = 0;

    static char *kwlist[] = {"stream", "source", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "Os#|i", kwlist,
            &py_stream,
            &source, &source_size))
        return NULL;

    struct compression_stream *stream = (struct compression_stream *)PyCapsule_GetPointer(py_stream, NULL);
    if (!stream) {
        PyErr_Format(PyExc_ValueError, "No stream supplied");
        return NULL;
    }

    if (source_size > stream->block_size) {
        PyErr_Format(PyExc_ValueError, "Source data is %d bytes. It must be less than or equal to the stream's block size, which is %d bytes", source_size, stream->block_size);
        return NULL;
    }

    char *input_pointer = stream->input_buffer[stream->input_buffer_index];
    int bytes_to_compress = source_size;

    memcpy(input_pointer, source, bytes_to_compress);

    int compressed_size = LZ4_compress_HC_continue(
            stream->stream,
            input_pointer,
            stream->compressed_buffer,
            source_size,
            stream->compressed_buffer_max_size);
    if (compressed_size <= 0) {
        PyErr_Format(PyExc_RuntimeError, "LZ4_compress_fast_continue failed with code: %d", compressed_size);
        return NULL;
    }

    stream->input_buffer_index = (stream->input_buffer_index + 1) % 2;

    return Py_BuildValue("s#", stream->compressed_buffer, compressed_size);
}


static struct PyModuleDef moduledef = {
        PyModuleDef_HEAD_INIT,
        "lz4hc",
        NULL,
        -1,
        module_methods
        };

MODULE_INIT_FUNC(lz4hc)
{
    PyObject *module = PyModule_Create(&moduledef);

    if (module == NULL)
        return NULL;

    /* VERSION and LZ4_VERSION are defined in setup.py and are passed in via
       the compilation command line. */
    PyModule_AddStringConstant(module, "VERSION", VERSION);
    PyModule_AddStringConstant(module, "__version__", VERSION);
    PyModule_AddStringConstant(module, "LZ4_VERSION", LZ4_VERSION);

    PyModule_AddIntConstant(module, "COMPRESSIONLEVEL_DEFAULT", 0);
    PyModule_AddIntConstant(module, "COMPRESSIONLEVEL_MAX",     16);

    return module;
}
