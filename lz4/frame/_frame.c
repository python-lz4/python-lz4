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
#include <lz4frame.h>

#ifndef Py_UNUSED		/* This is already defined for Python 3.4 onwards */
#ifdef __GNUC__
#define Py_UNUSED(name) _unused_ ## name __attribute__((unused))
#else
#define Py_UNUSED(name) _unused_ ## name
#endif
#endif

/* On Python < 2.7 the Capsule API isn't available, so work around that */
#if PY_MAJOR_VERSION < 3 && PY_MINOR_VERSION < 7
#define PyCapsule_New PyCObject_FromVoidPtrAndDesc
#define PyCapsule_GetPointer(o, n) PyCObject_GetDesc((o))
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

#if PY_MAJOR_VERSION >= 3
#define PyInt_FromSize_t(x) PyLong_FromSize_t(x)
#endif

static int
LZ4S_GetBlockSize_FromBlockId (int id)
{
  return (1 << (8 + (2 * id)));
}

/* Compression methods */

static PyObject *
py_lz4f_createCompCtx (PyObject * Py_UNUSED (self),
		       PyObject * Py_UNUSED (args))
{
  PyObject *result;
  LZ4F_compressionContext_t cCtx;
  size_t err;

  err = LZ4F_createCompressionContext (&cCtx, LZ4F_VERSION);
  if (LZ4F_isError (err))
    {
      return PyErr_Format (PyExc_MemoryError,
			   "LZ4F_createCompressionContext allocation failed: %s\n",
			   LZ4F_getErrorName (err));
    }

  result = PyCapsule_New (cCtx, NULL, NULL);

  return result;
}

static PyObject *
py_lz4f_freeCompCtx (PyObject * self, PyObject * args)
{
  PyObject *py_cCtx;
  LZ4F_compressionContext_t cCtx;

  (void) self;
  if (!PyArg_ParseTuple (args, "O", &py_cCtx))
    {
      return NULL;
    }

  cCtx = (LZ4F_compressionContext_t) PyCapsule_GetPointer (py_cCtx, NULL);
  LZ4F_freeCompressionContext (cCtx);

  Py_RETURN_NONE;
}

static PyObject *
py_lz4f_compressFrame (PyObject * Py_UNUSED (self), PyObject * args)
{
  PyObject *result;
  const char *source;
  char *dest;
  int src_size;
  size_t dest_size;
  size_t final_size;
  size_t ssrc_size;

  if (!PyArg_ParseTuple (args, "s#", &source, &src_size))
    {
      return NULL;
    }

  ssrc_size = (size_t) src_size;
  dest_size = LZ4F_compressFrameBound (ssrc_size, NULL);

  dest = (char *) malloc (dest_size);
  if (dest == NULL)
    {
      return PyErr_NoMemory();
    }

  final_size = LZ4F_compressFrame (dest, dest_size, source, ssrc_size, NULL);
  result = PyBytes_FromStringAndSize (dest, final_size);

  free (dest);

  return result;
}


static PyObject *
py_lz4f_makePrefs (PyObject * Py_UNUSED (self), PyObject * args,
		   PyObject * keywds)
{
  LZ4F_frameInfo_t frameInfo;
  LZ4F_preferences_t *prefs;
  PyObject *result = PyDict_New ();
  static char *kwlist[] = { "blockSizeID", "blockMode", "chkFlag"
      "autoFlush", NULL
  };
  unsigned int blkID = 7;
  unsigned int blkMode = 1;
  unsigned int chkSumFlag = 0;
  unsigned int autoFlush = 0;

  if (!PyArg_ParseTupleAndKeywords (args, keywds, "|IIII", kwlist, &blkID,
				    &blkMode, &chkSumFlag, &autoFlush))
    {
      return NULL;
    }

  prefs = calloc (1, sizeof (LZ4F_preferences_t));
  frameInfo = (LZ4F_frameInfo_t)
  {
    blkID, blkMode, chkSumFlag, 0, 0,
    {0, 0}
  };
  prefs->frameInfo = frameInfo;
  prefs->autoFlush = autoFlush;
  result = PyCapsule_New (prefs, NULL, NULL);

  return result;
}

