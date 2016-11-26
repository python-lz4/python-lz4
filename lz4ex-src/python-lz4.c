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
#include "py3c/capsulethunk.h"

#include "module_types.h"
#include <math.h>
#include <stdlib.h>

#include "lz4/lz4.h"

struct compression_stream {
    LZ4_stream_t *stream;
    int block_size;
    char *input_buffer[2];
    char *compressed_buffer;
    int compressed_buffer_max_size;
    int input_buffer_index;
};

struct decompression_stream {
    LZ4_streamDecode_t *stream;
    int block_size;
    char *decompressed_buffer[2];
    int decompressed_buffer_index;
};

static PyObject *compress_default(PyObject *self, PyObject *args, PyObject *keywds);
static PyObject *compress_fast(PyObject *self, PyObject *args, PyObject *keywds);
static PyObject *compress_bound(PyObject *self, PyObject *args, PyObject *keywds);
static PyObject *decompress_safe(PyObject *self, PyObject *args, PyObject *keywds);
static PyObject *create_stream(PyObject *self, PyObject *args, PyObject *keywds);
static PyObject *free_stream(PyObject *self, PyObject *args, PyObject *keywds);
static PyObject *compress_fast_continue(PyObject *self, PyObject *args, PyObject *keywds);
static PyObject *create_decode_stream(PyObject *self, PyObject *args, PyObject *keywds);
static PyObject *free_decode_stream(PyObject *self, PyObject *args, PyObject *keywds);
static PyObject *decompress_safe_continue(PyObject *self, PyObject *args, PyObject *keywds);


static PyMethodDef module_methods[] = {
    {"compress_default",            (PyCFunction)compress_default,          METH_VARARGS|METH_KEYWORDS, "Compresses source string of bytes and returns compressed data as a string of bytes."},
    {"compress_fast",               (PyCFunction)compress_fast,             METH_VARARGS|METH_KEYWORDS, "Same as compress_default(), but allows selection of an 'acceleration' factor."},
    {"compress_bound",              (PyCFunction)compress_bound,            METH_VARARGS|METH_KEYWORDS, "For a given input length, this provides the maximum size that LZ4 compression may output in a 'worst case' scenario."},
    {"decompress_safe",             (PyCFunction)decompress_safe,           METH_VARARGS|METH_KEYWORDS, "Decompresses source string of bytes and returns decompressed data as a string of bytes."},
    {"create_stream",               (PyCFunction)create_stream,             METH_VARARGS|METH_KEYWORDS, "Creates and returns a compression stream object."},
    {"free_stream",                 (PyCFunction)free_stream,               METH_VARARGS|METH_KEYWORDS, "Frees a compression stream object."},
    {"compress_fast_continue",      (PyCFunction)compress_fast_continue,    METH_VARARGS|METH_KEYWORDS, "Compresses a block in streaming mode."},
    {"create_decode_stream",        (PyCFunction)create_decode_stream,      METH_VARARGS|METH_KEYWORDS, "Creates and returns a decompression stream object."},
    {"free_decode_stream",          (PyCFunction)free_decode_stream,        METH_VARARGS|METH_KEYWORDS, "Frees a decompression stream object."},
    {"decompress_safe_continue",    (PyCFunction)decompress_safe_continue,  METH_VARARGS|METH_KEYWORDS, "Decompresses a block in streaming mode."},
    {NULL, NULL, 0, NULL} /* Sentinel */
};

static PyObject *compress_default(PyObject *self, PyObject *args, PyObject *keywds)
{
    (void)self;

    const char *source;
    int source_size;

    static char *kwlist[] = {"source", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "s#", kwlist,
            &source, &source_size))
        return NULL;

    if (source_size > LZ4_MAX_INPUT_SIZE) {
        PyErr_Format(PyExc_ValueError, "input data is %d bytes, which is larger than the maximum supported size of %d bytes", source_size, LZ4_MAX_INPUT_SIZE);
        return NULL;
    }
    if (source_size <= 0) {
        PyErr_Format(PyExc_ValueError, "input data is %d bytes, which is invalid", source_size);
        return NULL;
    }

    int dest_size = LZ4_compressBound(source_size);
    PyObject *py_dest = PyBytes_FromStringAndSize(NULL, dest_size);
    if (!py_dest) {
        return PyErr_NoMemory();
    }
    char *dest = PyBytes_AS_STRING(py_dest);

    int compressed_size = LZ4_compress_default(source, dest, source_size, dest_size);
    if (compressed_size <= 0) {
        Py_DECREF(py_dest);
        PyErr_Format(PyExc_RuntimeError, "LZ4_compress_default failed with code: %d", compressed_size);
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

    return py_dest;
}

