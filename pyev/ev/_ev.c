/*
 Copyright (C) 2008 Malek Hadj-Ali

 This file is part of 'pyev' python package.

 'pyev' is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License version 2 as published
 by the Free Software Foundation.

 'pyev' is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this package; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/


#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "structmember.h"

#include <ev.h>

#define _EV_VERSION "0.1.1"


/* The module object */
PyObject *_ev;


/* ev.Error exception */
static PyObject *EvError;


/* Loop */
typedef struct {
    PyObject_HEAD
    struct ev_loop *loop;
} Loop;

/* the 'default loop' */
Loop *_DefaultLoop = NULL;


/* _Watcher - not exposed */
typedef struct {
    PyObject_HEAD
    ev_watcher *watcher;
    Loop *loop;
    PyObject *callback;
    PyObject *data;
} _Watcher;


/* Io */
typedef struct {
    _Watcher _watcher;
    ev_io watcher;
} Io;


/* Timer */
typedef struct {
    _Watcher _watcher;
    ev_timer watcher;
} Timer;


/* Periodic */
typedef struct {
    _Watcher _watcher;
    ev_periodic watcher;
    PyObject *reschedule_cb;
} Periodic;


/* Signal */
typedef struct {
    _Watcher _watcher;
    ev_signal watcher;
} Signal;


/* Child */
typedef struct {
    _Watcher _watcher;
    ev_child watcher;
} Child;


/* Statdata */
typedef struct {
    PyObject_HEAD
    ev_statdata statdata;
} Statdata;


/* Stat */
typedef struct {
    _Watcher _watcher;
    ev_stat watcher;
} Stat;


/* Idle */
typedef struct {
    _Watcher _watcher;
    ev_idle watcher;
} Idle;


/* Prepare */
typedef struct {
    _Watcher _watcher;
    ev_prepare watcher;
} Prepare;


/* Check */
typedef struct {
    _Watcher _watcher;
    ev_check watcher;
} Check;


/* Fork */
typedef struct {
    _Watcher _watcher;
    ev_fork watcher;
} Fork;


/* Embed */
typedef struct {
    _Watcher _watcher;
    ev_embed watcher;
    Loop *other;
} Embed;


/* Async */
typedef struct {
    _Watcher _watcher;
    ev_async watcher;
} Async;


/* LoopType.tp_doc */
PyDoc_STRVAR(Loop_doc,
"Loop([flags])\n\n\
The 'flags' argument can be used to specify special behaviour or specific\n\
backends to use, it defaults to EVFLAG_AUTO. See documentation for EVFLAG_*\n\
and EVBACKEND_* at http://pod.tst.eu/http://cvs.schmorp.de/libev/ev.pod for\n\
more information.");


/* new - instanciate and init the Loop*/
static Loop *
new_loop(PyTypeObject *type, unsigned int flags, int default_loop)
{
    Loop *self;

    self = (Loop *)type->tp_alloc(type, 0);
    if (self == NULL) {
        return NULL;
    }

    /* self->loop */
    if (default_loop) {
        self->loop = ev_default_loop(flags);
    }
    else {
        self->loop = ev_loop_new(flags);
    }

    if (!self->loop) {
        PyErr_SetString(EvError, "could not initialise loop, bad 'flags'?");
        Py_DECREF(self);
        return NULL;
    }

    return self;
}


/* LoopType.tp_dealloc */
static void
Loop_dealloc(Loop *self)
{
    if (self->loop) {
        if (ev_is_default_loop(self->loop)) {
            ev_default_destroy();
        }
        else {
            ev_loop_destroy(self->loop);
        }
    }

    self->ob_type->tp_free((PyObject*)self);
}


/* LoopType.tp_new */
static PyObject *
Loop_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    unsigned int flags = EVFLAG_AUTO;

    if (!PyArg_ParseTuple(args, "|I:__new__", &flags)) {
        return NULL;
    }

    return (PyObject *)new_loop(type, flags, 0);
}


/* Loop.fork */
PyDoc_STRVAR(Loop_fork_doc,
"fork()\n\n\
This method sets a flag that causes subsequent loop iterations to reinitialise\n\
the kernel state for backends that have one. Despite the name, you can call it\n\
anytime, but it makes most sense after forking, in the child process (or both\n\
child and parent, but that again makes little sense). You must call it in the\n\
child before using any of the libev functions, and it will only take effect at\n\
the next loop iteration.\n\
On the other hand, you only need to call this method in the child process\n\
if and only if you want to use the event library in the child. If you just\n\
fork+exec, you don't have to call it at all.");

static PyObject *
Loop_fork(Loop *self, PyObject *unused)
{
    if (ev_is_default_loop(self->loop)) {
        ev_default_fork();
    }
    else {
        ev_loop_fork(self->loop);
    }

    Py_RETURN_NONE;
}


/* Loop.is_default_loop */
PyDoc_STRVAR(Loop_is_default_loop_doc,
"is_default_loop()\n\n\
Returns True when the loop actually is the 'default loop', False otherwise.");

static PyObject *
Loop_is_default_loop(Loop *self, PyObject *unused)
{
    if (ev_is_default_loop(self->loop)) {
        Py_RETURN_TRUE;
    }

    Py_RETURN_FALSE;
}


/* Loop.count */
PyDoc_STRVAR(Loop_count_doc,
"count() -> int\n\n\
Returns the count of loop iterations for the loop, which is identical to the\n\
number of times libev did poll for new events. It starts at 0 and happily wraps\n\
around with enough iterations.\n\
This value can sometimes be useful as a generation counter of sorts (it 'ticks'\n\
the number of loop iterations).");

static PyObject *
Loop_count(Loop *self, PyObject *unused)
{
    return Py_BuildValue("I", ev_loop_count(self->loop));
}


/* Loop.backend */
PyDoc_STRVAR(Loop_backend_doc,
"backend() -> int\n\n\
Returns one of the EVBACKEND_* flags indicating the event backend in use.");

static PyObject *
Loop_backend(Loop *self, PyObject *unused)
{
    return Py_BuildValue("I", ev_backend(self->loop));
}


/* Loop.now */
PyDoc_STRVAR(Loop_now_doc,
"now() -> float\n\n\
Returns the current 'event loop time', which is the time the event loop\n\
received events and started processing them. This timestamp does not change\n\
as long as callbacks are being processed, and this is also the base time used\n\
for relative timers. You can treat it as the timestamp of the event occurring\n\
(or more correctly, libev finding out about it).");

static PyObject *
Loop_now(Loop *self, PyObject *unused)
{
    return Py_BuildValue("d", ev_now(self->loop));
}


/* Loop.loop */
PyDoc_STRVAR(Loop_loop_doc,
"loop([flags])\n\n\
This method usually is called after you initialised all your watchers and\n\
you want to start handling events.\n\
If the 'flags' argument is specified as 0 (default), it will not return until\n\
either no event watchers are active anymore or unloop() was called.\n\
Please note that an explicit unloop() is usually better than relying on all\n\
watchers to be stopped when deciding when a program has finished (especially\n\
in interactive programs).\n\
A 'flags' value of EVLOOP_NONBLOCK will look for new events, will handle those\n\
events and any outstanding ones, but will not block your process in case\n\
there are no events and will return after one iteration of the loop.\n\
A 'flags' value of EVLOOP_ONESHOT will look for new events (waiting if\n\
neccessary) and will handle those and any outstanding ones. It will block\n\
your process until at least one new event arrives, and will return after one\n\
iteration of the loop. This is useful if you are waiting for some external\n\
event in conjunction with something not expressible using other libev watchers.");

static PyObject *
Loop_loop(Loop *self, PyObject *args)
{
    int flags = 0;

    if (!PyArg_ParseTuple(args, "|i:loop", &flags)) {
        return NULL;
    }

    Py_BEGIN_ALLOW_THREADS
    ev_loop(self->loop, flags);
    Py_END_ALLOW_THREADS

    Py_RETURN_NONE;
}


/* Loop.unloop */
PyDoc_STRVAR(Loop_unloop_doc,
"unloop([how])\n\n\
Can be used to make a call to loop() return early (but only after it has\n\
processed all outstanding events). The 'how' argument must be either\n\
EVUNLOOP_ONE, which will make the innermost loop() call return, or\n\
EVUNLOOP_ALL (default), which will make all nested loop() calls return.");

static PyObject *
Loop_unloop(Loop *self, PyObject *args)
{
    int how = EVUNLOOP_ALL;

    if (!PyArg_ParseTuple(args, "|i:unloop", &how)) {
        return NULL;
    }

    ev_unloop(self->loop, how);

    Py_RETURN_NONE;
}


/* Loop.ref / Loop.unref */
PyDoc_STRVAR(Loop_ref_unref_doc,
"ref()\n\
unref()\n\n\
Ref/unref can be used to add or remove a reference count on the event loop:\n\
Every watcher keeps one reference, and as long as the reference count is\n\
nonzero, the loop will not return on its own. If you have a watcher you never\n\
unregister that should not keep the loop from returning, unref() after starting,\n\
and ref() before stopping it. For example, libev itself uses this for its\n\
internal signal pipe: It is not visible to the libev user and should not keep\n\
the loop from exiting if no event watchers registered by it are active. It is\n\
also an excellent way to do this for generic recurring timers. Just remember to\n\
unref() after start() and ref() before stop() (but only if the watcher wasn't\n\
active before, or was active before, respectively).");

static PyObject *
Loop_ref(Loop *self, PyObject *unused)
{
    ev_ref(self->loop);

    Py_RETURN_NONE;
}

static PyObject *
Loop_unref(Loop *self, PyObject *unused)
{
    ev_unref(self->loop);

    Py_RETURN_NONE;
}


/* Loop.set_io_collect_interval / Loop.set_timeout_collect_interval */
PyDoc_STRVAR(Loop_set_interval_doc,
"set_io_collect_interval(interval)\n\
set_timeout_collect_interval(interval)\n\n\
These advanced methods influence the time that libev will spend waiting for\n\
events. Both are by default 0, meaning that libev will try to invoke timer,\n\
periodic and I/O callbacks with minimum latency.\n\
Setting these to a higher value (the interval must be >= 0) allows libev to\n\
delay invocation of I/O and timer/periodic callbacks to increase efficiency of\n\
loop iterations.\n\
The background is that sometimes your program runs just fast enough to handle\n\
one (or very few) event(s) per loop iteration. While this makes the program\n\
responsive, it also wastes a lot of CPU time to poll for new events, especially\n\
with backends like select() which have a high overhead for the actual polling\n\
but can deliver many events at once.\n\
By setting a higher io collect interval you allow libev to spend more time\n\
collecting I/O events, so you can handle more events per iteration, at the cost\n\
of increasing latency. Timeouts (both Periodic and Timer) will be not affected.\n\
Setting this to a non-null value will introduce an additional sleep() call into\n\
most loop iterations.\n\
Likewise, by setting a higher timeout collect interval you allow libev to spend\n\
more time collecting timeouts, at the expense of increased latency (the watcher\n\
callback will be called later). Io watchers will not be affected. Setting this\n\
to a non-null value will not introduce any overhead in libev.\n\
Many (busy) programs can usually benefit by setting the io collect interval to a\n\
value near 0.1 or so, which is often enough for interactive servers (of course\n\
not for games), likewise for timeouts. It usually doesn't make much sense to set\n\
it to a lower value than 0.01, as this approsaches the timing granularity of\n\
most systems.");

static PyObject *
Loop_set_io_interval(Loop *self, PyObject *args)
{
    double interval;

    if (!PyArg_ParseTuple(args, "d:set_io_collect_interval", &interval)) {
        return NULL;
    }

    ev_set_io_collect_interval(self->loop, interval);

    Py_RETURN_NONE;
}

static PyObject *
Loop_set_timeout_interval(Loop *self, PyObject *args)
{
    double interval;

    if (!PyArg_ParseTuple(args, "d:set_timeout_collect_interval", &interval)) {
        return NULL;
    }

    ev_set_timeout_collect_interval(self->loop, interval);

    Py_RETURN_NONE;
}


/* LoopType.tp_methods */
static PyMethodDef Loop_methods[] = {
    {"fork", (PyCFunction)Loop_fork, METH_NOARGS,
     Loop_fork_doc},
    {"is_default_loop", (PyCFunction)Loop_is_default_loop, METH_NOARGS,
     Loop_is_default_loop_doc},
    {"count", (PyCFunction)Loop_count, METH_NOARGS,
     Loop_count_doc},
    {"backend", (PyCFunction)Loop_backend, METH_NOARGS,
     Loop_backend_doc},
    {"now", (PyCFunction)Loop_now, METH_NOARGS,
     Loop_now_doc},
    {"loop", (PyCFunction)Loop_loop, METH_VARARGS,
     Loop_loop_doc},
    {"unloop", (PyCFunction)Loop_unloop, METH_VARARGS,
     Loop_unloop_doc},
    {"ref", (PyCFunction)Loop_ref, METH_NOARGS,
     Loop_ref_unref_doc},
    {"unref", (PyCFunction)Loop_unref, METH_NOARGS,
     Loop_ref_unref_doc},
    {"set_io_collect_interval", (PyCFunction)Loop_set_io_interval,
     METH_VARARGS, Loop_set_interval_doc},
    {"set_timeout_collect_interval", (PyCFunction)Loop_set_timeout_interval,
     METH_VARARGS, Loop_set_interval_doc},
    {NULL}  /* Sentinel */
};


/* LoopType */
static PyTypeObject LoopType = {
    PyObject_HEAD_INIT(NULL)
    0,                                        /*ob_size*/
    "ev.Loop",                                /*tp_name*/
    sizeof(Loop),                             /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    (destructor)Loop_dealloc,                 /*tp_dealloc*/
    0,                                        /*tp_print*/
    0,                                        /*tp_getattr*/
    0,                                        /*tp_setattr*/
    0,                                        /*tp_compare*/
    0,                                        /*tp_repr*/
    0,                                        /*tp_as_number*/
    0,                                        /*tp_as_sequence*/
    0,                                        /*tp_as_mapping*/
    0,                                        /*tp_hash */
    0,                                        /*tp_call*/
    0,                                        /*tp_str*/
    0,                                        /*tp_getattro*/
    0,                                        /*tp_setattro*/
    0,                                        /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    Loop_doc,                                 /*tp_doc*/
    0,                                        /*tp_traverse*/
    0,                                        /*tp_clear*/
    0,                                        /*tp_richcompare*/
    0,                                        /*tp_weaklistoffset*/
    0,                                        /*tp_iter*/
    0,                                        /*tp_iternext*/
    Loop_methods,                             /*tp_methods*/
    0,                                        /*tp_members*/
    0,                                        /*tp_getsets*/
    0,                                        /*tp_base*/
    0,                                        /*tp_dict*/
    0,                                        /*tp_descr_get*/
    0,                                        /*tp_descr_set*/
    0,                                        /*tp_dictoffset*/
    0,                                        /*tp_init*/
    0,                                        /*tp_alloc*/
    Loop_new,                                 /*tp_new*/
};