static PyObject *
py_lz4f_compressBegin (PyObject * Py_UNUSED (self), PyObject * args)
{
  char *dest;
  LZ4F_compressionContext_t cCtx;
  LZ4F_preferences_t prefs = { {7, 0, 0, 0, 0, {0}}, 0, 0, {0} };
  LZ4F_preferences_t *prefsPtr = &prefs;
  PyObject *result;
  PyObject *py_cCtx;
  PyObject *py_prefsPtr = Py_None;
  size_t dest_size;
  size_t final_size;

  if (!PyArg_ParseTuple (args, "O|O", &py_cCtx, &py_prefsPtr))
    {
      return NULL;
    }

  cCtx = (LZ4F_compressionContext_t) PyCapsule_GetPointer (py_cCtx, NULL);
  dest_size = 19;
  dest = (char *) malloc (dest_size);
  if (dest == NULL)
    {
      return PyErr_NoMemory();
    }

  if (py_prefsPtr != Py_None)
    {
      prefsPtr =
	(LZ4F_preferences_t *) PyCapsule_GetPointer (py_prefsPtr, NULL);
    }

  final_size = LZ4F_compressBegin (cCtx, dest, dest_size, prefsPtr);
  if (LZ4F_isError (final_size))
    {
      free (dest);
      return PyErr_Format (PyExc_RuntimeError, "LZ4F_compressBegin failed: %s\n",
			   LZ4F_getErrorName (final_size));
    }

  result = PyBytes_FromStringAndSize (dest, final_size);

  free (dest);

  return result;
}

static PyObject *
py_lz4f_compressUpdate (PyObject * Py_UNUSED (self), PyObject * args)
{
  const char *source;
  char *dest;
  int src_size;
  LZ4F_compressionContext_t cCtx;
  PyObject *result;
  PyObject *py_cCtx;
  size_t dest_size;
  size_t final_size;
  size_t ssrc_size;

  if (!PyArg_ParseTuple (args, "s#O", &source, &src_size, &py_cCtx))
    {
      return NULL;
    }

  cCtx = (LZ4F_compressionContext_t) PyCapsule_GetPointer (py_cCtx, NULL);

  ssrc_size = (size_t) src_size;
  dest_size = LZ4F_compressBound (ssrc_size, (LZ4F_preferences_t *) cCtx);

  dest = (char *) malloc (dest_size);
  if (dest == NULL)
    {
      return PyErr_NoMemory();
    }

  final_size =
    LZ4F_compressUpdate (cCtx, dest, dest_size, source, ssrc_size, NULL);
  if (LZ4F_isError (final_size))
    {
      free (dest);
      return PyErr_Format (PyExc_RuntimeError, "LZ4F_compressUpdate failed: %s\n",
			   LZ4F_getErrorName (final_size));
    }

  result = PyBytes_FromStringAndSize (dest, final_size);

  free (dest);

  return result;
}

static PyObject *
py_lz4f_compressEnd (PyObject * Py_UNUSED (self), PyObject * args)
{
  char *dest;
  LZ4F_compressionContext_t cCtx;
  PyObject *result;
  PyObject *py_cCtx;
  size_t dest_size;
  size_t final_size;

  if (!PyArg_ParseTuple (args, "O", &py_cCtx))
    {
      return NULL;
    }

  cCtx = (LZ4F_compressionContext_t) PyCapsule_GetPointer (py_cCtx, NULL);
  dest_size = LZ4F_compressBound (0, (LZ4F_preferences_t *) cCtx);
  dest = (char *) malloc (dest_size);
  if (dest == NULL)
    {
      return PyErr_NoMemory();
    }

  final_size = LZ4F_compressEnd (cCtx, dest, dest_size, NULL);
  if (LZ4F_isError (final_size))
    {
      free (dest);
      return PyErr_Format (PyExc_RuntimeError, "LZ4F_compressEnd failed: %s\n",
			   LZ4F_getErrorName (final_size));
    }

  result = PyBytes_FromStringAndSize (dest, final_size);

  free (dest);

  return result;
}