static PyObject *compress_fast(PyObject *self, PyObject *args, PyObject *keywds)
{
    (void)self;

    const char *source;
    int source_size;
    int acceleration_factor;

    static char *kwlist[] = {"source", "acceleration_factor", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "s#i", kwlist,
            &source, &source_size,
            &acceleration_factor))
        return NULL;

    if (source_size > LZ4_MAX_INPUT_SIZE) {
        PyErr_Format(PyExc_ValueError, "input data is %d bytes, which is larger than the maximum supported size of %d bytes", source_size, LZ4_MAX_INPUT_SIZE);
        return NULL;
    }
    if (source_size <= 0) {
        PyErr_Format(PyExc_ValueError, "input data is %d bytes, which is invalid", source_size);
        return NULL;
    }

    int dest_size = LZ4_compressBound(source_size);
    PyObject *py_dest = PyBytes_FromStringAndSize(NULL, dest_size);
    if (!py_dest) {
        return PyErr_NoMemory();
    }
    char *dest = PyBytes_AS_STRING(py_dest);
    if (source_size > 0) {
        int compressed_size = LZ4_compress_fast(source, dest, source_size, dest_size, acceleration_factor);
        if (compressed_size <= 0) {
            Py_DECREF(py_dest);
            PyErr_Format(PyExc_RuntimeError, "LZ4_compress_default failed with code: %d", compressed_size);
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

static PyObject *compress_bound(PyObject *self, PyObject *args, PyObject *keywds)
{
    (void)self;

    int source_size;

    static char *kwlist[] = {"source_size", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "i", kwlist,
            &source_size))
        return NULL;

    if (source_size > LZ4_MAX_INPUT_SIZE) {
        PyErr_Format(PyExc_ValueError, "input size is %d, which is larger than the maximum supported size of %d", source_size, LZ4_MAX_INPUT_SIZE);
        return NULL;
    }

    return Py_BuildValue("i", LZ4_compressBound(source_size));
}

static PyObject *decompress_safe(PyObject *self, PyObject *args, PyObject *keywds)
{
    (void)self;

    const char *source;
    int source_size;
    int dest_size;

    static char *kwlist[] = {"source", "dest_size", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "s#i", kwlist,
            &source, &source_size,
            &dest_size)) {
        return NULL;
    }

    if ((dest_size <= 0) || (dest_size > LZ4_MAX_INPUT_SIZE)) {
        PyErr_Format(PyExc_ValueError, "invalid uncompressed size: 0x%x", dest_size);
        return NULL;
    }

    PyObject *py_dest = PyBytes_FromStringAndSize(NULL, dest_size);
    if (!py_dest) {
        return PyErr_NoMemory();
    }

    char *dest = PyBytes_AS_STRING(py_dest);
    int decompressed_size = LZ4_decompress_safe(source, dest, source_size, dest_size);
    if (decompressed_size < 0) {
        Py_DECREF(py_dest);
        PyErr_Format(PyExc_ValueError, "compressed data is corrupt at byte %d", -decompressed_size);
        return NULL;
    }

    return py_dest;
}

static PyObject *create_stream(PyObject *self, PyObject *args, PyObject *keywds)
{
    (void)self;

    int block_size = 0;

    static char *kwlist[] = {"block_size", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "i", kwlist,
        &block_size)) {
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
    stream->stream = LZ4_createStream();

    if (!stream->input_buffer[0] ||
            !stream->input_buffer[1] ||
            !stream->compressed_buffer ||
            !stream->stream) {
        PyMem_Free(stream->input_buffer[0]);
        PyMem_Free(stream->input_buffer[1]);
        PyMem_Free(stream->compressed_buffer);
        if (stream->stream) {
            LZ4_freeStream(stream->stream);
        }
        PyMem_Free(stream);
        return PyErr_NoMemory();
    }

    return PyCapsule_New(stream, NULL, NULL);
}

static PyObject *free_stream(PyObject *self, PyObject *args, PyObject *keywds)
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

    LZ4_freeStream(stream->stream);
    PyMem_Free(stream->compressed_buffer);
    PyMem_Free(stream->input_buffer[1]);
    PyMem_Free(stream->input_buffer[0]);
    PyMem_Free(stream);

    Py_RETURN_NONE;
}

