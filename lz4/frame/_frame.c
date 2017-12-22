/*
 * Copyright (c) 2015, 2016 Jerry Ryle and Jonathan G. Underwood
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
 * 3. Neither the name of the copyright holders nor the names of its
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
#if defined(_WIN32) && defined(_MSC_VER)
#define inline __inline
#elif defined(__SUNPRO_C) || defined(__hpux) || defined(_AIX)
#define inline
#endif

#include <py3c.h>
#include <py3c/capsulethunk.h>

#include <stdlib.h>
#include <lz4frame.h>

#ifndef Py_UNUSED		/* This is already defined for Python 3.4 onwards */
#ifdef __GNUC__
#define Py_UNUSED(name) _unused_ ## name __attribute__((unused))
#else
#define Py_UNUSED(name) _unused_ ## name
#endif
#endif

static const char * compression_context_capsule_name = "_frame.LZ4F_cctx";
static const char * decompression_context_capsule_name = "_frame.LZ4F_dctx";

struct compression_context
{
  LZ4F_cctx * context;
  LZ4F_preferences_t preferences;
};

/*****************************
* create_compression_context *
******************************/
static void
destroy_compression_context (PyObject * py_context)
{
#ifndef PyCapsule_Type
  struct compression_context *context =
    PyCapsule_GetPointer (py_context, compression_context_capsule_name);
#else
  /* Compatibility with 2.6 via capsulethunk. */
  struct compression_context *context =  py_context;
#endif
  Py_BEGIN_ALLOW_THREADS
  LZ4F_freeCompressionContext (context->context);
  Py_END_ALLOW_THREADS

  PyMem_Free (context);
}

static PyObject *
create_compression_context (PyObject * Py_UNUSED (self))
{
  struct compression_context * context;
  LZ4F_errorCode_t result;

  context =
    (struct compression_context *)
    PyMem_Malloc (sizeof (struct compression_context));

  if (!context)
    {
      return PyErr_NoMemory ();
    }

  Py_BEGIN_ALLOW_THREADS

  result =
    LZ4F_createCompressionContext (&context->context,
                                   LZ4F_VERSION);
  Py_END_ALLOW_THREADS

  if (LZ4F_isError (result))
    {
      LZ4F_freeCompressionContext (context->context);
      PyMem_Free (context);
      PyErr_Format (PyExc_RuntimeError,
                    "LZ4F_createCompressionContext failed with code: %s",
                    LZ4F_getErrorName (result));
      return NULL;
    }

  return PyCapsule_New (context, compression_context_capsule_name,
                        destroy_compression_context);
}

static inline PyObject *
__buff_alloc (const Py_ssize_t dest_size, const int return_bytearray)
{
  PyObject * py_dest;

  if (return_bytearray)
    {
      py_dest = PyByteArray_FromStringAndSize (NULL, dest_size);
    }
  else
    {
      py_dest = PyBytes_FromStringAndSize (NULL, dest_size);
    }

  if (py_dest == NULL)
    {
      return PyErr_NoMemory();
    }
  else
    {
      return py_dest;
    }
}

static inline char *
__buff_to_string (PyObject const * buff, const int bytearray)
{
  if (bytearray)
    {
      return PyByteArray_AS_STRING (buff);
    }
  else
    {
      return PyBytes_AS_STRING (buff);
    }
}

static inline void
__buff_resize (PyObject ** buff, Py_ssize_t size,
               const int return_bytearray)
{
  int ret;

  if (return_bytearray)
    {
      ret = PyByteArray_Resize (*buff, size);
    }
  else
    {
      ret = _PyBytes_Resize (buff, size);
    }
  if (ret)
    {
      PyErr_SetString (PyExc_RuntimeError,
                       "Failed to resize buffer size");
    }
}

/************
 * compress *
 ************/
static PyObject *
compress (PyObject * Py_UNUSED (self), PyObject * args,
          PyObject * keywds)
{
  Py_buffer source;
  Py_ssize_t source_size;
  int store_size = 1;
  int return_bytearray = 0;
  int content_checksum = 0;
  int block_linked = 1;
  LZ4F_preferences_t preferences;
  size_t compressed_bound;
  size_t compressed_size;
  Py_ssize_t dest_size;
  PyObject *py_dest;
  char *dest;

  static char *kwlist[] = { "data",
                            "compression_level",
                            "block_size",
                            "content_checksum",
                            "block_linked",
                            "store_size",
                            "return_bytearray",
                            NULL
                          };


  memset (&preferences, 0, sizeof preferences);

#if IS_PY3
  if (!PyArg_ParseTupleAndKeywords (args, keywds, "y*|iipppp", kwlist,
                                    &source,
                                    &preferences.compressionLevel,
                                    &preferences.frameInfo.blockSizeID,
                                    &content_checksum,
                                    &block_linked,
                                    &store_size,
                                    &return_bytearray))
    {
      return NULL;
    }
#else
  if (!PyArg_ParseTupleAndKeywords (args, keywds, "s*|iiiiii", kwlist,
                                    &source,
                                    &preferences.compressionLevel,
                                    &preferences.frameInfo.blockSizeID,
                                    &content_checksum,
                                    &block_linked,
                                    &store_size,
                                    &return_bytearray))
    {
      return NULL;
    }
#endif

  if (content_checksum)
    {
      preferences.frameInfo.contentChecksumFlag = LZ4F_contentChecksumEnabled;
    }
  else
    {
      preferences.frameInfo.contentChecksumFlag = LZ4F_noContentChecksum;
    }

  if (block_linked)
    {
      preferences.frameInfo.blockMode = LZ4F_blockLinked;
    }
  else
    {
      preferences.frameInfo.blockMode = LZ4F_blockIndependent;
    }

  source_size = source.len;

  preferences.autoFlush = 0;
  if (store_size)
    {
      preferences.frameInfo.contentSize = source_size;
    }
  else
    {
      preferences.frameInfo.contentSize = 0;
    }

  Py_BEGIN_ALLOW_THREADS
  compressed_bound =
    LZ4F_compressFrameBound (source_size, &preferences);
  Py_END_ALLOW_THREADS

  if (compressed_bound > PY_SSIZE_T_MAX)
    {
      PyErr_Format (PyExc_ValueError,
                    "Input data could require %zu bytes, which is larger than the maximum supported size of %zd bytes",
                    compressed_bound, PY_SSIZE_T_MAX);
      return NULL;
    }

  dest_size = (Py_ssize_t) compressed_bound;

  py_dest = __buff_alloc(dest_size, return_bytearray);
  if (py_dest == NULL)
    {
      PyBuffer_Release(&source);
      return PyErr_NoMemory();
    }
  dest = __buff_to_string(py_dest, return_bytearray);

  Py_BEGIN_ALLOW_THREADS
  compressed_size =
    LZ4F_compressFrame (dest, dest_size, source.buf, source_size,
                        &preferences);
  Py_END_ALLOW_THREADS

  PyBuffer_Release(&source);

  if (LZ4F_isError (compressed_size))
    {
      Py_DECREF (py_dest);
      PyErr_Format (PyExc_RuntimeError,
                    "LZ4F_compressFrame failed with code: %s",
                    LZ4F_getErrorName (compressed_size));
      return NULL;
    }

  /* The actual compressed size might be less than we allocated (we allocated
     using a worst case guess). If the actual size is less than 75% of what we
     allocated, then it's worth performing an expensive resize operation to
     reclaim some space. */
  if ((Py_ssize_t) compressed_size < (dest_size / 4) * 3)
    {
      __buff_resize(&py_dest, (Py_ssize_t) compressed_size, return_bytearray);
    }
  else
    {
      Py_SIZE (py_dest) = (Py_ssize_t) compressed_size;
    }

  return py_dest;
}

