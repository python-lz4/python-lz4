/*
 * Copyright (c) 2012-2013, Steeve Morin, 2016 Jonathan Underwood
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

#if defined(_WIN32) && defined(_MSC_VER)
#define inline __inline
#elif defined(__SUNPRO_C) || defined(__hpux) || defined(_AIX)
#define inline
#endif

#include <py3c.h>
#include <py3c/capsulethunk.h>

#include <stdlib.h>
#include <math.h>
#include <lz4.h>
#include <lz4hc.h>

#ifndef Py_UNUSED /* This is already defined for Python 3.4 onwards */
#ifdef __GNUC__
#define Py_UNUSED(name) _unused_ ## name __attribute__((unused))
#else
#define Py_UNUSED(name) _unused_ ## name
#endif
#endif

#if defined(_WIN32) && defined(_MSC_VER)
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

static inline void
store_le32 (char *c, uint32_t x)
{
  c[0] = x & 0xff;
  c[1] = (x >> 8) & 0xff;
  c[2] = (x >> 16) & 0xff;
  c[3] = (x >> 24) & 0xff;
}

static inline uint32_t
load_le32 (const char *c)
{
  const uint8_t *d = (const uint8_t *) c;
  return d[0] | (d[1] << 8) | (d[2] << 16) | (d[3] << 24);
}

#ifdef inline
#undef inline
#endif

static const int hdr_size = sizeof (uint32_t);

typedef enum
{
  DEFAULT,
  FAST,
  HIGH_COMPRESSION
} compression_type;

static PyObject *
compress (PyObject * Py_UNUSED (self), PyObject * args, PyObject * kwargs)
{
  const char *mode = "default";
  int dest_size, total_size;
  int acceleration = 1, compression = 0;
  int store_size = 1;
  PyObject *py_dest;
  char *dest, *dest_start;
  compression_type comp;
  int output_size;
  static char *argnames[] = {
    "source",
    "mode",
    "store_size",
    "acceleration",
    "compression",
    NULL
  };

#if IS_PY3
  PyObject * py_source;
  Py_ssize_t source_size;
  char * source;
  if (!PyArg_ParseTupleAndKeywords (args, kwargs, "O|siii", argnames,
                                    &py_source,
                                    &mode, &store_size, &acceleration, &compression))
    {
      return NULL;
    }
  if (PyByteArray_Check(py_source))
    {
      source = PyByteArray_AsString(py_source);
      if (source == NULL)
        {
          PyErr_SetString (PyExc_ValueError, "Failed to access source bytearray object");
          return NULL;
        }
      source_size = PyByteArray_GET_SIZE(py_source);
    }
  else
    {
      source = PyBytes_AsString(py_source);
      if (source == NULL)
        {
          PyErr_SetString (PyExc_ValueError, "Failed to access source object");
          return NULL;
        }
      source_size = PyBytes_GET_SIZE(py_source);
    }
#else
  const char *source;
  int source_size;
  if (!PyArg_ParseTupleAndKeywords (args, kwargs, "s#|siii", argnames,
                                    &source, &source_size,
                                    &mode, &store_size, &acceleration, &compression))
    {
      return NULL;
    }
#endif

  if (!strncmp (mode, "default", sizeof ("default")))
    {
      comp = DEFAULT;
    }
  else if (!strncmp (mode, "fast", sizeof ("fast")))
    {
      comp = FAST;
    }
  else if (!strncmp (mode, "high_compression", sizeof ("high_compression")))
    {
      comp = HIGH_COMPRESSION;
    }
  else
    {
      PyErr_Format (PyExc_ValueError,
		    "Invalid mode argument: %s. Must be one of: standard, fast, high_compression",
		    mode);
      return NULL;
    }

  dest_size = LZ4_compressBound (source_size);

  if (store_size)
    {
      total_size = dest_size + hdr_size;
    }
  else
    {
      total_size = dest_size + hdr_size;
    }

#if IS_PY3
  py_dest = PyByteArray_FromStringAndSize (NULL, total_size);
#else
  py_dest = PyBytes_FromStringAndSize (NULL, total_size);
#endif

  if (py_dest == NULL)
    {
      return PyErr_NoMemory();
    }

#if IS_PY3
  dest = PyByteArray_AS_STRING (py_dest);
#else
  dest = PyBytes_AS_STRING (py_dest);
#endif

  Py_BEGIN_ALLOW_THREADS

  if (store_size)
    {
      store_le32 (dest, source_size);
      dest_start = dest + hdr_size;
    }
  else
    {
      dest_start = dest;
    }

  switch (comp)
    {
    case DEFAULT:
      output_size = LZ4_compress_default (source, dest_start, source_size,
                                          dest_size);
      break;
    case FAST:
      output_size = LZ4_compress_fast (source, dest_start, source_size,
                                       dest_size, acceleration);
      break;
    case HIGH_COMPRESSION:
      output_size = LZ4_compress_HC (source, dest_start, source_size,
                                     dest_size, compression);
      break;
    }

  if (output_size <= 0)
    {
      Py_BLOCK_THREADS
        PyErr_SetString (PyExc_ValueError, "Compression failed");
      Py_CLEAR (py_dest);
      return NULL;
    }

  if (store_size)
    {
      output_size += hdr_size;
    }

  Py_END_ALLOW_THREADS

  /* Resizes are expensive; tolerate some slop to avoid. */
  if (output_size < (dest_size / 4) * 3)
    {
#if IS_PY3
      PyByteArray_Resize (py_dest, output_size);
#else
      _PyBytes_Resize (&py_dest, output_size);
#endif
    }
  else
    {
      Py_SIZE (py_dest) = output_size;
    }

  return py_dest;
}

