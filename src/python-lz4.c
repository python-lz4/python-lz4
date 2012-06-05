#include "stdlib.h"
#include "math.h"
#include "Python.h"
#include "lz4.h"
#include "lz4hc.h"
#include "python-lz4.h"

#define MAX(a, b)               ((a) > (b) ? (a) : (b))

typedef int (*compressor)(const char *source, char *dest, int isize);

static inline void store_le32(char *c, uint32_t x)
{
	c[0] = x & 0xff;
	c[1] = (x >> 8) & 0xff;
	c[2] = (x >> 16) & 0xff;
	c[3] = (x >> 24) & 0xff;
}

static inline uint32_t load_le32(const char *c)
{
    const uint8_t *d = (const uint8_t *)c;
    return d[0] | (d[1] << 8) | (d[2] << 16) | (d[3] << 24);
}

static const int hdr_size = sizeof(uint32_t);

static PyObject *
compress_with(compressor compress, PyObject *self, PyObject *args)
{
    PyObject *result;
    const char *source;
    int source_size;
    char *dest;
    int dest_size;

    if (!PyArg_ParseTuple(args, "s#", &source, &source_size))
        return NULL;

    dest_size = hdr_size + LZ4_compressBound(source_size);
    result = PyString_FromStringAndSize(NULL, dest_size);
    if (result == NULL)
	return NULL;
    dest = PyString_AS_STRING(result);
    store_le32(dest, source_size);
    if (source_size > 0) {
	int osize = compress(source, dest + hdr_size, source_size);
	int actual_size = hdr_size + osize;
	/* Resizes are expensive; tolerate some slop to avoid. */
	if (actual_size < (dest_size / 4) * 3)
	    _PyString_Resize(&result, actual_size);
	else
	    Py_SIZE(result) = actual_size;
    }
    return result;
}

static PyObject *py_lz4_compress(PyObject *self, PyObject *args)
{
    return compress_with(LZ4_compress, self, args);
}

static PyObject *py_lz4_compressHC(PyObject *self, PyObject *args)
{
    return compress_with(LZ4_compressHC, self, args);
}

static PyObject *
py_lz4_uncompress(PyObject *self, PyObject *args)
{
    PyObject *result;
    const char *source;
    int source_size;
    uint32_t dest_size;

    if (!PyArg_ParseTuple(args, "s#", &source, &source_size))
        return NULL;

    if (source_size < hdr_size) {
	PyErr_SetString(PyExc_ValueError, "input too short");
	return NULL;
    }
    dest_size = load_le32(source);
    if (dest_size > INT_MAX) {
	PyErr_Format(PyExc_ValueError, "invalid size in header: 0x%x",
		     dest_size);
	return NULL;
    }
    result = PyString_FromStringAndSize(NULL, dest_size);
    if (result != NULL && dest_size > 0) {
	char *dest = PyString_AS_STRING(result);
	int osize = LZ4_uncompress(source + hdr_size, dest, dest_size);

	if (osize < 0) {
	    PyErr_Format(PyExc_ValueError, "corrupt input at byte %d", -osize);
	    Py_CLEAR(result);
	}
	else if (osize < source_size - hdr_size) {
	    PyErr_SetString(PyExc_ValueError, "decompression incomplete");
	    Py_CLEAR(result);
	}
    }

    return result;
}

static PyMethodDef Lz4Methods[] = {
    {"LZ4_compress",  py_lz4_compress, METH_VARARGS, COMPRESS_DOCSTRING},
    {"LZ4_uncompress",  py_lz4_uncompress, METH_VARARGS, UNCOMPRESS_DOCSTRING},
    {"compress",  py_lz4_compress, METH_VARARGS, COMPRESS_DOCSTRING},
    {"compressHC",  py_lz4_compressHC, METH_VARARGS, COMPRESSHC_DOCSTRING},
    {"uncompress",  py_lz4_uncompress, METH_VARARGS, UNCOMPRESS_DOCSTRING},
    {"decompress",  py_lz4_uncompress, METH_VARARGS, UNCOMPRESS_DOCSTRING},
    {"dumps",  py_lz4_compress, METH_VARARGS, COMPRESS_DOCSTRING},
    {"loads",  py_lz4_uncompress, METH_VARARGS, UNCOMPRESS_DOCSTRING},
    {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC
initlz4(void)
{
    PyObject *m;

    m = Py_InitModule("lz4", Lz4Methods);
    if (m == NULL)
        return;
}