/******************
 * compress_begin *
 ******************/
static PyObject *
compress_begin (PyObject * Py_UNUSED (self), PyObject * args,
                PyObject * keywds)
{
  PyObject *py_context = NULL;
  Py_ssize_t source_size = 0;
  int return_bytearray = 0;
  int content_checksum = 0;
  int block_linked = 1;
  LZ4F_preferences_t preferences;
  PyObject *py_destination;
  char * destination_buffer;
  /* The destination buffer needs to be large enough for a header, which is 15
   * bytes. Unfortunately, the lz4 library doesn't provide a #define for this.
   * We over-allocate to allow for larger headers in the future. */
  const size_t header_size = 32;
  struct compression_context *context;
  size_t result;
  static char *kwlist[] = { "context",
                            "source_size",
                            "compression_level",
                            "block_size",
                            "content_checksum",
                            "block_linked",
                            "auto_flush",
                            "return_bytearray",
                            NULL
                          };

  memset (&preferences, 0, sizeof preferences);

  /* Default to having autoFlush enabled unless specified otherwise via keyword
     argument */
  preferences.autoFlush = 1;

#if IS_PY3
  if (!PyArg_ParseTupleAndKeywords (args, keywds, "O|kiipppp", kwlist,
                                    &py_context,
                                    &source_size,
                                    &preferences.compressionLevel,
                                    &preferences.frameInfo.blockSizeID,
                                    &content_checksum,
                                    &block_linked,
                                    &preferences.autoFlush,
                                    &return_bytearray
                                    ))
    {
      return NULL;
    }
#else
  if (!PyArg_ParseTupleAndKeywords (args, keywds, "O|kiiiiii", kwlist,
                                    &py_context,
                                    &source_size,
                                    &preferences.compressionLevel,
                                    &preferences.frameInfo.blockSizeID,
                                    &content_checksum,
                                    &block_linked,
                                    &preferences.autoFlush,
                                    &return_bytearray
                                    ))
    {
      return NULL;
    }
#endif
  if (content_checksum)
    {
      preferences.frameInfo.contentChecksumFlag = LZ4F_contentChecksumEnabled;
    }
  else
    {
      preferences.frameInfo.contentChecksumFlag = LZ4F_noContentChecksum;
    }

  if (block_linked)
    {
      preferences.frameInfo.blockMode = LZ4F_blockLinked;
    }
  else
    {
      preferences.frameInfo.blockMode = LZ4F_blockIndependent;
    }

  preferences.frameInfo.contentSize = source_size;

  context =
    (struct compression_context *) PyCapsule_GetPointer (py_context, compression_context_capsule_name);

  if (!context || !context->context)
    {
      PyErr_SetString (PyExc_ValueError, "No valid compression context supplied");
      return NULL;
    }

  context->preferences = preferences;

  py_destination = __buff_alloc(header_size, return_bytearray);
  if (py_destination == NULL)
    {
      return PyErr_NoMemory();
    }
  destination_buffer = __buff_to_string (py_destination, return_bytearray);

  Py_BEGIN_ALLOW_THREADS
  result = LZ4F_compressBegin (context->context,
                               destination_buffer,
                               header_size,
                               &context->preferences);
  Py_END_ALLOW_THREADS

  if (LZ4F_isError (result))
    {
      PyErr_Format (PyExc_RuntimeError,
                    "LZ4F_compressBegin failed with code: %s",
                    LZ4F_getErrorName (result));
      return NULL;
    }

  Py_SIZE (py_destination) = result;
  return py_destination;
}

/******************
 * compress_chunk *
 ******************/
