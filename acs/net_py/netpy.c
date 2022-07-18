/**
 * @file netpy.c
 *  C wrapper functions for the Python interface to the Net Services Library.
 *  The Net Services Library is the standard network library for the LCRD MCS
 *  subsystem.  In order for any Python code to access this library, these
 *  wrapper functions are required.  This file gets built into a shared object
 *  file (netpy.so) and lives in the top level library directory.
 *
 * @par Project
 *  LCRD Optical Ground Station (OGS-1) \n
 *  Jet Propulsion Laboratory, Pasadena, CA
 *
 * @author Phil Irwin
 *
 * @date 30-Jun-2016 -- Build 2 delivery
 * @date 03-Apr-2017 -- Build 4 delivery
 *
 * Copyright (c) 2016-2020, California Institute of Technology
 */

#include <stdlib.h>
#include "net_appl.h"
#include "netpy.h"


static PyMethodDef net_methods[] = {
    {"net_init", netpy_init, METH_VARARGS,
     "Initialize the network."},
    {"net_connect", netpy_connect, METH_VARARGS,
     "Make a network connection (client side)."},
    {"net_accept", netpy_accept, METH_VARARGS,
     "Make a network connection (server side)."},
    {"net_send", netpy_send, METH_VARARGS,
     "Send data on existing network connection."},
    {"net_recv", netpy_recv, METH_VARARGS,
     "Receive data from existing network connection."},
    {"net_close", netpy_close, METH_VARARGS,
     "Close an existing network connection."},
    {NULL, NULL, 0, NULL}           /* sentinel */
};

static struct PyModuleDef netpy = {
   PyModuleDef_HEAD_INIT,
   "net", /* name of module */
   NULL,  /* module documentation, may be NULL */
   -1,    /* size of per-interpreter state of the module,
             or -1 if the module keeps state in global variables. */
   net_methods
};

PyMODINIT_FUNC
PyInit_netpy(void)
{
    return PyModule_Create(&netpy);
}

static PyObject *raise_exception(int errorNum)
{
  if (errorNum == -1) {
    return PyErr_SetFromErrno(PyExc_OSError);
  } else if (errorNum < -1) {
    PyErr_SetObject(PyExc_OSError, Py_BuildValue("is", errorNum, NET_ERRSTR(errorNum)));
    return NULL;
  } else {
    return NULL;
  }
}

static PyObject *netpy_init(PyObject *self, PyObject *args)
{
  char *endpt;
  int retval;
  PyObject *obj;

  if (!PyArg_ParseTuple(args, "s", &endpt))
    return NULL;
  retval = net_init(endpt);
  if (retval < 0)
    return raise_exception(retval);
  
  obj = Py_BuildValue("i", retval);
  Py_DECREF(&retval); 
  return obj;
}

static PyObject *netpy_connect(PyObject *self, PyObject *args)
{
  char *endpt, *hostname;
  int pname, mode;
  int retval;
  PyObject *obj;

  if (!PyArg_ParseTuple(args, "ssii", &endpt, &hostname, &pname, &mode))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  retval = net_connect(endpt, hostname, pname, mode);
  Py_END_ALLOW_THREADS
  if (retval < 0)
    return raise_exception(retval);

  obj = Py_BuildValue("i", retval);
  Py_DECREF(&retval);
  return obj;
}

static PyObject *netpy_accept(PyObject *self, PyObject *args)
{
  int sockfd, mode;
  int retval;
  PyObject *obj;

  if (!PyArg_ParseTuple(args, "ii", &sockfd, &mode))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  retval = net_accept(sockfd, mode);
  Py_END_ALLOW_THREADS
  if (retval < 0)
    return raise_exception(retval);

  obj = Py_BuildValue("i", retval);
  Py_DECREF(&retval);
  return obj;
}

static PyObject *netpy_send(PyObject *self, PyObject *args)
{
  char *msg;
  int sockfd, len, mode;
  int retval;

  if (!PyArg_ParseTuple(args, "is#i", &sockfd, &msg, &len, &mode))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
  retval = net_send(sockfd, msg, len, mode);
  Py_END_ALLOW_THREADS
  if (retval < 0)
    return raise_exception(retval);

  return Py_BuildValue("i", retval);
}

static PyObject *netpy_recv(PyObject *self, PyObject *args)
{
  char *buf;
  int sockfd, maxlen, mode, len;
  PyObject *retval;

  if (!PyArg_ParseTuple(args, "iii", &sockfd, &maxlen, &mode))
    return NULL;

  buf = (char *) malloc(maxlen);
  Py_BEGIN_ALLOW_THREADS
  len = net_recv(sockfd, buf, maxlen, mode);
  Py_END_ALLOW_THREADS
  if (len < 0) {
    free(buf);
    return raise_exception(len);
  }
  retval = Py_BuildValue("y#", buf, len);
  free(buf);
  return retval;
}

static PyObject *netpy_close(PyObject *self, PyObject *args)
{
  int sockfd;
  int retval;

  if (!PyArg_ParseTuple(args, "i", &sockfd))
    return NULL;
  retval = net_close(sockfd);
  if (retval < 0)
    return raise_exception(retval);

  return Py_BuildValue("i", retval);
}
