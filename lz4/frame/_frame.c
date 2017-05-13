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

static const char * capsule_name = "_frame.LZ4F_cctx";

struct compression_context
{
  LZ4F_compressionContext_t compression_context;
  LZ4F_preferences_t preferences;
};

/*****************************
* create_compression_context *
******************************/
PyDoc_STRVAR(create_compression_context__doc,
             "create_compression_context()\n\n"                         \
             "Creates a Compression Context object, which will be used in all\n" \
             "compression operations.\n\n"                              \
             "Returns:\n"                                               \
             "    cCtx: A compression context\n"
            );

static void
destruct_compression_context (PyObject * py_context)
{
#ifndef PyCapsule_Type
  struct compression_context *context =
    PyCapsule_GetPointer (py_context, capsule_name);
#else
  /* Compatibility with 2.6 via capsulethunk. */
  struct compression_context *context =  py_context;
#endif
  Py_BEGIN_ALLOW_THREADS
    LZ4F_freeCompressionContext (context->compression_context);
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
  memset (context, 0, sizeof (*context));

  result =
    LZ4F_createCompressionContext (&context->compression_context,
                                   LZ4F_VERSION);
  Py_END_ALLOW_THREADS

  if (LZ4F_isError (result))
    {
      LZ4F_freeCompressionContext (context->compression_context);
      PyMem_Free (context);
      PyErr_Format (PyExc_RuntimeError,
                    "LZ4F_createCompressionContext failed with code: %s",
                    LZ4F_getErrorName (result));
      return NULL;
    }

  return PyCapsule_New (context, capsule_name, destruct_compression_context);
}

/************
 * compress *
 ************/
#define __COMPRESS_KWARGS_DOCSTRING \
  "    block_size (int): Sepcifies the maximum blocksize to use.\n"     \
  "        Options:\n\n"                                                \
  "        - lz4.frame.BLOCKSIZE_DEFAULT or 0: the lz4 library default\n" \
  "        - lz4.frame.BLOCKSIZE_MAX64KB or 4: 64 kB\n"                 \
  "        - lz4.frame.BLOCKSIZE_MAX256KB or 5: 256 kB\n"               \
  "        - lz4.frame.BLOCKSIZE_MAX1MB or 6: 1 MB\n"                   \
  "        - lz4.frame.BLOCKSIZE_MAX4MB or 7: 4 MB\n\n"                 \
  "        If unspecified, will default to lz4.frame.BLOCKSIZE_DEFAULT.\n" \
  "    block_mode (int): Specifies whether to use block-linked\n"       \
  "        compression. Options:\n\n"                                   \
  "        - lz4.frame.BLOCKMODE_LINKED or 0: linked mode\n"          \
  "        - lz4.frame.BLOCKMODE_INDEPENDENT or 1: disable linked mode\n\n" \
  "        The default is lz4.frame.BLOCKMODE_LINKED.\n"           \
  "    compression_level (int): Specifies the level of compression used.\n" \
  "        Values between 0-16 are valid, with 0 (default) being the\n"     \
  "        lowest compression (0-2 are the same value), and 16 the highest.\n" \
  "        Values above 16 will be treated as 16.\n"             \
  "        Values between 4-9 are recommended.\n"      \
  "        The following module constants are provided as a convenience:\n\n" \
  "        - lz4.frame.COMPRESSIONLEVEL_MIN: Minimum compression (0, the default)\n" \
  "        - lz4.frame.COMPRESSIONLEVEL_MINHC: Minimum high-compression mode (3)\n" \
  "        - lz4.frame.COMPRESSIONLEVEL_MAX: Maximum compression (16)\n\n" \
  "    content_checksum (int): Specifies whether to enable checksumming of\n" \
  "        the payload content. Options:\n\n"                           \
  "        - lz4.frame.CONTENTCHECKSUM_DISABLED or 0: disables checksumming\n" \
  "        - lz4.frame.CONTENTCHECKSUM_ENABLED or 1: enables checksumming\n\n" \
  "        The default is CONTENTCHECKSUM_DISABLED.\n"                  \
  "    frame_type (int): Specifies whether user data can be injected between\n" \
  "        frames. Options:\n\n"                                        \
  "        - lz4.frame.FRAMETYPE_FRAME or 0: disables user data injection\n" \
  "        - lz4.frame.FRAMETYPE_SKIPPABLEFRAME or 1: enables user data injection\n\n" \
  "        The default is lz4.frame.FRAMETYPE_FRAME.\n"                 \

