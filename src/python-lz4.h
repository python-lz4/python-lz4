#include "Python.h"

static PyObject *py_lz4_compress(PyObject *self, PyObject *args);
static PyObject *py_lz4_uncompress(PyObject *self, PyObject *args);

PyMODINIT_FUNC initlz4(void);

#define COMPRESS_DOCSTRING      "Compress string, returning the compressed data.\nRaises an exception if any error occurs."
#define COMPRESSHC_DOCSTRING    COMPRESS_DOCSTRING "\n\nCompared to compress, this gives a better compression ratio, but is much slower."
#define UNCOMPRESS_DOCSTRING    "Decompress string, returning the uncompressed data.\nRaises an exception if any error occurs."

#if defined(_WIN32) && defined(_MSC_VER)
# define inline __inline
#endif

#if defined(__SUNPRO_C) || defined(__hpux) || defined(_AIX)
#define inline
#endif

#ifdef __linux
#define inline __inline
#endif