/* Decompression methods */
static PyObject *
py_lz4f_createDecompCtx (PyObject * Py_UNUSED (self),
			 PyObject * Py_UNUSED (args))
{
  PyObject *result;
  LZ4F_decompressionContext_t dCtx;
  size_t err;

  err = LZ4F_createDecompressionContext (&dCtx, LZ4F_VERSION);
  if (LZ4F_isError (err))
    {
      return PyErr_Format (PyExc_MemoryError,
			   "LZ4F_createDecompressionContext failed: %s\n",
			   LZ4F_getErrorName (err));
    }

  result = PyCapsule_New (dCtx, NULL, NULL);

  return result;
}

static PyObject *
py_lz4f_freeDecompCtx (PyObject * Py_UNUSED (self), PyObject * args)
{
  PyObject *py_dCtx;
  LZ4F_decompressionContext_t dCtx;

  if (!PyArg_ParseTuple (args, "O", &py_dCtx))
    {
      return NULL;
    }

  dCtx = (LZ4F_decompressionContext_t) PyCapsule_GetPointer (py_dCtx, NULL);
  LZ4F_freeDecompressionContext (dCtx);

  Py_RETURN_NONE;
}

static PyObject *
py_lz4f_getFrameInfo (PyObject * Py_UNUSED (self), PyObject * args)
{
  const char *source;
  int src_size;
  LZ4F_decompressionContext_t dCtx;
  LZ4F_frameInfo_t frameInfo;
  PyObject *blkSize;
  PyObject *blkMode;
  PyObject *contChkFlag;
  PyObject *py_dCtx;
  PyObject *result = PyDict_New ();
  size_t ssrc_size;
  size_t err;

  if (!PyArg_ParseTuple (args, "s#O", &source, &src_size, &py_dCtx))
    {
      return NULL;
    }

  dCtx = (LZ4F_decompressionContext_t) PyCapsule_GetPointer (py_dCtx, NULL);
  ssrc_size = (size_t) src_size;

  err =
    LZ4F_getFrameInfo (dCtx, &frameInfo, (unsigned char *) source,
		       &ssrc_size);
  if (LZ4F_isError (err))
    {
      return PyErr_Format (PyExc_RuntimeError, "LZ4F_getFrameInfo failed: %s\n",
			   LZ4F_getErrorName (err));
    }

  blkSize = PyInt_FromSize_t (frameInfo.blockSizeID);
  blkMode = PyInt_FromSize_t (frameInfo.blockMode);
  contChkFlag = PyInt_FromSize_t (frameInfo.contentChecksumFlag);
  PyDict_SetItemString (result, "blkSize", blkSize);
  PyDict_SetItemString (result, "blkMode", blkMode);
  PyDict_SetItemString (result, "chkFlag", contChkFlag);

  return result;
}

static PyObject *
py_lz4f_disableChecksum (PyObject * Py_UNUSED (self), PyObject * args)
{
  PyObject *py_dCtx;
  LZ4F_decompressionContext_t dCtx;
  LZ4F_frameInfo_t* dctxPtr;

  if (!PyArg_ParseTuple (args, "O", &py_dCtx))
    {
      return NULL;
    }

  /* This works because the first element of the LZ4F_dctx_t type is a
     LZ4F_frameInfo_t object. Eventually the 2nd and 3rd lines should
     be replaced by a call to LZ4F_disableChecksum() when it is merged
     in upstream lz4. This will be sometime after release 131 of
     lz4. See:  https://github.com/Cyan4973/lz4/pull/214 */
  dCtx = (LZ4F_decompressionContext_t) PyCapsule_GetPointer (py_dCtx, NULL);
  dctxPtr = (LZ4F_frameInfo_t *) dCtx;
  dctxPtr->contentChecksumFlag = 0;
  /* Replace above two lines with: LZ4F_disableChecksum(&dCtx); */
  Py_RETURN_NONE;
}

