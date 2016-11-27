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


#if defined(_WIN32) && defined(_MSC_VER)
#define inline __inline
#if _MSC_VER >= 1600
#include <stdint.h>
#else /* _MSC_VER >= 1600 */
typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
#endif /* _MSC_VER >= 1600 */
#endif

#if defined(__SUNPRO_C) || defined(__hpux) || defined(_AIX)
#define inline
#endif

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

static PyObject *
create_compression_context (PyObject * Py_UNUSED (self),
                            PyObject * Py_UNUSED (args),
                            PyObject * Py_UNUSED (keywds))
{
  struct compression_context *context =
    (struct compression_context *)
    PyMem_Malloc (sizeof (struct compression_context));

  if (!context)
    {
      return PyErr_NoMemory ();
    }

  memset (context, 0, sizeof (*context));

  LZ4F_errorCode_t result =
    LZ4F_createCompressionContext (&context->compression_context,
                                   LZ4F_VERSION);

  if (LZ4F_isError (result))
    {
      PyErr_Format (PyExc_RuntimeError,
                    "LZ4F_createCompressionContext failed with code: %s",
                    LZ4F_getErrorName (result));
      return NULL;
    }

  return PyCapsule_New (context, NULL, NULL);
}

/****************************
 * free_compression_context *
 ****************************/
PyDoc_STRVAR(free_compression_context__doc,
             "free_compression_context(context)\n\n"                                \
             "Releases the resources held by a compression context previously\n" \
             "created with create_compression_context.\n\n"             \
             "Args:\n"                                                  \
             "    context (cCtx): Compression context.\n"
             );

static PyObject *
free_compression_context (PyObject * Py_UNUSED (self), PyObject * args,
                          PyObject * keywds)
{
  PyObject *py_context = NULL;
  static char *kwlist[] = { "context", NULL };

  if (!PyArg_ParseTupleAndKeywords (args, keywds, "O", kwlist, &py_context))
    {
      return NULL;
    }

  struct compression_context *context =
    (struct compression_context *) PyCapsule_GetPointer (py_context, NULL);
  if (!context)
    {
      PyErr_Format (PyExc_ValueError, "No compression context supplied");
      return NULL;
    }

  LZ4F_errorCode_t result =
    LZ4F_freeCompressionContext (context->compression_context);
  if (LZ4F_isError (result))
    {
      PyErr_Format (PyExc_RuntimeError,
                    "LZ4F_freeCompressionContext failed with code: %s",
                    LZ4F_getErrorName (result));
      return NULL;
    }
  PyMem_Free (context);

  Py_RETURN_NONE;
}

/******************
 * compress_frame *
 ******************/
#define COMPRESS_KWARGS_DOCSTRING \
  "    block_size (int): Sepcifies the maximum blocksize to use.\n"     \
  "        Options:\n\n"                                                \
  "        - BLOCKSIZE_DEFAULT or 0: the lz4 library default\n"         \
  "        - BLOCKSIZE_MAX64KB or 4: 64 kB\n"                           \
  "        - BLOCKSIZE_MAX256KB or 5: 256 kB\n"                         \
  "        - BLOCKSIZE_MAX1MB or 6: 1 MB\n"                             \
  "        - BLOCKSIZE_MAX1MB or 7: 4 MB\n\n"                           \
  "        If unspecified, will default to BLOCKSIZE_DEFAULT.\n"        \
  "    block_mode (int): Specifies whether to use block-linked\n"       \
  "        compression. Options:\n\n"                                  \
  "        - BLOCKMODE_INDEPENDENT or 0: disable linked mode\n"         \
  "        - BLOCKMODE_LINKED or 1: linked mode\n\n"                    \
  "        The default is BLOCKMODE_INDEPENDENT.\n"                     \
  "    compression_level (int): Specifies the level of compression used.\n" \
  "        Values between 0-16 are valid, with 0 (default) being the\n" \
  "        lowest compression, and 16 the highest. Values above 16 will\n" \
  "        be treated as 16. Values betwee 3-6 are recommended.\n"      \
  "        The following module constants are provided as a convenience:\n\n" \
  "        - COMPRESSIONLEVEL_MIN: Minimum compression (0, the default)\n" \
  "        - COMPRESSIONLEVEL_MINHC: Minimum high-compression mode (3)\n" \
  "        - COMPRESSIONLEVEL_MAX: Maximum compression (16)\n\n"        \
  "    content_checksum (int): Specifies whether to enable checksumming of\n" \
  "        the payload content. Options:\n\n"                             \
  "        - CONTENTCHECKSUM_DISABLED or 0: disables checksumming\n"    \
  "        - CONTENTCHECKSUM_ENABLED or 1: enables checksumming\n\n"      \
  "        The default is CONTENTCHECKSUM_DISABLED.\n"                  \
  "    frame_type (int): Specifies whether user data can be injected between\n" \
  "        frames. Options:\n\n"                                          \
  "        - FRAMETYPE_FRAME or 0: disables user data injection\n"      \
  "        - FRAMETYPE_SKIPPABLEFRAME or 1: enables user data injection\n\n" \
  "        The default is FRAMETYPE_FRAME.\n"                           \
  "    source_size (int): This optionally specifies  the uncompressed size\n" \
  "        of the full frame content. This arument is optional, but can be\n" \

