#include <Python.h>
#include <structmember.h>

static unsigned char bitarray_BYTE_BIT_COUNT[] = {
     0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
     1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
     1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
     2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
     1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
     2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
     2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
     3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
     1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
     2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
     2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
     3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
     2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
     3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
     3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
     4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
};

/* on a 64-bit machine, Py_ssize_t is 8-bytes */
typedef struct {
    PyObject_HEAD
    /* Type-specific fields go here */
    unsigned char *array;
    Py_ssize_t nbytes;
    Py_ssize_t nbits;
} bitarray_BitArrayObject;

static PyTypeObject bitarray_BitArrayType;
#define bitarray_BitArray_Check(obj) PyObject_TypeCheck(obj, &BitArrayType)

typedef struct {
    PyObject_HEAD
    /* Type-specific fields go here */
    bitarray_BitArrayObject *bitArray;  /* BitArray we're iterating over */
    Py_ssize_t idx;                  /* current index in BitArray */
} bitarray_BitArrayIteratorObject;

static PyTypeObject bitarray_BitArrayIteratorType;
#define bitarray_BitArrayIterator_Check(obj) PyObject_TypeCheck(obj, &bitArrayIteratorType)

static PyObject *
bitarray_BitArrayIteratorObject_next(bitarray_BitArrayIteratorObject *it)
{
    Py_ssize_t byte_idx = 0;
    char bit_idx = 0;
    unsigned char bit_val = 0;

    /* only return indices that are set to 1 */ 
    while (it->idx < it->bitArray->nbits) {
        byte_idx = it->idx / 8;
        bit_idx = it->idx % 8;
        bit_val = (it->bitArray->array[byte_idx] >> bit_idx) & 0x01;
        it->idx++;
        if (bit_val == 1)
            return (PyInt_FromSsize_t(it->idx - 1));
    }

    Py_DECREF(it->bitArray);
    it->bitArray = NULL;
    return (NULL); /* stop iteration */ 
}

static void
bitarray_BitArrayIteratorObject_dealloc(bitarray_BitArrayIteratorObject *it)
{
    Py_XDECREF(it->bitArray);
}


static int
bitarray_BitArrayIteratorObject_traverse(bitarray_BitArrayIteratorObject *it, 
        visitproc visit, void *arg)
{
    Py_VISIT(it->bitArray);
    return (0);
}

static PyTypeObject bitarray_BitArrayIteratorType = {
    PyObject_HEAD_INIT(NULL)
    .tp_name        = "bitarray.BitArrayIterator",
    .tp_basicsize   = sizeof(bitarray_BitArrayIteratorObject),
    .tp_dealloc     = (destructor)bitarray_BitArrayIteratorObject_dealloc,
    .tp_iter        = PyObject_SelfIter,
    .tp_iternext    = (iternextfunc)bitarray_BitArrayIteratorObject_next,
    .tp_traverse    = (traverseproc)bitarray_BitArrayIteratorObject_traverse,
    .tp_flags       = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC
};


static void
bitarray_BitArrayObject_dealloc(bitarray_BitArrayObject *self)
{
    PyMem_Free(self->array);
    self->ob_type->tp_free((PyObject *)self);
}

static int
bitarray_BitArrayObject_init(bitarray_BitArrayObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"nbits", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "n", kwlist, &self->nbits))
        return (-1);

    if (self->nbits <= 0) {
        PyErr_SetString(PyExc_ValueError, "BitArray: nbits must be > 0");
        return (-1);
    }
    
    self->nbytes = (self->nbits / 8) + 1;
    self->array = PyMem_Malloc(self->nbytes);
    if (self->array == NULL) {
        PyErr_NoMemory();
        return (-1);
    }

    memset(self->array, 0x00, self->nbytes);
    return (0);
}