static PyObject *
compress_chunk (PyObject * Py_UNUSED (self), PyObject * args,
                 PyObject * keywds)
{
  PyObject *py_context = NULL;
  Py_buffer source;
  Py_ssize_t source_size;
  struct compression_context *context;
  size_t compressed_bound;
  PyObject *py_destination;
  char *destination_buffer;
  LZ4F_compressOptions_t compress_options;
  size_t result;
  int return_bytearray = 0;
  static char *kwlist[] = { "context",
                            "data",
                            "return_bytearray",
                            NULL
  };

#if IS_PY3
  if (!PyArg_ParseTupleAndKeywords (args, keywds, "Oy*|p", kwlist,
                                    &py_context,
                                    &source,
                                    &return_bytearray))
    {
      return NULL;
    }
#else
  if (!PyArg_ParseTupleAndKeywords (args, keywds, "Os*|i", kwlist,
                                    &py_context,
                                    &source,
                                    &return_bytearray))
    {
      return NULL;
    }
#endif

  source_size = source.len;

  context =
    (struct compression_context *) PyCapsule_GetPointer (py_context, compression_context_capsule_name);
  if (!context || !context->context)
    {
      PyBuffer_Release(&source);
      PyErr_Format (PyExc_ValueError, "No compression context supplied");
      return NULL;
    }

  /* If autoFlush is enabled, then the destination buffer only needs to be as
     big as LZ4F_compressFrameBound specifies for this source size. However, if
     autoFlush is disabled, previous calls may have resulted in buffered data,
     and so we need instead to use LZ4F_compressBound to find the size required
     for the destination buffer. This means that with autoFlush disabled we may
     frequently allocate more memory than needed. */
  Py_BEGIN_ALLOW_THREADS
  if (context->preferences.autoFlush == 1)
    {
      compressed_bound =
        LZ4F_compressFrameBound (source_size, &context->preferences);
    }
  else
    {
      compressed_bound =
        LZ4F_compressBound (source_size, &context->preferences);
    }
  Py_END_ALLOW_THREADS

  if (compressed_bound > PY_SSIZE_T_MAX)
    {
      PyBuffer_Release(&source);
      PyErr_Format (PyExc_ValueError,
                    "input data could require %zu bytes, which is larger than the maximum supported size of %zd bytes",
                    compressed_bound, PY_SSIZE_T_MAX);
      return NULL;
    }

  py_destination = __buff_alloc((Py_ssize_t) compressed_bound, return_bytearray);
  if (py_destination == NULL)
    {
      PyBuffer_Release(&source);
      return PyErr_NoMemory();
    }
  destination_buffer = __buff_to_string (py_destination, return_bytearray);

  compress_options.stableSrc = 0;

  Py_BEGIN_ALLOW_THREADS
  result =
    LZ4F_compressUpdate (context->context, destination_buffer,
                         compressed_bound, source.buf, source_size,
                         &compress_options);
  Py_END_ALLOW_THREADS

  PyBuffer_Release(&source);

  if (LZ4F_isError (result))
    {
      PyMem_Free (destination_buffer);
      PyErr_Format (PyExc_RuntimeError,
                    "LZ4F_compressUpdate failed with code: %s",
                    LZ4F_getErrorName (result));
      return NULL;
    }

  if (result < (compressed_bound / 4) * 3)
    {
      __buff_resize (&py_destination, (Py_ssize_t) result, return_bytearray);
    }
  else
    {
      Py_SIZE (py_destination) = (Py_ssize_t) result;
    }

  return py_destination;
}

/****************
 * compress_end *
 ****************/
static PyObject *
compress_end (PyObject * Py_UNUSED (self), PyObject * args, PyObject * keywds)
{
  PyObject *py_context = NULL;
  LZ4F_compressOptions_t compress_options;
  struct compression_context *context;
  size_t destination_size;
  int return_bytearray = 0;
  PyObject *py_destination;
  char * destination_buffer;
  size_t result;
  static char *kwlist[] = { "context",
                            "return_bytearray",
                            NULL
  };
#if IS_PY3
  if (!PyArg_ParseTupleAndKeywords (args, keywds, "O|p", kwlist,
                                    &py_context,
                                    &return_bytearray))
    {
      return NULL;
    }
#else
  if (!PyArg_ParseTupleAndKeywords (args, keywds, "O|i", kwlist,
                                    &py_context,
                                    &return_bytearray))
    {
      return NULL;
    }
#endif
  context =
    (struct compression_context *) PyCapsule_GetPointer (py_context, compression_context_capsule_name);
  if (!context || !context->context)
    {
      PyErr_SetString (PyExc_ValueError, "No compression context supplied");
      return NULL;
    }

  compress_options.stableSrc = 0;

  /* Calling LZ4F_compressBound with srcSize equal to 1 returns a size
     sufficient to fit (i) any remaining buffered data (when autoFlush is
     disabled) and the footer size, which is either 4 or 8 bytes depending on
     whether checksums are enabled. https://github.com/lz4/lz4/issues/280 */
  Py_BEGIN_ALLOW_THREADS
  destination_size = LZ4F_compressBound (1, &(context->preferences));
  Py_END_ALLOW_THREADS

  py_destination = __buff_alloc((Py_ssize_t) destination_size, return_bytearray);
  if (py_destination == NULL)
    {
      return PyErr_NoMemory();
    }
  destination_buffer = __buff_to_string (py_destination, return_bytearray);

  Py_BEGIN_ALLOW_THREADS
  result =
    LZ4F_compressEnd (context->context, destination_buffer,
                      destination_size, &compress_options);
  Py_END_ALLOW_THREADS

  if (LZ4F_isError (result))
    {
      PyMem_Free (destination_buffer);
      PyErr_Format (PyExc_RuntimeError,
                    "LZ4F_compressEnd failed with code: %s",
                    LZ4F_getErrorName (result));
      return NULL;
    }

  if (result < (destination_size / 4) * 3)
    {
      __buff_resize (&py_destination, (Py_ssize_t) result, return_bytearray);
    }
  else
    {
      Py_SIZE (py_destination) = (Py_ssize_t) result;
    }

  return py_destination;
}

/******************
 * get_frame_info *
 ******************/
