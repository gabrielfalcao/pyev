/*******************************************************************************
*
* Copyright (c) 2009, 2010 Malek Hadj-Ali
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
* 3. Neither the name of the copyright holders nor the names of its contributors
*    may be used to endorse or promote products derived from this software
*    without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
* THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
* OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
* OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
* IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
* THE POSSIBILITY OF SUCH DAMAGE.
*
*
* Alternatively, the contents of this file may be used under the terms of the
* GNU General Public License (the GNU GPL) version 3 or (at your option) any
* later version, in which case the provisions of the GNU GPL are applicable
* instead of those of the modified BSD license above.
* If you wish to allow use of your version of this file only under the terms
* of the GNU GPL and not to allow others to use your version of this file under
* the modified BSD license above, indicate your decision by deleting
* the provisions above and replace them with the notice and other provisions
* required by the GNU GPL. If you do not delete the provisions above,
* a recipient may use your version of this file under either the modified BSD
* license above or the GNU GPL.
*
*******************************************************************************/

#ifndef _PYEV_H
#define _PYEV_H


#define PY_SSIZE_T_CLEAN
#include "Python.h"
#include "structmember.h"
#include "structseq.h"

/* set EV_VERIFY */
#ifndef EV_VERIFY
#ifdef Py_DEBUG
#define EV_VERIFY 3
#else
#define EV_VERIFY 0
#endif /* Py_DEBUG */
#endif /* !EV_VERIFY */

/* pyev requirements */
#undef EV_FEATURES
#undef EV_MULTIPLICITY
#undef EV_PERIODIC_ENABLE
#undef EV_SIGNAL_ENABLE
#undef EV_CHILD_ENABLE
#undef EV_STAT_ENABLE
#undef EV_IDLE_ENABLE
#undef EV_PREPARE_ENABLE
#undef EV_CHECK_ENABLE
#undef EV_EMBED_ENABLE
#undef EV_FORK_ENABLE
#undef EV_ASYNC_ENABLE

#include "libev/ev.c"


/*******************************************************************************
* objects
*******************************************************************************/

/* Error */
static PyObject *Error;


/* Loop */
typedef struct {
    PyObject_HEAD
    struct ev_loop *loop;
    PyObject *pending_cb;
    PyObject *data;
    char debug;
} Loop;

/* the 'default_loop' */
Loop *DefaultLoop = NULL;


/* Watcher - not exposed */
typedef struct {
    PyObject_HEAD
    ev_watcher *watcher;
    int type;
    Loop *loop;
    PyObject *callback;
    PyObject *data;
} Watcher;


/* Io */
typedef struct {
    Watcher watcher;
    ev_io io;
} Io;
static PyTypeObject IoType;


/* Timer */
typedef struct {
    Watcher watcher;
    ev_timer timer;
} Timer;
static PyTypeObject TimerType;


/* Periodic */
typedef struct {
    Watcher watcher;
    ev_periodic periodic;
    PyObject *reschedule_cb;
} Periodic;
static PyTypeObject PeriodicType;


/* Signal */
typedef struct {
    Watcher watcher;
    ev_signal signal;
} Signal;
static PyTypeObject SignalType;


/* Child */
typedef struct {
    Watcher watcher;
    ev_child child;
} Child;
static PyTypeObject ChildType;


/* Stat */
static int initialized;
static PyTypeObject StatdataType;

typedef struct {
    Watcher watcher;
    ev_stat stat;
    PyObject *attr;
    PyObject *prev;
} Stat;
static PyTypeObject StatType;


/* Idle */
typedef struct {
    Watcher watcher;
    ev_idle idle;
} Idle;
static PyTypeObject IdleType;


/* Prepare */
typedef struct {
    Watcher watcher;
    ev_prepare prepare;
} Prepare;
static PyTypeObject PrepareType;


/* Check */
typedef struct {
    Watcher watcher;
    ev_check check;
} Check;
static PyTypeObject CheckType;


/* Embed */
typedef struct {
    Watcher watcher;
    ev_embed embed;
    Loop *other;
} Embed;
static PyTypeObject EmbedType;


/* Fork */
typedef struct {
    Watcher watcher;
    ev_fork fork;
} Fork;
static PyTypeObject ForkType;


/* Async */
typedef struct {
    Watcher watcher;
    ev_async async;
} Async;
static PyTypeObject AsyncType;


/*******************************************************************************
* utilities
*******************************************************************************/

#if PY_MAJOR_VERSION >= 3
#define PyString_FromFormat PyUnicode_FromFormat
#define PyString_FromString PyUnicode_FromString
#define PyInt_FromLong PyLong_FromLong
#define PyInt_FromUnsignedLong PyLong_FromUnsignedLong
#define PyString_FromPath PyUnicode_DecodeFSDefault
#else
PyObject *
PyInt_FromUnsignedLong(unsigned long value)
{
    if (value > INT_MAX) {
        return PyLong_FromUnsignedLong(value);
    }
    return PyInt_FromLong((long)value);
}
#define PyString_FromPath PyString_FromString
#endif /* PY_MAJOR_VERSION >= 3 */


/* Py[Int/Long] -> int */
int
PyNum_AsInt(PyObject *pyvalue)
{
    long value;

    value = PyLong_AsLong(pyvalue);
    if (value == -1 && PyErr_Occurred()) {
        return -1;
    }
    if (value < INT_MIN) {
        PyErr_SetString(PyExc_OverflowError, "int is less than minimum");
        return -1;
    }
    if (value > INT_MAX) {
        PyErr_SetString(PyExc_OverflowError, "int is greater than maximum");
        return -1;
    }
    return (int)value;
}


/* I need to investigate how the 100 opcodes rule works out exactly for the GIL.
   Until then, better safe than sorry :). */
#define PYEV_GIL_ENSURE \
    { \
        PyGILState_STATE gstate = PyGILState_Ensure(); \
        PyObject *err_type, *err_value, *err_traceback; \
        int had_error = PyErr_Occurred() ? 1 : 0; \
        if (had_error) { \
            PyErr_Fetch(&err_type, &err_value, &err_traceback); \
        }

#define PYEV_GIL_RELEASE \
        if (had_error) { \
            PyErr_Restore(err_type, err_value, err_traceback); \
        } \
        PyGILState_Release(gstate); \
    }


/* check for a positive float */
int
positive_float(double value)
{
    if (value < 0.0) {
        PyErr_SetString(PyExc_ValueError, "a positive float or 0.0 is required");
        return 0;
    }
    return 1;
}


#endif /* _PYEV_H */