/* set - called by subtypes before calling ev_TYPE_set */
static int
set_watcher(_Watcher *self)
{
    if (ev_is_active(self->watcher)) {
        PyErr_SetString(EvError,
                        "you must not set a watcher while it is active");
        return -1;
    }

    return 0;
}


/* watcher callback */
static void
_Watcher_cb(struct ev_loop *loop, ev_watcher *watcher, int events)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    _Watcher *_watcher = watcher->data;

    if (_watcher->callback != Py_None) {
        PyObject *result, *err_type, *err_value, *err_traceback;
        int had_error = PyErr_Occurred() ? 1 : 0;

        if (had_error) {
            PyErr_Fetch(&err_type, &err_value, &err_traceback);
        }

        result = PyObject_CallFunction(_watcher->callback, "Ok", _watcher,
                                       (unsigned long)events);
        if (result == NULL) {
            PyErr_WriteUnraisable(_watcher->callback);
        }
        else {
            Py_DECREF(result);
        }

        if (had_error) {
            PyErr_Restore(err_type, err_value, err_traceback);
        }
    }

    PyGILState_Release(gstate);
}


/* _WatcherType.tp_traverse */
static int
_Watcher_traverse(_Watcher *self, visitproc visit, void *arg)
{
    Py_VISIT(self->data);

    return 0;
}


/* _WatcherType.tp_clear */
static int
_Watcher_clear(_Watcher *self)
{
    Py_CLEAR(self->data);

    return 0;
}


/* _WatcherType.tp_dealloc */
static void
_Watcher_dealloc(_Watcher *self)
{
    Py_XDECREF(self->loop);
    Py_XDECREF(self->callback);
    Py_XDECREF(self->data);

    self->ob_type->tp_free((PyObject*)self);
}


/* init the watcher - called by subtypes tp_new */
static void
_Watcher_new(_Watcher *self, ev_watcher *watcher)
{
    /* our ev_watcher */
    self->watcher = watcher;

    /* self->watcher->data */
    self->watcher->data = (void *)self;

    /* init the watcher*/
    ev_init(self->watcher, _Watcher_cb);
}


/* _Watcher.callback */
PyDoc_STRVAR(_Watcher_callback_doc,
"The callback must be a callable whose signature is: (watcher, events).\n\
The 'watcher' argument will be the python watcher object receiving the events.\n\
The 'events' argument  will be a python int representing ored EV_* flags\n\
corresponding to the received events.");

static PyObject *
_Watcher_callback_get(_Watcher *self, void *closure)
{
    Py_INCREF(self->callback);
    return self->callback;
}

static int
_Watcher_callback_set(_Watcher *self, PyObject *value, void *closure)
{
    PyObject *tmp;

    if (value) {
        if (value != Py_None && !PyCallable_Check(value)) {
            PyErr_SetString(PyExc_TypeError, "a callable or None is required");
            return -1;
        }
    }
    else {
        value = Py_None;
    }

    tmp = self->callback;
    Py_INCREF(value);
    self->callback = value;
    Py_XDECREF(tmp);

    return 0;
}


/* init the watcher - called by subtypes tp_init */
static int
_Watcher_init(_Watcher *self, Loop *loop, PyObject *callback, PyObject *data,
              int default_loop)
{
    PyObject *tmp;

    /* self->loop */
    if (!PyObject_TypeCheck(loop, &LoopType)) {
        PyErr_SetString(PyExc_TypeError, "an ev.Loop is required");
        return -1;
    }
    else {
        if (default_loop && !ev_is_default_loop(loop->loop)) {
            PyErr_SetString(EvError, "loop must be the 'default loop'");
            return -1;
        }
    }
    tmp = (PyObject *)self->loop;
    Py_INCREF(loop);
    self->loop = loop;
    Py_XDECREF(tmp);

    /* self->callback */
    if (_Watcher_callback_set(self, callback, NULL) < 0) {
        return -1;
    }

    /* self->data */
    if (data) {
        tmp = self->data;
        Py_INCREF(data);
        self->data = data;
        Py_XDECREF(tmp);
    }

    return 0;
}


/* _Watcher.is_active */
PyDoc_STRVAR(_Watcher_is_active_doc,
"is_active() -> bool\n\n\
Returns True if the watcher is active (i.e. it has been started and not yet\n\
been stopped).");

static PyObject *
_Watcher_is_active(_Watcher *self, PyObject *unused)
{
    if (ev_is_active(self->watcher)) {
        Py_RETURN_TRUE;
    }

    Py_RETURN_FALSE;
}


/* _Watcher.is_pending */
PyDoc_STRVAR(_Watcher_is_pending_doc,
"is_pending() -> bool\n\n\
Returns True if the watcher is pending, (i.e. it has outstanding events but\n\
its callback has not yet been invoked).");

static PyObject *
_Watcher_is_pending(_Watcher *self, PyObject *unused)
{
    if (ev_is_pending(self->watcher)) {
        Py_RETURN_TRUE;
    }

    Py_RETURN_FALSE;
}


/* _Watcher.clear_pending */
PyDoc_STRVAR(_Watcher_clear_pending_doc,
"clear_pending() -> int\n\n\
If the watcher is pending, this function clears its pending status and\n\
returns its events bitset (as if its callback was invoked). If the watcher\n\
isn't pending it does nothing and returns 0.");

static PyObject *
_Watcher_clear_pending(_Watcher *self, PyObject *unused)
{
    return Py_BuildValue("i", ev_clear_pending(self->loop->loop, self->watcher));
}


/* _Watcher.invoke */
PyDoc_STRVAR(_Watcher_invoke_doc,
"invoke(events)\n\n\
Invoke the watcher with the given 'events' flag.");

static PyObject *
_Watcher_invoke(_Watcher *self, PyObject *args)
{
    unsigned long events;

    if (!PyArg_ParseTuple(args, "k:invoke", &events)) {
        return NULL;
    }

    ev_invoke(self->loop->loop, self->watcher, events);

    Py_RETURN_NONE;
}


/* _Watcher.start - doc only */
PyDoc_STRVAR(_Watcher_start_doc,
"start()\n\n\
Starts (activates) the watcher. Only active watchers will receive events.\n\
If the watcher is already active nothing will happen.");


/* _Watcher.stop - doc only */
PyDoc_STRVAR(_Watcher_stop_doc,
"stop()\n\n\
Stops the watcher again (if active) and clears the pending status.\n\
It is possible that stopped watchers are pending (for example, non-repeating\n\
timers are being stopped when they become pending), but stop() ensures that\n\
the watcher is neither active nor pending.");


/* _Watcher.priority */
PyDoc_STRVAR(_Watcher_priority_doc,
"Set and query the priority of the watcher. The priority is a small integer\n\
between EV_MAXPRI (default: 2) and EV_MINPRI (default: -2). Pending watchers\n\
with higher priority will be invoked before watchers with lower priority, but\n\
priority will not keep watchers from being executed (except for Idle watchers).\n\
This means that priorities are only used for ordering callback invocation after\n\
new events have been received. This is useful, for example, to reduce latency\n\
after idling, or more often, to bind two watchers on the same event and make\n\
sure one is called first.\n\
If you need to suppress invocation when higher priority events are pending you\n\
need to look at Idle watchers, which provide this functionality.\n\
You must not change the priority of a watcher as long as it is active or pending.\n\
The default priority used by watchers when no priority has been set is always 0,\n\
which is supposed to not be too high and not be too low :).\n\
Setting a priority outside the range of EV_MINPRI to EV_MAXPRI is fine, as long\n\
as you do not mind that the priority value you query might or might not have\n\
been adjusted to be within valid range.");

static PyObject *
_Watcher_priority_get(_Watcher *self, void *closure)
{
    return Py_BuildValue("i", ev_priority(self->watcher));
}

static int
_Watcher_priority_set(_Watcher *self, PyObject *value, void *closure)
{
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "readonly attribute");
        return -1;
    }

    if (!PyInt_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "an integer is required");
        return -1;
    }

    if (ev_is_active(self->watcher) || ev_is_pending(self->watcher)) {
        PyErr_SetString(EvError,
                        "you must not change the 'priority' attribute of a "
                        "watcher while it is active or pending");
        return -1;
    }

    long priority = PyInt_AsLong(value);

    if (priority == -1 && PyErr_Occurred()) {
        return -1;
    }

    ev_set_priority(self->watcher, (int)priority);

    return 0;

}


/* _WatcherType.tp_methods*/
static PyMethodDef _Watcher_methods[] = {
    {"is_active", (PyCFunction)_Watcher_is_active, METH_NOARGS,
     _Watcher_is_active_doc},
    {"is_pending", (PyCFunction)_Watcher_is_pending, METH_NOARGS,
     _Watcher_is_pending_doc},
    {"clear_pending", (PyCFunction)_Watcher_clear_pending, METH_NOARGS,
     _Watcher_clear_pending_doc},
    {"invoke", (PyCFunction)_Watcher_invoke, METH_VARARGS,
     _Watcher_invoke_doc},
    {NULL}  /* Sentinel */
};


/* _WatcherType.tp_members */
static PyMemberDef _Watcher_members[] = {
    {"loop", T_OBJECT_EX, offsetof(_Watcher, loop), READONLY,
     "ev.Loop object to which this watcher is attached"},
    {"data", T_OBJECT, offsetof(_Watcher, data), 0,
     "watcher data"},
    {NULL}  /* Sentinel */
};


/* _WatcherType.tp_getsets */
static PyGetSetDef _Watcher_getsets[] = {
    {"callback", (getter)_Watcher_callback_get, (setter)_Watcher_callback_set,
     _Watcher_callback_doc, NULL},
    {"priority", (getter)_Watcher_priority_get, (setter)_Watcher_priority_set,
     _Watcher_priority_doc, NULL},
    {NULL}  /* Sentinel */
};


/* _WatcherType */
static PyTypeObject _WatcherType = {
    PyObject_HEAD_INIT(NULL)
    0,                                        /*ob_size*/
    "ev._Watcher",                            /*tp_name*/
    sizeof(_Watcher),                         /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    (destructor)_Watcher_dealloc,             /*tp_dealloc*/
    0,                                        /*tp_print*/
    0,                                        /*tp_getattr*/
    0,                                        /*tp_setattr*/
    0,                                        /*tp_compare*/
    0,                                        /*tp_repr*/
    0,                                        /*tp_as_number*/
    0,                                        /*tp_as_sequence*/
    0,                                        /*tp_as_mapping*/
    0,                                        /*tp_hash */
    0,                                        /*tp_call*/
    0,                                        /*tp_str*/
    0,                                        /*tp_getattro*/
    0,                                        /*tp_setattro*/
    0,                                        /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,  /*tp_flags*/
    0,                                        /*tp_doc*/
    (traverseproc)_Watcher_traverse,          /*tp_traverse*/
    (inquiry)_Watcher_clear,                  /*tp_clear*/
    0,                                        /*tp_richcompare*/
    0,                                        /*tp_weaklistoffset*/
    0,                                        /*tp_iter*/
    0,                                        /*tp_iternext*/
    _Watcher_methods,                         /*tp_methods*/
    _Watcher_members,                         /*tp_members*/
    _Watcher_getsets,                         /*tp_getsets*/
    0,                                        /*tp_base*/
    0,                                        /*tp_dict*/
    0,                                        /*tp_descr_get*/
    0,                                        /*tp_descr_set*/
    0,                                        /*tp_dictoffset*/
    0,                                        /*tp_init*/
    0,                                        /*tp_alloc*/
    PyType_GenericNew,                        /*tp_new*/
};


/* IoType.tp_doc */
PyDoc_STRVAR(Io_doc,
"Io(fd, events, loop, [callback=None, [data=None]])\n\n\
I/O watchers check whether a file descriptor is readable or writable in each\n\
iteration of the event loop, or, more precisely, when reading would not block\n\
the process and writing would at least be able to write some data.\n\
This behaviour is called level-triggering because you keep receiving events as\n\
long as the condition persists. Remember you can stop the watcher if you don't\n\
want to act on the event and neither want to receive future events.\n\
In general you can register as many read and/or write event watchers per fd as\n\
you want. Setting all file descriptors to non-blocking mode is also usually a\n\
good idea.\n\
See ev_io libev documentation for a more detailed description.\n\
See the set() method documentation.");


/* set up the io */
static int
set_io(Io *self, int fd, int events)
{
    if (set_watcher((_Watcher *)self) < 0) {
        return -1;
    }

    ev_io_set(&self->watcher, fd, events);

    return 0;
}


/* IoType.tp_dealloc */
static void
Io_dealloc(Io *self)
{
    _Watcher *_watcher = (_Watcher *)self;

    if (_watcher->loop && &self->watcher) {
        ev_io_stop(_watcher->loop->loop, &self->watcher);
    }

    _WatcherType.tp_dealloc((PyObject *)self);
}


/* IoType.tp_new */
static PyObject *
Io_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Io *self = (Io *)_WatcherType.tp_new(type, args, kwargs);
    if (self == NULL) {
        return NULL;
    }

    _Watcher_new((_Watcher *)self, (ev_watcher *)&self->watcher);

    return (PyObject *)self;
}


/* IoType.tp_init */
static int
Io_init(Io *self, PyObject *args, PyObject *kwargs)
{
    int fd;
    int events;
    Loop *loop;
    PyObject *callback = NULL;
    PyObject *data = NULL;

    static char *kwlist[] = {"fd", "events",
                             "loop", "callback", "data", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "iiO|OO:__init__", kwlist,
                                     &fd, &events,
                                     &loop, &callback, &data)) {
        return -1;
    }

    if (_Watcher_init((_Watcher *)self, loop, callback, data, 0) < 0) {
        return -1;
    }

    if (set_io(self, fd, events) < 0) {
        return -1;
    }

    return 0;
}


/* Io.set */
PyDoc_STRVAR(Io_set_doc,
"set(fd, events)\n\n\
The 'fd' is the file descriptor to receive events for and 'events' is either\n\
EV_READ, EV_WRITE or EV_READ | EV_WRITE to receive the given events.\n\
You must not call this method on a watcher that is active.");

