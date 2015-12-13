/*
 * Copyright (c) 2014, Christoper Jackson
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
 * 3. Neither the name of Christoper Jackson nor the names of its contributors may be
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

#include "Python.h"

static PyObject *py_lz4f_createCompCtx(PyObject *self, PyObject *args);
static PyObject *py_lz4f_compressFrame(PyObject *self, PyObject *args);
static PyObject *py_lz4f_compressBegin(PyObject *self, PyObject *args);
static PyObject *py_lz4f_compressUpdate(PyObject *self, PyObject *args);
static PyObject *py_lz4f_compressEnd(PyObject *self, PyObject *args);
static PyObject *py_lz4f_freeCompCtx(PyObject *self, PyObject *args);
static PyObject *py_lz4f_createDecompCtx(PyObject *self, PyObject *args);
static PyObject *py_lz4f_freeDecompCtx(PyObject *self, PyObject *args);
static PyObject *py_lz4f_getFrameInfo(PyObject *self, PyObject *args);
static PyObject *py_lz4f_decompress(PyObject *self, PyObject *args, PyObject *keywds);
static PyObject *py_lz4f_disableChecksum(PyObject *self, PyObject *args);

PyMODINIT_FUNC initlz4f(void);

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

#if defined(_WIN32) && defined(_MSC_VER)
# define inline __inline
# if _MSC_VER >= 1600
#  include <stdint.h>
# else /* _MSC_VER >= 1600 */
   typedef signed char       int8_t;
   typedef signed short      int16_t;
   typedef signed int        int32_t;
   typedef unsigned char     uint8_t;
   typedef unsigned short    uint16_t;
   typedef unsigned int      uint32_t;
# endif /* _MSC_VER >= 1600 */
#endif

#if defined(__SUNPRO_C) || defined(__hpux) || defined(_AIX)
#define inline
#endif

#ifdef __linux
#define inline __inline
#endif
