/**
 * @file netpy.h
 * C wrapper function header file for the Python interface to the Net Services Library.
 * Function definitions for the wrapper functions are located in this file.  In order
 * to use this library, type:
 * import netpy
 * in Python.  The location of netpy.so (nominally, the top level library directory)
 * must be in your PYTHONPATH environment variable.  For a complete description of the
 * C API, see net_API.lp.txt in the doc directory.
 *
 * @author Phil Irwin
 * @par Address:
 * JPL
 * 4800 Oak Grove Drive
 * M/S T1721
 * Pasadena, CA  91109
 * @par Copyright:
 * (c) 2015, California Institute of Technology, Pasadena, CA
 * @version $Revision$
 * @date $Date$
 *
 */

#ifndef NETPY_H
#define NETPY_H

#include <Python.h>

static PyObject *raise_exception(int errorNum);

/**
 * Interface to net_init().
 * net_init() initializes a network connection to a given endpoint string.  The
 * name of the function in C is netpy_init.  From Python, the function name is net_init()
 * and its prototype is as follows:
 *
 * int net_init(string endpoint_name)
 *
 * @param[in] endpoint_name Endpoint name of socket to connect to (string)
 * @returns On success, returns socket file descriptor (integer) that can be passed into 
 *          net_accept().  On failure, raises OSError exeption.
 */
static PyObject *netpy_init(PyObject *self, PyObject *args);

/**
 * Interface to net_connect().
 * net_connect() connects to a server on a given endpoint name.  The name of the function
 * in C is netpy_connect.  From Python, the function name is net_connect() and its prototype
 * is as follows:
 *
 * int net_connect(string endpoint_name, string hostname, int pname, int blocking)
 *
 * @param[in] endpoint_name Endpoint name of socket to connect to (string).
 * @param[in] hostname      The hostname to connect to (string).
 * @param[in] pname         Peer name used to identify this user to the server (integer).
 * @param[in] blocking      Takes a 0 (blocking) or a 1 (non-blocking) to tell whether or
 *                          not this function will block waiting for the connection or
 *                          simply return.
 * @returns On success, returns socket file descriptor (integer) that can be used to
 *          communicate with the server via net_send() and net_recv().  On failure, raises
 *          OSError Exception.
 */
static PyObject *netpy_connect(PyObject *self, PyObject *args);

/**
 * Interface to net_accept().
 * net_accept() waits for connections on a given socket descriptor.  The name of the
 * function in C is netpy_accept.  From Python, the function name is net_accept() and
 * its prototype is as follows:
 *
 * int net_accept(int socket, int blocking)
 *
 * @param[in] socket   The socket file descriptor on which to wait (integer).
 * @param[in] blocking Takes a 0 (blocking) or a 1 (non-blocking) to tell whether or
 *                     not this function will block waiting for the connection or
 *                     simply return.
 * @returns On success, returns socket file descriptor (integer) that can be used to
 *          communicate with the client via net_send() and net_recv().  On failure,
 *          raises OSError exception.
 */
static PyObject *netpy_accept(PyObject *self, PyObject *args);

/**
 * Interface to net_send().
 * net_send() sends a string to a host on a connected socket.  The name of the function
 * in C is netpy_send.  From Python, the function name is net_send() and its prototype is
 * as follows:
 *
 * int net_send(int socket, bytes msg, int blocking)
 *
 * @param[in] socket   The socket to send the data out (integer).
 * @param[in] msg      The message to send the host (bytes).
 * @param[in] blocking Takes a 0 (blocking) or a 1 (non-blocking) to tell whether or
 *                     not this function will block waiting for the read or return.
 * @returns On success, returns the number of bytes sent.  On failure, raises OSError
 *          exception.
 */
static PyObject *netpy_send(PyObject *self, PyObject *args);

/**
 * Interface to net_recv().
 * net_recv() receives a string from a host on a connected socket.  The name of the
 * function in C is netpy_recv.  From Python, the function name is net_recv() and its
 * prototype is as follows:
 *
 * bytes net_recv(int socket, int maxlen, int blocking)
 * 
 * @param[in] socket The socket on which to receive the data (integer).
 * @param[in] maxlen The maximum length of data to receive (integer).
 * @param[in] blocking Takes a 0 (blocking) or a 1 (non-blocking) to tell whether or
 *                     not this function will block waiting for the data or return.
 * @returns On success, returns data sent from remote host
 *          On failure, raises OSError exception
 */
static PyObject *netpy_recv(PyObject *self, PyObject *args);

/**
 * Interface to net_close().
 * net_close() closes a socket.  The name of the function in C is netpy_close.  From
 * Python, the function name is net_close() and its prototype is as follows:
 *
 * int net_close(int socket)
 *
 * @param[in] socket The socket descriptor to close (integer).
 * @returns On success, returns SUCCESS,
 *          On failure, raises, OSError exception.
 */
static PyObject *netpy_close(PyObject *self, PyObject *args);

#endif