static PyObject *
get_frame_info (PyObject * Py_UNUSED (self), PyObject * args,
                PyObject * keywds)
{
  Py_buffer py_source;
  char *source;
  size_t source_size;
  LZ4F_decompressionContext_t context;
  LZ4F_frameInfo_t frame_info;
  size_t result;
  unsigned int block_size;
  unsigned int block_size_id;
  int block_linked;
  int content_checksum;
  int block_checksum;
  int skippable;

  static char *kwlist[] = { "data",
                            NULL
  };

#if IS_PY3
  if (!PyArg_ParseTupleAndKeywords (args, keywds, "y*", kwlist,
                                    &py_source))
    {
      return NULL;
    }
#else
  if (!PyArg_ParseTupleAndKeywords (args, keywds, "s*", kwlist,
                                    &py_source))
    {
      return NULL;
    }
#endif

  Py_BEGIN_ALLOW_THREADS

  result = LZ4F_createDecompressionContext (&context, LZ4F_VERSION);

  if (LZ4F_isError (result))
    {
      Py_BLOCK_THREADS
      PyBuffer_Release (&py_source);
      PyErr_Format (PyExc_RuntimeError,
                    "LZ4F_createDecompressionContext failed with code: %s",
                    LZ4F_getErrorName (result));
      return NULL;
    }

  source = (char *) py_source.buf;
  source_size = (size_t) py_source.len;

  result =
    LZ4F_getFrameInfo (context, &frame_info, source, &source_size);

  if (LZ4F_isError (result))
    {
      LZ4F_freeDecompressionContext (context);
      Py_BLOCK_THREADS
      PyBuffer_Release (&py_source);
      PyErr_Format (PyExc_RuntimeError,
                    "LZ4F_getFrameInfo failed with code: %s",
                    LZ4F_getErrorName (result));
      return NULL;
    }

  result = LZ4F_freeDecompressionContext (context);

  Py_END_ALLOW_THREADS

  PyBuffer_Release (&py_source);

  if (LZ4F_isError (result))
    {
      PyErr_Format (PyExc_RuntimeError,
                    "LZ4F_freeDecompressionContext failed with code: %s",
                    LZ4F_getErrorName (result));
      return NULL;
    }

#define KB *(1<<10)
#define MB *(1<<20)
  switch (frame_info.blockSizeID)
    {
    case LZ4F_default:
    case LZ4F_max64KB:
      block_size = 64 KB;
      block_size_id = LZ4F_max64KB;
      break;
    case LZ4F_max256KB:
      block_size = 256 KB;
      block_size_id = LZ4F_max256KB;
      break;
    case LZ4F_max1MB:
      block_size = 1 MB;
      block_size_id = LZ4F_max1MB;
      break;
    case LZ4F_max4MB:
      block_size = 4 MB;
      block_size_id = LZ4F_max4MB;
      break;
    default:
      PyErr_Format (PyExc_RuntimeError,
                    "Unrecognized blockSizeID in get_frame_info: %d",
                    frame_info.blockSizeID);
      return NULL;
    }
#undef KB
#undef MB

  if (frame_info.blockMode == LZ4F_blockLinked)
    {
      block_linked = 1;
    }
  else if (frame_info.blockMode == LZ4F_blockIndependent)
    {
      block_linked = 0;
    }
  else
    {
      PyErr_Format (PyExc_RuntimeError,
                    "Unrecognized blockMode in get_frame_info: %d",
                    frame_info.blockMode);
      return NULL;
    }

  if (frame_info.contentChecksumFlag == LZ4F_noContentChecksum)
    {
      content_checksum = 0;
    }
  else if (frame_info.contentChecksumFlag == LZ4F_contentChecksumEnabled)
    {
      content_checksum = 1;
    }
  else
    {
      PyErr_Format (PyExc_RuntimeError,
                    "Unrecognized contentChecksumFlag in get_frame_info: %d",
                    frame_info.contentChecksumFlag);
      return NULL;
    }

  if (frame_info.blockChecksumFlag == LZ4F_noBlockChecksum)
    {
      block_checksum = 0;
    }
  else if (frame_info.blockChecksumFlag == LZ4F_blockChecksumEnabled)
    {
      block_checksum = 1;
    }
  else
    {
      PyErr_Format (PyExc_RuntimeError,
                    "Unrecognized blockChecksumFlag in get_frame_info: %d",
                    frame_info.blockChecksumFlag);
      return NULL;
    }

  if (frame_info.frameType == LZ4F_frame)
    {
      skippable = 0;
    }
  else if (frame_info.frameType == LZ4F_skippableFrame)
    {
      skippable = 1;
    }
  else
    {
      PyErr_Format (PyExc_RuntimeError,
                    "Unrecognized frameType in get_frame_info: %d",
                    frame_info.frameType);
      return NULL;
    }

  return Py_BuildValue ("{s:I,s:I,s:O,s:O,s:O,s:O,s:K}",
                        "block_size", block_size,
                        "block_size_id", block_size_id,
                        "block_linked", block_linked ? Py_True : Py_False,
                        "content_checksum", content_checksum ? Py_True : Py_False,
                        "block_checksum", block_checksum ? Py_True : Py_False,
                        "skippable", skippable ? Py_True : Py_False,
                        "content_size", frame_info.contentSize);
}

/*******************************
* create_decompression_context *
********************************/
static void
destroy_decompression_context (PyObject * py_context)
{
#ifndef PyCapsule_Type
  LZ4F_dctx * context =
    PyCapsule_GetPointer (py_context, decompression_context_capsule_name);
#else
  /* Compatibility with 2.6 via capsulethunk. */
  LZ4F_dctx * context =  py_context;
#endif
  Py_BEGIN_ALLOW_THREADS
  LZ4F_freeDecompressionContext (context);
  Py_END_ALLOW_THREADS
}