static PyObject *
Io_set(Io *self, PyObject *args)
{
    int fd;
    int events;

    if (!PyArg_ParseTuple(args, "ii:set", &fd, &events)) {
        return NULL;
    }

    if (set_io(self, fd, events) < 0) {
        return NULL;
    }

    Py_RETURN_NONE;
}


/* Io.start */
static PyObject *
Io_start(Io *self, PyObject *unused)
{
    ev_io_start(((_Watcher *)self)->loop->loop, &self->watcher);

    Py_RETURN_NONE;
}


/* Io.stop */
static PyObject *
Io_stop(Io *self, PyObject *unused)
{
    ev_io_stop(((_Watcher *)self)->loop->loop, &self->watcher);

    Py_RETURN_NONE;
}


/* IoType.tp_methods */
static PyMethodDef Io_methods[] = {
    {"set", (PyCFunction)Io_set, METH_VARARGS,
     Io_set_doc},
    {"start", (PyCFunction)Io_start, METH_NOARGS,
     _Watcher_start_doc},
    {"stop", (PyCFunction)Io_stop, METH_NOARGS,
     _Watcher_stop_doc},
    {NULL}  /* Sentinel */
};


/* IoType.tp_members */
static PyMemberDef Io_members[] = {
    {"fd", T_INT, offsetof(Io, watcher.fd), READONLY,
     "The file descriptor being watched."},
    {"events", T_INT, offsetof(Io, watcher.events), READONLY,
     "The events being watched."},
    {NULL}  /* Sentinel */
};


/* IoType */
static PyTypeObject IoType = {
    PyObject_HEAD_INIT(NULL)
    0,                                        /*ob_size*/
    "ev.Io",                                  /*tp_name*/
    sizeof(Io),                               /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    (destructor)Io_dealloc,                   /*tp_dealloc*/
    0,                                        /*tp_print*/
    0,                                        /*tp_getattr*/
    0,                                        /*tp_setattr*/
    0,                                        /*tp_compare*/
    0,                                        /*tp_repr*/
    0,                                        /*tp_as_number*/
    0,                                        /*tp_as_sequence*/
    0,                                        /*tp_as_mapping*/
    0,                                        /*tp_hash */
    0,                                        /*tp_call*/
    0,                                        /*tp_str*/
    0,                                        /*tp_getattro*/
    0,                                        /*tp_setattro*/
    0,                                        /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    Io_doc,                                   /*tp_doc*/
    0,                                        /*tp_traverse*/
    0,                                        /*tp_clear*/
    0,                                        /*tp_richcompare*/
    0,                                        /*tp_weaklistoffset*/
    0,                                        /*tp_iter*/
    0,                                        /*tp_iternext*/
    Io_methods,                               /*tp_methods*/
    Io_members,                               /*tp_members*/
    0,                                        /*tp_getsets*/
    0,                                        /*tp_base*/
    0,                                        /*tp_dict*/
    0,                                        /*tp_descr_get*/
    0,                                        /*tp_descr_set*/
    0,                                        /*tp_dictoffset*/
    (initproc)Io_init,                        /*tp_init*/
    0,                                        /*tp_alloc*/
    Io_new,                                   /*tp_new*/
};


/* TimerType.tp_doc */
PyDoc_STRVAR(Timer_doc,
"Timer(after, repeat, loop, [callback=None, [data=None]])\n\n\
Timer watchers are simple relative timers that generate an event after a given\n\
time, and optionally repeating in regular intervals after that.\n\
The timers are based on real time, that is, if you register an event that times\n\
out after an hour and you reset your system clock to last years time, it will\n\
still time out after (roughly) and hour. 'Roughly' because detecting time jumps\n\
is hard, and some inaccuracies are unavoidable.\n\
See the set() method documentation.");


/* set up the timer */
static int
set_timer(Timer *self, double after, double repeat)
{
    if (set_watcher((_Watcher *)self) < 0) {
        return -1;
    }

    if (repeat < 0.0) {
        PyErr_SetString(PyExc_TypeError, "a positive float or 0.0 is required");
        return -1;
    }

    ev_timer_set(&self->watcher, after, repeat);

    return 0;
}


/* TimerType.tp_dealloc */
static void
Timer_dealloc(Timer *self)
{
    _Watcher *_watcher = (_Watcher *)self;

    if (_watcher->loop && &self->watcher) {
        ev_timer_stop(_watcher->loop->loop, &self->watcher);
    }

    _WatcherType.tp_dealloc((PyObject *)self);
}


/* TimerType.tp_new */
static PyObject *
Timer_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Timer *self = (Timer *)_WatcherType.tp_new(type, args, kwargs);
    if (self == NULL) {
        return NULL;
    }

    _Watcher_new((_Watcher *)self, (ev_watcher *)&self->watcher);

    return (PyObject *)self;
}


/* TimerType.tp_init */
static int
Timer_init(Timer *self, PyObject *args, PyObject *kwargs)
{
    double after;
    double repeat;
    Loop *loop;
    PyObject *callback = NULL;
    PyObject *data = NULL;

    static char *kwlist[] = {"after", "repeat",
                             "loop", "callback", "data", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ddO|OO:__init__", kwlist,
                                     &after, &repeat,
                                     &loop, &callback, &data)) {
        return -1;
    }

    if (_Watcher_init((_Watcher *)self, loop, callback, data, 0) < 0) {
        return -1;
    }

    if (set_timer(self, after, repeat) < 0) {
        return -1;
    }

    return 0;
}


/* Timer.set */
PyDoc_STRVAR(Timer_set_doc,
"set(after, repeat)\n\n\
Configure the timer to trigger after 'after' seconds.\n\
If 'repeat' is 0.0, then it will automatically be stopped. If it is positive,\n\
then the timer will automatically be configured to trigger again 'repeat'\n\
seconds later, again, and again, until stopped manually.\n\
You must not call this method on a watcher that is active.");

static PyObject *
Timer_set(Timer *self, PyObject *args)
{
    double after;
    double repeat;

    if (!PyArg_ParseTuple(args, "dd:set", &after, &repeat)) {
        return NULL;
    }

    if (set_timer(self, after, repeat) < 0) {
        return NULL;
    }

    Py_RETURN_NONE;
}


/* Timer.start */
static PyObject *
Timer_start(Timer *self, PyObject *unused)
{
    ev_timer_start(((_Watcher *)self)->loop->loop, &self->watcher);

    Py_RETURN_NONE;
}


/* Timer.stop */
static PyObject *
Timer_stop(Timer *self, PyObject *unused)
{
    ev_timer_stop(((_Watcher *)self)->loop->loop, &self->watcher);

    Py_RETURN_NONE;
}


/* Timer.again */
PyDoc_STRVAR(Timer_again_doc,
"again()\n\n\
This will act as if the timer timed out and restart it again if it is repeating.\n\
The exact semantics are:\n\
If the timer is pending, its pending status is cleared.\n\
If the timer is started but nonrepeating, stop it (as if it timed out).\n\
If the timer is repeating, either start it if necessary (with the 'repeat'\n\
value), or reset the running timer to the 'repeat' value.");

static PyObject *
Timer_again(Timer *self, PyObject *unused)
{
    ev_timer_again(((_Watcher *)self)->loop->loop, &self->watcher);

    Py_RETURN_NONE;
}


/* Timer.repeat */
static PyObject *
Timer_repeat_get(Timer *self, void *closure)
{
    return Py_BuildValue("d", self->watcher.repeat);
}

static int
Timer_repeat_set(Timer *self, PyObject *value, void *closure)
{
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "readonly attribute");
        return -1;
    }

    if (!PyFloat_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "a float is required");
        return -1;
    }

    double repeat = PyFloat_AsDouble(value);

    if (repeat < 0.0) {
        PyErr_SetString(PyExc_TypeError, "a positive float or 0.0 is required");
        return -1;
    }

    self->watcher.repeat = repeat;

    return 0;
}


/* TimerType.tp_methods */
static PyMethodDef Timer_methods[] = {
    {"set", (PyCFunction)Timer_set, METH_VARARGS,
     Timer_set_doc},
    {"start", (PyCFunction)Timer_start, METH_NOARGS,
     _Watcher_start_doc},
    {"stop", (PyCFunction)Timer_stop, METH_NOARGS,
     _Watcher_stop_doc},
    {"again", (PyCFunction)Timer_again, METH_NOARGS,
     Timer_again_doc},
    {NULL}  /* Sentinel */
};


/* TimerType.tp_getsets */
static PyGetSetDef Timer_getsets[] = {
    {"repeat", (getter)Timer_repeat_get, (setter)Timer_repeat_set,
     "The current repeat value.", NULL},
    {NULL}  /* Sentinel */
};


/* TimerType */
static PyTypeObject TimerType = {
    PyObject_HEAD_INIT(NULL)
    0,                                        /*ob_size*/
    "ev.Timer",                               /*tp_name*/
    sizeof(Timer),                            /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    (destructor)Timer_dealloc,                /*tp_dealloc*/
    0,                                        /*tp_print*/
    0,                                        /*tp_getattr*/
    0,                                        /*tp_setattr*/
    0,                                        /*tp_compare*/
    0,                                        /*tp_repr*/
    0,                                        /*tp_as_number*/
    0,                                        /*tp_as_sequence*/
    0,                                        /*tp_as_mapping*/
    0,                                        /*tp_hash */
    0,                                        /*tp_call*/
    0,                                        /*tp_str*/
    0,                                        /*tp_getattro*/
    0,                                        /*tp_setattro*/
    0,                                        /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    Timer_doc,                                /*tp_doc*/
    0,                                        /*tp_traverse*/
    0,                                        /*tp_clear*/
    0,                                        /*tp_richcompare*/
    0,                                        /*tp_weaklistoffset*/
    0,                                        /*tp_iter*/
    0,                                        /*tp_iternext*/
    Timer_methods,                            /*tp_methods*/
    0,                                        /*tp_members*/
    Timer_getsets,                            /*tp_getsets*/
    0,                                        /*tp_base*/
    0,                                        /*tp_dict*/
    0,                                        /*tp_descr_get*/
    0,                                        /*tp_descr_set*/
    0,                                        /*tp_dictoffset*/
    (initproc)Timer_init,                     /*tp_init*/
    0,                                        /*tp_alloc*/
    Timer_new,                                /*tp_new*/
};


/* PeriodicType.tp_doc */
PyDoc_STRVAR(Periodic_doc,
"Periodic(at, interval, reschedule_cb, loop, [callback=None, [data=None]])\n\n\
Periodic watchers are also timers of a kind, but they are very versatile (and\n\
unfortunately a bit complex).\n\
Unlike Timer's, they are not based on real time (or relative time) but on\n\
wallclock time (absolute time). You can tell a periodic watcher to trigger 'at'\n\
some specific point in time. For example, if you tell a periodic watcher to\n\
trigger in 10 seconds (by specifiying e.g. loop.now() + 10.0) and then reset\n\
your system clock to the last year, then it will take a year to trigger the\n\
event (unlike a Timer, which would trigger roughly 10 seconds later).\n\
See the set() method documentation.");


/* periodic reschedule stop callback */
static void
periodic_reschedule_stop(struct ev_loop *loop, ev_prepare *prepare, int events)
{
    PyGILState_STATE gstate = PyGILState_Ensure();

    ev_periodic_stop(loop, (ev_periodic *)prepare->data);
    ev_prepare_stop(loop, prepare);

    PyMem_Free(prepare);

    PyGILState_Release(gstate);
}


/* periodic reschedule callback */
static double
periodic_reschedule_cb(ev_periodic *watcher, double now)
{
    PyGILState_STATE gstate = PyGILState_Ensure();

    Periodic *periodic = watcher->data;
    double result;
    PyObject *py_result;

    PyObject *err_type, *err_value, *err_traceback;
    int had_error = PyErr_Occurred() ? 1 : 0;
    if (had_error) {
        PyErr_Fetch(&err_type, &err_value, &err_traceback);
    }

    py_result = PyObject_CallFunction(periodic->reschedule_cb, "Od", periodic,
                                      now);
    if (py_result == NULL) {
        goto error;
    }
    else {
        if (!PyFloat_Check(py_result)) {
            PyErr_SetString(PyExc_TypeError, "must return a float");
            goto error;
        }
        else {
            result = PyFloat_AsDouble(py_result);
            if (result <= now) {
                PyErr_SetString(EvError, "return value must be > 'now' param");
                goto error;
            }
            goto finish;
        }
    }

error:
    PyErr_WriteUnraisable(periodic->reschedule_cb);
    PyErr_Format(EvError, "due to previous error, "
                 "<ev.Periodic object at %p> will be stopped", periodic);
    PyErr_WriteUnraisable(periodic->reschedule_cb);

    ev_prepare *prepare = PyMem_Malloc(sizeof(ev_prepare));
    prepare->data = (void *)watcher;
    ev_prepare_init(prepare, periodic_reschedule_stop);
    ev_prepare_start(((_Watcher *)periodic)->loop->loop, prepare);

    result = 1e30;

finish:
    Py_XDECREF(py_result);

    if (had_error) {
        PyErr_Restore(err_type, err_value, err_traceback);
    }

    PyGILState_Release(gstate);

    return result;
}


/* set up the periodic */
static int
set_periodic(Periodic *self, double at, double interval, PyObject *reschedule_cb)
{
    if (set_watcher((_Watcher *)self) < 0) {
        return -1;
    }

    if (interval < 0.0) {
        PyErr_SetString(PyExc_TypeError, "a positive float or 0.0 is required");
        return -1;
    }

    if (reschedule_cb != Py_None) {
        ev_periodic_set(&self->watcher, at, interval, periodic_reschedule_cb);
    }
    else{
        ev_periodic_set(&self->watcher, at, interval, 0);
    }

    return 0;
}


/* PeriodicType.tp_dealloc */
static void
Periodic_dealloc(Periodic *self)
{
    _Watcher *_watcher = (_Watcher *)self;

    if (_watcher->loop && &self->watcher) {
        ev_periodic_stop(_watcher->loop->loop, &self->watcher);
    }

    _WatcherType.tp_dealloc((PyObject *)self);
}


/* PeriodicType.tp_new */
static PyObject *
Periodic_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Periodic *self = (Periodic *)_WatcherType.tp_new(type, args, kwargs);
    if (self == NULL) {
        return NULL;
    }

    _Watcher_new((_Watcher *)self, (ev_watcher *)&self->watcher);

    return (PyObject *)self;
}


/* Periodic.reschedule_cb */
PyDoc_STRVAR(Periodic_reschedule_cb_doc,
"The current reschedule callback, or None, if this functionality is switched off.");