static PyObject *
py_lz4f_decompress (PyObject * Py_UNUSED (self), PyObject * args,
		    PyObject * keywds)
{
  const char *source;
  char *dest;
  LZ4F_decompressionContext_t dCtx;
  int src_size;
  PyObject *decomp;
  PyObject *next;
  PyObject *py_dCtx;
  PyObject *result = PyDict_New ();
  size_t ssrc_size;
  size_t dest_size;
  size_t err;
  static char *kwlist[] = { "source", "dCtx", "blkSizeID", NULL };
  unsigned int blkID = 7;

  if (!PyArg_ParseTupleAndKeywords (args, keywds, "s#O|i", kwlist, &source,
				    &src_size, &py_dCtx, &blkID))
    {
      return NULL;
    }

  dest_size = LZ4S_GetBlockSize_FromBlockId (blkID);
  dCtx = (LZ4F_decompressionContext_t) PyCapsule_GetPointer (py_dCtx, NULL);
  ssrc_size = (size_t) src_size;

  dest = (char *) malloc (dest_size);
  if (dest == NULL)
    {
      return PyErr_NoMemory();
    }

  err = LZ4F_decompress (dCtx, dest, &dest_size, source, &ssrc_size, NULL);
  if (LZ4F_isError (err))
    {
      free (dest);
      return PyErr_Format (PyExc_RuntimeError, "LZ4F_decompress failed: %s\n",
			   LZ4F_getErrorName (err));
    }

  decomp = PyBytes_FromStringAndSize (dest, dest_size);
  next = PyInt_FromSize_t (err);
  PyDict_SetItemString (result, "decomp", decomp);
  PyDict_SetItemString (result, "next", next);

  Py_XDECREF (decomp);
  Py_XDECREF (next);
  free (dest);

  return result;
}

#define CTX_DOCSTRING    "context, to be used with all respective functions."
#define CCCTX_DOCSTRING  "::rtype cCtx::\nGenerates a compression " CTX_DOCSTRING
#define COMPF_DOCSTRING  "(source)\n :type string: source\n::rtype string::\n" \
                         "Accepts a string, and compresses the string in one go, returning the compressed string. \n" \
                         "This generates a header, compressed data and endmark, making the result ready to be written to file."
#define MKPFS_DOCSTRING  "(blockSizeID=7, blockMode=1, chkFlag=0, autoFlush=0)\n:type int: blockSizeID\n:type int: blockMode\n" \
                         ":type int: chkFlag\n:type int: autoFlush\n::rtype PyCapsule:\n" \
                         "All accepted arguments are keyword args. Valid entries for blockSizeID are 4-7. Valid values for the\n" \
                         "remaining optional arguments are 0-1."
#define COMPB_DOCSTRING  "(cCtx)\n:type cCtx: cCtx\n::rtype string::\n" \
                         "Accepts a compression context as a PyCObject. Returns a frame header, based on context variables."
#define COMPU_DOCSTRING  "(source, cCtx)\n:type string: source\n:type cCtx: cCtx\n::rtype string::\n" \
                         "Accepts a string, and a compression context. Returns the string as a compressed block, if the\n" \
                         "block is filled. If not, it will return a blank string and hold the compressed data until the\n" \
                         "block is filled, flush is called or compressEnd is called."
#define COMPE_DOCSTRING  "(cCtx)\n:type cCtx: cCtx\n::rtype string::\n" \
                         "Accepts a compression context as a PyCObject. Flushed the holding buffer, applies endmark and if\n" \
                         "applicable will generate a checksum. Returns a string."