PyDoc_STRVAR(compress__doc,
             "compress(source, compression_level=0, block_size=0, content_checksum=0, block_mode=0, frame_type=0,  content_size_header=1)\n\n" \
             "Accepts a string, and compresses the string in one go, returning the\n" \
             "compressed string as a string of bytes. The compressed string includes\n" \
             "a header and endmark and so is suitable for writing to a file.\n\n" \
             "Args:\n"                                                  \
             "    source (str): String to compress\n\n"                 \
             "Keyword Args:\n"                                          \
             __COMPRESS_KWARGS_DOCSTRING                                \
             "    content_size_header (bool): Specifies whether to include an optional\n" \
             "        8-byte header field that is the uncompressed size of data included\n" \
             "        within the frame. Including the content-size header is optional\n" \
             "        and is enabled by default.\n\n"                   \
             "Returns:\n"                                               \
             "    str: Compressed data as a string\n"
             );

static PyObject *
compress (PyObject * Py_UNUSED (self), PyObject * args,
          PyObject * keywds)
{
  const char *source;
  int source_size;
  int content_size_header = 1;
  LZ4F_preferences_t preferences;
  size_t compressed_bound;
  Py_ssize_t dest_size;
  PyObject *py_dest;
  char *dest;

  static char *kwlist[] = { "source",
                            "compression_level",
                            "block_size",
                            "content_checksum",
                            "block_mode",
                            "frame_type",
                            "content_size_header",
                            NULL
                          };


  memset (&preferences, 0, sizeof (preferences));

  if (!PyArg_ParseTupleAndKeywords (args, keywds, "s#|iiiiii", kwlist,
                                    &source, &source_size,
                                    &preferences.compressionLevel,
                                    &preferences.frameInfo.blockSizeID,
                                    &preferences.frameInfo.contentChecksumFlag,
                                    &preferences.frameInfo.blockMode,
                                    &preferences.frameInfo.frameType,
                                    &content_size_header))
    {
      return NULL;
    }

  preferences.autoFlush = 0;
  if (content_size_header)
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

  py_dest = PyBytes_FromStringAndSize (NULL, dest_size);
  if (py_dest == NULL)
    {
      return NULL;
    }

  dest = PyBytes_AS_STRING (py_dest);
  if (source_size > 0)
    {
      size_t compressed_size;
      Py_BEGIN_ALLOW_THREADS
      compressed_size =
        LZ4F_compressFrame (dest, dest_size, source, source_size,
                            &preferences);
      Py_END_ALLOW_THREADS

      if (LZ4F_isError (compressed_size))
        {
          Py_DECREF (py_dest);
          PyErr_Format (PyExc_RuntimeError,
                        "LZ4F_compressFrame failed with code: %s",
                        LZ4F_getErrorName (compressed_size));
          return NULL;
        }
      /* The actual compressed size might be less than we allocated
         (we allocated using a worst case guess). If the actual size is
         less than 75% of what we allocated, then it's worth performing an
         expensive resize operation to reclaim some space. */
      if ((Py_ssize_t) compressed_size < (dest_size / 4) * 3)
        {
          _PyBytes_Resize (&py_dest, (Py_ssize_t) compressed_size);
        }
      else
        {
          Py_SIZE (py_dest) = (Py_ssize_t) compressed_size;
        }
    }

  return py_dest;
}

/******************
 * compress_begin *
 ******************/
