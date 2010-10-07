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

#include "pyev.h"
#include "pyevdoc.h" //documentation


/*******************************************************************************
* types
*******************************************************************************/

#include "Loop.c"
#include "Watcher.c"
#include "Io.c"
#include "Timer.c"
#include "Periodic.c"
#include "Signal.c"
#include "Child.c"
#include "Stat.c"
#include "Idle.c"
#include "Prepare.c"
#include "Check.c"
#include "Embed.c"
#include "Fork.c"
#include "Async.c"


/*******************************************************************************
* utilities
*******************************************************************************/

/* Create and document an exception */
PyObject *
PyErr_Create(char *name, const char *doc, PyObject *base)
{
    PyObject *dict = NULL;
    PyObject *_doc, *err;

    if (doc) {
        dict = PyDict_New();
        if (!dict) {
            return NULL;
        }
        _doc = PyString_FromString(doc);
        if (!_doc || PyDict_SetItemString(dict, "__doc__", _doc)) {
            Py_XDECREF(_doc);
            Py_DECREF(dict);
            return NULL;
        }
        Py_DECREF(_doc);
    }
    err = PyErr_NewException(name, base, dict);
    Py_XDECREF(dict);
    return err;
}


/* Add a type to a module */
int
PyModule_AddType(PyObject *module, const char *name, PyTypeObject *type)
{
    Py_INCREF(type);
    if (PyModule_AddObject(module, name, (PyObject *)type)) {
        Py_DECREF(type);
        return -1;
    }
    return 0;
}


/* Add an unsigned integer constant to a module */
int
PyModule_AddUnsignedIntConstant(PyObject *module, const char *name,
                                unsigned long value)
{
    PyObject *object = PyInt_FromUnsignedLong(value);
    if (!object) {
        return -1;
    }
    if (PyModule_AddObject(module, name, object)) {
        Py_DECREF(object);
        return -1;
    }
    return 0;
}


/* allocate memory from the Python heap - this is a bit messy */
static void *
pyev_allocator(void *ptr, long size)
{
#ifdef PYMALLOC_DEBUG
    PyGILState_STATE gstate = PyGILState_Ensure();
#endif /* PYMALLOC_DEBUG */
    void *result = NULL;

    if (size > 0) {
#if SIZEOF_LONG > SIZEOF_SIZE_T
        if ((unsigned long)size <= (unsigned long)PY_SIZE_MAX) {
            result = PyMem_Realloc(ptr, (size_t)size);
        }
#else
        result = PyMem_Realloc(ptr, (size_t)size);
#endif /* SIZEOF_LONG > SIZEOF_SIZE_T */
    }
    else if (!size) {
        PyMem_Free(ptr);
    }
#ifdef PYMALLOC_DEBUG
    PyGILState_Release(gstate);
#endif /* PYMALLOC_DEBUG */
    return result;
}


/* syscall errors will call Py_FatalError */
static void
pyev_syserr_cb(const char *msg)
{
    PyGILState_Ensure();
    if (PyErr_Occurred()) {
        PyErr_Print();
    }
    Py_FatalError(msg);
}


/*******************************************************************************
* pyev_module
*******************************************************************************/

/* pyev.version() -> (str, str) */
static PyObject *
pyev_version(PyObject *module)
{
    return Py_BuildValue("(ss)", PYEV_VERSION, LIBEV_VERSION);
}


/* pyev.abi_version() -> (int, int) */
static PyObject *
pyev_abi_version(PyObject *module)
{
    return Py_BuildValue("(ii)", ev_version_major(), ev_version_minor());
}


/* pyev.time() -> float */
static PyObject *
pyev_time(PyObject *module)
{
    return PyFloat_FromDouble(ev_time());
}