static PyObject *compress_fast_continue(PyObject *self, PyObject *args, PyObject *keywds)
{
    (void)self;

    PyObject *py_stream = NULL;
    const char *source = NULL;
    int source_size = 0;
    int acceleration_factor = 1;

    static char *kwlist[] = {"stream", "source", "acceleration_factor", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "Os#|i", kwlist,
            &py_stream,
            &source, &source_size,
            &acceleration_factor))
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

    int compressed_size = LZ4_compress_fast_continue(
            stream->stream,
            input_pointer,
            stream->compressed_buffer,
            source_size,
            stream->compressed_buffer_max_size,
            acceleration_factor);
    if (compressed_size <= 0) {
        PyErr_Format(PyExc_RuntimeError, "LZ4_compress_fast_continue failed with code: %d", compressed_size);
        return NULL;
    }

    stream->input_buffer_index = (stream->input_buffer_index + 1) % 2;

    return PyBytes_FromStringAndSize(stream->compressed_buffer, compressed_size);
}

static PyObject *create_decode_stream(PyObject *self, PyObject *args, PyObject *keywds)
{
    (void)self;

    int block_size = 0;

    static char *kwlist[] = {"block_size", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "i", kwlist,
        &block_size)) {
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

    struct decompression_stream *stream = (struct decompression_stream *)PyMem_Malloc(sizeof(struct decompression_stream));
    if (!stream) {
        return PyErr_NoMemory();
    }

    stream->block_size = block_size;
    stream->decompressed_buffer_index = 0;
    stream->decompressed_buffer[0] = (char *)PyMem_Malloc(block_size);
    stream->decompressed_buffer[1] = (char *)PyMem_Malloc(block_size);
    stream->stream = LZ4_createStreamDecode();

    if (!stream->decompressed_buffer[0] ||
            !stream->decompressed_buffer[1] ||
            !stream->stream) {
        PyMem_Free(stream->decompressed_buffer[0]);
        PyMem_Free(stream->decompressed_buffer[1]);
        if (stream->stream) {
            LZ4_freeStreamDecode(stream->stream);
        }
        PyMem_Free(stream);
        return PyErr_NoMemory();
    }

    return PyCapsule_New(stream, NULL, NULL);
}

static PyObject *free_decode_stream(PyObject *self, PyObject *args, PyObject *keywds)
{
    (void)self;

    PyObject *py_stream = NULL;

    static char *kwlist[] = {"stream", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "O", kwlist,
        &py_stream)) {
        return NULL;
    }

    struct decompression_stream *stream = (struct decompression_stream *)PyCapsule_GetPointer(py_stream, NULL);
    if (!stream) {
        PyErr_Format(PyExc_ValueError, "No stream supplied");
        return NULL;
    }

    LZ4_freeStreamDecode(stream->stream);
    PyMem_Free(stream->decompressed_buffer[1]);
    PyMem_Free(stream->decompressed_buffer[0]);
    PyMem_Free(stream);

    Py_RETURN_NONE;
}

static PyObject *decompress_safe_continue(PyObject *self, PyObject *args, PyObject *keywds)
{
    (void)self;

    PyObject *py_stream = NULL;
    const char *source = NULL;
    int source_size = 0;

    static char *kwlist[] = {"stream", "source", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "Os#", kwlist,
            &py_stream,
            &source, &source_size))
        return NULL;

    struct decompression_stream *stream = (struct decompression_stream *)PyCapsule_GetPointer(py_stream, NULL);
    if (!stream) {
        PyErr_Format(PyExc_ValueError, "No stream supplied");
        return NULL;
    }

    char *decompress_pointer = stream->decompressed_buffer[stream->decompressed_buffer_index];

    int decompressed_size = LZ4_decompress_safe_continue(
            stream->stream,
            source,
            decompress_pointer,
            source_size,
            stream->block_size);
    if (decompressed_size <= 0) {
        PyErr_Format(PyExc_RuntimeError, "LZ4_decompress_safe_continue failed with code: %d", decompressed_size);
        return NULL;
    }

    stream->decompressed_buffer_index = (stream->decompressed_buffer_index + 1) % 2;

    return PyBytes_FromStringAndSize(decompress_pointer, decompressed_size);
}

static struct PyModuleDef moduledef = {
        PyModuleDef_HEAD_INIT,
        "lz4",
        NULL,
        -1,
        module_methods
        };

MODULE_INIT_FUNC(lz4)
{
    PyObject *module = PyModule_Create(&moduledef);

    if (module == NULL)
        return NULL;

    /* VERSION and LZ4_VERSION are defined in setup.py and are passed in via
       the compilation command line. */
    PyModule_AddStringConstant(module, "VERSION", VERSION);
    PyModule_AddStringConstant(module, "__version__", VERSION);
    PyModule_AddStringConstant(module, "LZ4_VERSION", LZ4_VERSION);

    return module;
}