PyDoc_STRVAR(compress_begin__doc,
             "compress_begin(cCtx, source_size=0, compression_level=0, block_size=0,\n" \
             "    content_checksum=0, content_size=1, block_mode=0, frame_type=0, auto_flush=1)\n\n"\
             "Creates a frame header from a compression context.\n\n"   \
             "Args:\n"                                                  \
             "    context (cCtx): A compression context.\n\n"           \
             "Keyword Args:\n"                                          \
             __COMPRESS_KWARGS_DOCSTRING                                \
             "    auto_flush (int): Enable (1, default) or disable (0) autoFlush.\n" \
             "         When autoFlush is disabled, the LZ4 library may buffer data\n" \
             "         until a block is full\n\n"                       \
             "    source_size (int): This optionally specifies the uncompressed size\n" \
             "        of the source content. This arument is optional, but if specified\n" \
             "        will be stored in the frame header for use during decompression.\n"
             "Returns:\n"                                               \
             "    str (str): Frame header.\n"
             );

#undef __COMPRESS_KWARGS_DOCSTRING

static PyObject *
compress_begin (PyObject * Py_UNUSED (self), PyObject * args,
                PyObject * keywds)
{
  PyObject *py_context = NULL;
  unsigned long source_size = 0;
  LZ4F_preferences_t preferences;
  /* Only needs to be large enough for a header, which is 15 bytes.
   * Unfortunately, the lz4 library doesn't provide a #define for this.
   * We over-allocate to allow for larger headers in the future. */
  char destination_buffer[64];
  struct compression_context *context;
  size_t result;
  static char *kwlist[] = { "context",
                            "source_size",
                            "compression_level",
                            "block_size",
                            "content_checksum",
                            "block_mode",
                            "frame_type",
                            "auto_flush",
                            NULL
                          };

  memset (&preferences, 0, sizeof (preferences));

  /* Default to having autoFlush enabled unless specified otherwise via keyword
     argument */
  preferences.autoFlush = 1;

  if (!PyArg_ParseTupleAndKeywords (args, keywds, "O|kiiiiii", kwlist,
                                    &py_context,
                                    &source_size,
                                    &preferences.compressionLevel,
                                    &preferences.frameInfo.blockSizeID,
                                    &preferences.frameInfo.contentChecksumFlag,
                                    &preferences.frameInfo.blockMode,
                                    &preferences.frameInfo.frameType,
                                    &preferences.autoFlush
                                    ))
    {
      return NULL;
    }

  preferences.frameInfo.contentSize = source_size;

  context =
    (struct compression_context *) PyCapsule_GetPointer (py_context, capsule_name);

  if (!context || !context->compression_context)
    {
      PyErr_SetString (PyExc_ValueError, "No compression context supplied");
      return NULL;
    }

  context->preferences = preferences;

  Py_BEGIN_ALLOW_THREADS
  result = LZ4F_compressBegin (context->compression_context,
                               destination_buffer,
                               sizeof (destination_buffer),
                               &context->preferences);
  Py_END_ALLOW_THREADS

  if (LZ4F_isError (result))
    {
      PyErr_Format (PyExc_RuntimeError,
                    "LZ4F_compressBegin failed with code: %s",
                    LZ4F_getErrorName (result));
      return NULL;
    }

  return PyBytes_FromStringAndSize (destination_buffer, result);
}

/*******************
 * compress_update *
 *******************/
PyDoc_STRVAR(compress_update__doc,
             "compress_update(context, source)\n\n" \
             "Compresses blocks of data and returns the compressed data in a string of bytes.\n" \
             "Args:\n"                                                  \
             "    context (cCtx): compression context\n"                \
             "    source (str): data to compress\n\n"                   \
             "Returns:\n"                                               \
             "    str: Compressed data as a string\n\n"                 \
             "Notes:\n"                                               \
             "    If autoFlush is disabled (auto_flush=0 when calling compress_begin)\n" \
             "    this function may return an empty string if LZ4 decides to buffer.\n" \
             "    the input.\n"
             );