static PyObject *
Periodic_reschedule_cb_get(Periodic *self, void *closure)
{
    Py_INCREF(self->reschedule_cb);
    return self->reschedule_cb;
}

static int
Periodic_reschedule_cb_set(Periodic *self, PyObject *value, void *closure)
{
    PyObject *tmp;

    if (value) {
        if (value != Py_None && !PyCallable_Check(value)) {
            PyErr_SetString(PyExc_TypeError, "a callable or None is required");
            return -1;
        }
    }
    else {
        value = Py_None;
    }
    tmp = self->reschedule_cb;
    Py_INCREF(value);
    self->reschedule_cb = value;
    Py_XDECREF(tmp);

    if (closure) {
        if (value != Py_None) {
            self->watcher.reschedule_cb = periodic_reschedule_cb;
        }
        else {
            self->watcher.reschedule_cb = 0;
        }
    }

    return 0;
}


/* PeriodicType.tp_init */
static int
Periodic_init(Periodic *self, PyObject *args, PyObject *kwargs)
{
    double at;
    double interval;
    PyObject *reschedule_cb;
    Loop *loop;
    PyObject *callback = NULL;
    PyObject *data = NULL;

    static char *kwlist[] = {"at", "interval", "reschedule_cb",
                             "loop", "callback", "data", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ddOO|OO:__init__", kwlist,
                                     &at, &interval, &reschedule_cb,
                                     &loop, &callback, &data)) {
        return -1;
    }

    if (_Watcher_init((_Watcher *)self, loop, callback, data, 0) < 0) {
        return -1;
    }

    /* self->reschedule_cb */
    if (Periodic_reschedule_cb_set(self, reschedule_cb, NULL) < 0) {
        return -1;
    }

    if (set_periodic(self, at, interval, reschedule_cb) < 0) {
        return -1;
    }

    return 0;
}


/* Periodic.set */
PyDoc_STRVAR(Periodic_set_doc,
"set(at, interval, reschedule_cb)\n\n\
You must not call this method on a watcher that is active.");

static PyObject *
Periodic_set(Periodic *self, PyObject *args)
{
    double at;
    double interval;
    PyObject *reschedule_cb;

    if (!PyArg_ParseTuple(args, "ddO:set", &at, &interval, &reschedule_cb)) {
        return NULL;
    }

    /* self->reschedule_cb */
    if (Periodic_reschedule_cb_set(self, reschedule_cb, NULL) < 0) {
        return NULL;
    }


    if (set_periodic(self, at, interval, reschedule_cb) < 0) {
        return NULL;
    }

    Py_RETURN_NONE;
}


/* Periodic.start */
static PyObject *
Periodic_start(Periodic *self, PyObject *unused)
{
    ev_periodic_start(((_Watcher *)self)->loop->loop, &self->watcher);

    Py_RETURN_NONE;
}


/* Periodic.stop */
static PyObject *
Periodic_stop(Periodic *self, PyObject *unused)
{
    ev_periodic_stop(((_Watcher *)self)->loop->loop, &self->watcher);

    Py_RETURN_NONE;
}


/* Periodic.again */
PyDoc_STRVAR(Periodic_again_doc,
"again()\n\n\
Simply stops and restarts the periodic watcher again. This is only useful when\n\
you changed some parameters.");

static PyObject *
Periodic_again(Periodic *self, PyObject *unused)
{
    ev_periodic_again(((_Watcher *)self)->loop->loop, &self->watcher);

    Py_RETURN_NONE;
}


/* Periodic.interval */
static PyObject *
Periodic_interval_get(Periodic *self, void *closure)
{
    return Py_BuildValue("d", self->watcher.interval);
}

static int
Periodic_interval_set(Periodic *self, PyObject *value, void *closure)
{
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "readonly attribute");
        return -1;
    }

    if (!PyFloat_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "a float is required");
        return -1;
    }

    double interval = PyFloat_AsDouble(value);

    if (interval < 0.0) {
        PyErr_SetString(PyExc_TypeError, "a positive float or 0.0 is required");
        return -1;
    }

    self->watcher.interval = interval;

    return 0;
}


/* PeriodicType.tp_methods */
static PyMethodDef Periodic_methods[] = {
    {"set", (PyCFunction)Periodic_set, METH_VARARGS,
     Periodic_set_doc},
    {"start", (PyCFunction)Periodic_start, METH_NOARGS,
     _Watcher_start_doc},
    {"stop", (PyCFunction)Periodic_stop, METH_NOARGS,
     _Watcher_stop_doc},
    {"again", (PyCFunction)Periodic_again, METH_NOARGS,
     Periodic_again_doc},
    {NULL}  /* Sentinel */
};


/* PeriodicType.tp_members */
static PyMemberDef Periodic_members[] = {
    {"offset", T_DOUBLE, offsetof(Periodic, watcher.offset), 0,
     "When repeating, this contains the offset value, otherwise this is the "
     "absolute point in time."},
    {"at", T_DOUBLE, offsetof(Periodic, watcher.at), READONLY,
     "When active, contains the absolute time that the watcher is supposed to "
     "trigger next."},
    {NULL}  /* Sentinel */
};


/* PeriodicType.tp_getsets */
static PyGetSetDef Periodic_getsets[] = {
    {"reschedule_cb", (getter)Periodic_reschedule_cb_get,
    (setter)Periodic_reschedule_cb_set,
    Periodic_reschedule_cb_doc, (void *)1},
    {"interval", (getter)Periodic_interval_get, (setter)Periodic_interval_set,
     "The current interval value.", NULL},
    {NULL}  /* Sentinel */
};


/* PeriodicType */
static PyTypeObject PeriodicType = {
    PyObject_HEAD_INIT(NULL)
    0,                                        /*ob_size*/
    "ev.Periodic",                            /*tp_name*/
    sizeof(Periodic),                         /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    (destructor)Periodic_dealloc,             /*tp_dealloc*/
    0,                                        /*tp_print*/
    0,                                        /*tp_getattr*/
    0,                                        /*tp_setattr*/
    0,                                        /*tp_compare*/
    0,                                        /*tp_repr*/
    0,                                        /*tp_as_number*/
    0,                                        /*tp_as_sequence*/
    0,                                        /*tp_as_mapping*/
    0,                                        /*tp_hash */
    0,                                        /*tp_call*/
    0,                                        /*tp_str*/
    0,                                        /*tp_getattro*/
    0,                                        /*tp_setattro*/
    0,                                        /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    Periodic_doc,                             /*tp_doc*/
    0,                                        /*tp_traverse*/
    0,                                        /*tp_clear*/
    0,                                        /*tp_richcompare*/
    0,                                        /*tp_weaklistoffset*/
    0,                                        /*tp_iter*/
    0,                                        /*tp_iternext*/
    Periodic_methods,                         /*tp_methods*/
    Periodic_members,                         /*tp_members*/
    Periodic_getsets,                         /*tp_getsets*/
    0,                                        /*tp_base*/
    0,                                        /*tp_dict*/
    0,                                        /*tp_descr_get*/
    0,                                        /*tp_descr_set*/
    0,                                        /*tp_dictoffset*/
    (initproc)Periodic_init,                  /*tp_init*/
    0,                                        /*tp_alloc*/
    Periodic_new,                             /*tp_new*/
};


/* SignalType.tp_doc */
PyDoc_STRVAR(Signal_doc,
"Signal(signum, loop, [callback=None, [data=None]])\n\n\
Signal watchers will trigger an event when the process receives a specific\n\
signal one or more times. Even though signals are very asynchronous, libev will\n\
try it's best to deliver signals synchronously, i.e. as part of the normal event\n\
processing, like any other event.\n\
You can configure as many watchers as you like per signal. Only when the first\n\
watcher gets started will libev actually register a signal watcher with the\n\
kernel (thus it coexists with your own signal handlers as long as you don't\n\
register any with libev). Similarly, when the last signal watcher for a signal\n\
is stopped libev will reset the signal handler to SIG_DFL (regardless of what\n\
it was set to before).\n\
The 'loop' argument must be the 'default loop'.\n\
See the set() method documentation.");


/* set up the signal */
static int
set_signal(Signal *self, int signum)
{
    if (set_watcher((_Watcher *)self) < 0) {
        return -1;
    }

    ev_signal_set(&self->watcher, signum);

    return 0;
}


/* SignalType.tp_dealloc */
static void
Signal_dealloc(Signal *self)
{
    _Watcher *_watcher = (_Watcher *)self;

    if (_watcher->loop && &self->watcher) {
        ev_signal_stop(_watcher->loop->loop, &self->watcher);
    }

    _WatcherType.tp_dealloc((PyObject *)self);
}


/* SignalType.tp_new */
static PyObject *
Signal_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Signal *self = (Signal *)_WatcherType.tp_new(type, args, kwargs);
    if (self == NULL) {
        return NULL;
    }

    _Watcher_new((_Watcher *)self, (ev_watcher *)&self->watcher);

    return (PyObject *)self;
}


/* SignalType.tp_init */
static int
Signal_init(Signal *self, PyObject *args, PyObject *kwargs)
{
    int signum;
    Loop *loop;
    PyObject *callback = NULL;
    PyObject *data = NULL;

    static char *kwlist[] = {"signum",
                             "loop", "callback", "data", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "iO|OO:__init__", kwlist,
                                     &signum,
                                     &loop, &callback, &data)) {
        return -1;
    }

    if (_Watcher_init((_Watcher *)self, loop, callback, data, 1) < 0) {
        return -1;
    }

    if (set_signal(self, signum) < 0) {
        return -1;
    }

    return 0;
}


/* Signal.set */
PyDoc_STRVAR(Signal_set_doc,
"set(signum)\n\n\
Configures the watcher to trigger on the given signal number (usually one of\n\
the SIGxxx constants).\n\
You must not call this method on a watcher that is active.");

static PyObject *
Signal_set(Signal *self, PyObject *args)
{
    int signum;

    if (!PyArg_ParseTuple(args, "i:set", &signum)) {
        return NULL;
    }

    if (set_signal(self, signum) < 0) {
        return NULL;
    }

    Py_RETURN_NONE;
}


/* Signal.start */
static PyObject *
Signal_start(Signal *self, PyObject *unused)
{
    ev_signal_start(((_Watcher *)self)->loop->loop, &self->watcher);

    Py_RETURN_NONE;
}


/* Signal.stop */
static PyObject *
Signal_stop(Signal *self, PyObject *unused)
{
    ev_signal_stop(((_Watcher *)self)->loop->loop, &self->watcher);

    Py_RETURN_NONE;
}


/* SignalType.tp_methods */
static PyMethodDef Signal_methods[] = {
    {"set", (PyCFunction)Signal_set, METH_VARARGS,
     Signal_set_doc},
    {"start", (PyCFunction)Signal_start, METH_NOARGS,
     _Watcher_start_doc},
    {"stop", (PyCFunction)Signal_stop, METH_NOARGS,
     _Watcher_stop_doc},
    {NULL}  /* Sentinel */
};


/* SignalType.tp_members */
static PyMemberDef Signal_members[] = {
    {"signum", T_INT, offsetof(Signal, watcher.signum), READONLY,
     "The signal the watcher watches out for."},
    {NULL}  /* Sentinel */
};


/* SignalType */
static PyTypeObject SignalType = {
    PyObject_HEAD_INIT(NULL)
    0,                                        /*ob_size*/
    "ev.Signal",                              /*tp_name*/
    sizeof(Signal),                           /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    (destructor)Signal_dealloc,               /*tp_dealloc*/
    0,                                        /*tp_print*/
    0,                                        /*tp_getattr*/
    0,                                        /*tp_setattr*/
    0,                                        /*tp_compare*/
    0,                                        /*tp_repr*/
    0,                                        /*tp_as_number*/
    0,                                        /*tp_as_sequence*/
    0,                                        /*tp_as_mapping*/
    0,                                        /*tp_hash */
    0,                                        /*tp_call*/
    0,                                        /*tp_str*/
    0,                                        /*tp_getattro*/
    0,                                        /*tp_setattro*/
    0,                                        /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    Signal_doc,                               /*tp_doc*/
    0,                                        /*tp_traverse*/
    0,                                        /*tp_clear*/
    0,                                        /*tp_richcompare*/
    0,                                        /*tp_weaklistoffset*/
    0,                                        /*tp_iter*/
    0,                                        /*tp_iternext*/
    Signal_methods,                           /*tp_methods*/
    Signal_members,                           /*tp_members*/
    0,                                        /*tp_getsets*/
    0,                                        /*tp_base*/
    0,                                        /*tp_dict*/
    0,                                        /*tp_descr_get*/
    0,                                        /*tp_descr_set*/
    0,                                        /*tp_dictoffset*/
    (initproc)Signal_init,                    /*tp_init*/
    0,                                        /*tp_alloc*/
    Signal_new,                               /*tp_new*/
};


/* ChildType.tp_doc */
PyDoc_STRVAR(Child_doc,
"Child(pid, trace, loop, [callback=None, [data=None]])\n\n\
Child watchers trigger when your process receives a SIGCHLD in response to some\n\
child status changes (most typically when a child of yours dies).\n\
The 'loop' argument must be the 'default loop'.\n\
See the set() method documentation.");


/* set up the child */
static int
set_child(Child *self, int pid, int trace)
{
    if (set_watcher((_Watcher *)self) < 0) {
        return -1;
    }

    ev_child_set(&self->watcher, pid, trace);

    return 0;
}


/* ChildType.tp_dealloc */
static void
Child_dealloc(Child *self)
{
    _Watcher *_watcher = (_Watcher *)self;

    if (_watcher->loop && &self->watcher) {
        ev_child_stop(_watcher->loop->loop, &self->watcher);
    }

    _WatcherType.tp_dealloc((PyObject *)self);
}


/* ChildType.tp_new */
static PyObject *
Child_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Child *self = (Child *)_WatcherType.tp_new(type, args, kwargs);
    if (self == NULL) {
        return NULL;
    }

    _Watcher_new((_Watcher *)self, (ev_watcher *)&self->watcher);

    return (PyObject *)self;
}


/* ChildType.tp_init */
static int
Child_init(Child *self, PyObject *args, PyObject *kwargs)
{
    int pid;
    int trace;
    Loop *loop;
    PyObject *callback = NULL;
    PyObject *data = NULL;

    static char *kwlist[] = {"pid", "trace",
                             "loop", "callback", "data", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "iiO|OO:__init__", kwlist,
                                     &pid, &trace,
                                     &loop, &callback, &data)) {
        return -1;
    }

    if (_Watcher_init((_Watcher *)self, loop, callback, data, 1) < 0) {
        return -1;
    }

    if (set_child(self, pid, trace) < 0) {
        return -1;
    }

    return 0;
}


