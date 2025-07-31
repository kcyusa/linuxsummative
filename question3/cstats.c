#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <math.h>
#include <stdlib.h>

/*
==========================================================
 C Extension: cstats
----------------------------------------------------------
 Provides fast statistical functions for Python lists:
   1. sum_values  - Sum of values (O(n))
   2. mean_values - Mean (average) of values (O(n))
   3. std_dev     - Sample-based standard deviation (O(n))
   4. mode_value  - Mode (most frequent value) (O(n^2))
   5. count_values- Number of values (O(1))

 Memory Usage:
   - Each function converts a Python list into a C array of doubles.
   - Memory is dynamically allocated using malloc() and freed after computation.
   - This avoids Python object overhead and improves speed for large datasets.

 Time Complexity:
   - sum_values, mean_values, std_dev: O(n)
   - mode_value: O(n^2) (naive pairwise comparison)
   - count_values: O(1) (directly uses PyList_Size)

 Error Handling:
   - Type checks for valid list input.
   - Empty list checks for mean and std_dev.
   - Memory allocation failure handled with PyErr_NoMemory.
==========================================================
*/

/* 
----------------------------------------------------------
 Helper: pylist_to_carray
----------------------------------------------------------
 Converts a Python list of numbers to a C array of doubles.
 Returns: 1 (success), 0 (failure with Python exception).
*/
static int pylist_to_carray(PyObject *listObj, double **out_array, Py_ssize_t *out_size) {
    if (!PyList_Check(listObj)) {
        PyErr_SetString(PyExc_TypeError, "Expected a list of floats.");
        return 0;
    }

    Py_ssize_t size = PyList_Size(listObj);
    *out_size = size;
    *out_array = (double *)malloc(size * sizeof(double));

    if (!*out_array) {
        PyErr_NoMemory();
        return 0;
    }

    for (Py_ssize_t i = 0; i < size; i++) {
        PyObject *item = PyList_GetItem(listObj, i);
        if (!PyFloat_Check(item) && !PyLong_Check(item)) {
            free(*out_array);
            PyErr_SetString(PyExc_TypeError, "List must contain only numbers.");
            return 0;
        }
        (*out_array)[i] = PyFloat_AsDouble(item);
    }
    return 1;
}

/*
----------------------------------------------------------
 sum_values
----------------------------------------------------------
 Returns the sum of all float values in the list.
 Algorithm: Single pass summation.
 Time Complexity: O(n)
 Memory: O(n) for temporary C array.
*/
static PyObject* sum_values(PyObject *self, PyObject *args) {
    PyObject *listObj;
    if (!PyArg_ParseTuple(args, "O", &listObj)) return NULL;

    double *arr;
    Py_ssize_t size;
    if (!pylist_to_carray(listObj, &arr, &size)) return NULL;

    double sum = 0.0;
    for (Py_ssize_t i = 0; i < size; i++) sum += arr[i];

    free(arr);
    return PyFloat_FromDouble(sum);
}

/*
----------------------------------------------------------
 mean_values
----------------------------------------------------------
 Returns the average (mean) of the values.
 Algorithm: sum / n
 Time Complexity: O(n)
 Memory: O(n) for temporary C array.
 Error: Raises ValueError for empty lists.
*/
static PyObject* mean_values(PyObject *self, PyObject *args) {
    PyObject *listObj;
    if (!PyArg_ParseTuple(args, "O", &listObj)) return NULL;

    double *arr;
    Py_ssize_t size;
    if (!pylist_to_carray(listObj, &arr, &size)) return NULL;

    if (size == 0) {
        free(arr);
        PyErr_SetString(PyExc_ValueError, "Empty list.");
        return NULL;
    }

    double sum = 0.0;
    for (Py_ssize_t i = 0; i < size; i++) sum += arr[i];

    free(arr);
    return PyFloat_FromDouble(sum / size);
}