static PyObject *
compress_update (PyObject * Py_UNUSED (self), PyObject * args,
                 PyObject * keywds)
{
  PyObject *py_context = NULL;
  const char *source = NULL;
  unsigned long source_size = 0;
  struct compression_context *context;
  size_t compressed_bound;
  char *destination_buffer;
  LZ4F_compressOptions_t compress_options;
  size_t result;
  PyObject *bytes;
  static char *kwlist[] = { "context", "source", NULL };

  if (!PyArg_ParseTupleAndKeywords (args, keywds, "Os#", kwlist,
                                    &py_context, &source, &source_size))
    {
      return NULL;
    }

  context =
    (struct compression_context *) PyCapsule_GetPointer (py_context, capsule_name);
  if (!context || !context->compression_context)
    {
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
      PyErr_Format (PyExc_ValueError,
                    "input data could require %zu bytes, which is larger than the maximum supported size of %zd bytes",
                    compressed_bound, PY_SSIZE_T_MAX);
      return NULL;
    }

  destination_buffer = (char *) PyMem_Malloc (compressed_bound);
  if (!destination_buffer)
    {
      return PyErr_NoMemory ();
    }

  compress_options.stableSrc = 0;

  Py_BEGIN_ALLOW_THREADS
  result =
    LZ4F_compressUpdate (context->compression_context, destination_buffer,
                         compressed_bound, source, source_size,
                         &compress_options);
  Py_END_ALLOW_THREADS

  if (LZ4F_isError (result))
    {
      PyMem_Free (destination_buffer);
      PyErr_Format (PyExc_RuntimeError,
                    "LZ4F_compressUpdate failed with code: %s",
                    LZ4F_getErrorName (result));
      return NULL;
    }
  bytes = PyBytes_FromStringAndSize (destination_buffer, result);
  PyMem_Free (destination_buffer);

  return bytes;
}

/****************
 * compress_end *
 ****************/
PyDoc_STRVAR(compress_end__doc,
             "compress_end(context)\n\n" \
             "Flushes a compression context returning an endmark and optional checksum\n" \
             "as a string of bytes.\n" \
             "Args:\n"                                                  \
             "    context (cCtx): compression context\n"                \
             "Returns:\n"                                               \
             "    str: Remaining (buffered) compressed data, end mark and optional checksum as a string\n"
             );