static PyObject *
bitarray_BitArrayObject_iter(bitarray_BitArrayObject *self)
{
    bitarray_BitArrayIteratorObject *it;

    it = PyObject_GC_New(bitarray_BitArrayIteratorObject, &bitarray_BitArrayIteratorType);
    if (it == NULL)
        return (NULL);
    it->idx = 0;
    Py_INCREF(self);
    it->bitArray = self;
    return (PyObject *)it;
}

static PyObject *
bitarray_BitArrayObject_set(bitarray_BitArrayObject *self, PyObject *pos)
{
    Py_ssize_t idx = 0;
    Py_ssize_t byte_idx = 0;
    char bit_idx = 0;

    if (!PyInt_Check(pos) && !PyLong_Check(pos)) {
        PyErr_SetString(PyExc_ValueError, "BitArray index must be an int or a long");
        return NULL;
    }

    idx = PyInt_AsSsize_t(pos);
    if ((idx == -1) && (PyErr_Occurred() != NULL))
        return NULL;

    if ((idx < 0) ||  (idx >= self->nbits)) {
        PyErr_SetString(PyExc_IndexError, "BitArray index out of range");
        return NULL;
    }

    byte_idx = idx / 8;
    bit_idx = idx % 8;
    self->array[byte_idx] |= 1 << bit_idx;

    Py_RETURN_NONE;
}

static PyObject *
bitarray_BitArrayObject_setv(bitarray_BitArrayObject *self, PyObject *vec)
{
    Py_ssize_t i = 0;
    Py_ssize_t n = 0;
    PyObject *item = 0;
    Py_ssize_t idx = 0;
    Py_ssize_t byte_idx = 0;
    char bit_idx = 0;

    if (!PySequence_Check(vec))
        return NULL;

    n = PySequence_Length(vec);
    if (n == -1)
        return NULL;

    for (i = 0; i < n; i++) {
        /* new ref*/
        item = PySequence_GetItem(vec, i);
        if (item == NULL)
            return NULL; /* not a sequence, or other failure */

        if (!PyInt_Check(item) && !PyLong_Check(item)) {
            PyErr_SetString(PyExc_ValueError, "BitArray index must be an int or a long");
            Py_DECREF(item);
            return NULL;
        }

        idx = PyInt_AsSsize_t(item);
        if ((idx == (unsigned long)(-1)) && (PyErr_Occurred() != NULL)) {
            Py_DECREF(item);
            return NULL;
        }

        if ((idx < 0) || (idx >= self->nbits)) {
            PyErr_SetString(PyExc_IndexError, "BitArray index out of range");
            Py_DECREF(item);
            return NULL;
        }

        byte_idx = idx / 8;
        bit_idx = idx % 8;
        self->array[byte_idx] |= 1 << bit_idx;
        Py_DECREF(item);
    }

    Py_RETURN_NONE;
}

/* in short, setvfast is the same as setv, does not perform error checking */
static PyObject *
bitarray_BitArrayObject_setvfast(bitarray_BitArrayObject *self, PyObject *vec)
{
    Py_ssize_t i = 0;
    Py_ssize_t n = 0;
    PyObject *item = 0;
    PyObject *seq = 0;
    Py_ssize_t idx = 0;
    Py_ssize_t byte_idx = 0;
    char bit_idx = 0;

    /* new ref*/
    seq = PySequence_Fast(vec, "argument 1 of BitArrayObject.setvfast must be a sequence");
    if (!seq)
        return NULL;

    n = PySequence_Fast_GET_SIZE(seq);
    for (i = 0; i < n; i++) {
        /* borrowed ref */
        item = PySequence_Fast_GET_ITEM(seq, i);
        idx = PyInt_AsSsize_t(item);
        byte_idx = idx / 8;
        bit_idx = idx % 8;
        self->array[byte_idx] |= 1 << bit_idx;
    }

    Py_DECREF(seq);
    Py_RETURN_NONE;
}

