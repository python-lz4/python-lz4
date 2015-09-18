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

#include <math.h>
#include <stdlib.h>

#include "app_types.h"
#include "lz4/lz4.h"
#include "lz4/lz4frame.h"
#include "lz4/lz4hc.h"

struct compression_context {
    LZ4F_compressionContext_t compression_context;
    LZ4F_preferences_t preferences;
};

static PyObject *compress_frame(PyObject *self, PyObject *args, PyObject *keywds);

static PyObject *create_compression_context(PyObject *self, PyObject *args, PyObject *keywds);
static PyObject *free_compression_context(PyObject *self, PyObject *args, PyObject *keywds);
static PyObject *compress_begin(PyObject *self, PyObject *args, PyObject *keywds);
static PyObject *compress_update(PyObject *self, PyObject *args, PyObject *keywds);
static PyObject *compress_end(PyObject *self, PyObject *args, PyObject *keywds);

static PyObject *get_frame_info(PyObject *self, PyObject *args, PyObject *keywds);
static PyObject *decompress(PyObject *self, PyObject *args, PyObject *keywds);

static PyMethodDef module_methods[] = {
    {"compress_frame",              (PyCFunction)compress_frame,                METH_VARARGS|METH_KEYWORDS, "Compresses an entire frame of data and returns it as a string of bytes."},

    {"create_compression_context",  (PyCFunction)create_compression_context,    METH_VARARGS|METH_KEYWORDS, "Creates a Compression Context object, which will be used in all compression operations."},
    {"free_compression_context",    (PyCFunction)free_compression_context,      METH_VARARGS|METH_KEYWORDS, "Frees a Compression Context object, previously created by create_compression_context."},
    {"compress_begin",              (PyCFunction)compress_begin,                METH_VARARGS|METH_KEYWORDS, "Begins the frame compression. Returns the frame header in a string of bytes."},
    {"compress_update",             (PyCFunction)compress_update,               METH_VARARGS|METH_KEYWORDS, "Compresses blocks of data and returns the compressed data in a string of bytes. Returned strings may be empty if auto-flush is disabled."},
    {"compress_end",                (PyCFunction)compress_end,                  METH_VARARGS|METH_KEYWORDS, "Flushes and returns any remaining compressed data along with the endmark and optional checksum as a string of bytes."},

    {"get_frame_info",              (PyCFunction)get_frame_info,                METH_VARARGS|METH_KEYWORDS, "Given a frame of compressed data, returns information about the frame."},
    {"decompress",                  (PyCFunction)decompress,                    METH_VARARGS|METH_KEYWORDS, "Decompressed a frame of data and returns it as a string of bytes."},
    {NULL, NULL, 0, NULL} /* Sentinel */
};


static PyObject *compress_frame(PyObject *self, PyObject *args, PyObject *keywds)
{
    (void)self;

    const char *source;
    int source_size;
    LZ4F_preferences_t preferences;

    memset(&preferences, 0, sizeof(preferences));

    static char *kwlist[] = {"source", "compression_level", "block_size", "content_checksum", "block_mode", "frame_type", "auto_flush", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "s#|iiiiii", kwlist,
        &source, &source_size,
        &preferences.compressionLevel,
        &preferences.frameInfo.blockSizeID,
        &preferences.frameInfo.contentChecksumFlag,
        &preferences.frameInfo.blockMode,
        &preferences.frameInfo.frameType,
        &preferences.autoFlush)) {
        return NULL;
    }
    preferences.frameInfo.contentSize = source_size;

    size_t compressed_bound = LZ4F_compressFrameBound(source_size, &preferences);
    if (compressed_bound > PY_SSIZE_T_MAX) {
        PyErr_Format(PyExc_ValueError, "input data could require %zu bytes, which is larger than the maximum supported size of %zd bytes", compressed_bound, PY_SSIZE_T_MAX);
        return NULL;
    }
    Py_ssize_t dest_size = (Py_ssize_t)compressed_bound;

    PyObject *py_dest = PyBytes_FromStringAndSize(NULL, dest_size);
    if (py_dest == NULL) {
        return NULL;
    }

    char *dest = PyBytes_AS_STRING(py_dest);
    if (source_size > 0) {
        size_t compressed_size = LZ4F_compressFrame(dest, dest_size, source, source_size, &preferences);
        if (LZ4F_isError(compressed_size)) {
            Py_DECREF(py_dest);
            PyErr_Format(PyExc_RuntimeError, "LZ4F_compressFrame failed with code failed with code: %s", LZ4F_getErrorName(compressed_size));
            return NULL;
        }
        /* The actual compressed size might be less than we allocated
           (we allocated using a worst case guess). If the actual size is
           less than 75% of what we allocated, then it's worth performing an
           expensive resize operation to reclaim some space. */
        if ((Py_ssize_t)compressed_size < (dest_size / 4) * 3) {
            _PyBytes_Resize(&py_dest, (Py_ssize_t)compressed_size);
        } else {
            Py_SIZE(py_dest) = (Py_ssize_t)compressed_size;
        }
    }
    return py_dest;
}