PyDoc_STRVAR(compress_frame__doc,
             "compress_frame(source)\n\n"                               \
             "Accepts a string, and compresses the string in one go, returning the\n" \
             "compressed string as a string of bytes. The compressed string includes\n" \
             "a header and endmark and so is suitable for writing to a file.\n\n" \
             "Args:\n"                                                  \
             "    source (str): String to compress\n\n"                 \
             "Keyword Args:\n"                                          \
             COMPRESS_KWARGS_DOCSTRING                                  \
             "auto_flush:\n"                                            \
             "     \n"                                                  \
             "Returns:\n"                                               \
             "    str: Compressed data as a string\n"
             );

static PyObject *
compress_frame (PyObject * Py_UNUSED (self), PyObject * args,
                PyObject * keywds)
{
  const char *source;
  int source_size;
  LZ4F_preferences_t preferences;

  memset (&preferences, 0, sizeof (preferences));

  static char *kwlist[] = { "source",
                            "compression_level",
                            "block_size",
                            "content_checksum",
                            "block_mode",
                            "frame_type",
                            NULL
                          };

  if (!PyArg_ParseTupleAndKeywords (args, keywds, "s#|iiiii", kwlist,
                                    &source, &source_size,
                                    &preferences.compressionLevel,
                                    &preferences.frameInfo.blockSizeID,
                                    &preferences.
                                    frameInfo.contentChecksumFlag,
                                    &preferences.frameInfo.blockMode,
                                    &preferences.frameInfo.frameType))
    {
      return NULL;
    }
  preferences.autoFlush = 0;
  preferences.frameInfo.contentSize = source_size;

  size_t compressed_bound =
    LZ4F_compressFrameBound (source_size, &preferences);

  if (compressed_bound > PY_SSIZE_T_MAX)
    {
      PyErr_Format (PyExc_ValueError,
                    "input data could require %zu bytes, which is larger than the maximum supported size of %zd bytes",
                    compressed_bound, PY_SSIZE_T_MAX);
      return NULL;
    }

  Py_ssize_t dest_size = (Py_ssize_t) compressed_bound;

  PyObject *py_dest = PyBytes_FromStringAndSize (NULL, dest_size);
  if (py_dest == NULL)
    {
      return NULL;
    }

  char *dest = PyBytes_AS_STRING (py_dest);
  if (source_size > 0)
    {
      size_t compressed_size =
        LZ4F_compressFrame (dest, dest_size, source, source_size,
                            &preferences);
      if (LZ4F_isError (compressed_size))
        {
          Py_DECREF (py_dest);
          PyErr_Format (PyExc_RuntimeError,
                        "LZ4F_compressFrame failed with code failed with code: %s",
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
             "compressBegin(cCtx)\n\n"                                  \
             "Creates a frame header from a compression context.\n\n"   \
             "Args:\n"                                                  \
             "    context (cCtx): A compression context.\n\n"           \
             "Keyword Args:\n"                                          \
             COMPRESS_KWARGS_DOCSTRING \
             "Returns:\n"                                               \
             "    str (str): Frame header.\n"
             );

static PyObject *
compress_begin (PyObject * Py_UNUSED (self), PyObject * args,
                PyObject * keywds)
{
  PyObject *py_context = NULL;
  unsigned long source_size = 0;
  LZ4F_preferences_t preferences;
  static char *kwlist[] = { "context",
                            "source_size",
                            "compression_level",
                            "block_size",
                            "content_checksum",
                            "block_mode",
                            "frame_type",
                            NULL
                          };

  memset (&preferences, 0, sizeof (preferences));

  if (!PyArg_ParseTupleAndKeywords (args, keywds, "O|kiiiii", kwlist,
                                    &py_context,
                                    &source_size,
                                    &preferences.compressionLevel,
                                    &preferences.frameInfo.blockSizeID,
                                    &preferences.frameInfo.contentChecksumFlag,
                                    &preferences.frameInfo.blockMode,
                                    &preferences.frameInfo.frameType))
    {
      return NULL;
    }
  preferences.autoFlush = 1;
  preferences.frameInfo.contentSize = source_size;

  struct compression_context *context =
    (struct compression_context *) PyCapsule_GetPointer (py_context, NULL);

  if (!context || !context->compression_context)
    {
      PyErr_Format (PyExc_ValueError, "No compression context supplied");
      return NULL;
    }

  context->preferences = preferences;

  /* Only needs to be large enough for a header, which is 15 bytes.
   * Unfortunately, the lz4 library doesn't provide a #define for this.
   * We over-allocate to allow for larger headers in the future. */
  char destination_buffer[64];

  size_t result = LZ4F_compressBegin (context->compression_context,
                                      destination_buffer,
                                      sizeof (destination_buffer),
                                      &context->preferences);
  if (LZ4F_isError (result))
    {
      PyErr_Format (PyExc_RuntimeError,
                    "LZ4F_compressBegin failed with code: %s",
                    LZ4F_getErrorName (result));
      return NULL;
    }

  return PyBytes_FromStringAndSize (destination_buffer, result);
}

static PyObject *
compress_update (PyObject * Py_UNUSED (self), PyObject * args,
                 PyObject * keywds)
{
  PyObject *py_context = NULL;
  const char *source = NULL;
  unsigned long source_size = 0;

  static char *kwlist[] = { "context", "source", NULL };

  if (!PyArg_ParseTupleAndKeywords (args, keywds, "Os#", kwlist,
                                    &py_context, &source, &source_size))
    {
      return NULL;
    }

  struct compression_context *context =
    (struct compression_context *) PyCapsule_GetPointer (py_context, NULL);
  if (!context || !context->compression_context)
    {
      PyErr_Format (PyExc_ValueError, "No compression context supplied");
      return NULL;
    }

  size_t compressed_bound =
    LZ4F_compressFrameBound (source_size, &context->preferences);
  if (compressed_bound > PY_SSIZE_T_MAX)
    {
      PyErr_Format (PyExc_ValueError,
                    "input data could require %zu bytes, which is larger than the maximum supported size of %zd bytes",
                    compressed_bound, PY_SSIZE_T_MAX);
      return NULL;
    }

  char *destination_buffer = (char *) PyMem_Malloc (compressed_bound);
  if (!destination_buffer)
    {
      return PyErr_NoMemory ();
    }

  LZ4F_compressOptions_t compress_options;
  compress_options.stableSrc = 0;

  size_t result =
    LZ4F_compressUpdate (context->compression_context, destination_buffer,
                         compressed_bound, source, source_size,
                         &compress_options);
  if (LZ4F_isError (result))
    {
      PyMem_Free (destination_buffer);
      PyErr_Format (PyExc_RuntimeError,
                    "LZ4F_compressBegin failed with code: %s",
                    LZ4F_getErrorName (result));
      return NULL;
    }
  PyObject *bytes = PyBytes_FromStringAndSize (destination_buffer, result);
  PyMem_Free (destination_buffer);

  return bytes;
}

static PyObject *
compress_end (PyObject * Py_UNUSED (self), PyObject * args, PyObject * keywds)
{
  PyObject *py_context = NULL;

  static char *kwlist[] = { "context", NULL };

  if (!PyArg_ParseTupleAndKeywords (args, keywds, "O", kwlist, &py_context))
    {
      return NULL;
    }

  struct compression_context *context =
    (struct compression_context *) PyCapsule_GetPointer (py_context, NULL);
  if (!context || !context->compression_context)
    {
      PyErr_Format (PyExc_ValueError, "No compression context supplied");
      return NULL;
    }

  LZ4F_compressOptions_t compress_options;
  compress_options.stableSrc = 0;

  /* Because we compress with auto-flush enabled, this only needs to be large
   * enough for a footer, which is 4-8 bytes. Unfortunately, the lz4 library
   * doesn't provide a #define for this. We over-allocate to allow for larger
   * footers in the future. */
  char destination_buffer[64];

  size_t result =
    LZ4F_compressEnd (context->compression_context, destination_buffer,
                      sizeof (destination_buffer), &compress_options);
  if (LZ4F_isError (result))
    {
      PyErr_Format (PyExc_RuntimeError,
                    "LZ4F_compressBegin failed with code: %s",
                    LZ4F_getErrorName (result));
      return NULL;
    }

  return PyBytes_FromStringAndSize (destination_buffer, result);
}

static PyObject *
get_frame_info (PyObject * Py_UNUSED (self), PyObject * args,
                PyObject * keywds)
{
  const char *source;
  int source_size;

  static char *kwlist[] = { "source", NULL };

  if (!PyArg_ParseTupleAndKeywords (args, keywds, "s#", kwlist,
                                    &source, &source_size))
    {
      return NULL;
    }

  LZ4F_decompressionContext_t context;
  size_t result = LZ4F_createDecompressionContext (&context, LZ4F_VERSION);
  if (LZ4F_isError (result))
    {
      PyErr_Format (PyExc_RuntimeError,
                    "LZ4F_createDecompressionContext failed with code: %s",
                    LZ4F_getErrorName (result));
      return NULL;
    }

  LZ4F_frameInfo_t frame_info;
  size_t source_size_copy = source_size;
  result =
    LZ4F_getFrameInfo (context, &frame_info, source, &source_size_copy);
  if (LZ4F_isError (result))
    {
      LZ4F_freeDecompressionContext (context);
      PyErr_Format (PyExc_RuntimeError,
                    "LZ4F_getFrameInfo failed with code: %s",
                    LZ4F_getErrorName (result));
      return NULL;
    }

  result = LZ4F_freeDecompressionContext (context);
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

static PyObject *
decompress (PyObject * Py_UNUSED (self), PyObject * args, PyObject * keywds)
{
  char const *source;
  int source_size;
  int uncompressed_size = 0;
  static char *kwlist[] = { "source", "uncompressed_size", NULL };

  if (!PyArg_ParseTupleAndKeywords (args, keywds, "s#|i", kwlist,
                                    &source, &source_size,
                                    &uncompressed_size))
    {
      return NULL;
    }

  LZ4F_decompressionContext_t context;
  size_t result = LZ4F_createDecompressionContext (&context, LZ4F_VERSION);
  if (LZ4F_isError (result))
    {
      PyErr_Format (PyExc_RuntimeError,
                    "LZ4F_createDecompressionContext failed with code: %s",
                    LZ4F_getErrorName (result));
      return NULL;
    }

  LZ4F_frameInfo_t frame_info;
  size_t source_size_copy = source_size;
  result =
    LZ4F_getFrameInfo (context, &frame_info, source, &source_size_copy);
  if (LZ4F_isError (result))
    {
      LZ4F_freeDecompressionContext (context);
      PyErr_Format (PyExc_RuntimeError,
                    "LZ4F_getFrameInfo failed with code: %s",
                    LZ4F_getErrorName (result));
      return NULL;
    }
  /* advance the source pointer past the header */
  source += source_size_copy;

  if (frame_info.contentSize > PY_SSIZE_T_MAX)
    {
      LZ4F_freeDecompressionContext (context);
      PyErr_Format (PyExc_ValueError,
                    "decompressed data would require %zu bytes, which is larger than the maximum supported size of %zd bytes",
                    (size_t) frame_info.contentSize, PY_SSIZE_T_MAX);
      return NULL;
    }

  if (frame_info.contentSize == 0)
    {
      frame_info.contentSize = uncompressed_size;
    }
  if (frame_info.contentSize == 0)
    {
      LZ4F_freeDecompressionContext (context);
      PyErr_Format (PyExc_ValueError,
                    "Decompressed content size was not encoded in this compressed data; therefore, you must specify it manually when calling decompress.");
      return NULL;
    }
  size_t destination_size = (size_t) frame_info.contentSize;

  char *destination_buffer = (char *) PyMem_Malloc (destination_size);
  if (!destination_buffer)
    {
      LZ4F_freeDecompressionContext (context);
      return PyErr_NoMemory ();
    }

  LZ4F_decompressOptions_t options;
  options.stableDst = 1;

  size_t destination_size_copy = destination_size;
  source_size_copy = source_size;

  result = LZ4F_decompress (context,
                            destination_buffer,
                            &destination_size_copy,
                            source, &source_size_copy, &options);
  if (result != 0)
    {
      LZ4F_freeDecompressionContext (context);
      PyMem_Free (destination_buffer);
      if (LZ4F_isError (result))
        {
          PyErr_Format (PyExc_RuntimeError,
                        "LZ4F_decompress failed with code: %s",
                        LZ4F_getErrorName (result));
          return NULL;
        }
      else
        {
          PyErr_Format (PyExc_RuntimeError,
                        "Something unexpected happened and decompress wants to be called again with %zu more bytes",
                        result);
          return NULL;
        }
    }

  result = LZ4F_freeDecompressionContext (context);
  if (LZ4F_isError (result))
    {
      PyMem_Free (destination_buffer);
      PyErr_Format (PyExc_RuntimeError,
                    "LZ4F_freeDecompressionContext failed with code: %s",
                    LZ4F_getErrorName (result));
      return NULL;
    }

  PyObject *py_dest =
    PyBytes_FromStringAndSize (destination_buffer, destination_size_copy);
  PyMem_Free (destination_buffer);
  return py_dest;
}


#define CDCTX_DOCSTRING \
  "createDecompContext()\n\n"				\
  "Generates a decompression context.\n\n"		\
  "Returns:\n"						\
  "    dCtx: A compression context\n"


#define MKPFS_DOCSTRING \
  "makePrefs(blockSizeID=7, blockMode=1, chkFlag=0, autoFlush=0)\n\n"	\
  "Create an object defining compression settings.\n\n"			\
  "Args:\n"								\
  "    blockSizeID (int): Sepcifies the blocksize to use. Options are:\n" \
  "        0 (the lz4 library specified default), 4 (64 KB), 5 (256 kB),\n" \
  "        6 (1 MB), 7 (4 MB). If unspecified, will default to 0.\n"	\
  "    blockMode (int): Specifies whether to use block-linked\n"	\
  "        compression (blockMode=1) or independent block compression\n" \
  "        (blockMode=0). The default is 0.\n"				\
  "    chkFlag (int): Specifies whether to enable checksumming of the\n" \
  "        payload content. The value 0 disables the checksum, and 1\n" \
  "        enables it. The default is 0.\n" \
  "    autoFlush (int): Specify whether to enable always flush the buffer\n" \
  "        To always flush, specify 1. To disable auto-flushing specify 0.\n" \
  "        The default is 0.\n\n"					\
  "Returns:\n"								\
  "    PyCapsule: preferences object.\n"


#define COMPU_DOCSTRING  \
  "compressUpdate(source, cCtx)\n\n" \
  "Accepts a string, and a compression context and returns the string as\n" \
  "a compressed block if the block is filled. If not, it will return a\n" \
  "blank string and hold the compressed data until the block is filled,\n" \
  "flush is called or compressEnd is called.\n\n"			\
  "Args:\n"								\
  "    source (str): Source string to compress.\n"			\
  "    cCtx (cCtx): Compression context.\n\n"				\
  "Returns:\n"								\
  "    str: compressed block or empty string.\n"

#define COMPE_DOCSTRING  \
  "compressEnd(cCtx)\n\n" \
  "Ends a compression session by flushing the holding buffer, applying\n" \
  "an end mark, and if applicable, creating a checksum.\n\n"		\
  "Args:\n"								\
  "    cCtx (cCtx): a compression context.\n\n"				\
  "Returns:\n"								\
  "    str: String containing the remaining compressed data, end mark\n" \
  "        and optional checksum.\n"


#define FDCTX_DOCSTRING  \
  "freeDecompContext(dCtx)\n\n"						\
  "Releases the resources held by a decompression context.\n\n"		\
  "Args:\n"								\
  "    dCtx (dCtx): Decompression context.\n"

#define GETFI_DOCSTRING \
  "getFrameInfo(header, dctx)\n\n"					\
  "Take a 7 byte string corresponding to the a compresed frame header\n" \
  "together with a decompression context, and return a dictionary\n" \
  "object containing the frame information corresponding to the header\n\n" \
  "Args:\n"								\
  "    header (str): LZ4 header\n"					\
  "    dCtx (dCtx): Decompression context\n\n"				\
  "Returns:\n"								\
  "    dict: a dictionary object with keys `chkFlag`, `blkSize` and\n" \
  "        `blkMode`.\n"

#define DCHKS_DOCSTRING \
  "disableChecksum(dCtx)\n\n" \
  "Disables the checksum portion of a the frameInfo struct in the dCtx.\n" \
  "This is required for arbitrary seeking within a lz4 file. Without\n" \
  "this, decompress will error out if blocks are read out of order.\n\n" \
  "Args:\n"								\
  "    dCtx (dCtx): Decompression context.\n"

#define DCOMP_DOCSTRING \
  "decompressFrame(source, dCtx, blkSizeID = 7)\n\n"			\
  "Accepts required source string and decompressionContext, and returns\n" \
  "a dictionary containing the uncompressed data and next block size if\n" \
  "block was complete. If block was incomplete, returns characters\n"	\
  "remaining to complete block. Raises an exception if any error\n"	\
  "occurs.\n\n"								\
  "Args:\n"								\
  "    source (str): Data to decompress.\n"				\
  "    dCtx (dCtx): Decompression context.\n\n"				\
  "Returns:\n"								\
  "    dict: A dictionary with keys `decomp` and `next`\n"

static PyMethodDef module_methods[] =
{
  {
    "create_compression_context", (PyCFunction) create_compression_context,
    METH_VARARGS | METH_KEYWORDS, create_compression_context__doc
  },
  {
    "free_compression_context", (PyCFunction) free_compression_context,
    METH_VARARGS | METH_KEYWORDS, free_compression_context__doc
  },
  {
    "compress_frame", (PyCFunction) compress_frame,
    METH_VARARGS | METH_KEYWORDS, compress_frame__doc
  },
  {
    "compress_begin", (PyCFunction) compress_begin,
    METH_VARARGS | METH_KEYWORDS, compress_begin__doc
  },
  {
    "compress_update", (PyCFunction) compress_update,
    METH_VARARGS | METH_KEYWORDS,
    "Compresses blocks of data and returns the compressed data in a string of bytes. Returned strings may be empty if auto-flush is disabled."
  },
  {
    "compress_end", (PyCFunction) compress_end, METH_VARARGS | METH_KEYWORDS,
    "Flushes and returns any remaining compressed data along with the endmark and optional checksum as a string of bytes."
  },

  {
    "get_frame_info", (PyCFunction) get_frame_info,
    METH_VARARGS | METH_KEYWORDS,
    "Given a frame of compressed data, returns information about the frame."
  },
  {
    "decompress", (PyCFunction) decompress, METH_VARARGS | METH_KEYWORDS,
    "Decompressed a frame of data and returns it as a string of bytes."
  },
  {NULL, NULL, 0, NULL}		/* Sentinel */
};

#undef CCCTX_DOCSTRING
#undef COMPF_DOCSTRING
#undef MKPFS_DOCSTRING
#undef COMPB_DOCSTRING
#undef COMPU_DOCSTRING
#undef COMPE_DOCSTRING
#undef FCCTX_DOCSTRING
#undef CDCTX_DOCSTRING
#undef FDCTX_DOCSTRING
#undef GETFI_DOCSTRING
#undef DCHKS_DOCSTRING
#undef DCOMP_DOCSTRING

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