#define FCCTX_DOCSTRING  "(cCtx)\n:type cCtx: cCtx\n::NO RETURN::\nFrees a compression context, passed as a PyCObject."
#define CDCTX_DOCSTRING  "::rtype dCtx::\nGenerates a decompression " CTX_DOCSTRING
#define FDCTX_DOCSTRING  "(dCtx)\n:type dCtx: dCtx\n::NO RETURN::\nFrees a decompression context, passed as a PyCObject."
#define GETFI_DOCSTRING  "(header, dctx)\n:type string: header\n:type dCtx: dCtx\n" \
                         "::rtype dict:: {'chkFlag': int, 'blkSize': int, 'blkMode': int}\n" \
                         "Accepts a string, which should be the first 7 bytes of a lz4 file, the 'header,'  and a dCtx PyCObject.\n" \
                         "Returns a dictionary object containing the frame info for the given header."
#define DCHKS_DOCSTRING  "(dCtx)\n:type dCtx: dCtx\nDisables the checksum portion of a the frameInfo struct in the dCtx. \n" \
                         "This is required for arbitrary seeking of a lz4 file. Without this, decompress will error out if blocks\n" \
                         "are read out of order."
#define DCOMP_DOCSTRING  "(source, dCtx, blkSizeID = 7)\n:type string: source\n:type dCtx: dCtx\n:type int: blkSizeID\n" \
                         "::rtype dict :: {'decomp': '', 'next': ''}\n" \
                         "Accepts required source string and decompressionContext, returns a dictionary containing the uncompressed\n" \
                         "data if block was complete and next block size. If block was incomplete, returns characters remaining to\n" \
                         "complete block. Raises an exception if any error occurs."

static PyMethodDef Lz4fMethods[] = {
  {"createCompContext", py_lz4f_createCompCtx, METH_VARARGS, CCCTX_DOCSTRING},
  {"compressFrame", py_lz4f_compressFrame, METH_VARARGS, COMPF_DOCSTRING},
  {"makePrefs", (PyCFunction) py_lz4f_makePrefs, METH_VARARGS | METH_KEYWORDS,
   MKPFS_DOCSTRING},
  {"compressBegin", py_lz4f_compressBegin, METH_VARARGS, COMPB_DOCSTRING},
  {"compressUpdate", py_lz4f_compressUpdate, METH_VARARGS, COMPU_DOCSTRING},
  {"compressEnd", py_lz4f_compressEnd, METH_VARARGS, COMPE_DOCSTRING},
  {"freeCompContext", py_lz4f_freeCompCtx, METH_VARARGS, FCCTX_DOCSTRING},
  {"createDecompContext", py_lz4f_createDecompCtx, METH_VARARGS,
   CDCTX_DOCSTRING},
  {"freeDecompContext", py_lz4f_freeDecompCtx, METH_VARARGS, FDCTX_DOCSTRING},
  {"getFrameInfo", py_lz4f_getFrameInfo, METH_VARARGS, GETFI_DOCSTRING},
  {"decompressFrame", (PyCFunction) py_lz4f_decompress,
   METH_VARARGS | METH_KEYWORDS, DCOMP_DOCSTRING},
  {"disableChecksum", py_lz4f_disableChecksum, METH_VARARGS, DCHKS_DOCSTRING},
  {NULL, NULL, 0, NULL}
};

#undef CTX_DOCSTRING
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

#if PY_MAJOR_VERSION >= 3
static struct PyModuleDef moduledef = {
  PyModuleDef_HEAD_INIT,
  "_frame",
  NULL,
  -1,
  Lz4fMethods,
  NULL,
  NULL,
  NULL,
  NULL
};

PyObject *
PyInit__frame (void)
{
  PyObject *module = PyModule_Create (&moduledef);

  if (module == NULL)
    {
      return NULL;
    }

  return module;
}

#else /* Python 2 */
PyMODINIT_FUNC
init_frame (void)
{
  (void) Py_InitModule ("_frame", Lz4fMethods);
}
#endif /* PY_MAJOR_VERSION >= 3 */