/* pyev.sleep(interval) */
static PyObject *
pyev_sleep(PyObject *module, PyObject *args)
{
    double interval;

    if (!PyArg_ParseTuple(args, "d:sleep", &interval)) {
        return NULL;
    }
    Py_BEGIN_ALLOW_THREADS
    ev_sleep(interval);
    Py_END_ALLOW_THREADS
    Py_RETURN_NONE;
}


/* pyev.supported_backends() -> int */
static PyObject *
pyev_supported_backends(PyObject *module)
{
    return PyInt_FromUnsignedLong(ev_supported_backends());
}


/* pyev.recommended_backends() -> int */
static PyObject *
pyev_recommended_backends(PyObject *module)
{
    return PyInt_FromUnsignedLong(ev_recommended_backends());
}


/* pyev.embeddable_backends() -> int */
static PyObject *
pyev_embeddable_backends(PyObject *module)
{
    return PyInt_FromUnsignedLong(ev_embeddable_backends());
}


/* pyev.default_loop([flags=EVFLAG_AUTO, pending_cb=None, data=None, debug=False,
                      io_interval=0.0, timeout_interval=0.0]) -> 'default_loop' */
static PyObject *
pyev_default_loop(PyObject *module, PyObject *args, PyObject *kwargs)
{
    if (!DefaultLoop) {
        DefaultLoop = new_Loop(&LoopType, args, kwargs, 1);
        if (!DefaultLoop) {
            return NULL;
        }
    }
    else {
        if (PyErr_WarnEx(PyExc_UserWarning, "returning the 'default loop' "
                "created earlier, arguments ignored (if provided).", 1)) {
            return NULL;
        }
        Py_INCREF(DefaultLoop);
    }
    return (PyObject *)DefaultLoop;
}


/* pyev_module.m_methods */
static PyMethodDef pyev_m_methods[] = {
    {"version", (PyCFunction)pyev_version, METH_NOARGS, pyev_version_doc},
    {"abi_version", (PyCFunction)pyev_abi_version,
     METH_NOARGS, pyev_abi_version_doc},
    {"time", (PyCFunction)pyev_time, METH_NOARGS, pyev_time_doc},
    {"sleep", (PyCFunction)pyev_sleep, METH_VARARGS, pyev_sleep_doc},
    {"supported_backends", (PyCFunction)pyev_supported_backends,
     METH_NOARGS, pyev_supported_backends_doc},
    {"recommended_backends", (PyCFunction)pyev_recommended_backends,
     METH_NOARGS, pyev_recommended_backends_doc},
    {"embeddable_backends", (PyCFunction)pyev_embeddable_backends,
     METH_NOARGS, pyev_embeddable_backends_doc},
    {"default_loop", (PyCFunction)pyev_default_loop,
     METH_VARARGS | METH_KEYWORDS, pyev_default_loop_doc},
    {NULL} /* Sentinel */
};


#if PY_MAJOR_VERSION >= 3
/* pyev_module */
static PyModuleDef pyev_module = {
    PyModuleDef_HEAD_INIT,
    "pyev",                                   /*m_name*/
    pyev_m_doc,                               /*m_doc*/
    -1,                                       /*m_size*/
    pyev_m_methods,                           /*m_methods*/
};
#endif /* PY_MAJOR_VERSION >= 3 */