static PyObject *
bitarray_BitArrayObject_unset(bitarray_BitArrayObject *self, PyObject *pos)
{
    Py_ssize_t idx = 0;
    Py_ssize_t byte_idx = 0;
    char bit_idx = 0;

    if (!PyInt_Check(pos) && !PyLong_Check(pos)) {
        PyErr_SetString(PyExc_ValueError, "BitArray index must be an int or a long");
        return (NULL);
    }

    idx = PyInt_AsSsize_t(pos);
    if ((idx == -1) && (PyErr_Occurred() != NULL))
        return (NULL);

    if ((idx < 0) || (idx >= self->nbits)) {
        PyErr_SetString(PyExc_IndexError, "BitArray index out of range");
        return (NULL);
    }

    byte_idx = idx / 8;
    bit_idx = idx % 8;
    self->array[byte_idx] &= ~(1 << bit_idx);

    Py_RETURN_NONE;
}

static PyObject *
bitarray_BitArrayObject_get(bitarray_BitArrayObject *self, PyObject *pos)
{
    Py_ssize_t idx = 0;
    Py_ssize_t byte_idx = 0;
    char bit_idx = 0;
    unsigned char bit_val = 0;

    /* pos is not a numeric type; pos is out of range */
    if (!PyInt_Check(pos) && !PyLong_Check(pos)) {
        PyErr_SetString(PyExc_ValueError, "BitArray index must be an int or a long");
        return NULL;
    }

    idx = PyInt_AsSsize_t(pos);
    if ((idx == -1) && (PyErr_Occurred() != NULL))
        return NULL;

    if ((idx < 0) || (idx >= self->nbits)) {
        PyErr_SetString(PyExc_IndexError, "BitArray index out of range");
        return NULL;
    }

    byte_idx = idx / 8;
    bit_idx = idx % 8;
    bit_val = (self->array[byte_idx] >> bit_idx) & 0x01;

    return (PyInt_FromLong(bit_val));
}

static PyObject *
bitarray_BitArrayObject_nset(bitarray_BitArrayObject *self)
{
    Py_ssize_t i = 0;
    unsigned long n = 0;
    unsigned char remainder = 0;
    unsigned char j = 0;

    for (i = 0; i < (self->nbytes - 1); i++)
        n += bitarray_BYTE_BIT_COUNT[self->array[i]];

    /* handle last byte */
    remainder = self->nbits % 8;
    if (remainder > 0) {
        for (j = 0; j < remainder; j++) {
            n += (self->array[self->nbytes - 1] >> j) & 0x01;
        }
    } 
    
    return (PyLong_FromUnsignedLong(n));
}

static PyObject *
bitarray_BitArrayObject_clear(bitarray_BitArrayObject *self)
{
    memset(self->array, 0x00, self->nbytes);
    Py_RETURN_NONE;
}

static PyMemberDef bitarray_BitArrayObject_members[] = {
    {"nbits", T_PYSSIZET, offsetof(bitarray_BitArrayObject, nbits), READONLY, 
            "number of bits in the array"},
    {NULL}  /* Sentinel */
};


/* TODO: make set, get, unset take either a single index or a sequence of
 * indices.
 */
static PyMethodDef bitarray_BitArrayObject_methods[] = {
    {"set",     (PyCFunction)bitarray_BitArrayObject_set,       METH_O, 
            "set bit at given index"},
    {"setv",    (PyCFunction)bitarray_BitArrayObject_setv,      METH_O, 
            "sets many bits; the indices to set are input as a sequence of integers."},
    {"setvfast",(PyCFunction)bitarray_BitArrayObject_setvfast,  METH_O, 
            "like setv, but faster since it foregoes type checking"},
    {"unset",   (PyCFunction)bitarray_BitArrayObject_unset,     METH_O, 
            "unsets the bit at the given index"},
    {"get",     (PyCFunction)bitarray_BitArrayObject_get,       METH_O, 
            "return the value of bit at given index"},
    {"nset",    (PyCFunction)bitarray_BitArrayObject_nset,      METH_NOARGS, 
            "return the number of bits set"},
    {"clear",   (PyCFunction)bitarray_BitArrayObject_clear,     METH_NOARGS, 
            "zero the array"},
    {NULL}  /* Sentinel */
};


