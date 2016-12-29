/*
 * Copyright (c) 2016 Jonathan Underwood
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
#include <lz4.h>
#include <lz4hc.h>

#ifndef Py_UNUSED /* This is already defined for Python 3.4 onwards */
#ifdef __GNUC__
#define Py_UNUSED(name) _unused_ ## name __attribute__((unused))
#else
#define Py_UNUSED(name) _unused_ ## name
#endif
#endif

PyDoc_STRVAR(lz4version__doc,
             "lz4version()\n\n"                                         \
             "Returns the version number of the LZ4 library");

static PyObject *
lz4version (PyObject * Py_UNUSED (self), PyObject * Py_UNUSED (args))
{
  return Py_BuildValue ("i", LZ4_versionNumber ());
}

static PyMethodDef module_methods[] = {
  {
    "lz4version",
    (PyCFunction) lz4version,
    METH_VARARGS,
    lz4version__doc
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
    "_version",
    NULL,
    -1,
    module_methods
  };

MODULE_INIT_FUNC (_version)
{
  PyObject *module = PyModule_Create (&moduledef);

  if (module == NULL)
    return NULL;

  return module;
}