static PyObject *
compress_end (PyObject * Py_UNUSED (self), PyObject * args, PyObject * keywds)
{
  PyObject *py_context = NULL;
  LZ4F_compressOptions_t compress_options;
  struct compression_context *context;
  size_t destination_size;
  char * destination_buffer;
  size_t result;
  PyObject *bytes;
  static char *kwlist[] = { "context", NULL };

  if (!PyArg_ParseTupleAndKeywords (args, keywds, "O", kwlist, &py_context))
    {
      return NULL;
    }

  context =
    (struct compression_context *) PyCapsule_GetPointer (py_context, capsule_name);
  if (!context || !context->compression_context)
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

  destination_buffer = (char *) PyMem_Malloc(destination_size * sizeof(char));
  if (destination_buffer == NULL)
    {
      return PyErr_NoMemory ();
    }

  Py_BEGIN_ALLOW_THREADS
  result =
    LZ4F_compressEnd (context->compression_context, destination_buffer,
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

  bytes = PyBytes_FromStringAndSize (destination_buffer, result);
  if (bytes == NULL)
    {
      PyErr_SetString (PyExc_RuntimeError,
                       "Failed to create Python object from destination_buffer");
    }
  PyMem_Free (destination_buffer);

  return bytes;
}

/******************
 * get_frame_info *
 ******************/
PyDoc_STRVAR(get_frame_info__doc,
             "get_frame_info(frame)\n\n"                                \
             "Given a frame of compressed data, returns information about the frame.\n" \
             "Args:\n"                                                  \
             "    frame (str): LZ4 frame as a string\n"                \
             "Returns:\n"                                               \
             "    dict: Dictionary with keys blockSizeID, blockMode, contentChecksumFlag\n" \
             "         frameType and contentSize.\n"
             );

static PyObject *
get_frame_info (PyObject * Py_UNUSED (self), PyObject * args,
                PyObject * keywds)
{
  const char *source;
  int source_size;
  size_t source_size_copy;
  LZ4F_decompressionContext_t context;
  LZ4F_frameInfo_t frame_info;
  size_t result;
  static char *kwlist[] = { "source", NULL };

  if (!PyArg_ParseTupleAndKeywords (args, keywds, "s#", kwlist,
                                    &source, &source_size))
    {
      return NULL;
    }

  Py_BEGIN_ALLOW_THREADS

  result = LZ4F_createDecompressionContext (&context, LZ4F_VERSION);

  if (LZ4F_isError (result))
    {
      Py_BLOCK_THREADS
      PyErr_Format (PyExc_RuntimeError,
                    "LZ4F_createDecompressionContext failed with code: %s",
                    LZ4F_getErrorName (result));
      return NULL;
    }

  source_size_copy = source_size;

  result =
    LZ4F_getFrameInfo (context, &frame_info, source, &source_size_copy);

  if (LZ4F_isError (result))
    {
      LZ4F_freeDecompressionContext (context);
      Py_BLOCK_THREADS
      PyErr_Format (PyExc_RuntimeError,
                    "LZ4F_getFrameInfo failed with code: %s",
                    LZ4F_getErrorName (result));
      return NULL;
    }

  result = LZ4F_freeDecompressionContext (context);

  Py_END_ALLOW_THREADS

  if (LZ4F_isError (result))
    {
      PyErr_Format (PyExc_RuntimeError,
                    "LZ4F_freeDecompressionContext failed with code: %s",
                    LZ4F_getErrorName (result));
      return NULL;
    }

  return Py_BuildValue ("{s:i,s:i,s:i,s:i,s:i}",
                        "blockSizeID", frame_info.blockSizeID,
                        "blockMode", frame_info.blockMode,
                        "contentChecksumFlag", frame_info.contentChecksumFlag,
                        "frameType", frame_info.frameType,
                        "contentSize", frame_info.contentSize);
}

/**************
 * decompress *
 **************/
PyDoc_STRVAR(decompress__doc,
             "decompress(source)\n\n"                                   \
             "Decompressed a frame of data and returns it as a string of bytes.\n" \
             "Args:\n"                                                  \
             "    source (str): LZ4 frame as a string\n\n"              \
             "Returns:\n"                                               \
             "    str: Uncompressed data as a string.\n"
             );

static PyObject *
decompress (PyObject * Py_UNUSED (self), PyObject * args, PyObject * keywds)
{
  char const *source;
  int source_size;
  LZ4F_decompressionContext_t context;
  LZ4F_frameInfo_t frame_info;
  LZ4F_decompressOptions_t options;
  size_t result;
  size_t source_read;
  size_t destination_size;
  char * destination_buffer;
  size_t destination_write;
  char * destination_cursor;
  size_t destination_written;
  const char * source_cursor;
  const char * source_end;
  PyObject *py_dest;
  static char *kwlist[] = { "source", NULL };

  if (!PyArg_ParseTupleAndKeywords (args, keywds, "s#", kwlist,
                                    &source, &source_size))
    {
      return NULL;
    }

  Py_BEGIN_ALLOW_THREADS

  result = LZ4F_createDecompressionContext (&context, LZ4F_VERSION);
  if (LZ4F_isError (result))
    {
      Py_BLOCK_THREADS
      PyErr_Format (PyExc_RuntimeError,
                    "LZ4F_createDecompressionContext failed with code: %s",
                    LZ4F_getErrorName (result));
      return NULL;
    }

  source_read = source_size;

  result =
    LZ4F_getFrameInfo (context, &frame_info, source, &source_read);

  if (LZ4F_isError (result))
    {
      LZ4F_freeDecompressionContext (context);
      Py_BLOCK_THREADS
      PyErr_Format (PyExc_RuntimeError,
                    "LZ4F_getFrameInfo failed with code: %s",
                    LZ4F_getErrorName (result));
      return NULL;
    }

  /* Advance the source pointer past the header - the call to getFrameInfo above
     replaces the passed source_read value with the number of bytes
     read. Also reduce source_size accordingly. */
  source += source_read;
  source_size -= source_read;

  if (frame_info.contentSize == 0)
    {
      /* We'll allocate twice the source buffer size as the output size, and
         later increase it if needed. */
      destination_size = 2 * source_size;
    }
  else
    {
      destination_size = frame_info.contentSize;
    }

  Py_END_ALLOW_THREADS

  destination_buffer = (char *) PyMem_Malloc (destination_size);
  if (!destination_buffer)
    {
      LZ4F_freeDecompressionContext (context);
      return PyErr_NoMemory ();
    }

  Py_BEGIN_ALLOW_THREADS

  options.stableDst = 1;

  source_read = source_size;
  source_cursor = source;
  source_end = source + source_size;

  destination_write = destination_size;
  destination_cursor = destination_buffer;
  destination_written = 0;

  while (source_cursor < source_end)
    {
      /* Decompress from the source string and write to the destination_buffer
         until there's no more source string to read.

         On calling LZ4F_decompress, source_read is set to the remaining length
         of source available to read. On return, source_read is set to the
         actual number of bytes read from source, which may be less than
         available. NB: LZ4F_decompress does not explicitly fail on empty input.

         On calling LZ4F_decompres, destination_write is the number of bytes in
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
          LZ4F_freeDecompressionContext (context);
          Py_BLOCK_THREADS
          PyErr_Format (PyExc_RuntimeError,
                        "LZ4F_decompress failed with code: %s",
                        LZ4F_getErrorName (result));
          PyMem_Free (destination_buffer);
          return NULL;
        }

      destination_written += destination_write;
      source_cursor += source_read;

      if (result == 0)
        {
          break;
        }

      if (destination_written == destination_size)
        {
          /* Destination_buffer is full, so need to expand it. result is an
             indication of number of source bytes remaining, so we'll use this
             to estimate the new size of the destination buffer. */
          char * destination_buffer_new;
          destination_size += 3 * result;
          Py_BLOCK_THREADS
          destination_buffer_new = PyMem_Realloc(destination_buffer, destination_size);
          if (!destination_buffer_new)
            {
              LZ4F_freeDecompressionContext (context);
              PyErr_SetString (PyExc_RuntimeError,
                               "Failed to increase destination buffer size");
              PyMem_Free (destination_buffer);
              return NULL;
            }
          Py_UNBLOCK_THREADS
          destination_buffer = destination_buffer_new;
        }
      /* Data still remaining to be decompressed, so increment the source and
         destination cursor locations, and reset source_read and
         destination_write ready for the next iteration. Important to
         re-initialize destination_cursor here (as opposed to simply
         incrementing it) so we're pointing to the realloc'd memory location. */
      destination_cursor = destination_buffer + destination_written;
      destination_write = destination_size - destination_written;
      source_read = source_end - source_cursor;
    }

  result = LZ4F_freeDecompressionContext (context);

  Py_END_ALLOW_THREADS

  if (LZ4F_isError (result))
    {
      PyMem_Free (destination_buffer);
      PyErr_Format (PyExc_RuntimeError,
                    "LZ4F_freeDecompressionContext failed with code: %s",
                    LZ4F_getErrorName (result));
      return NULL;
    }
  else if (result != 0)
    {
      PyMem_Free (destination_buffer);
      PyErr_Format (PyExc_RuntimeError,
                    "LZ4F_freeDecompressionContext reported unclean decompressor state (truncated frame?): %zu", result);
      return NULL;
    }
  else if (source_cursor != source_end)
    {
      PyMem_Free (destination_buffer);
      PyErr_Format (PyExc_ValueError,
                    "Extra data: %zd trailing bytes", source_end - source_cursor);
      return NULL;
    }

  py_dest = PyBytes_FromStringAndSize (destination_buffer, destination_written);

  if (py_dest == NULL)
    {
      PyErr_SetString (PyExc_RuntimeError,
                       "Failed to create Python object from destination_buffer");
    }

  PyMem_Free (destination_buffer);
  return py_dest;
}

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
    "compress_update", (PyCFunction) compress_update,
    METH_VARARGS | METH_KEYWORDS, compress_update__doc
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
    "decompress", (PyCFunction) decompress,
    METH_VARARGS | METH_KEYWORDS, decompress__doc
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