static PyObject *
decompress (PyObject * Py_UNUSED (self), PyObject * args, PyObject * kwargs)
{
  const char * source_start;
  PyObject *py_dest;
  char *dest;
  int output_size;
  size_t dest_size;
  int uncompressed_size = -1;
  static char *argnames[] = {
    "source",
    "uncompressed_size",
    NULL
  };

#if IS_PY3
  PyObject * py_source;
  Py_ssize_t source_size;
  char * source;
  if (!PyArg_ParseTupleAndKeywords (args, kwargs, "O|i", argnames,
                                    &py_source, &uncompressed_size))
    {
      return NULL;
    }
  if (PyByteArray_Check(py_source))
    {
      source = PyByteArray_AsString(py_source);
      if (source == NULL)
        {
          PyErr_SetString (PyExc_ValueError, "Failed to access source bytearray object");
          return NULL;
        }
      source_size = PyByteArray_Size(py_source);
    }
  else
    {
      source = PyBytes_AsString(py_source);
      if (source == NULL)
        {
          PyErr_SetString (PyExc_ValueError, "Failed to access source object");
          return NULL;
        }
      source_size = PyBytes_Size(py_source);
    }
#else
  const char *source;
  int source_size = 0;
  if (!PyArg_ParseTupleAndKeywords (args, kwargs, "s#|i", argnames,
                                    &source, &source_size, &uncompressed_size))
    {
      return NULL;
    }
#endif
  if (uncompressed_size > 0)
    {
      dest_size = uncompressed_size;
      source_start = source;
    }
  else
    {
      if (source_size < hdr_size)
        {
          PyErr_SetString (PyExc_ValueError, "Input source data size too small");
          return NULL;
        }
      dest_size = load_le32 (source);
      source_start = source + hdr_size;
      source_size -= hdr_size;
    }

  if (dest_size < 0 || dest_size > PY_SSIZE_T_MAX)
    {
      PyErr_Format (PyExc_ValueError, "Invalid size in header: 0x%zu",
                    dest_size);
      return NULL;
    }

#if IS_PY3
  py_dest = PyByteArray_FromStringAndSize (NULL, dest_size);
#else
  py_dest = PyBytes_FromStringAndSize (NULL, dest_size);
#endif
  if (py_dest == NULL)
    {
      return PyErr_NoMemory();
    }

#if IS_PY3
  dest = PyByteArray_AS_STRING (py_dest);
#else
  dest = PyBytes_AS_STRING (py_dest);
#endif

  Py_BEGIN_ALLOW_THREADS

  output_size =
    LZ4_decompress_safe (source_start, dest, source_size, dest_size);

  Py_END_ALLOW_THREADS

  if (output_size < 0)
    {
      PyErr_Format (PyExc_ValueError, "Corrupt input at byte %d", -output_size);
      Py_CLEAR (py_dest);
    }
  else if ((size_t)output_size != dest_size)
    {
      /* Better to fail explicitly than to allow fishy data to pass through. */
      PyErr_Format (PyExc_ValueError,
                    "Decompressor wrote %d bytes, but %zu bytes expected from header",
                    output_size, dest_size);
      Py_CLEAR (py_dest);
    }

  return py_dest;
}