static PyObject *
create_decompression_context (PyObject * Py_UNUSED (self))
{
  LZ4F_dctx * context;
  LZ4F_errorCode_t result;

  Py_BEGIN_ALLOW_THREADS
  result = LZ4F_createDecompressionContext (&context, LZ4F_VERSION);
  if (LZ4F_isError (result))
    {
      Py_BLOCK_THREADS
      LZ4F_freeDecompressionContext (context);
      PyErr_Format (PyExc_RuntimeError,
                    "LZ4F_createDecompressionContext failed with code: %s",
                    LZ4F_getErrorName (result));
      return NULL;
    }
  Py_END_ALLOW_THREADS

  return PyCapsule_New (context, decompression_context_capsule_name,
                        destroy_decompression_context);
}

static inline PyObject *
__decompress(LZ4F_dctx * context, char * source, size_t source_size,
             int full_frame, int return_bytearray, int return_bytes_read)
{
  size_t source_remain;
  size_t source_read;
  char * source_cursor;
  char * source_end;
  char * destination_buffer;
  size_t destination_write;
  char * destination_cursor;
  size_t destination_written;
  size_t destination_buffer_size;
  PyObject *py_destination;
  size_t result = 0;
  LZ4F_frameInfo_t frame_info;
  LZ4F_decompressOptions_t options;

  Py_BEGIN_ALLOW_THREADS

  source_cursor = source;
  source_end = source + source_size;
  source_remain = source_size;

  if (full_frame)
    {
      source_read = source_size;

      result =
        LZ4F_getFrameInfo (context, &frame_info,
                           source_cursor, &source_read);

      if (LZ4F_isError (result))
        {
          Py_BLOCK_THREADS
          PyErr_Format (PyExc_RuntimeError,
                        "LZ4F_getFrameInfo failed with code: %s",
                        LZ4F_getErrorName (result));
          return NULL;
        }

      /* Advance the source_cursor pointer past the header - the call to
         getFrameInfo above replaces the passed source_read value with the
         number of bytes read. Also reduce source_remain accordingly. */
      source_cursor += source_read;
      source_remain -= source_read;
      if (frame_info.contentSize > 0)
        {
          destination_buffer_size = frame_info.contentSize;
        }
      else
        {
          destination_buffer_size = 2 * source_remain;
        }
    }
  else
    {
      /* Choose an initial destination size as either twice the source size, and
         we'll grow the allocation as needed. */
      destination_buffer_size = 2 * source_remain;
    }

  Py_BLOCK_THREADS

  py_destination = __buff_alloc ((Py_ssize_t) destination_buffer_size, return_bytearray);
  if (py_destination == NULL)
    {
      return PyErr_NoMemory();
    }
  destination_buffer = __buff_to_string (py_destination, return_bytearray);

  Py_UNBLOCK_THREADS

  if (full_frame)
    {
      options.stableDst = 1;
    }
  else
    {
      options.stableDst = 0;
    }

  source_read = source_remain;

  destination_write = destination_buffer_size;
  destination_cursor = destination_buffer;
  destination_written = 0;

  while (1)
    {
      /* Decompress from the source string and write to the destination_buffer
         until there's no more source string to read, or until we've reached the
         frame end.

         On calling LZ4F_decompress, source_read is set to the remaining length
         of source available to read. On return, source_read is set to the
         actual number of bytes read from source, which may be less than
         available. NB: LZ4F_decompress does not explicitly fail on empty input.

         On calling LZ4F_decompress, destination_write is the number of bytes in
         destination available for writing. On exit, destination_write is set to
         the actual number of bytes written to destination. */
      result = LZ4F_decompress (context,
                                destination_cursor,
                                &destination_write,
                                source_cursor,
                                &source_read,
                                &options);

      if (LZ4F_isError (result))
        {
          Py_BLOCK_THREADS
          PyErr_Format (PyExc_RuntimeError,
                        "LZ4F_decompress failed with code: %s",
                        LZ4F_getErrorName (result));
          return NULL;
        }

      destination_written += destination_write;
      source_cursor += source_read;
      source_read = source_end - source_cursor;

      if (result == 0)
        {
          /* We've reached the end of the frame. */
          break;
        }
      else if (source_cursor == source_end)
        {
          /* We've reached end of input. */
          break;
        }
      else if (destination_written == destination_buffer_size)
        {
          /* Destination_buffer is full, so need to expand it. result is an
             indication of number of source bytes remaining, so we'll use this
             to estimate the new size of the destination buffer. */
          destination_buffer_size += 3 * result;

          Py_BLOCK_THREADS
          __buff_resize (&py_destination, (Py_ssize_t) destination_buffer_size,
                         return_bytearray);
          if (py_destination == NULL)
            {
              /* PyErr_SetString already called in __buff_resize */
              return NULL;
            }
          destination_buffer = __buff_to_string (py_destination, return_bytearray);
          Py_UNBLOCK_THREADS
        }
      /* Data still remaining to be decompressed, so increment the destination
         cursor location, and reset destination_write ready for the next
         iteration. Important to re-initialize destination_cursor here (as
         opposed to simply incrementing it) so we're pointing to the realloc'd
         memory location. */
      destination_cursor = destination_buffer + destination_written;
      destination_write = destination_buffer_size - destination_written;
    }

  Py_END_ALLOW_THREADS

  if (result != 0 && full_frame)
    {
      PyErr_Format (PyExc_RuntimeError,
                    "full_frame=True specified, but data did not contain complete frame. LZ4F_decompress returned: %zu", result);
      Py_DECREF(py_destination);
      return NULL;
    }

  if (LZ4F_isError (result))
    {
      PyErr_Format (PyExc_RuntimeError,
                    "LZ4F_freeDecompressionContext failed with code: %s",
                    LZ4F_getErrorName (result));
      Py_DECREF(py_destination);
      return NULL;
    }

  if (destination_written < (destination_buffer_size / 4) * 3)
    {
      __buff_resize (&py_destination, (Py_ssize_t) destination_written,
                     return_bytearray);
    }
  else
    {
      Py_SIZE (py_destination) = destination_written;
    }

  if (return_bytes_read)
    {
      return Py_BuildValue ("Oi", py_destination, source_cursor - source);
    }
  else
    {
      return Py_BuildValue ("O", py_destination);
    }
}