static Py_ssize_t
bitarray_BitArrayObject_length(bitarray_BitArrayObject *self)
{
    /* return the number of objects in seqeunce on success, and -1 on failure */
    return (self->nbits);
}

static PyObject *
bitarray_BitArrayObject_getItem(bitarray_BitArrayObject *self, Py_ssize_t i)
{
    /* return a new reference.  return NULL on failure */
    Py_ssize_t byte_idx = 0;
    char bit_idx = 0;
    unsigned char bit_val = 0;

    if ((i < 0) || (i > self->nbits)) {
        PyErr_SetString(PyExc_IndexError, "BitArray assignment index out of range");
        return (NULL);
    }

    byte_idx = i / 8;
    bit_idx = i % 8;
    bit_val = (self->array[byte_idx] >> bit_idx) & 0x01;
    return (PyInt_FromSize_t(bit_val));
}

static int
bitarray_BitArrayObject_setItem(bitarray_BitArrayObject *self, Py_ssize_t i, PyObject *v)
{
    /* does not steal a reference to v.  Return -1 on failure */

    long bit_val = 0;
    Py_ssize_t byte_idx = 0;
    char bit_idx = 0;

    if ((i < 0) || (i >= self->nbits)) {
        PyErr_SetString(PyExc_IndexError, "BitArray assignment index out of range");
        return (-1);
    }

    bit_val = PyInt_AsLong(v);
    if ((bit_val == -1) && (PyErr_Occurred() != NULL))
        return (-1);

    if ((bit_val != 0) && (bit_val != 1)) {
        PyErr_SetString(PyExc_ValueError, "BitArray bit value must be 0 or 1");
        return (-1);
    }

    byte_idx = i / 8;
    bit_idx = i % 8;

    if (bit_val == 1)
        self->array[byte_idx] |= 1 << bit_idx;
    else
        self->array[byte_idx] &= ~(1 << bit_idx);

    return (0);
}

static PySequenceMethods bitarray_BitArrayObject_seqmethods = {
    .sq_length      = (lenfunc)bitarray_BitArrayObject_length,
    .sq_item        = (ssizeargfunc)bitarray_BitArrayObject_getItem,
    .sq_ass_item    = (ssizeobjargproc)bitarray_BitArrayObject_setItem
};

static PyTypeObject bitarray_BitArrayType = {
    PyObject_HEAD_INIT(NULL)
    .tp_name        = "bitarray.BitArray",
    .tp_basicsize   = sizeof(bitarray_BitArrayObject),
    .tp_init        = (initproc)bitarray_BitArrayObject_init,
    .tp_dealloc     = (destructor)bitarray_BitArrayObject_dealloc,
    .tp_members     = bitarray_BitArrayObject_members,
    .tp_methods     = bitarray_BitArrayObject_methods,
    .tp_as_sequence = &bitarray_BitArrayObject_seqmethods,
    .tp_iter        = (getiterfunc)bitarray_BitArrayObject_iter,
    .tp_flags       = Py_TPFLAGS_DEFAULT,
    .tp_doc         = "BitArray objects"
};

static PyMethodDef bitarray_methods[] = {
    {NULL}  /* Sentinel */
};

PyMODINIT_FUNC
initbitarray(void)
{
    PyObject *m = NULL;

    bitarray_BitArrayType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&bitarray_BitArrayType) < 0)
        return;

    m = Py_InitModule3("bitarray", bitarray_methods, "BitArray module.");
    if (m == NULL)
        return;

    Py_INCREF(&bitarray_BitArrayType);
    PyModule_AddObject(m, "BitArray", (PyObject *)&bitarray_BitArrayType);
}