/* Child.set */
PyDoc_STRVAR(Child_set_doc,
"set(pid, trace)\n\n\
Configures the watcher to wait for status changes of process 'pid' (or any\n\
process if 'pid' is specified as 0).\n\
The 'trace' argument must be either 0 (only activate the watcher when the\n\
process terminates) or 1 (additionally activate the watcher when the process\n\
is stopped or continued).\n\
You must not call this method on a watcher that is active.");

static PyObject *
Child_set(Child *self, PyObject *args)
{
    int pid;
    int trace;

    if (!PyArg_ParseTuple(args, "ii:set", &pid, &trace)) {
        return NULL;
    }

    if (set_child(self, pid, trace) < 0) {
        return NULL;
    }

    Py_RETURN_NONE;
}


/* Child.start */
static PyObject *
Child_start(Child *self, PyObject *unused)
{
    ev_child_start(((_Watcher *)self)->loop->loop, &self->watcher);

    Py_RETURN_NONE;
}


/* Child.stop */
static PyObject *
Child_stop(Child *self, PyObject *unused)
{
    ev_child_stop(((_Watcher *)self)->loop->loop, &self->watcher);

    Py_RETURN_NONE;
}


/* ChildType.tp_methods */
static PyMethodDef Child_methods[] = {
    {"set", (PyCFunction)Child_set, METH_VARARGS,
     Child_set_doc},
    {"start", (PyCFunction)Child_start, METH_NOARGS,
     _Watcher_start_doc},
    {"stop", (PyCFunction)Child_stop, METH_NOARGS,
     _Watcher_stop_doc},
    {NULL}  /* Sentinel */
};


/* ChildType.tp_members */
static PyMemberDef Child_members[] = {
    {"pid", T_INT, offsetof(Child, watcher.pid), READONLY,
     "The process id this watcher watches out for, or 0, meaning any process id."},
    {"rpid", T_INT, offsetof(Child, watcher.rpid), 0,
     "The process id that detected a status change."},
    {"rstatus", T_INT, offsetof(Child, watcher.rstatus), 0,
     "The process exit/trace status caused by rpid."},
    {NULL}  /* Sentinel */
};


/* ChildType */
static PyTypeObject ChildType = {
    PyObject_HEAD_INIT(NULL)
    0,                                        /*ob_size*/
    "ev.Child",                               /*tp_name*/
    sizeof(Child),                            /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    (destructor)Child_dealloc,                /*tp_dealloc*/
    0,                                        /*tp_print*/
    0,                                        /*tp_getattr*/
    0,                                        /*tp_setattr*/
    0,                                        /*tp_compare*/
    0,                                        /*tp_repr*/
    0,                                        /*tp_as_number*/
    0,                                        /*tp_as_sequence*/
    0,                                        /*tp_as_mapping*/
    0,                                        /*tp_hash */
    0,                                        /*tp_call*/
    0,                                        /*tp_str*/
    0,                                        /*tp_getattro*/
    0,                                        /*tp_setattro*/
    0,                                        /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    Child_doc,                                /*tp_doc*/
    0,                                        /*tp_traverse*/
    0,                                        /*tp_clear*/
    0,                                        /*tp_richcompare*/
    0,                                        /*tp_weaklistoffset*/
    0,                                        /*tp_iter*/
    0,                                        /*tp_iternext*/
    Child_methods,                            /*tp_methods*/
    Child_members,                            /*tp_members*/
    0,                                        /*tp_getsets*/
    0,                                        /*tp_base*/
    0,                                        /*tp_dict*/
    0,                                        /*tp_descr_get*/
    0,                                        /*tp_descr_set*/
    0,                                        /*tp_dictoffset*/
    (initproc)Child_init,                     /*tp_init*/
    0,                                        /*tp_alloc*/
    Child_new,                                /*tp_new*/
};


/* StatdataType.tp_dealloc */
static void
Statdata_dealloc(Statdata* self)
{
    self->ob_type->tp_free((PyObject*)self);
}


/* new */
static Statdata *
new_statdata(PyTypeObject *type, ev_statdata *statdata)
{
    Statdata *self;

    self = (Statdata *)type->tp_alloc(type, 0);
    if (self == NULL) {
        return NULL;
    }

    self->statdata = *statdata;

    return self;
}


/* StatdataType.tp_members */
static PyMemberDef Statdata_members[] = {
    {"nlink", T_LONG, offsetof(Statdata, statdata.st_nlink), READONLY,
     "number of hard links"},
    {"mode", T_LONG, offsetof(Statdata, statdata.st_mode), READONLY,
     "protection bits"},
    {"uid", T_LONG, offsetof(Statdata, statdata.st_uid), READONLY,
     "user ID of owner"},
    {"gid", T_LONG, offsetof(Statdata, statdata.st_gid), READONLY,
     "group ID of owner"},
    {"atime", T_LONG, offsetof(Statdata, statdata.st_atime), READONLY,
     "time of last access"},
    {"mtime", T_LONG, offsetof(Statdata, statdata.st_mtime), READONLY,
     "time of last modification"},
    {"ctime", T_LONG, offsetof(Statdata, statdata.st_ctime), READONLY,
     "time of last status change"},
#ifdef HAVE_LONG_LONG
    {"dev", T_LONGLONG, offsetof(Statdata, statdata.st_dev), READONLY,
     "device"},
    {"rdev", T_LONGLONG, offsetof(Statdata, statdata.st_rdev), READONLY,
     "device type"},
#else
    {"dev", T_LONG, offsetof(Statdata, statdata.st_dev), READONLY,
     "device"},
    {"rdev", T_LONG, offsetof(Statdata, statdata.st_rdev), READONLY,
     "device type"},
#endif
#ifdef HAVE_LARGEFILE_SUPPORT
    {"ino", T_LONGLONG, offsetof(Statdata, statdata.st_ino), READONLY,
     "inode"},
    {"size", T_LONGLONG, offsetof(Statdata, statdata.st_size), READONLY,
     "total size, in bytes"},
#else
    {"ino", T_LONG, offsetof(Statdata, statdata.st_ino), READONLY,
     "inode"},
    {"size", T_LONG, offsetof(Statdata, statdata.st_size), READONLY,
     "total size, in bytes"},
#endif
    {NULL}  /* Sentinel */
};


/* StatdataType */
static PyTypeObject StatdataType = {
    PyObject_HEAD_INIT(NULL)
    0,                                        /*ob_size*/
    "ev.Statdata",                            /*tp_name*/
    sizeof(Statdata),                         /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    (destructor)Statdata_dealloc,             /*tp_dealloc*/
    0,                                        /*tp_print*/
    0,                                        /*tp_getattr*/
    0,                                        /*tp_setattr*/
    0,                                        /*tp_compare*/
    0,                                        /*tp_repr*/
    0,                                        /*tp_as_number*/
    0,                                        /*tp_as_sequence*/
    0,                                        /*tp_as_mapping*/
    0,                                        /*tp_hash */
    0,                                        /*tp_call*/
    0,                                        /*tp_str*/
    0,                                        /*tp_getattro*/
    0,                                        /*tp_setattro*/
    0,                                        /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,                       /*tp_flags*/
    0,                                        /*tp_doc*/
    0,                                        /*tp_traverse*/
    0,                                        /*tp_clear*/
    0,                                        /*tp_richcompare*/
    0,                                        /*tp_weaklistoffset*/
    0,                                        /*tp_iter*/
    0,                                        /*tp_iternext*/
    0,                                        /*tp_methods*/
    Statdata_members,                         /*tp_members*/
};


/* StatType.tp_doc */
PyDoc_STRVAR(Stat_doc,
"Stat(path, interval, loop, [callback=None, [data=None]])\n\n\
This watches a filesystem path for attribute changes. That is, it calls stat\n\
regularly (or when the OS says it changed) and sees if it changed compared\n\
to the last time, invoking the callback if it did.\n\
See the set() method documentation.");


/* set up the stat */
static int
set_stat(Stat *self, const char *path, double interval)
{
    if (set_watcher((_Watcher *)self) < 0) {
        return -1;
    }

    ev_stat_set(&self->watcher, path, interval);

    return 0;
}


/* StatType.tp_dealloc */
static void
Stat_dealloc(Stat *self)
{
    _Watcher *_watcher = (_Watcher *)self;

    if (_watcher->loop && &self->watcher) {
        ev_stat_stop(_watcher->loop->loop, &self->watcher);
    }

    _WatcherType.tp_dealloc((PyObject *)self);
}


/* StatType.tp_new */
static PyObject *
Stat_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Stat *self = (Stat *)_WatcherType.tp_new(type, args, kwargs);
    if (self == NULL) {
        return NULL;
    }

    _Watcher_new((_Watcher *)self, (ev_watcher *)&self->watcher);

    return (PyObject *)self;
}


/* StatType.tp_init */
static int
Stat_init(Stat *self, PyObject *args, PyObject *kwargs)
{
    const char *path;
    double interval;
    Loop *loop;
    PyObject *callback = NULL;
    PyObject *data = NULL;

    static char *kwlist[] = {"path", "interval",
                             "loop", "callback", "data", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "sdO|OO:__init__", kwlist,
                                     &path, &interval,
                                     &loop, &callback, &data)) {
        return -1;
    }

    if (_Watcher_init((_Watcher *)self, loop, callback, data, 0) < 0) {
        return -1;
    }

    if (set_stat(self, path, interval) < 0) {
        return -1;
    }

    return 0;
}


/* Stat.set */
PyDoc_STRVAR(Stat_set_doc,
"set(path, interval)\n\n\
Configures the watcher to wait for status changes of the given path.\n\
The path does not need to exist: changing from 'path exists' to 'path does not\n\
exist' is a status change like any other. The condition 'path does not exist'\n\
is signified by the nlink field being zero (which is otherwise always forced\n\
to be at least one) and all the other attributes of the Statdata object\n\
(Stat.attr or Stat.prev) having unspecified contents.\n\
The path should be absolute and must not end in a slash. If it is relative and\n\
your working directory changes, the behaviour is undefined.\n\
The interval is a hint on how quickly a change is expected to be detected and\n\
should normally be specified as 0 to let libev choose a suitable value.\n\
You must not call this method on a watcher that is active.");

static PyObject *
Stat_set(Stat *self, PyObject *args)
{
    const char *path;
    double interval;

    if (!PyArg_ParseTuple(args, "sd:set", &path, &interval)) {
        return NULL;
    }

    if (set_stat(self, path, interval) < 0) {
        return NULL;
    }

    Py_RETURN_NONE;
}


/* Stat.start */
static PyObject *
Stat_start(Stat *self, PyObject *unused)
{
    ev_stat_start(((_Watcher *)self)->loop->loop, &self->watcher);

    Py_RETURN_NONE;
}


/* Stat.stop */
static PyObject *
Stat_stop(Stat *self, PyObject *unused)
{
    ev_stat_stop(((_Watcher *)self)->loop->loop, &self->watcher);

    Py_RETURN_NONE;
}


/* Stat.stat */
PyDoc_STRVAR(Stat_stat_doc,
"stat()\n\n\
Updates the Stat.attr object immediately with new values. If you change the\n\
watched path in your callback, you could call this method to avoid detecting\n\
this change (while introducing a race condition). Can also be useful simply to\n\
find out the new values.");

static PyObject *
Stat_stat(Stat *self, PyObject *unused)
{
    ev_stat_stat(((_Watcher *)self)->loop->loop, &self->watcher);

    if (self->watcher.attr.st_nlink == 0) {
        return PyErr_SetFromErrnoWithFilename(PyExc_OSError,
                                              (char *)self->watcher.path);
    }

    Py_RETURN_NONE;
}


/* Stat.attr */
PyDoc_STRVAR(Stat_attr_doc,
"The most-recently detected attributes of the file. If the 'nlink' attribute\n\
is 0, then there was some error while stating the file.");

static PyObject *
Stat_attr_get(Stat *self, void *closure)
{
    return (PyObject *)new_statdata(&StatdataType, &self->watcher.attr);
}


/* Stat.prev */
PyDoc_STRVAR(Stat_prev_doc,
"The previous attributes of the file.\n\
The callback gets invoked whenever Stat.prev != Stat.attr.");

static PyObject *
Stat_prev_get(Stat *self, void *closure)
{
    return (PyObject *)new_statdata(&StatdataType, &self->watcher.prev);
}


/* StatType.tp_methods */
static PyMethodDef Stat_methods[] = {
    {"set", (PyCFunction)Stat_set, METH_VARARGS,
     Stat_set_doc},
    {"start", (PyCFunction)Stat_start, METH_NOARGS,
     _Watcher_start_doc},
    {"stop", (PyCFunction)Stat_stop, METH_NOARGS,
     _Watcher_stop_doc},
    {"stat", (PyCFunction)Stat_stat, METH_NOARGS,
     Stat_stat_doc},
    {NULL}  /* Sentinel */
};


/* StatType.tp_members */
static PyMemberDef Stat_members[] = {
    {"interval", T_DOUBLE, offsetof(Stat, watcher.interval), READONLY,
     "The specified interval."},
    {"path", T_STRING, offsetof(Stat, watcher.path), READONLY,
     "The filesystem path that is being watched."},
    {NULL}  /* Sentinel */
};


/* StatType.tp_getsets */
static PyGetSetDef Stat_getsets[] = {
    {"attr", (getter)Stat_attr_get, NULL,
     Stat_attr_doc, NULL},
    {"prev", (getter)Stat_prev_get, NULL,
     Stat_prev_doc, NULL},
    {NULL}  /* Sentinel */
};


/* StatType */
static PyTypeObject StatType = {
    PyObject_HEAD_INIT(NULL)
    0,                                        /*ob_size*/
    "ev.Stat",                                /*tp_name*/
    sizeof(Stat),                             /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    (destructor)Stat_dealloc,                 /*tp_dealloc*/
    0,                                        /*tp_print*/
    0,                                        /*tp_getattr*/
    0,                                        /*tp_setattr*/
    0,                                        /*tp_compare*/
    0,                                        /*tp_repr*/
    0,                                        /*tp_as_number*/
    0,                                        /*tp_as_sequence*/
    0,                                        /*tp_as_mapping*/
    0,                                        /*tp_hash */
    0,                                        /*tp_call*/
    0,                                        /*tp_str*/
    0,                                        /*tp_getattro*/
    0,                                        /*tp_setattro*/
    0,                                        /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    Stat_doc,                                 /*tp_doc*/
    0,                                        /*tp_traverse*/
    0,                                        /*tp_clear*/
    0,                                        /*tp_richcompare*/
    0,                                        /*tp_weaklistoffset*/
    0,                                        /*tp_iter*/
    0,                                        /*tp_iternext*/
    Stat_methods,                             /*tp_methods*/
    Stat_members,                             /*tp_members*/
    Stat_getsets,                             /*tp_getsets*/
    0,                                        /*tp_base*/
    0,                                        /*tp_dict*/
    0,                                        /*tp_descr_get*/
    0,                                        /*tp_descr_set*/
    0,                                        /*tp_dictoffset*/
    (initproc)Stat_init,                      /*tp_init*/
    0,                                        /*tp_alloc*/
    Stat_new,                                 /*tp_new*/
};