PyDoc_STRVAR(compress__doc,
             "compress(source, mode='default', acceleration=1, compression=0)\n\n" \
             "Compress source, returning the compressed data as a string.\n" \
             "Raises an exception if any error occurs.\n\n"             \
             "Args:\n"                                                  \
             "    source (str, bytes or bytearray): Data to compress\n"                     \
             "    mode (str): If 'default' or unspecified use the default LZ4\n" \
             "        compression mode. Set to 'fast' to use the fast compression\n" \
             "        LZ4 mode at the expense of compression. Set to\n" \
             "        'high_compression' to use the LZ4 high-compression mode at\n" \
             "        the exepense of speed\n"                          \
             "    acceleration (int): When mode is set to 'fast' this argument\n" \
             "        specifies the acceleration. The larger the acceleration, the\n" \
             "        faster the but the lower the compression. The default\n" \
             "        compression corresponds to a value of 1.\n"       \
             "    compression (int): When mode is set to `high_compression` this\n" \
             "        argument specifies the compression. Valid values are between\n" \
             "        0 and 16. Values between 4-9 are recommended, and 0 is the\n" \
             "        default.\n\n"                                     \
             "    store_size (bool): If True (the default) then the size of the\n" \
             "        uncompressed data is stored at the start of the compressed\n" \
             "        block.\n\n"                                       \
             "Returns:\n"                                               \
             "    str or bytearray: Compressed data. For Python 2 a str is returned\n" \
             "        and for Python 3 a bytearray is returned\n");

PyDoc_STRVAR(decompress__doc,
             "decompress(source, uncompressed_size=-1)\n\n"                                 \
             "Decompress source, returning the uncompressed data as a string.\n" \
             "Raises an exception if any error occurs.\n\n"             \
             "Args:\n"                                                  \
             "    source (str, bytes or bytearray): Data to decompress\n\n"                 \
             "    uncompressed_size (int): If not specified or < 0, the uncompressed data\n" \
             "        size is read from the start of the source block. If specified,\n" \
             "        it is assumed that the full source data is compressed data.\n\n" \
             "Returns:\n"                                             \
             "    str or bytearray: Decompressed data. For Python 2 a str is returned\n" \
             "        and for Python 3 a bytearray is returned\n");

PyDoc_STRVAR(lz4block__doc,
             "A Python wrapper for the LZ4 block protocol"
             );

static PyMethodDef module_methods[] = {
  {
    "compress",
    (PyCFunction) compress,
    METH_VARARGS | METH_KEYWORDS,
    compress__doc
  },
  {
    "decompress",
    (PyCFunction) decompress,
    METH_VARARGS | METH_KEYWORDS,
    decompress__doc
  },
  {
    /* Sentinel */
    NULL,
    NULL,
    0,
    NULL
  }
};

static struct PyModuleDef moduledef =
{
  PyModuleDef_HEAD_INIT,
  "_block",
  lz4block__doc,
  -1,
  module_methods
};

MODULE_INIT_FUNC (_block)
{
  PyObject *module = PyModule_Create (&moduledef);

  if (module == NULL)
    return NULL;

  return module;
}