/**************
 * decompress *
 **************/
static PyObject *
decompress (PyObject * Py_UNUSED (self), PyObject * args,
            PyObject * keywds)
{
  LZ4F_dctx * context;
  LZ4F_errorCode_t result;
  Py_buffer py_source;
  char * source;
  size_t source_size;
  PyObject * decompressed;
  int return_bytearray = 0;
  int return_bytes_read = 0;
  static char *kwlist[] = { "data",
                            "return_bytearray",
                            "return_bytes_read",
                            NULL
                          };

#if IS_PY3
  if (!PyArg_ParseTupleAndKeywords (args, keywds, "y*|pp", kwlist,
                                    &py_source,
                                    &return_bytearray,
                                    &return_bytes_read
                                    ))
    {
      return NULL;
    }
#else
  if (!PyArg_ParseTupleAndKeywords (args, keywds, "s*|ii", kwlist,
                                    &py_source,
                                    &return_bytearray,
                                    &return_bytes_read
                                    ))
    {
      return NULL;
    }
#endif

  Py_BEGIN_ALLOW_THREADS
  result = LZ4F_createDecompressionContext (&context, LZ4F_VERSION);
  if (LZ4F_isError (result))
    {
      LZ4F_freeDecompressionContext (context);
      Py_BLOCK_THREADS
      PyBuffer_Release(&py_source);
      PyErr_Format (PyExc_RuntimeError,
                    "LZ4F_createDecompressionContext failed with code: %s",
                    LZ4F_getErrorName (result));
      return NULL;
    }
  Py_END_ALLOW_THREADS

  /* MSVC can't do pointer arithmetic on void * pointers, so cast to char * */
  source = (char *) py_source.buf;
  source_size = py_source.len;

  decompressed = __decompress (context, source, source_size, 1, return_bytearray,
                               return_bytes_read);

  PyBuffer_Release(&py_source);

  Py_BEGIN_ALLOW_THREADS
  LZ4F_freeDecompressionContext (context);
  Py_END_ALLOW_THREADS

  return decompressed;
}

/********************
 * decompress_chunk *
 ********************/
static PyObject *
decompress_chunk (PyObject * Py_UNUSED (self), PyObject * args,
                  PyObject * keywds)
{
  PyObject * py_context = NULL;
  PyObject * decompressed;
  LZ4F_dctx * context;
  Py_buffer py_source;
  char * source;
  size_t source_size;
  int return_bytearray = 0;
  int return_bytes_read = 0;
  static char *kwlist[] = { "context",
                            "data",
                            "return_bytearray",
                            "return_bytes_read",
                            NULL
                          };

#if IS_PY3
  if (!PyArg_ParseTupleAndKeywords (args, keywds, "Oy*|pp", kwlist,
                                    &py_context,
                                    &py_source,
                                    &return_bytearray,
                                    &return_bytes_read
                                    ))
    {
      return NULL;
    }
#else
  if (!PyArg_ParseTupleAndKeywords (args, keywds, "Os*|ii", kwlist,
                                    &py_context,
                                    &py_source,
                                    &return_bytearray,
                                    &return_bytes_read
                                    ))
    {
      return NULL;
    }
#endif

  context = (LZ4F_dctx *)
    PyCapsule_GetPointer (py_context, decompression_context_capsule_name);

  if (!context)
    {
      PyBuffer_Release(&py_source);
      PyErr_SetString (PyExc_ValueError,
                       "No valid decompression context supplied");
      return NULL;
    }

  /* MSVC can't do pointer arithmetic on void * pointers, so cast to char * */
  source = (char *) py_source.buf;
  source_size = py_source.len;

  decompressed = __decompress (context, source, source_size, 0, return_bytearray,
                               return_bytes_read);

  PyBuffer_Release(&py_source);

  return decompressed;
}

PyDoc_STRVAR(
 create_compression_context__doc,
 "create_compression_context()\n\n"                                     \
 "Creates a Compression Context object, which will be used in all\n"    \
 "compression operations.\n\n"                                          \
 "Returns:\n"                                                           \
 "    cCtx: A compression context\n"
 );

#define COMPRESS_KWARGS_DOCSTRING                                     \
  "    block_size (int): Sepcifies the maximum blocksize to use.\n"     \
  "        Options:\n\n"                                                \
  "        - lz4.frame.BLOCKSIZE_DEFAULT or 0: the lz4 library default\n" \
  "        - lz4.frame.BLOCKSIZE_MAX64KB or 4: 64 kB\n"                 \
  "        - lz4.frame.BLOCKSIZE_MAX256KB or 5: 256 kB\n"               \
  "        - lz4.frame.BLOCKSIZE_MAX1MB or 6: 1 MB\n"                   \
  "        - lz4.frame.BLOCKSIZE_MAX4MB or 7: 4 MB\n\n"                 \
  "        If unspecified, will default to lz4.frame.BLOCKSIZE_DEFAULT\n" \
  "        which is currently equal to lz4.frame.BLOCKSIZE_MAX64KB.\n"  \
  "    block_linked (bool): Specifies whether to use block-linked\n"    \
  "        compression. If True, the compression ratio is improved,\n" \
  "        particularly for small block sizes. Default is True.\n"                   \
  "    compression_level (int): Specifies the level of compression used.\n" \
  "        Values between 0-16 are valid, with 0 (default) being the\n"     \
  "        lowest compression (0-2 are the same value), and 16 the highest.\n" \
  "        Values below 0 will enable \"fast acceleration\", proportional\n" \
  "        to the value. Values above 16 will be treated as 16.\n"             \
  "        The following module constants are provided as a convenience:\n\n" \
  "        - lz4.frame.COMPRESSIONLEVEL_MIN: Minimum compression (0, the default)\n" \
  "        - lz4.frame.COMPRESSIONLEVEL_MINHC: Minimum high-compression mode (3)\n" \
  "        - lz4.frame.COMPRESSIONLEVEL_MAX: Maximum compression (16)\n\n" \
  "    content_checksum (bool): Specifies whether to enable checksumming of\n" \
  "        the payload content. If True, a checksum is stored at the end of\n" \
  "        the frame, and checked during decompression. Default is False.\n" \
  "    return_bytearray (bool): If True a bytearray object will be returned.\n" \
  "        If False, a string of bytes is returned. The default is False.\n" \

