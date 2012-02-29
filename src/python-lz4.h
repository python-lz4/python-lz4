#include "Python.h"

static PyObject *py_lz4_compress(PyObject *self, PyObject *args);
static PyObject *py_lz4_uncompress(PyObject *self, PyObject *args);

PyMODINIT_FUNC initlz4(void);

#define COMPRESS_DOCSTRING      ("Compresses the data in string, returning a string contained compressed data. Raises the error exception if any error occurs.")
#define UNCOMPRESS_DOCSTRING    ("Decompresses the data in string, returning a string containing the uncompressed data. Raises the error exception if any error occurs.")