/* pyev_module initialization */
PyObject *
init_pyev(void)
{
    PyObject *pyev, *version;

    /* fill in deferred data addresses */
    WatcherType.tp_new = PyType_GenericNew;
    IoType.tp_base = &WatcherType;
    TimerType.tp_base = &WatcherType;
    PeriodicType.tp_base = &WatcherType;
    SignalType.tp_base = &WatcherType;
    ChildType.tp_base = &WatcherType;
    StatType.tp_base = &WatcherType;
    IdleType.tp_base = &WatcherType;
    PrepareType.tp_base = &WatcherType;
    CheckType.tp_base = &WatcherType;
    EmbedType.tp_base = &WatcherType;
    ForkType.tp_base = &WatcherType;
    AsyncType.tp_base = &WatcherType;

    /* checking types */
    if (
        PyType_Ready(&LoopType) ||
        PyType_Ready(&WatcherType) ||
        PyType_Ready(&IoType) ||
        PyType_Ready(&TimerType) ||
        PyType_Ready(&PeriodicType) ||
        PyType_Ready(&SignalType) ||
        PyType_Ready(&ChildType) ||
        PyType_Ready(&StatType) ||
        PyType_Ready(&IdleType) ||
        PyType_Ready(&PrepareType) ||
        PyType_Ready(&CheckType) ||
        PyType_Ready(&EmbedType) ||
        PyType_Ready(&ForkType) ||
        PyType_Ready(&AsyncType)
       ) {
        return NULL;
    }
    /* init StatdataType */
    if (!initialized) {
        PyStructSequence_InitType(&StatdataType, &Statdata_desc);
    }
    initialized = 1;
    /* pyev */
#if PY_MAJOR_VERSION >= 3
    pyev = PyModule_Create(&pyev_module);
#else
    pyev = Py_InitModule3("pyev", pyev_m_methods, pyev_m_doc);
#endif /* PY_MAJOR_VERSION >= 3 */
    if (!pyev) {
        return NULL;
    }
    /* pyev.Error */
    Error = PyErr_Create("pyev.Error", pyev_Error_doc, NULL);
    if (!Error || PyModule_AddObject(pyev, "Error", Error)) {
        Py_XDECREF(Error);
        goto fail;
    }
    /* pyev.__version__ */
    version = PyString_FromFormat("%s-%s", PYEV_VERSION, LIBEV_VERSION);
    if (!version || PyModule_AddObject(pyev, "__version__", version)) {
        Py_XDECREF(version);
        goto fail;
    }
    /* adding types and constants */
    if (
        /* types */
        PyModule_AddType(pyev, "Loop", &LoopType) ||
        PyModule_AddType(pyev, "Io", &IoType) ||
        PyModule_AddType(pyev, "Timer", &TimerType) ||
        PyModule_AddType(pyev, "Periodic", &PeriodicType) ||
        PyModule_AddType(pyev, "Signal", &SignalType) ||
        PyModule_AddType(pyev, "Child", &ChildType) ||
        PyModule_AddType(pyev, "Stat", &StatType) ||
        PyModule_AddType(pyev, "Idle", &IdleType) ||
        PyModule_AddType(pyev, "Prepare", &PrepareType) ||
        PyModule_AddType(pyev, "Check", &CheckType) ||
        PyModule_AddType(pyev, "Embed", &EmbedType) ||
        PyModule_AddType(pyev, "Fork", &ForkType) ||
        PyModule_AddType(pyev, "Async", &AsyncType) ||
        /* Loop() and default_loop() flags */
        PyModule_AddUnsignedIntConstant(
            pyev, "EVFLAG_AUTO", (unsigned int)EVFLAG_AUTO) ||
        PyModule_AddUnsignedIntConstant(
            pyev, "EVFLAG_NOENV", (unsigned int)EVFLAG_NOENV) ||
        PyModule_AddUnsignedIntConstant(
            pyev, "EVFLAG_FORKCHECK", (unsigned int)EVFLAG_FORKCHECK) ||
        PyModule_AddUnsignedIntConstant(
            pyev, "EVFLAG_NOINOTIFY", (unsigned int)EVFLAG_NOINOTIFY) ||
        PyModule_AddUnsignedIntConstant(
            pyev, "EVFLAG_SIGNALFD", (unsigned int)EVFLAG_SIGNALFD) ||
        /* backends */
        PyModule_AddUnsignedIntConstant(
            pyev, "EVBACKEND_SELECT", (unsigned int)EVBACKEND_SELECT) ||
        PyModule_AddUnsignedIntConstant(
            pyev, "EVBACKEND_POLL", (unsigned int)EVBACKEND_POLL) ||
        PyModule_AddUnsignedIntConstant(
            pyev, "EVBACKEND_EPOLL", (unsigned int)EVBACKEND_EPOLL) ||
        PyModule_AddUnsignedIntConstant(
            pyev, "EVBACKEND_KQUEUE", (unsigned int)EVBACKEND_KQUEUE) ||
        PyModule_AddUnsignedIntConstant(
            pyev, "EVBACKEND_DEVPOLL", (unsigned int)EVBACKEND_DEVPOLL) ||
        PyModule_AddUnsignedIntConstant(
            pyev, "EVBACKEND_PORT", (unsigned int)EVBACKEND_PORT) ||
        PyModule_AddUnsignedIntConstant(
            pyev, "EVBACKEND_ALL", (unsigned int)EVBACKEND_ALL) ||
        /* Loop.loop() flags */
        PyModule_AddIntConstant(pyev, "EVLOOP_NONBLOCK", (int)EVLOOP_NONBLOCK) ||
        PyModule_AddIntConstant(pyev, "EVLOOP_ONESHOT", (int)EVLOOP_ONESHOT) ||
        /* Loop.unloop() how */
        PyModule_AddIntConstant(pyev, "EVUNLOOP_ONE", (int)EVUNLOOP_ONE) ||
        PyModule_AddIntConstant(pyev, "EVUNLOOP_ALL", (int)EVUNLOOP_ALL) ||
        /* priorities */
        PyModule_AddIntConstant(pyev, "EV_MINPRI", (int)EV_MINPRI) ||
        PyModule_AddIntConstant(pyev, "EV_MAXPRI", (int)EV_MAXPRI) ||
        /* events */
        PyModule_AddIntConstant(pyev, "EV_READ", (int)EV_READ) ||
        PyModule_AddIntConstant(pyev, "EV_WRITE", (int)EV_WRITE) ||
        PyModule_AddIntConstant(pyev, "EV_TIMER", (int)EV_TIMER) ||
        PyModule_AddIntConstant(pyev, "EV_PERIODIC", (int)EV_PERIODIC) ||
        PyModule_AddIntConstant(pyev, "EV_SIGNAL", (int)EV_SIGNAL) ||
        PyModule_AddIntConstant(pyev, "EV_CHILD", (int)EV_CHILD) ||
        PyModule_AddIntConstant(pyev, "EV_STAT", (int)EV_STAT) ||
        PyModule_AddIntConstant(pyev, "EV_IDLE", (int)EV_IDLE) ||
        PyModule_AddIntConstant(pyev, "EV_PREPARE", (int)EV_PREPARE) ||
        PyModule_AddIntConstant(pyev, "EV_CHECK", (int)EV_CHECK) ||
        PyModule_AddIntConstant(pyev, "EV_EMBED", (int)EV_EMBED) ||
        PyModule_AddIntConstant(pyev, "EV_FORK", (int)EV_FORK) ||
        PyModule_AddIntConstant(pyev, "EV_ASYNC", (int)EV_ASYNC) ||
        PyModule_AddIntConstant(pyev, "EV_CUSTOM", (int)EV_CUSTOM) ||
        PyModule_AddIntConstant(pyev, "EV_ERROR", (int)EV_ERROR)
       ) {
        goto fail;
    }
    /* setup libev */
    ev_set_allocator(pyev_allocator);
    ev_set_syserr_cb(pyev_syserr_cb);
    return pyev;

fail:
#if PY_MAJOR_VERSION >= 3
    Py_DECREF(pyev);
#endif /* PY_MAJOR_VERSION >= 3 */
    return NULL;
}

#if PY_MAJOR_VERSION >= 3
PyMODINIT_FUNC
PyInit_pyev(void)
{
    return init_pyev();
}
#else
PyMODINIT_FUNC
initpyev(void)
{
    init_pyev();
}
#endif /* PY_MAJOR_VERSION >= 3 */