PyDoc_STRVAR(
 compress__doc,
 "compress(data, compression_level=0, block_size=0, content_checksum=0,\n" \
 "         block_linked=True, store_size=True, return_bytearray=False)\n\n" \
 "Compresses `data` returning the compressed data as a complete frame.\n" \
 "The returned data includes a header and endmark and so is suitable\n" \
 "for writing to a file.\n\n"                                           \
 "Args:\n"                                                              \
 "    data (str, bytes or buffer-compatible object): data to compress\n\n" \
 "Keyword Args:\n"                                                      \
 COMPRESS_KWARGS_DOCSTRING                                              \
 "    store_size (bool): If True then the frame will include an 8-byte\n" \
 "        header field that is the uncompressed size of data included\n" \
 "        within the frame. Default is True.\n\n"                       \
 "Returns:\n"                                                           \
 "    str or bytearray: Compressed data\n"
 );
PyDoc_STRVAR
(
 compress_begin__doc,
 "compress_begin(context, source_size=0, compression_level=0, block_size=0,\n" \
 "    content_checksum=0, content_size=1, block_mode=0, frame_type=0,\n" \
 "    auto_flush=1)\n\n"                                                \
 "Creates a frame header from a compression context.\n\n"               \
 "Args:\n"                                                              \
 "    context (cCtx): A compression context.\n\n"                       \
 "Keyword Args:\n"                                                      \
 COMPRESS_KWARGS_DOCSTRING                                              \
 "    auto_flush (int): Enable (1, default) or disable (0) autoFlush.\n" \
 "         When autoFlush is disabled, the LZ4 library may buffer data\n" \
 "         until a block is full\n\n"                                   \
 "    source_size (int): This optionally specifies the uncompressed size\n" \
 "        of the source data to be compressed. If specified, the size\n" \
 "        will be stored in the frame header for use during decompression.\n" \
 "    return_bytearray (bool): If True a bytearray object will be returned.\n" \
 "        If False, a string of bytes is returned. The default is False.\n\n" \
 "Returns:\n"                                                           \
 "    str or bytearray: Frame header.\n"
 );

#undef COMPRESS_KWARGS_DOCSTRING

PyDoc_STRVAR
(
 compress_chunk__doc,
 "compress_chunk(context, data)\n\n"                                    \
 "Compresses blocks of data and returns the compressed data. The returned\n" \
 "data should be concatenated with the data returned from `compress_begin`\n" \
 " and any previous calls to `compress`. \n"                            \
 "Args:\n"                                                              \
 "    context (cCtx): compression context\n"                            \
 "    data (str, bytes or buffer-compatible object): data to compress\n\n" \
 "Keyword Args:\n"                                                      \
 "    return_bytearray (bool): If True a bytearray object will be returned.\n" \
 "        If False, a string of bytes is returned. The default is False.\n\n" \
 "Returns:\n"                                                           \
 "    str or bytearray: Compressed data as a string\n\n"                \
 "Notes:\n"                                                             \
 "    If auto flush is disabled (`auto_flush`=False when calling\n"     \
 "    compress_begin) this function may buffer and retain some or all of\n" \
 "    the compressed data for future calls to `compress`.\n"
 );

PyDoc_STRVAR
(
 compress_end__doc,
 "compress_end(context, return_bytearray=False)\n\n"                    \
 "Flushes a compression context returning an endmark and optional checksum.\n" \
 "The returned data should be appended to the output of previous calls to.\n" \
 "`compress`\n\n"                                                       \
 "Args:\n"                                                              \
 "    context (cCtx): compression context\n\n"                          \
 "Keyword Args:\n"                                                      \
 "    return_bytearray (bool): If True a bytearray object will be returned.\n" \
 "        If False, a string of bytes is returned. The default is False.\n\n" \
 "Returns:\n"                                                           \
 "    str or bytearray: Remaining (buffered) compressed data, end mark and\n" \
 "        optional checksum.\n"
 );

PyDoc_STRVAR
(
 get_frame_info__doc,
 "get_frame_info(frame)\n\n"                                            \
 "Given a frame of compressed data, returns information about the frame.\n" \
 "Args:\n"                                                              \
 "    frame (str, bytes or buffer-compatible object): LZ4 compressed frame\n\n"                            \
 "Returns:\n"                                                           \
 "    dict: Dictionary with keys:\n"                                    \
 "    - block_size (int): the maximum size (in bytes) of each block\n"  \
 "    - block_size_id (int): identifier for maximum block size\n"       \
 "    - content_checksum (bool): specifies whether the frame\n"         \
 "         contains a checksum of the uncompressed content\n"           \
 "    - content_size (int): uncompressed size in bytes of\n"            \
 "         frame content\n"                                             \
 "    - block_linked (bool): specifies whether the frame contains\n"    \
 "         blocks which are independently compressed (False) or\n"      \
 "         linked\n"                                                    \
 "    - block_checksum (bool): specifies whether each block\n"          \
 "         contains a checksum of its contents\n"                       \
 "    - skippable (bool): whether the block is skippable (True)\n"      \
 "         or not\n"
 );

