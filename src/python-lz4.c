#include "stdlib.h"
#include "math.h"
#include "Python.h"
#include "lz4.h"
#include "python-lz4.h"

#define MAX(a, b)               ((a) > (b) ? (a) : (b))

static PyObject *
py_lz4_compress(PyObject *self, PyObject *args)
{
    const char *source = NULL;
    int source_size = 0;
    char *dest = NULL;
    int dest_size = 0;
    int osize = 0;

    if (!PyArg_ParseTuple(args, "s#", &source, &source_size))
        return NULL;

    dest_size = sizeof(int) + source_size + MAX(8, (int)ceil(source_size * 0.004f));
    PyObject *result = PyString_FromStringAndSize(NULL, (Py_ssize_t)dest_size);
    dest = (char *)PyString_AsString(result);
    *((int *)dest) = source_size;
    osize = LZ4_compress(source, dest + sizeof(int), source_size);
    return result;
}

static PyObject *
py_lz4_uncompress(PyObject *self, PyObject *args)
{
    const char *source = NULL;
    int source_size = 0;
    char *dest = NULL;
    int dest_size = 0;
    int osize = 0;

    if (!PyArg_ParseTuple(args, "s#", &source, &source_size))
        return NULL;

    dest_size = *(int *)source;
    PyObject *result = PyString_FromStringAndSize(NULL, (Py_ssize_t)dest_size);
    dest = (char *)PyString_AsString(result);
    osize = LZ4_uncompress(source + sizeof(int), dest, dest_size);

    return result;
}

static PyMethodDef Lz4Methods[] = {
    {"LZ4_compress",  py_lz4_compress, METH_VARARGS, COMPRESS_DOCSTRING},
    {"LZ4_uncompress",  py_lz4_uncompress, METH_VARARGS, UNCOMPRESS_DOCSTRING},
    {"compress",  py_lz4_compress, METH_VARARGS, COMPRESS_DOCSTRING},
    {"uncompress",  py_lz4_uncompress, METH_VARARGS, UNCOMPRESS_DOCSTRING},
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