static PyObject *create_compression_context(PyObject *self, PyObject *args, PyObject *keywds)
{
    (void)self;
    (void)args;
    (void)keywds;

    struct compression_context *context = (struct compression_context *)PyMem_Malloc(sizeof(struct compression_context));
    if (!context) {
        return PyErr_NoMemory();
    }

    memset(context, 0, sizeof(*context));

    LZ4F_errorCode_t result = LZ4F_createCompressionContext(&context->compression_context, LZ4F_VERSION);
    if (LZ4F_isError(result)) {
        PyErr_Format(PyExc_RuntimeError, "LZ4F_createCompressionContext failed with code: %s", LZ4F_getErrorName(result));
        return NULL;
    }

    return PyCapsule_New(context, NULL, NULL);
}

static PyObject *free_compression_context(PyObject *self, PyObject *args, PyObject *keywds)
{
    (void)self;

    PyObject *py_context = NULL;

    static char *kwlist[] = {"context", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "O", kwlist,
        &py_context)) {
        return NULL;
    }

    struct compression_context *context = (struct compression_context *)PyCapsule_GetPointer(py_context, NULL);
    if (!context) {
        PyErr_Format(PyExc_ValueError, "No compression context supplied");
        return NULL;
    }

    LZ4F_errorCode_t result = LZ4F_freeCompressionContext(context->compression_context);
    if (LZ4F_isError(result)) {
        PyErr_Format(PyExc_RuntimeError, "LZ4F_freeCompressionContext failed with code: %s", LZ4F_getErrorName(result));
        return NULL;
    }
    PyMem_Free(context);

    Py_RETURN_NONE;
}

static PyObject *compress_begin(PyObject *self, PyObject *args, PyObject *keywds)
{
    (void)self;

    PyObject *py_context = NULL;
    unsigned long source_size = 0;
    LZ4F_preferences_t preferences;

    memset(&preferences, 0, sizeof(preferences));

    static char *kwlist[] = {"context", "source_size", "compression_level", "block_size", "content_checksum", "block_mode", "frame_type", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "O|kiiiii", kwlist,
        &py_context,
        &source_size,
        &preferences.compressionLevel,
        &preferences.frameInfo.blockSizeID,
        &preferences.frameInfo.contentChecksumFlag,
        &preferences.frameInfo.blockMode,
        &preferences.frameInfo.frameType)) {
        return NULL;
    }
    preferences.autoFlush = 1;
    preferences.frameInfo.contentSize = source_size;

    struct compression_context *context = (struct compression_context *)PyCapsule_GetPointer(py_context, NULL);

    if (!context || !context->compression_context) {
        PyErr_Format(PyExc_ValueError, "No compression context supplied");
        return NULL;
    }

    context->preferences = preferences;

    /* Only needs to be large enough for a header, which is 15 bytes.
     * Unfortunately, the lz4 library doesn't provide a #define for this.
     * We over-allocate to allow for larger headers in the future. */
    char destination_buffer[64];

    size_t result = LZ4F_compressBegin(context->compression_context, destination_buffer, sizeof(destination_buffer), &context->preferences);
    if (LZ4F_isError(result)) {
        PyErr_Format(PyExc_RuntimeError, "LZ4F_compressBegin failed with code: %s", LZ4F_getErrorName(result));
        return NULL;
    }

    return PyBytes_FromStringAndSize(destination_buffer, result);
}

static PyObject *compress_update(PyObject *self, PyObject *args, PyObject *keywds)
{
    (void)self;

    PyObject *py_context = NULL;
    const char *source = NULL;
    unsigned long source_size = 0;

    static char *kwlist[] = {"context", "source", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "Os#", kwlist,
        &py_context,
        &source, &source_size)) {
        return NULL;
    }

    struct compression_context *context = (struct compression_context *)PyCapsule_GetPointer(py_context, NULL);
    if (!context || !context->compression_context) {
        PyErr_Format(PyExc_ValueError, "No compression context supplied");
        return NULL;
    }

    size_t compressed_bound = LZ4F_compressFrameBound(source_size, &context->preferences);
    if (compressed_bound > PY_SSIZE_T_MAX) {
        PyErr_Format(PyExc_ValueError, "input data could require %zu bytes, which is larger than the maximum supported size of %zd bytes", compressed_bound, PY_SSIZE_T_MAX);
        return NULL;
    }

    char *destination_buffer = (char *)PyMem_Malloc(compressed_bound);
    if (!destination_buffer) {
        return PyErr_NoMemory();
    }

    LZ4F_compressOptions_t compress_options;
    compress_options.stableSrc = 0;

    size_t result = LZ4F_compressUpdate(context->compression_context, destination_buffer, compressed_bound, source, source_size, &compress_options);
    if (LZ4F_isError(result)) {
        PyMem_Free(destination_buffer);
        PyErr_Format(PyExc_RuntimeError, "LZ4F_compressBegin failed with code: %s", LZ4F_getErrorName(result));
        return NULL;
    }
    PyObject *bytes = PyBytes_FromStringAndSize(destination_buffer, result);
    PyMem_Free(destination_buffer);

    return bytes;
}