/* IdleType.tp_doc */
PyDoc_STRVAR(Idle_doc,
"Idle(loop, [callback=None, [data=None]])\n\n\
Idle watchers trigger events when no other events of the same or higher priority\n\
are pending (Prepare, Check and other Idle watchers do not count).\n\
That is, as long as your process is busy handling sockets or timeouts (or even\n\
signals, imagine) of the same or higher priority it will not be triggered.\n\
But when your process is idle (or only lower-priority watchers are pending),\n\
the idle watchers are being called once per event loop iteration - until\n\
stopped, that is, or your process receives more events and becomes busy again\n\
with higher priority stuff.\n\
The most noteworthy effect is that as long as any Idle watchers are active,\n\
the process will not block when waiting for new events.\n\
Apart from keeping your process non-blocking (which is a useful effect on its\n\
own sometimes), Idle watchers are a good place to do 'pseudo-background\n\
processing', or delay processing stuff to after the event loop has handled all\n\
outstanding events.");


/* IdleType.tp_dealloc */
static void
Idle_dealloc(Idle *self)
{
    _Watcher *_watcher = (_Watcher *)self;

    if (_watcher->loop && &self->watcher) {
        ev_idle_stop(_watcher->loop->loop, &self->watcher);
    }

    _WatcherType.tp_dealloc((PyObject *)self);
}


/* IdleType.tp_new */
static PyObject *
Idle_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Idle *self = (Idle *)_WatcherType.tp_new(type, args, kwargs);
    if (self == NULL) {
        return NULL;
    }

    _Watcher_new((_Watcher *)self, (ev_watcher *)&self->watcher);

    return (PyObject *)self;
}


/* IdleType.tp_init */
static int
Idle_init(Idle *self, PyObject *args, PyObject *kwargs)
{
    Loop *loop;
    PyObject *callback = NULL;
    PyObject *data = NULL;

    static char *kwlist[] = {"loop", "callback", "data", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|OO:__init__", kwlist,
                                     &loop, &callback, &data)) {
        return -1;
    }

    if (_Watcher_init((_Watcher *)self, loop, callback, data, 0) < 0) {
        return -1;
    }

    if (set_watcher((_Watcher *)self) < 0) {
        return -1;
    }

    ev_idle_set(&self->watcher);

    return 0;
}


/* Idle.start */
static PyObject *
Idle_start(Idle *self, PyObject *unused)
{
    ev_idle_start(((_Watcher *)self)->loop->loop, &self->watcher);

    Py_RETURN_NONE;
}


/* Idle.stop */
static PyObject *
Idle_stop(Idle *self, PyObject *unused)
{
    ev_idle_stop(((_Watcher *)self)->loop->loop, &self->watcher);

    Py_RETURN_NONE;
}


/* IdleType.tp_methods */
static PyMethodDef Idle_methods[] = {
    {"start", (PyCFunction)Idle_start, METH_NOARGS,
     _Watcher_start_doc},
    {"stop", (PyCFunction)Idle_stop, METH_NOARGS,
     _Watcher_stop_doc},
    {NULL}  /* Sentinel */
};


/* IdleType */
static PyTypeObject IdleType = {
    PyObject_HEAD_INIT(NULL)
    0,                                        /*ob_size*/
    "ev.Idle",                                /*tp_name*/
    sizeof(Idle),                             /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    (destructor)Idle_dealloc,                 /*tp_dealloc*/
    0,                                        /*tp_print*/
    0,                                        /*tp_getattr*/
    0,                                        /*tp_setattr*/
    0,                                        /*tp_compare*/
    0,                                        /*tp_repr*/
    0,                                        /*tp_as_number*/
    0,                                        /*tp_as_sequence*/
    0,                                        /*tp_as_mapping*/
    0,                                        /*tp_hash */
    0,                                        /*tp_call*/
    0,                                        /*tp_str*/
    0,                                        /*tp_getattro*/
    0,                                        /*tp_setattro*/
    0,                                        /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    Idle_doc,                                 /*tp_doc*/
    0,                                        /*tp_traverse*/
    0,                                        /*tp_clear*/
    0,                                        /*tp_richcompare*/
    0,                                        /*tp_weaklistoffset*/
    0,                                        /*tp_iter*/
    0,                                        /*tp_iternext*/
    Idle_methods,                             /*tp_methods*/
    0,                                        /*tp_members*/
    0,                                        /*tp_getsets*/
    0,                                        /*tp_base*/
    0,                                        /*tp_dict*/
    0,                                        /*tp_descr_get*/
    0,                                        /*tp_descr_set*/
    0,                                        /*tp_dictoffset*/
    (initproc)Idle_init,                      /*tp_init*/
    0,                                        /*tp_alloc*/
    Idle_new,                                 /*tp_new*/
};


/* PrepareType.tp_doc */
PyDoc_STRVAR(Prepare_doc,
"Prepare(loop, [callback=None, [data=None]])\n\n\
");


/* PrepareType.tp_dealloc */
static void
Prepare_dealloc(Prepare *self)
{
    _Watcher *_watcher = (_Watcher *)self;

    if (_watcher->loop && &self->watcher) {
        ev_prepare_stop(_watcher->loop->loop, &self->watcher);
    }

    _WatcherType.tp_dealloc((PyObject *)self);
}


/* PrepareType.tp_new */
static PyObject *
Prepare_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Prepare *self = (Prepare *)_WatcherType.tp_new(type, args, kwargs);
    if (self == NULL) {
        return NULL;
    }

    _Watcher_new((_Watcher *)self, (ev_watcher *)&self->watcher);

    return (PyObject *)self;
}


/* PrepareType.tp_init */
static int
Prepare_init(Prepare *self, PyObject *args, PyObject *kwargs)
{
    Loop *loop;
    PyObject *callback = NULL;
    PyObject *data = NULL;

    static char *kwlist[] = {"loop", "callback", "data", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|OO:__init__", kwlist,
                                     &loop, &callback, &data)) {
        return -1;
    }

    if (_Watcher_init((_Watcher *)self, loop, callback, data, 0) < 0) {
        return -1;
    }

    if (set_watcher((_Watcher *)self) < 0) {
        return -1;
    }

    ev_prepare_set(&self->watcher);

    return 0;
}


/* Prepare.start */
static PyObject *
Prepare_start(Prepare *self, PyObject *unused)
{
    ev_prepare_start(((_Watcher *)self)->loop->loop, &self->watcher);

    Py_RETURN_NONE;
}


/* Prepare.stop */
static PyObject *
Prepare_stop(Prepare *self, PyObject *unused)
{
    ev_prepare_stop(((_Watcher *)self)->loop->loop, &self->watcher);

    Py_RETURN_NONE;
}


/* PrepareType.tp_methods */
static PyMethodDef Prepare_methods[] = {
    {"start", (PyCFunction)Prepare_start, METH_NOARGS,
     _Watcher_start_doc},
    {"stop", (PyCFunction)Prepare_stop, METH_NOARGS,
     _Watcher_stop_doc},
    {NULL}  /* Sentinel */
};


/* PrepareType */
static PyTypeObject PrepareType = {
    PyObject_HEAD_INIT(NULL)
    0,                                        /*ob_size*/
    "ev.Prepare",                             /*tp_name*/
    sizeof(Prepare),                          /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    (destructor)Prepare_dealloc,              /*tp_dealloc*/
    0,                                        /*tp_print*/
    0,                                        /*tp_getattr*/
    0,                                        /*tp_setattr*/
    0,                                        /*tp_compare*/
    0,                                        /*tp_repr*/
    0,                                        /*tp_as_number*/
    0,                                        /*tp_as_sequence*/
    0,                                        /*tp_as_mapping*/
    0,                                        /*tp_hash */
    0,                                        /*tp_call*/
    0,                                        /*tp_str*/
    0,                                        /*tp_getattro*/
    0,                                        /*tp_setattro*/
    0,                                        /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    Prepare_doc,                              /*tp_doc*/
    0,                                        /*tp_traverse*/
    0,                                        /*tp_clear*/
    0,                                        /*tp_richcompare*/
    0,                                        /*tp_weaklistoffset*/
    0,                                        /*tp_iter*/
    0,                                        /*tp_iternext*/
    Prepare_methods,                          /*tp_methods*/
    0,                                        /*tp_members*/
    0,                                        /*tp_getsets*/
    0,                                        /*tp_base*/
    0,                                        /*tp_dict*/
    0,                                        /*tp_descr_get*/
    0,                                        /*tp_descr_set*/
    0,                                        /*tp_dictoffset*/
    (initproc)Prepare_init,                   /*tp_init*/
    0,                                        /*tp_alloc*/
    Prepare_new,                              /*tp_new*/
};


/* CheckType.tp_doc */
PyDoc_STRVAR(Check_doc,
"Check(loop, [callback=None, [data=None]])\n\n\
");


/* CheckType.tp_dealloc */
static void
Check_dealloc(Check *self)
{
    _Watcher *_watcher = (_Watcher *)self;

    if (_watcher->loop && &self->watcher) {
        ev_check_stop(_watcher->loop->loop, &self->watcher);
    }

    _WatcherType.tp_dealloc((PyObject *)self);
}


/* CheckType.tp_new */
static PyObject *
Check_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Check *self = (Check *)_WatcherType.tp_new(type, args, kwargs);
    if (self == NULL) {
        return NULL;
    }

    _Watcher_new((_Watcher *)self, (ev_watcher *)&self->watcher);

    return (PyObject *)self;
}


/* CheckType.tp_init */
static int
Check_init(Check *self, PyObject *args, PyObject *kwargs)
{
    Loop *loop;
    PyObject *callback = NULL;
    PyObject *data = NULL;

    static char *kwlist[] = {"loop", "callback", "data", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|OO:__init__", kwlist,
                                     &loop, &callback, &data)) {
        return -1;
    }

    if (_Watcher_init((_Watcher *)self, loop, callback, data, 0) < 0) {
        return -1;
    }

    if (set_watcher((_Watcher *)self) < 0) {
        return -1;
    }

    ev_check_set(&self->watcher);

    return 0;
}


/* Check.start */
static PyObject *
Check_start(Check *self, PyObject *unused)
{
    ev_check_start(((_Watcher *)self)->loop->loop, &self->watcher);

    Py_RETURN_NONE;
}


/* Check.stop */
static PyObject *
Check_stop(Check *self, PyObject *unused)
{
    ev_check_stop(((_Watcher *)self)->loop->loop, &self->watcher);

    Py_RETURN_NONE;
}


/* CheckType.tp_methods */
static PyMethodDef Check_methods[] = {
    {"start", (PyCFunction)Check_start, METH_NOARGS,
     _Watcher_start_doc},
    {"stop", (PyCFunction)Check_stop, METH_NOARGS,
     _Watcher_stop_doc},
    {NULL}  /* Sentinel */
};


/* CheckType */
static PyTypeObject CheckType = {
    PyObject_HEAD_INIT(NULL)
    0,                                        /*ob_size*/
    "ev.Check",                               /*tp_name*/
    sizeof(Check),                            /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    (destructor)Check_dealloc,                /*tp_dealloc*/
    0,                                        /*tp_print*/
    0,                                        /*tp_getattr*/
    0,                                        /*tp_setattr*/
    0,                                        /*tp_compare*/
    0,                                        /*tp_repr*/
    0,                                        /*tp_as_number*/
    0,                                        /*tp_as_sequence*/
    0,                                        /*tp_as_mapping*/
    0,                                        /*tp_hash */
    0,                                        /*tp_call*/
    0,                                        /*tp_str*/
    0,                                        /*tp_getattro*/
    0,                                        /*tp_setattro*/
    0,                                        /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    Check_doc,                                /*tp_doc*/
    0,                                        /*tp_traverse*/
    0,                                        /*tp_clear*/
    0,                                        /*tp_richcompare*/
    0,                                        /*tp_weaklistoffset*/
    0,                                        /*tp_iter*/
    0,                                        /*tp_iternext*/
    Check_methods,                            /*tp_methods*/
    0,                                        /*tp_members*/
    0,                                        /*tp_getsets*/
    0,                                        /*tp_base*/
    0,                                        /*tp_dict*/
    0,                                        /*tp_descr_get*/
    0,                                        /*tp_descr_set*/
    0,                                        /*tp_dictoffset*/
    (initproc)Check_init,                     /*tp_init*/
    0,                                        /*tp_alloc*/
    Check_new,                                /*tp_new*/
};


/* ForkType.tp_doc */
PyDoc_STRVAR(Fork_doc,
"Fork(loop, [callback=None, [data=None]])\n\n\
Fork watchers are called when a fork() was detected (usually because whoever is\n\
a good citizen cared to tell libev about it by calling Loop.fork()).\n\
The invocation is done before the event loop blocks next and before Check\n\
watchers are being called, and only in the child after the fork. If whoever\n\
good citizen calling Loop.fork() cheats and calls it in the wrong process,\n\
the fork handlers will be invoked, too, of course.");


/* ForkType.tp_dealloc */
static void
Fork_dealloc(Fork *self)
{
    _Watcher *_watcher = (_Watcher *)self;

    if (_watcher->loop && &self->watcher) {
        ev_fork_stop(_watcher->loop->loop, &self->watcher);
    }

    _WatcherType.tp_dealloc((PyObject *)self);
}


/* ForkType.tp_new */
static PyObject *
Fork_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Fork *self = (Fork *)_WatcherType.tp_new(type, args, kwargs);
    if (self == NULL) {
        return NULL;
    }

    _Watcher_new((_Watcher *)self, (ev_watcher *)&self->watcher);

    return (PyObject *)self;
}


