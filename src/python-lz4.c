#include "stdlib.h"
#include "math.h"
#include "Python.h"
#include "lz4.h"
#include "python-lz4.h"

#define MAX_STACK_CHUNK_SIZE    (65536) // 64kb
#define STACK_DEST_SIZE         (sizeof(int) + 65799) // sizeof(int) + ceil(64kb * 1.004)
#define MAX(a, b)               ((a) > (b) ? (a) : (b))

static PyObject *
py_lz4_compress_on_stack(char *source, int source_size)
{
    char dest[STACK_DEST_SIZE];
    int osize = 0;

    *((int *)dest) = source_size;
    osize = LZ4_compress(source, dest + sizeof(int), source_size);
    return Py_BuildValue("s#", dest, sizeof(int) + osize);
}

static PyObject *
py_lz4_uncompress_on_stack(char *source, int dest_size)
{
    char dest[dest_size];
    int osize = 0;

    osize = LZ4_uncompress(source + sizeof(int), dest, dest_size);
    return Py_BuildValue("s#", dest, dest_size);
}

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

    /* if the source is inferior to MAX_STACK_CHUNK_SIZE,
    ** we allocate dest to the stack.
    */
    if (source_size < MAX_STACK_CHUNK_SIZE)
        return py_lz4_compress_on_stack(source, source_size);

    dest_size = sizeof(int) + source_size + MAX(8, (int)ceil(source_size * 0.004f));
    dest = (char *)malloc(dest_size);
    *((int *)dest) = source_size;
    osize = LZ4_compress(source, dest + sizeof(int), source_size);
    PyObject *result = Py_BuildValue("s#", dest, sizeof(int) + osize);
    free(dest);
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

    /* can we fill the uncompressed data in MAX_STACK_CHUNK_SIZE ?
    ** in which case, use the stack to hold the data.
    */
    if (dest_size < MAX_STACK_CHUNK_SIZE)
        return py_lz4_uncompress_on_stack(source, dest_size);

    dest = (char *)malloc(dest_size);
    osize = LZ4_uncompress(source + sizeof(int), dest, dest_size);
    PyObject *result = Py_BuildValue("s#", dest, osize);
    free(dest);

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