static PyObject *compress_end(PyObject *self, PyObject *args, PyObject *keywds)
{
    (void)self;

    PyObject *py_context = NULL;

    static char *kwlist[] = {"context", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "O", kwlist,
        &py_context)) {
        return NULL;
    }

    struct compression_context *context = (struct compression_context *)PyCapsule_GetPointer(py_context, NULL);
    if (!context || !context->compression_context) {
        PyErr_Format(PyExc_ValueError, "No compression context supplied");
        return NULL;
    }

    LZ4F_compressOptions_t compress_options;
    compress_options.stableSrc = 0;

    /* Because we compress with auto-flush enabled, this only needs to be large
     * enough for a footer, which is 4-8 bytes. Unfortunately, the lz4 library
     * doesn't provide a #define for this. We over-allocate to allow for larger
     * footers in the future. */
    char destination_buffer[64];

    size_t result = LZ4F_compressEnd(context->compression_context, destination_buffer, sizeof(destination_buffer), &compress_options);
    if (LZ4F_isError(result)) {
        PyErr_Format(PyExc_RuntimeError, "LZ4F_compressBegin failed with code: %s", LZ4F_getErrorName(result));
        return NULL;
    }

    return PyBytes_FromStringAndSize(destination_buffer, result);
}

static PyObject *get_frame_info(PyObject *self, PyObject *args, PyObject *keywds)
{
    (void)self;

    const char *source;
    int source_size;

    static char *kwlist[] = {"source", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "s#", kwlist,
        &source, &source_size)) {
        return NULL;
    }

    LZ4F_decompressionContext_t context;
    size_t result = LZ4F_createDecompressionContext(&context, LZ4F_VERSION);
    if (LZ4F_isError(result)) {
        PyErr_Format(PyExc_RuntimeError, "LZ4F_createDecompressionContext failed with code: %s", LZ4F_getErrorName(result));
        return NULL;
    }

    LZ4F_frameInfo_t frame_info;
    size_t source_size_copy = source_size;
    result = LZ4F_getFrameInfo(context, &frame_info, source, &source_size_copy);
    if (LZ4F_isError(result)) {
        LZ4F_freeDecompressionContext(context);
        PyErr_Format(PyExc_RuntimeError, "LZ4F_getFrameInfo failed with code: %s", LZ4F_getErrorName(result));
        return NULL;
    }

    result = LZ4F_freeDecompressionContext(context);
    if (LZ4F_isError(result)) {
        PyErr_Format(PyExc_RuntimeError, "LZ4F_freeDecompressionContext failed with code: %s", LZ4F_getErrorName(result));
        return NULL;
    }

    return Py_BuildValue("{s:i,s:i,s:i,s:i,s:i}",
            "blockSizeID", frame_info.blockSizeID,
            "blockMode", frame_info.blockMode,
            "contentChecksumFlag", frame_info.contentChecksumFlag,
            "frameType", frame_info.frameType,
            "contentSize", frame_info.contentSize
            );
}