/* ForkType.tp_init */
static int
Fork_init(Fork *self, PyObject *args, PyObject *kwargs)
{
    Loop *loop;
    PyObject *callback = NULL;
    PyObject *data = NULL;

    static char *kwlist[] = {"loop", "callback", "data", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|OO:__init__", kwlist,
                                     &loop, &callback, &data)) {
        return -1;
    }

    if (_Watcher_init((_Watcher *)self, loop, callback, data, 0) < 0) {
        return -1;
    }

    if (set_watcher((_Watcher *)self) < 0) {
        return -1;
    }

    ev_fork_set(&self->watcher);

    return 0;
}


/* Fork.start */
static PyObject *
Fork_start(Fork *self, PyObject *unused)
{
    ev_fork_start(((_Watcher *)self)->loop->loop, &self->watcher);

    Py_RETURN_NONE;
}


/* Fork.stop */
static PyObject *
Fork_stop(Fork *self, PyObject *unused)
{
    ev_fork_stop(((_Watcher *)self)->loop->loop, &self->watcher);

    Py_RETURN_NONE;
}


/* ForkType.tp_methods */
static PyMethodDef Fork_methods[] = {
    {"start", (PyCFunction)Fork_start, METH_NOARGS,
     _Watcher_start_doc},
    {"stop", (PyCFunction)Fork_stop, METH_NOARGS,
     _Watcher_stop_doc},
    {NULL}  /* Sentinel */
};


/* ForkType */
static PyTypeObject ForkType = {
    PyObject_HEAD_INIT(NULL)
    0,                                        /*ob_size*/
    "ev.Fork",                                /*tp_name*/
    sizeof(Fork),                             /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    (destructor)Fork_dealloc,                 /*tp_dealloc*/
    0,                                        /*tp_print*/
    0,                                        /*tp_getattr*/
    0,                                        /*tp_setattr*/
    0,                                        /*tp_compare*/
    0,                                        /*tp_repr*/
    0,                                        /*tp_as_number*/
    0,                                        /*tp_as_sequence*/
    0,                                        /*tp_as_mapping*/
    0,                                        /*tp_hash */
    0,                                        /*tp_call*/
    0,                                        /*tp_str*/
    0,                                        /*tp_getattro*/
    0,                                        /*tp_setattro*/
    0,                                        /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    Fork_doc,                                 /*tp_doc*/
    0,                                        /*tp_traverse*/
    0,                                        /*tp_clear*/
    0,                                        /*tp_richcompare*/
    0,                                        /*tp_weaklistoffset*/
    0,                                        /*tp_iter*/
    0,                                        /*tp_iternext*/
    Fork_methods,                             /*tp_methods*/
    0,                                        /*tp_members*/
    0,                                        /*tp_getsets*/
    0,                                        /*tp_base*/
    0,                                        /*tp_dict*/
    0,                                        /*tp_descr_get*/
    0,                                        /*tp_descr_set*/
    0,                                        /*tp_dictoffset*/
    (initproc)Fork_init,                      /*tp_init*/
    0,                                        /*tp_alloc*/
    Fork_new,                                 /*tp_new*/
};


/* EmbedType.tp_doc */
PyDoc_STRVAR(Embed_doc,
"Embed(other, loop, [callback=None, [data=None]])\n\n\
See the set() method documentation.");


/* set up the embed */
static int
set_embed(Embed *self, Loop *other)
{
    if (set_watcher((_Watcher *)self) < 0) {
        return -1;
    }

    ev_embed_set(&self->watcher, other->loop);

    return 0;
}


/* embed callback */
static void
Embed_cb(struct ev_loop *loop, ev_watcher *watcher, int events)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    _Watcher *_watcher = watcher->data;

    if (_watcher->callback != Py_None) {
        _Watcher_cb(loop, watcher, events);
    }
    else {
        ev_embed_sweep(_watcher->loop->loop, (ev_embed *)watcher);
    }

    PyGILState_Release(gstate);
}


/* EmbedType.tp_dealloc */
static void
Embed_dealloc(Embed *self)
{
    _Watcher *_watcher = (_Watcher *)self;

    if (_watcher->loop && &self->watcher) {
        ev_embed_stop(_watcher->loop->loop, &self->watcher);
    }

    _WatcherType.tp_dealloc((PyObject *)self);
}


/* EmbedType.tp_new */
static PyObject *
Embed_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Embed *self = (Embed *)_WatcherType.tp_new(type, args, kwargs);
    if (self == NULL) {
        return NULL;
    }

    _Watcher_new((_Watcher *)self, (ev_watcher *)&self->watcher);

    /* unfortunately we have to re-init here cause the callback is different*/
    ev_init((ev_watcher *)&self->watcher, Embed_cb);


    return (PyObject *)self;
}

static int
embed_other_set(Embed *self, Loop *other)
{
    PyObject *tmp;

    if (!PyObject_TypeCheck(other, &LoopType)) {
        PyErr_SetString(PyExc_TypeError, "an ev.Loop is required");
        return -1;
    }

    if (!(ev_backend(other->loop) & ev_embeddable_backends())) {
        PyErr_SetString(EvError, "'other' must be embeddable");
        return -1;
    }

    tmp = (PyObject *)self->other;
    Py_INCREF(other);
    self->other = other;
    Py_XDECREF(tmp);

    return 0;
}


/* EmbedType.tp_init */
static int
Embed_init(Embed *self, PyObject *args, PyObject *kwargs)
{
    Loop *other;
    Loop *loop;
    PyObject *callback = NULL;
    PyObject *data = NULL;

    static char *kwlist[] = {"other",
                             "loop", "callback", "data", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO|OO:__init__", kwlist,
                                     &other,
                                     &loop, &callback, &data)) {
        return -1;
    }

    if (_Watcher_init((_Watcher *)self, loop, callback, data, 0) < 0) {
        return -1;
    }

    /* self->other */
    if (embed_other_set(self, other) < 0) {
        return -1;
    }

    if (set_embed(self, other) < 0) {
        return -1;
    }

    return 0;
}


/* Embed.set */
PyDoc_STRVAR(Embed_set_doc,
"set(other)\n\n\
Configures the watcher to embed the given loop, which must be embeddable.\n\
If the Embed.callback is None, then Embed.sweep() will be invoked automatically,\n\
otherwise it is the responsibility of the callback to invoke it (it will\n\
continue to be called until the sweep has been done, if you do not want that,\n\
you need to temporarily stop the embed watcher).\n\
You must not call this method on a watcher that is active.");

static PyObject *
Embed_set(Embed *self, PyObject *args)
{
    Loop *other;

    if (!PyArg_ParseTuple(args, "O:set", &other)) {
        return NULL;
    }

    /* self->other */
    if (embed_other_set(self, other) < 0) {
        return NULL;
    }

    if (set_embed(self, other) < 0) {
        return NULL;
    }

    Py_RETURN_NONE;
}


/* Embed.start */
static PyObject *
Embed_start(Embed *self, PyObject *unused)
{
    ev_embed_start(((_Watcher *)self)->loop->loop, &self->watcher);

    Py_RETURN_NONE;
}


/* Embed.stop */
static PyObject *
Embed_stop(Embed *self, PyObject *unused)
{
    ev_embed_stop(((_Watcher *)self)->loop->loop, &self->watcher);

    Py_RETURN_NONE;
}


/* Embed.sweep */
PyDoc_STRVAR(Embed_sweep_doc,
"sweep()\n\n\
Make a single, non-blocking sweep over the embedded loop.");

static PyObject *
Embed_sweep(Embed *self, PyObject *unused)
{
    ev_embed_sweep(((_Watcher *)self)->loop->loop, &self->watcher);

    Py_RETURN_NONE;
}


/* EmbedType.tp_methods */
static PyMethodDef Embed_methods[] = {
    {"set", (PyCFunction)Embed_set, METH_VARARGS,
     Embed_set_doc},
    {"start", (PyCFunction)Embed_start, METH_NOARGS,
     _Watcher_start_doc},
    {"stop", (PyCFunction)Embed_stop, METH_NOARGS,
     _Watcher_stop_doc},
    {"sweep", (PyCFunction)Embed_sweep, METH_NOARGS,
     Embed_sweep_doc},
    {NULL}  /* Sentinel */
};


/* EmbedType.tp_members */
static PyMemberDef Embed_members[] = {
    {"other", T_OBJECT_EX, offsetof(Embed, other), READONLY,
     "The embedded event loop."},
    {NULL}  /* Sentinel */
};


/* EmbedType */
static PyTypeObject EmbedType = {
    PyObject_HEAD_INIT(NULL)
    0,                                        /*ob_size*/
    "ev.Embed",                               /*tp_name*/
    sizeof(Embed),                            /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    (destructor)Embed_dealloc,                /*tp_dealloc*/
    0,                                        /*tp_print*/
    0,                                        /*tp_getattr*/
    0,                                        /*tp_setattr*/
    0,                                        /*tp_compare*/
    0,                                        /*tp_repr*/
    0,                                        /*tp_as_number*/
    0,                                        /*tp_as_sequence*/
    0,                                        /*tp_as_mapping*/
    0,                                        /*tp_hash */
    0,                                        /*tp_call*/
    0,                                        /*tp_str*/
    0,                                        /*tp_getattro*/
    0,                                        /*tp_setattro*/
    0,                                        /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    Embed_doc,                                /*tp_doc*/
    0,                                        /*tp_traverse*/
    0,                                        /*tp_clear*/
    0,                                        /*tp_richcompare*/
    0,                                        /*tp_weaklistoffset*/
    0,                                        /*tp_iter*/
    0,                                        /*tp_iternext*/
    Embed_methods,                            /*tp_methods*/
    Embed_members,                            /*tp_members*/
    0,                                        /*tp_getsets*/
    0,                                        /*tp_base*/
    0,                                        /*tp_dict*/
    0,                                        /*tp_descr_get*/
    0,                                        /*tp_descr_set*/
    0,                                        /*tp_dictoffset*/
    (initproc)Embed_init,                     /*tp_init*/
    0,                                        /*tp_alloc*/
    Embed_new,                                /*tp_new*/
};


/* AsyncType.tp_doc */
PyDoc_STRVAR(Async_doc,
"Async(loop, [callback=None, [data=None]])\n\n\
In general, you cannot use a Loop from multiple threads or other asynchronous\n\
sources such as signal handlers (as opposed to multiple event loops - those are\n\
of course safe to use in different threads).\n\
Sometimes, however, you need to wake up another event loop you do not control,\n\
for example because it belongs to another thread. This is what Async watchers\n\
do: as long as the Async watcher is active, you can signal it by calling\n\
Async.send(), which is thread- and signal safe.\n\
This functionality is very similar to Signal watchers, as signals, too, are\n\
asynchronous in nature, and signals, too, will be compressed (i.e. the number\n\
of callback invocations may be less than the number of ev_async_sent calls).\n\
Unlike Signal watchers, Async works with any event loop, not just the default loop.");


/* AsyncType.tp_dealloc */
static void
Async_dealloc(Async *self)
{
    _Watcher *_watcher = (_Watcher *)self;

    if (_watcher->loop && &self->watcher) {
        ev_async_stop(_watcher->loop->loop, &self->watcher);
    }

    _WatcherType.tp_dealloc((PyObject *)self);
}


/* AsyncType.tp_new */
static PyObject *
Async_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Async *self = (Async *)_WatcherType.tp_new(type, args, kwargs);
    if (self == NULL) {
        return NULL;
    }

    _Watcher_new((_Watcher *)self, (ev_watcher *)&self->watcher);

    return (PyObject *)self;
}


/* AsyncType.tp_init */
static int
Async_init(Async *self, PyObject *args, PyObject *kwargs)
{
    Loop *loop;
    PyObject *callback = NULL;
    PyObject *data = NULL;

    static char *kwlist[] = {"loop", "callback", "data", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|OO:__init__", kwlist,
                                     &loop, &callback, &data)) {
        return -1;
    }

    if (_Watcher_init((_Watcher *)self, loop, callback, data, 0) < 0) {
        return -1;
    }

    if (set_watcher((_Watcher *)self) < 0) {
        return -1;
    }

    ev_async_set(&self->watcher);

    return 0;
}


/* Async.start */
static PyObject *
Async_start(Async *self, PyObject *unused)
{
    ev_async_start(((_Watcher *)self)->loop->loop, &self->watcher);

    Py_RETURN_NONE;
}


/* Async.stop */
static PyObject *
Async_stop(Async *self, PyObject *unused)
{
    ev_async_stop(((_Watcher *)self)->loop->loop, &self->watcher);

    Py_RETURN_NONE;
}


/* Async.send */
PyDoc_STRVAR(Async_send_doc,
"send()\n\n\
Sends/signals/activates the Async watcher, that is, feeds an EV_ASYNC event on\n\
the watcher into the event loop. This call is safe to do in other threads,\n\
signal or similar contexts.\n\
This call incurs the overhead of a syscall only once per loop iteration, so\n\
while the overhead might be noticable, it doesn't apply to repeated calls send().");

static PyObject *
Async_send(Async *self, PyObject *unused)
{
    ev_async_send(((_Watcher *)self)->loop->loop, &self->watcher);

    Py_RETURN_NONE;
}


/* AsyncType.tp_methods */
static PyMethodDef Async_methods[] = {
    {"start", (PyCFunction)Async_start, METH_NOARGS,
     _Watcher_start_doc},
    {"stop", (PyCFunction)Async_stop, METH_NOARGS,
     _Watcher_stop_doc},
    {"send", (PyCFunction)Async_send, METH_NOARGS,
     Async_send_doc},
    {NULL}  /* Sentinel */
};


/* AsyncType */
static PyTypeObject AsyncType = {
    PyObject_HEAD_INIT(NULL)
    0,                                        /*ob_size*/
    "ev.Async",                               /*tp_name*/
    sizeof(Async),                            /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    (destructor)Async_dealloc,                /*tp_dealloc*/
    0,                                        /*tp_print*/
    0,                                        /*tp_getattr*/
    0,                                        /*tp_setattr*/
    0,                                        /*tp_compare*/
    0,                                        /*tp_repr*/
    0,                                        /*tp_as_number*/
    0,                                        /*tp_as_sequence*/
    0,                                        /*tp_as_mapping*/
    0,                                        /*tp_hash */
    0,                                        /*tp_call*/
    0,                                        /*tp_str*/
    0,                                        /*tp_getattro*/
    0,                                        /*tp_setattro*/
    0,                                        /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    Async_doc,                                /*tp_doc*/
    0,                                        /*tp_traverse*/
    0,                                        /*tp_clear*/
    0,                                        /*tp_richcompare*/
    0,                                        /*tp_weaklistoffset*/
    0,                                        /*tp_iter*/
    0,                                        /*tp_iternext*/
    Async_methods,                            /*tp_methods*/
    0,                                        /*tp_members*/
    0,                                        /*tp_getsets*/
    0,                                        /*tp_base*/
    0,                                        /*tp_dict*/
    0,                                        /*tp_descr_get*/
    0,                                        /*tp_descr_set*/
    0,                                        /*tp_dictoffset*/
    (initproc)Async_init,                     /*tp_init*/
    0,                                        /*tp_alloc*/
    Async_new,                                /*tp_new*/
};