PyDoc_STRVAR
(
 create_decompression_context__doc,
 "create_decompression_context()\n\n"                                 \
 "Creates a decompression context object, which will be used for\n"   \
 "decompression operations.\n\n"                                      \
 "Returns:\n"                                                         \
 "    dCtx: A decompression context\n"
 );

PyDoc_STRVAR
(
 decompress__doc,
 "decompress(data)\n\n"                                                 \
 "Decompresses a frame of data and returns it as a string of bytes.\n"  \
 "Args:\n"                                                              \
 "    data (str, bytes or buffer-compatible object): data to decompress.\n" \
 "       This should contain a complete LZ4 frame of compressed data.\n\n" \
 "Keyword Args:\n"                                                      \
 "    return_bytearray (bool): If True a bytearray object will be returned.\n" \
 "        If False, a string of bytes is returned. The default is False.\n" \
 "    return_bytes_read (bool): If ``True`` then the number of bytes read\n" \
 "        from ``data`` will also be returned.\n"                        \
 "\n"                                                                   \
 "Returns:\n"                                                           \
 "    str or bytearray: Uncompressed data\n"                            \
 "    int: (Optional) Number of bytes consumed from source. See\n"      \
 "        ``return_bytes_read`` keyword argument\n"
 );

PyDoc_STRVAR
(
 decompress_chunk__doc,
 "decompress(context, data)\n\n"                                        \
 "Decompresses part of a frame of data. The returned uncompressed data\n" \
 "should be catenated with the data returned from previous calls to\n"  \
 "`decompress_chunk`\n\n"                                               \
 "Args:\n"                                                              \
 "    context (dCtx): decompression context\n"                          \
 "    data (str, bytes or buffer-compatible object): part of a LZ4\n"   \
 "        frame of compressed data\n\n"                                 \
 "Keyword Args:\n"                                                      \
 "    return_bytearray (bool): If True a bytearray object will be returned.\n" \
 "        If False, a string of bytes is returned. The default is False.\n\n" \
 "    return_bytes_read (bool): If ``True`` then the number of bytes read\n" \
 "        from ``data`` will also be returned.\n"                        \
 "\n"                                                                   \
 "Returns:\n"                                                           \
 "    str or bytearray: Uncompressed data\n"                            \
 "    int: (Optional) Number of bytes consumed from source. See\n"      \
 "        ``return_bytes_read`` keyword argument\n"
 );

static PyMethodDef module_methods[] =
{
  {
    "create_compression_context", (PyCFunction) create_compression_context,
    METH_NOARGS, create_compression_context__doc
  },
  {
    "compress", (PyCFunction) compress,
    METH_VARARGS | METH_KEYWORDS, compress__doc
  },
  {
    "compress_begin", (PyCFunction) compress_begin,
    METH_VARARGS | METH_KEYWORDS, compress_begin__doc
  },
  {
    "compress_chunk", (PyCFunction) compress_chunk,
    METH_VARARGS | METH_KEYWORDS, compress_chunk__doc
  },
  {
    "compress_end", (PyCFunction) compress_end,
    METH_VARARGS | METH_KEYWORDS, compress_end__doc
  },
  {
    "get_frame_info", (PyCFunction) get_frame_info,
    METH_VARARGS | METH_KEYWORDS, get_frame_info__doc
  },
  {
    "create_decompression_context", (PyCFunction) create_decompression_context,
    METH_NOARGS, create_decompression_context__doc
  },
  {
    "decompress", (PyCFunction) decompress,
    METH_VARARGS | METH_KEYWORDS, decompress__doc
  },
  {
    "decompress_chunk", (PyCFunction) decompress_chunk,
    METH_VARARGS | METH_KEYWORDS, decompress_chunk__doc
  },
  {NULL, NULL, 0, NULL}		/* Sentinel */
};

PyDoc_STRVAR(lz4frame__doc,
             "A Python wrapper for the LZ4 frame protocol"
             );

static struct PyModuleDef moduledef =
{
  PyModuleDef_HEAD_INIT,
  "_frame",
  lz4frame__doc,
  -1,
  module_methods
};

MODULE_INIT_FUNC (_frame)
{
  PyObject *module = PyModule_Create (&moduledef);

  if (module == NULL)
    return NULL;

  PyModule_AddIntConstant (module, "BLOCKSIZE_DEFAULT", LZ4F_default);
  PyModule_AddIntConstant (module, "BLOCKSIZE_MAX64KB", LZ4F_max64KB);
  PyModule_AddIntConstant (module, "BLOCKSIZE_MAX256KB", LZ4F_max256KB);
  PyModule_AddIntConstant (module, "BLOCKSIZE_MAX1MB", LZ4F_max1MB);
  PyModule_AddIntConstant (module, "BLOCKSIZE_MAX4MB", LZ4F_max4MB);

  PyModule_AddIntConstant (module, "BLOCKMODE_LINKED", LZ4F_blockLinked);
  PyModule_AddIntConstant (module, "BLOCKMODE_INDEPENDENT",
                           LZ4F_blockIndependent);

  PyModule_AddIntConstant (module, "CONTENTCHECKSUM_DISABLED",
                           LZ4F_noContentChecksum);
  PyModule_AddIntConstant (module, "CONTENTCHECKSUM_ENABLED",
                           LZ4F_contentChecksumEnabled);

  PyModule_AddIntConstant (module, "FRAMETYPE_FRAME", LZ4F_frame);
  PyModule_AddIntConstant (module, "FRAMETYPE_SKIPPABLEFRAME",
                           LZ4F_skippableFrame);

  PyModule_AddIntConstant (module, "COMPRESSIONLEVEL_MIN", 0);
  PyModule_AddIntConstant (module, "COMPRESSIONLEVEL_MINHC", 3);
  PyModule_AddIntConstant (module, "COMPRESSIONLEVEL_MAX", 16);

  return module;
}