static PyObject *decompress(PyObject *self, PyObject *args, PyObject *keywds)
{
    (void)self;

    char const *source;
    int source_size;
    int uncompressed_size = 0;

    static char *kwlist[] = {"source", "uncompressed_size", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "s#|i", kwlist,
            &source, &source_size,
            &uncompressed_size)) {
        return NULL;
    }

    LZ4F_decompressionContext_t context;
    size_t result = LZ4F_createDecompressionContext(&context, LZ4F_VERSION);
    if (LZ4F_isError(result)) {
        PyErr_Format(PyExc_RuntimeError, "LZ4F_createDecompressionContext failed with code: %s", LZ4F_getErrorName(result));
        return NULL;
    }

    LZ4F_frameInfo_t frame_info;
    size_t source_size_copy = source_size;
    result = LZ4F_getFrameInfo(context, &frame_info, source, &source_size_copy);
    if (LZ4F_isError(result)) {
        LZ4F_freeDecompressionContext(context);
        PyErr_Format(PyExc_RuntimeError, "LZ4F_getFrameInfo failed with code: %s", LZ4F_getErrorName(result));
        return NULL;
    }
    /* advance the source pointer past the header */
    source+=source_size_copy;

    if (frame_info.contentSize > PY_SSIZE_T_MAX) {
        LZ4F_freeDecompressionContext(context);
        PyErr_Format(PyExc_ValueError, "decompressed data would require %zu bytes, which is larger than the maximum supported size of %zd bytes", (size_t)frame_info.contentSize, PY_SSIZE_T_MAX);
        return NULL;
    }

    if (frame_info.contentSize == 0) {
        frame_info.contentSize = uncompressed_size;
    }
    if (frame_info.contentSize == 0) {
        LZ4F_freeDecompressionContext(context);
        PyErr_Format(PyExc_ValueError, "Decompressed content size was not encoded in this compressed data; therefore, you must specify it manually when calling decompress.");
        return NULL;
    }
    size_t destination_size = (size_t)frame_info.contentSize;

    char *destination_buffer = (char *)PyMem_Malloc(destination_size);
    if (!destination_buffer) {
        LZ4F_freeDecompressionContext(context);
        return PyErr_NoMemory();
    }

    LZ4F_decompressOptions_t options;
    options.stableDst = 1;

    size_t destination_size_copy = destination_size;
    source_size_copy = source_size;

    result = LZ4F_decompress(
            context,
            destination_buffer,
            &destination_size_copy,
            source,
            &source_size_copy,
            &options);
    if (result != 0) {
        LZ4F_freeDecompressionContext(context);
        PyMem_Free(destination_buffer);
        if (LZ4F_isError(result)) {
            PyErr_Format(PyExc_RuntimeError, "LZ4F_decompress failed with code: %s", LZ4F_getErrorName(result));
            return NULL;
        } else {
            PyErr_Format(PyExc_RuntimeError, "Something unexpected happened and decompress wants to be called again with %zu more bytes", result);
            return NULL;
        }
    }

    result = LZ4F_freeDecompressionContext(context);
    if (LZ4F_isError(result)) {
        PyMem_Free(destination_buffer);
        PyErr_Format(PyExc_RuntimeError, "LZ4F_freeDecompressionContext failed with code: %s", LZ4F_getErrorName(result));
        return NULL;
    }

    PyObject *py_dest = PyBytes_FromStringAndSize(destination_buffer, destination_size_copy);
    PyMem_Free(destination_buffer);
    return py_dest;
}

static struct PyModuleDef moduledef = {
        PyModuleDef_HEAD_INIT,
        "lz4frame",
        NULL,
        -1,
        module_methods
        };

MODULE_INIT_FUNC(lz4frame)
{
    PyObject *module = PyModule_Create(&moduledef);

    if (module == NULL)
        return NULL;

    /* VERSION and LZ4_VERSION are defined in setup.py and are passed in via
       the compilation command line. */
    PyModule_AddStringConstant(module, "VERSION", VERSION);
    PyModule_AddStringConstant(module, "__version__", VERSION);
    PyModule_AddStringConstant(module, "LZ4_VERSION", LZ4_VERSION);

    PyModule_AddIntConstant(module, "BLOCKSIZE_DEFAULT",        LZ4F_default);
    PyModule_AddIntConstant(module, "BLOCKSIZE_MAX64KB",        LZ4F_max64KB);
    PyModule_AddIntConstant(module, "BLOCKSIZE_MAX256KB",       LZ4F_max256KB);
    PyModule_AddIntConstant(module, "BLOCKSIZE_MAX1MB",         LZ4F_max1MB);
    PyModule_AddIntConstant(module, "BLOCKSIZE_MAX4MB",         LZ4F_max4MB);

    PyModule_AddIntConstant(module, "BLOCKMODE_LINKED",         LZ4F_blockLinked);
    PyModule_AddIntConstant(module, "BLOCKMODE_INDEPENDENT",    LZ4F_blockIndependent);

    PyModule_AddIntConstant(module, "CONTENTCHECKSUM_DISABLED", LZ4F_noContentChecksum);
    PyModule_AddIntConstant(module, "CONTENTCHECKSUM_ENABLED",  LZ4F_contentChecksumEnabled);

    PyModule_AddIntConstant(module, "FRAMETYPE_FRAME",          LZ4F_frame);
    PyModule_AddIntConstant(module, "FRAMETYPE_SKIPPABLEFRAME", LZ4F_skippableFrame);

    PyModule_AddIntConstant(module, "COMPRESSIONLEVEL_MIN",     0);
    PyModule_AddIntConstant(module, "COMPRESSIONLEVEL_MINHC",   3);
    PyModule_AddIntConstant(module, "COMPRESSIONLEVEL_MAX",     16);

    return module;
}