/*
----------------------------------------------------------
 std_dev
----------------------------------------------------------
 Returns the sample-based standard deviation.
 Algorithm:
   1. Compute mean
   2. Sum squared differences
   3. Divide by (n-1) for sample std dev
   4. Return sqrt of variance
 Time Complexity: O(n)
 Memory: O(n)
 Error: Raises ValueError if fewer than 2 values.
*/
static PyObject* std_dev(PyObject *self, PyObject *args) {
    PyObject *listObj;
    if (!PyArg_ParseTuple(args, "O", &listObj)) return NULL;

    double *arr;
    Py_ssize_t size;
    if (!pylist_to_carray(listObj, &arr, &size)) return NULL;

    if (size < 2) {
        free(arr);
        PyErr_SetString(PyExc_ValueError, "At least two values required.");
        return NULL;
    }

    double sum = 0.0;
    for (Py_ssize_t i = 0; i < size; i++) sum += arr[i];
    double mean = sum / size;

    double variance = 0.0;
    for (Py_ssize_t i = 0; i < size; i++) {
        double diff = arr[i] - mean;
        variance += diff * diff;
    }
    variance /= (size - 1); // Sample-based

    free(arr);
    return PyFloat_FromDouble(sqrt(variance));
}

/*
----------------------------------------------------------
 mode_value
----------------------------------------------------------
 Returns the mode (most frequent value) using a naive O(n^2) algorithm.
 Algorithm:
   1. For each element, count frequency by comparing with all subsequent elements.
   2. Track the highest frequency and corresponding value.
 Time Complexity: O(n^2)
 Memory: O(n)
*/
static PyObject* mode_value(PyObject *self, PyObject *args) {
    PyObject *listObj;
    if (!PyArg_ParseTuple(args, "O", &listObj)) return NULL;

    double *arr;
    Py_ssize_t size;
    if (!pylist_to_carray(listObj, &arr, &size)) return NULL;

    if (size == 0) {
        free(arr);
        PyErr_SetString(PyExc_ValueError, "Empty list.");
        return NULL;
    }

    double mode = arr[0];
    int max_count = 1;

    for (Py_ssize_t i = 0; i < size; i++) {
        int count = 1;
        for (Py_ssize_t j = i + 1; j < size; j++) {
            if (arr[i] == arr[j]) count++;
        }
        if (count > max_count) {
            max_count = count;
            mode = arr[i];
        }
    }

    free(arr);
    return PyFloat_FromDouble(mode);
}

/*
----------------------------------------------------------
 count_values
----------------------------------------------------------
 Returns the number of values in the list.
 Algorithm: Uses PyList_Size directly.
 Time Complexity: O(1)
 Memory: No additional memory allocation.
*/
static PyObject* count_values(PyObject *self, PyObject *args) {
    PyObject *listObj;
    if (!PyArg_ParseTuple(args, "O", &listObj)) return NULL;

    if (!PyList_Check(listObj)) {
        PyErr_SetString(PyExc_TypeError, "Expected a list.");
        return NULL;
    }

    Py_ssize_t size = PyList_Size(listObj);
    return PyLong_FromSsize_t(size);
}

/* 
----------------------------------------------------------
 Method Table
----------------------------------------------------------
 Maps Python function names to C implementations.
*/
static PyMethodDef CStatsMethods[] = {
    {"sum_values", sum_values, METH_VARARGS, "Return the sum of float values."},
    {"mean_values", mean_values, METH_VARARGS, "Return the mean of values."},
    {"std_dev", std_dev, METH_VARARGS, "Return the sample standard deviation."},
    {"mode_value", mode_value, METH_VARARGS, "Return the mode (most frequent value)."},
    {"count_values", count_values, METH_VARARGS, "Return the number of values in the list."},
    {NULL, NULL, 0, NULL}
};

/* 
----------------------------------------------------------
 Module Definition
----------------------------------------------------------
*/
static struct PyModuleDef cstatsmodule = {
    PyModuleDef_HEAD_INIT,
    "cstats",
    "C extension for statistical operations.",
    -1,
    CStatsMethods
};

/*
----------------------------------------------------------
 Module Initializer
----------------------------------------------------------
 Initializes the cstats module.
*/
PyMODINIT_FUNC PyInit_cstats(void) {
    return PyModule_Create(&cstatsmodule);
}