/* ev.__doc__ */
PyDoc_STRVAR(_ev_doc,
"Libev is an event loop: you register interest in certain events (such as\n\
a file descriptor being readable or a timeout occurring), and it will manage\n\
these event sources and provide your program with events.\n\
To do this, it must take more or less complete control over your process\n\
(or thread) by executing the event loop handler, and will then communicate\n\
events via a callback mechanism.\n\
You register interest in certain events by registering so-called event\n\
watchers, which you initialise with the details of the event, and then\n\
hand it over to libev by starting the watcher.\n\
The library knows two types of event loops, the 'default loop', which supports\n\
signals and child events, and dynamically created loops which do not.\n\
If you use threads, a common model is to run the default event loop in your\n\
main thread (or in a separate thread) and for each thread you create, you also\n\
create another event loop. Libev itself does no locking whatsoever, so if you\n\
mix calls to the same event loop in different threads, make sure you lock.\n\
The main documentation for libev is at:\n\
http://pod.tst.eu/http://cvs.schmorp.de/libev/ev.pod");


/* ev.default_loop */
PyDoc_STRVAR(_ev_default_loop_doc,
"default_loop([flags]) -> ev.Loop\n\n\
Returns the 'default loop'. The 'default loop' is the only loop that can\n\
handle signal and child watchers, and to do this, it always registers a\n\
handler for SIGCHLD.\n\
The 'flags' argument can be used to specify special behaviour or specific\n\
backends to use, it defaults to EVFLAG_AUTO. See documentation for EVFLAG_*\n\
and EVBACKEND_* at http://pod.tst.eu/http://cvs.schmorp.de/libev/ev.pod for\n\
more information.");

static PyObject *
_ev_default_loop(PyObject *self, PyObject *args)
{
    unsigned int flags = EVFLAG_AUTO;

    if (!PyArg_ParseTuple(args, "|I:default_loop", &flags)) {
        return NULL;
    }

    if (!_DefaultLoop) {
        _DefaultLoop = new_loop(&LoopType, flags, 1);
        if (_DefaultLoop == NULL) {
            return NULL;
        }
        PyModule_AddObject(_ev, "_DefaultLoop", (PyObject *)_DefaultLoop);
    }
    else {
        if (PyErr_WarnEx(PyExc_UserWarning,
                         "returning the 'default_loop' created earlier, "
                         "'flags' argument ignored", 1) < 0) {
            return NULL;
        }
    }

    Py_INCREF(_DefaultLoop);
    return (PyObject *)_DefaultLoop;
}


/* ev.sleep */
PyDoc_STRVAR(_ev_sleep_doc,
"sleep(interval)\n\n\
Sleep for the given interval: The current thread will be blocked\n\
until either it is interrupted or the given time interval has passed.");

static PyObject *
_ev_sleep(PyObject *self, PyObject *args)
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


/* ev.version */
PyDoc_STRVAR(_ev_version_doc,
"version() -> (int, int)\n\n\
Returns a tuple of major, minor ABI version numbers of the linked libev library.");

static PyObject *
_ev_version(PyObject *self, PyObject *unused)
{
    return Py_BuildValue("(ii)", ev_version_major(), ev_version_minor());
}


/* ev.time */
PyDoc_STRVAR(_ev_time_doc,
"time() -> float\n\n\
Returns the current time as libev would use it.\n\
Please note that the now() method of the Loop object is usually faster and\n\
also often returns the timestamp you actually want to know.");

static PyObject *
_ev_time(PyObject *self, PyObject *unused)
{
    return Py_BuildValue("d", ev_time());
}


/* ev.supported_backends */
PyDoc_STRVAR(_ev_supported_backends_doc,
"supported_backends() -> int\n\n\
Return the set of all backends (i.e. their corresponding EVBACKEND_* value)\n\
compiled into this binary of libev (independent of their availability\n\
on the system you are running on).");

static PyObject *
_ev_supported_backends(PyObject *self, PyObject *unused)
{
    return Py_BuildValue("I", ev_supported_backends());
}


/* ev.recommended_backends */
PyDoc_STRVAR(_ev_recommended_backends_doc,
"recommended_backends() -> int\n\n\
Return the set of all backends compiled into this binary of libev and\n\
also recommended for this platform. This set is often smaller than the\n\
one returned by supported_backends(), as for example kqueue is broken\n\
on most BSDs and will not be autodetected unless you explicitly request it.\n\
This is the set of backends that libev will probe for if you specify no\n\
backends explicitly.");

static PyObject *
_ev_recommended_backends(PyObject *self, PyObject *unused)
{
    return Py_BuildValue("I", ev_recommended_backends());
}


/* ev.embeddable_backends */
PyDoc_STRVAR(_ev_embeddable_backends_doc,
"embeddable_backends() -> int\n\n\
Returns the set of backends that are embeddable in other event loops.\n\
This is the theoretical, all-platform, value. To find which backends\n\
might be supported on the current system, you would need to look at\n\
embeddable_backends() & supported_backends(), likewise for recommended ones.");

static PyObject *
_ev_embeddable_backends(PyObject *self, PyObject *unused)
{
    return Py_BuildValue("I", ev_embeddable_backends());
}


/* module method table */
static PyMethodDef _ev_methods[] = {
    {"default_loop", (PyCFunction)_ev_default_loop, METH_VARARGS,
     _ev_default_loop_doc},
    {"sleep", (PyCFunction)_ev_sleep, METH_VARARGS,
     _ev_sleep_doc},
    {"version", (PyCFunction)_ev_version, METH_NOARGS,
     _ev_version_doc},
    {"time", (PyCFunction)_ev_time, METH_NOARGS,
     _ev_time_doc},
    {"supported_backends", (PyCFunction)_ev_supported_backends, METH_NOARGS,
     _ev_supported_backends_doc},
    {"recommended_backends", (PyCFunction)_ev_recommended_backends, METH_NOARGS,
     _ev_recommended_backends_doc},
    {"embeddable_backends", (PyCFunction)_ev_embeddable_backends, METH_NOARGS,
     _ev_embeddable_backends_doc},
    {NULL} /* Sentinel */
};


static void *
_ev_realloc(void *ptr, long size)
{
    return PyMem_Realloc(ptr, (size_t)size);
}


static void
_ev_fatal_error(const char *msg)
{
    Py_FatalError(msg);
}


/* module initialization */
#ifndef PyMODINIT_FUNC /* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif
PyMODINIT_FUNC
init_ev(void)
{
    /* fill in deferred data addresses */
    IoType.tp_base = &_WatcherType;
    TimerType.tp_base = &_WatcherType;
    PeriodicType.tp_base = &_WatcherType;
    SignalType.tp_base = &_WatcherType;
    ChildType.tp_base = &_WatcherType;
    StatType.tp_base = &_WatcherType;
    IdleType.tp_base = &_WatcherType;
    PrepareType.tp_base = &_WatcherType;
    CheckType.tp_base = &_WatcherType;
    ForkType.tp_base = &_WatcherType;
    EmbedType.tp_base = &_WatcherType;
    AsyncType.tp_base = &_WatcherType;

    /* checking types */
    if (PyType_Ready(&LoopType) < 0 ||
        PyType_Ready(&_WatcherType) < 0 ||
        PyType_Ready(&IoType) < 0 ||
        PyType_Ready(&TimerType) < 0 ||
        PyType_Ready(&PeriodicType) < 0 ||
        PyType_Ready(&SignalType) < 0 ||
        PyType_Ready(&ChildType) < 0 ||
        PyType_Ready(&StatdataType) < 0 ||
        PyType_Ready(&StatType) < 0 ||
        PyType_Ready(&IdleType) < 0 ||
        PyType_Ready(&PrepareType) < 0 ||
        PyType_Ready(&CheckType) < 0 ||
        PyType_Ready(&ForkType) < 0 ||
        PyType_Ready(&EmbedType) < 0 ||
        PyType_Ready(&AsyncType) < 0) {
        return;
    }

    /* init module */
    _ev = Py_InitModule3("_ev", _ev_methods, _ev_doc);
    if (_ev == NULL) {
        return;
    }

    /* adding types */
    Py_INCREF(&LoopType);
    PyModule_AddObject(_ev, "Loop", (PyObject *)&LoopType);
    Py_INCREF(&IoType);
    PyModule_AddObject(_ev, "Io", (PyObject *)&IoType);
    Py_INCREF(&TimerType);
    PyModule_AddObject(_ev, "Timer", (PyObject *)&TimerType);
    Py_INCREF(&PeriodicType);
    PyModule_AddObject(_ev, "Periodic", (PyObject *)&PeriodicType);
    Py_INCREF(&SignalType);
    PyModule_AddObject(_ev, "Signal", (PyObject *)&SignalType);
    Py_INCREF(&ChildType);
    PyModule_AddObject(_ev, "Child", (PyObject *)&ChildType);
    Py_INCREF(&StatType);
    PyModule_AddObject(_ev, "Stat", (PyObject *)&StatType);
    Py_INCREF(&IdleType);
    PyModule_AddObject(_ev, "Idle", (PyObject *)&IdleType);
    Py_INCREF(&PrepareType);
    PyModule_AddObject(_ev, "Prepare", (PyObject *)&PrepareType);
    Py_INCREF(&CheckType);
    PyModule_AddObject(_ev, "Check", (PyObject *)&CheckType);
    Py_INCREF(&ForkType);
    PyModule_AddObject(_ev, "Fork", (PyObject *)&ForkType);
    Py_INCREF(&EmbedType);
    PyModule_AddObject(_ev, "Embed", (PyObject *)&EmbedType);
    Py_INCREF(&AsyncType);
    PyModule_AddObject(_ev, "Async", (PyObject *)&AsyncType);

    /* adding ev.Error exception */
    EvError = PyErr_NewException("ev.Error", NULL, NULL);
    if (EvError == NULL) {
        return;
    }
    Py_INCREF(EvError);
    PyModule_AddObject(_ev, "Error", EvError);

    /* checking libev version */
    int major = ev_version_major(), minor = ev_version_minor();

    if (major != EV_VERSION_MAJOR || minor < EV_VERSION_MINOR) {
        PyErr_Format(EvError, "libev version mismatch: "
                     "compiled against %d.%d, running %d.%d",
                     EV_VERSION_MAJOR, EV_VERSION_MINOR, major, minor);
        return;
    }

    /* ev.__version__ */
    PyModule_AddStringConstant(_ev, "__version__", _EV_VERSION);

    /* Loop() flags */
    PyModule_AddObject(_ev, "EVFLAG_AUTO",
                       PyLong_FromUnsignedLong(EVFLAG_AUTO));
    PyModule_AddObject(_ev, "EVFLAG_NOENV",
                       PyLong_FromUnsignedLong(EVFLAG_NOENV));
    PyModule_AddObject(_ev, "EVFLAG_FORKCHECK",
                       PyLong_FromUnsignedLong(EVFLAG_FORKCHECK));

    /* Loop() backends flags */
    PyModule_AddObject(_ev, "EVBACKEND_SELECT",
                       PyLong_FromUnsignedLong(EVBACKEND_SELECT));
    PyModule_AddObject(_ev, "EVBACKEND_POLL",
                       PyLong_FromUnsignedLong(EVBACKEND_POLL));
    PyModule_AddObject(_ev, "EVBACKEND_EPOLL",
                       PyLong_FromUnsignedLong(EVBACKEND_EPOLL));
    PyModule_AddObject(_ev, "EVBACKEND_KQUEUE",
                       PyLong_FromUnsignedLong(EVBACKEND_KQUEUE));
    PyModule_AddObject(_ev, "EVBACKEND_DEVPOLL",
                       PyLong_FromUnsignedLong(EVBACKEND_DEVPOLL));
    PyModule_AddObject(_ev, "EVBACKEND_PORT",
                       PyLong_FromUnsignedLong(EVBACKEND_PORT));

    /* Loop.loop() flags */
    PyModule_AddIntConstant(_ev, "EVLOOP_NONBLOCK", EVLOOP_NONBLOCK);
    PyModule_AddIntConstant(_ev, "EVLOOP_ONESHOT", EVLOOP_ONESHOT);

    /* Loop.unloop() flags */
    PyModule_AddIntConstant(_ev, "EVUNLOOP_ONE", EVUNLOOP_ONE);
    PyModule_AddIntConstant(_ev, "EVUNLOOP_ALL", EVUNLOOP_ALL);

    /* events */
    PyModule_AddObject(_ev, "EV_READ", PyLong_FromUnsignedLong(EV_READ));
    PyModule_AddObject(_ev, "EV_WRITE", PyLong_FromUnsignedLong(EV_WRITE));
    PyModule_AddObject(_ev, "EV_TIMEOUT", PyLong_FromUnsignedLong(EV_TIMEOUT));
    PyModule_AddObject(_ev, "EV_PERIODIC", PyLong_FromUnsignedLong(EV_PERIODIC));
    PyModule_AddObject(_ev, "EV_SIGNAL", PyLong_FromUnsignedLong(EV_SIGNAL));
    PyModule_AddObject(_ev, "EV_CHILD", PyLong_FromUnsignedLong(EV_CHILD));
    PyModule_AddObject(_ev, "EV_STAT", PyLong_FromUnsignedLong(EV_STAT));
    PyModule_AddObject(_ev, "EV_IDLE", PyLong_FromUnsignedLong(EV_IDLE));
    PyModule_AddObject(_ev, "EV_PREPARE", PyLong_FromUnsignedLong(EV_PREPARE));
    PyModule_AddObject(_ev, "EV_CHECK", PyLong_FromUnsignedLong(EV_CHECK));
    PyModule_AddObject(_ev, "EV_EMBED", PyLong_FromUnsignedLong(EV_EMBED));
    PyModule_AddObject(_ev, "EV_FORK", PyLong_FromUnsignedLong(EV_FORK));
    PyModule_AddObject(_ev, "EV_ASYNC", PyLong_FromUnsignedLong(EV_ASYNC));
    PyModule_AddObject(_ev, "EV_ERROR", PyLong_FromUnsignedLong(EV_ERROR));

    /* allocate memory from the Python heap */
    ev_set_allocator(_ev_realloc);

    /* syscall errors will call Py_FatalError */
    ev_set_syserr_cb(_ev_fatal_error);
}


