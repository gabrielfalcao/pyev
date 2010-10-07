/*******************************************************************************
* utilities
*******************************************************************************/

/* fwd decl */
static int
Loop_pending_cb_set(Loop *self, PyObject *value, void *closure);


/* report errors and bail out if needed */
void
report_error_Loop(Loop *self, PyObject *context)
{
    if (!self->debug && context) {
        PyErr_WriteUnraisable(context);
    }
    else {
        PyErr_Print();
    }
}

void
set_error_Loop(Loop *self, PyObject *context, char force)
{
    report_error_Loop(self, context);
    if (force || self->debug) {
        ev_unloop(self->loop, EVUNLOOP_ALL);
    }
}


/* loop pending callback */
static void
pending_Loop(struct ev_loop *loop)
{
    PYEV_GIL_ENSURE
    Loop *self = ev_userdata(loop);
    PyObject *result;

    result = PyObject_CallFunctionObjArgs(self->pending_cb, self, NULL);
    if (!result) {
        set_error_Loop(self, NULL, 1); //XXX: should we really bail out here?
    }
    else {
        Py_DECREF(result);
    }
    PYEV_GIL_RELEASE
}


/* new_loop - instanciate a Loop */
Loop *
new_Loop(PyTypeObject *type, PyObject *args, PyObject *kwargs, char default_loop)
{
    unsigned int flags = EVFLAG_AUTO;
    PyObject *pending_cb = Py_None;
    PyObject *data = NULL;
    PyObject *debug = Py_False;
    double io_interval = 0.0, timeout_interval = 0.0;
    Loop *self;
    PyObject *tmp;

    static char *kwlist[] = {"flags", "pending_cb", "data", "debug",
                             "io_interval", "timeout_interval", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|IOOO!dd:__new__", kwlist,
            &flags, &pending_cb, &data, &PyBool_Type, &debug,
            &io_interval, &timeout_interval)) {
        return NULL;
    }
    /* self */
    self = (Loop *)type->tp_alloc(type, 0);
    if (!self) {
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
        PyErr_SetString(Error, "could not create Loop, bad 'flags'?");
        Py_DECREF(self);
        return NULL;
    }
    if (Loop_pending_cb_set(self, pending_cb, NULL) ||
        !positive_float(io_interval) ||
        !positive_float(timeout_interval)) {
        Py_DECREF(self);
        return NULL;
    }
    /* self->data */
    if (data) {
        tmp = self->data;
        Py_INCREF(data);
        self->data = data;
        Py_XDECREF(tmp);
    }
    /* self->debug */
    self->debug = (debug == Py_True) ? 1 : 0;
    /* done */
    ev_set_io_collect_interval(self->loop, io_interval);
    ev_set_timeout_collect_interval(self->loop, timeout_interval);
    ev_set_userdata(self->loop, (void *)self);
    return self;
}


/*******************************************************************************
* LoopType
*******************************************************************************/

/* LoopType.tp_traverse */
static int
Loop_tp_traverse(Loop *self, visitproc visit, void *arg)
{
    Py_VISIT(self->data);
    Py_VISIT(self->pending_cb);
    return 0;
}


/* LoopType.tp_clear */
static int
Loop_tp_clear(Loop *self)
{
    Py_CLEAR(self->data);
    Py_CLEAR(self->pending_cb);
    return 0;
}


/* LoopType.tp_dealloc */
static void
Loop_tp_dealloc(Loop *self)
{
    Loop_tp_clear(self);
    if (self->loop) {
        if (ev_is_default_loop(self->loop)) {
            ev_default_destroy();
            DefaultLoop = NULL;
        }
        else {
            ev_loop_destroy(self->loop);
        }
    }
    Py_TYPE(self)->tp_free((PyObject *)self);
}


/* LoopType.tp_new */
static PyObject *
Loop_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    return (PyObject *)new_Loop(type, args, kwargs, 0);
}


/* Loop.fork() */
static PyObject *
Loop_fork(Loop *self)
{
    if (ev_is_default_loop(self->loop)) {
        ev_default_fork();
    }
    else {
        ev_loop_fork(self->loop);
    }
    Py_RETURN_NONE;
}


/* Loop.now() -> float */
static PyObject *
Loop_now(Loop *self)
{
    return PyFloat_FromDouble(ev_now(self->loop));
}


/* Loop.now_update() */
static PyObject *
Loop_now_update(Loop *self)
{
    ev_now_update(self->loop);
    Py_RETURN_NONE;
}


/* Loop.suspend()
   Loop.resume() */
static PyObject *
Loop_suspend(Loop *self)
{
    ev_suspend(self->loop);
    Py_RETURN_NONE;
}

static PyObject *
Loop_resume(Loop *self)
{
    ev_resume(self->loop);
    Py_RETURN_NONE;
}


/* Loop.loop([flags]) */
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
    if (PyErr_Occurred()) {
        return NULL;
    }
    Py_RETURN_NONE;
}


/* Loop.unloop([how]) */
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


/* Loop.ref()
   Loop.unref() */
static PyObject *
Loop_ref(Loop *self)
{
    ev_ref(self->loop);
    Py_RETURN_NONE;
}

static PyObject *
Loop_unref(Loop *self)
{
    ev_unref(self->loop);
    Py_RETURN_NONE;
}


/* Loop.set_io_collect_interval(interval)
   Loop.set_timeout_collect_interval(interval) */
static PyObject *
Loop_set_io_collect_interval(Loop *self, PyObject *args)
{
    double interval;

    if (!PyArg_ParseTuple(args, "d:set_io_collect_interval", &interval)) {
        return NULL;
    }
    if (!positive_float(interval)) {
        return NULL;
    }
    ev_set_io_collect_interval(self->loop, interval);
    Py_RETURN_NONE;
}

static PyObject *
Loop_set_timeout_collect_interval(Loop *self, PyObject *args)
{
    double interval;

    if (!PyArg_ParseTuple(args, "d:set_timeout_collect_interval", &interval)) {
        return NULL;
    }
    if (!positive_float(interval)) {
        return NULL;
    }
    ev_set_timeout_collect_interval(self->loop, interval);
    Py_RETURN_NONE;
}


/* Loop.pending_invoke() */
static PyObject *
Loop_pending_invoke(Loop *self)
{
    ev_invoke_pending(self->loop);
    Py_RETURN_NONE;
}


/* Loop.verify() */
static PyObject *
Loop_verify(Loop *self)
{
    ev_verify(self->loop);
    Py_RETURN_NONE;
}


/* watchers methods */

/* Loop.Io(fd, events, callback[, data]) */
static PyObject *
Loop_Io(Loop *self, PyObject *args)
{
    PyObject *fd, *events;
    PyObject *callback, *data = Py_None;

    if (!PyArg_ParseTuple(args, "OOO|O:Io", &fd, &events, &callback, &data)) {
        return NULL;
    }
    return PyObject_CallFunctionObjArgs((PyObject *)&IoType, fd, events,
                self, callback, data, NULL);
}


/* Loop.Timer(after, repeat, callback[, data]) */
static PyObject *
Loop_Timer(Loop *self, PyObject *args)
{
    PyObject *after, *repeat;
    PyObject *callback, *data = Py_None;

    if (!PyArg_ParseTuple(args, "OOO|O:Timer", &after, &repeat, &callback, &data)) {
        return NULL;
    }
    return PyObject_CallFunctionObjArgs((PyObject *)&TimerType, after, repeat,
                self, callback, data, NULL);
}


/* Loop.Periodic(offset, interval, reschedule_cb, callback[, data]) */
static PyObject *
Loop_Periodic(Loop *self, PyObject *args)
{
    PyObject *offset, *interval, *reschedule_cb;
    PyObject *callback, *data = Py_None;

    if (!PyArg_ParseTuple(args, "OOOO|O:Periodic", &offset, &interval,
            &reschedule_cb, &callback, &data)) {
        return NULL;
    }
    return PyObject_CallFunctionObjArgs((PyObject *)&PeriodicType, offset,
                interval, reschedule_cb, self, callback, data, NULL);
}


/* Loop.Signal(signum, callback[, data]) */
static PyObject *
Loop_Signal(Loop *self, PyObject *args)
{
    PyObject *signum;
    PyObject *callback, *data = Py_None;

    if (!PyArg_ParseTuple(args, "OO|O:Signal", &signum, &callback, &data)) {
        return NULL;
    }
    return PyObject_CallFunctionObjArgs((PyObject *)&SignalType, signum,
                self, callback, data, NULL);
}


/* Loop.Child(pid, trace, callback[, data]) */
static PyObject *
Loop_Child(Loop *self, PyObject *args)
{
    PyObject *pid, *trace;
    PyObject *callback, *data = Py_None;

    if (!PyArg_ParseTuple(args, "OOO|O:Child", &pid, &trace, &callback, &data)) {
        return NULL;
    }
    return PyObject_CallFunctionObjArgs((PyObject *)&ChildType, pid, trace,
                self, callback, data, NULL);
}


/* Loop.Stat(path, interval, callback[, data]) */
static PyObject *
Loop_Stat(Loop *self, PyObject *args)
{
    PyObject *path, *interval;
    PyObject *callback, *data = Py_None;

    if (!PyArg_ParseTuple(args, "OOO|O:Stat", &path, &interval,
            &callback, &data)) {
        return NULL;
    }
    return PyObject_CallFunctionObjArgs((PyObject *)&StatType, path, interval,
                self, callback, data, NULL);
}


/* Loop.Idle(callback[, data]) */
static PyObject *
Loop_Idle(Loop *self, PyObject *args)
{
    PyObject *callback, *data = Py_None;

    if (!PyArg_ParseTuple(args, "O|O:Idle", &callback, &data)) {
        return NULL;
    }
    return PyObject_CallFunctionObjArgs((PyObject *)&IdleType,
                self, callback, data, NULL);
}


/* Loop.Prepare(callback[, data]) */
static PyObject *
Loop_Prepare(Loop *self, PyObject *args)
{
    PyObject *callback, *data = Py_None;

    if (!PyArg_ParseTuple(args, "O|O:Prepare", &callback, &data)) {
        return NULL;
    }
    return PyObject_CallFunctionObjArgs((PyObject *)&PrepareType,
                self, callback, data, NULL);
}


/* Loop.Check(callback[, data]) */
static PyObject *
Loop_Check(Loop *self, PyObject *args)
{
    PyObject *callback, *data = Py_None;

    if (!PyArg_ParseTuple(args, "O|O:Check", &callback, &data)) {
        return NULL;
    }
    return PyObject_CallFunctionObjArgs((PyObject *)&CheckType,
                self, callback, data, NULL);
}


/* Loop.Embed(other[, callback[, data]]) */
static PyObject *
Loop_Embed(Loop *self, PyObject *args)
{
    PyObject *other;
    PyObject *callback = Py_None, *data = Py_None;

    if (!PyArg_ParseTuple(args, "O|OO:Embed", &other, &callback, &data)) {
        return NULL;
    }
    return PyObject_CallFunctionObjArgs((PyObject *)&EmbedType, other,
                self, callback, data, NULL);
}


/* Loop.Fork(callback[, data]) */
static PyObject *
Loop_Fork(Loop *self, PyObject *args)
{
    PyObject *callback, *data = Py_None;

    if (!PyArg_ParseTuple(args, "O|O:Fork", &callback, &data)) {
        return NULL;
    }
    return PyObject_CallFunctionObjArgs((PyObject *)&ForkType,
                self, callback, data, NULL);
}


/* Loop.Async(callback[, data]) */
static PyObject *
Loop_Async(Loop *self, PyObject *args)
{
    PyObject *callback, *data = Py_None;

    if (!PyArg_ParseTuple(args, "O|O:Async", &callback, &data)) {
        return NULL;
    }
    return PyObject_CallFunctionObjArgs((PyObject *)&AsyncType,
                self, callback, data, NULL);
}


/* LoopType.tp_methods */
static PyMethodDef Loop_tp_methods[] = {
    {"fork", (PyCFunction)Loop_fork, METH_NOARGS, Loop_fork_doc},
    {"now", (PyCFunction)Loop_now, METH_NOARGS, Loop_now_doc},
    {"now_update", (PyCFunction)Loop_now_update, METH_NOARGS, Loop_now_update_doc},
    {"suspend", (PyCFunction)Loop_suspend, METH_NOARGS, Loop_suspend_resume_doc},
    {"resume", (PyCFunction)Loop_resume, METH_NOARGS, Loop_suspend_resume_doc},
    {"loop", (PyCFunction)Loop_loop, METH_VARARGS, Loop_loop_doc},
    {"unloop", (PyCFunction)Loop_unloop, METH_VARARGS, Loop_unloop_doc},
    {"ref", (PyCFunction)Loop_ref, METH_NOARGS, Loop_ref_unref_doc},
    {"unref", (PyCFunction)Loop_unref, METH_NOARGS, Loop_ref_unref_doc},
    {"set_io_collect_interval", (PyCFunction)Loop_set_io_collect_interval,
     METH_VARARGS, Loop_set_collect_interval_doc},
    {"set_timeout_collect_interval",
     (PyCFunction)Loop_set_timeout_collect_interval, METH_VARARGS,
     Loop_set_collect_interval_doc},
    {"pending_invoke", (PyCFunction)Loop_pending_invoke, METH_NOARGS,
     Loop_pending_invoke_doc},
    {"verify", (PyCFunction)Loop_verify, METH_NOARGS, Loop_verify_doc},
    /* watchers methods */
    {"Io", (PyCFunction)Loop_Io, METH_VARARGS, Loop_Io_doc},
    {"Timer", (PyCFunction)Loop_Timer, METH_VARARGS, Loop_Timer_doc},
    {"Periodic", (PyCFunction)Loop_Periodic, METH_VARARGS, Loop_Periodic_doc},
    {"Signal", (PyCFunction)Loop_Signal, METH_VARARGS, Loop_Signal_doc},
    {"Child", (PyCFunction)Loop_Child, METH_VARARGS, Loop_Child_doc},
    {"Stat", (PyCFunction)Loop_Stat, METH_VARARGS, Loop_Stat_doc},
    {"Idle", (PyCFunction)Loop_Idle, METH_VARARGS, Loop_Idle_doc},
    {"Prepare", (PyCFunction)Loop_Prepare, METH_VARARGS, Loop_Prepare_doc},
    {"Check", (PyCFunction)Loop_Check, METH_VARARGS, Loop_Check_doc},
    {"Embed", (PyCFunction)Loop_Embed, METH_VARARGS, Loop_Embed_doc},
    {"Fork", (PyCFunction)Loop_Fork, METH_VARARGS, Loop_Fork_doc},
    {"Async", (PyCFunction)Loop_Async, METH_VARARGS, Loop_Async_doc},
    {NULL}  /* Sentinel */
};


/* LoopType.tp_members */
static PyMemberDef Loop_tp_members[] = {
    {"data", T_OBJECT, offsetof(Loop, data), 0, Loop_data_doc},
    {"debug", T_BOOL, offsetof(Loop, debug), 0, Loop_debug_doc},
    {NULL}  /* Sentinel */
};


/* Loop.default_loop */
static PyObject *
Loop_default_loop_get(Loop *self, void *closure)
{
    if (ev_is_default_loop(self->loop)) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}


/* Loop.iteration */
static PyObject *
Loop_iteration_get(Loop *self, void *closure)
{
    return PyInt_FromUnsignedLong(ev_iteration(self->loop));
}


/* Loop.depth */
static PyObject *
Loop_depth_get(Loop *self, void *closure)
{
    return PyInt_FromUnsignedLong(ev_depth(self->loop));
}


/* Loop.backend */
static PyObject *
Loop_backend_get(Loop *self, void *closure)
{
    return PyInt_FromUnsignedLong(ev_backend(self->loop));
}


/* Loop.pending_count */
static PyObject *
Loop_pending_count_get(Loop *self, void *closure)
{
    return PyInt_FromUnsignedLong(ev_pending_count(self->loop));
}


/* Loop.pending_cb */
static PyObject *
Loop_pending_cb_get(Loop *self, void *closure)
{
    Py_INCREF(self->pending_cb);
    return self->pending_cb;
}

static int
Loop_pending_cb_set(Loop *self, PyObject *value, void *closure)
{
    PyObject *tmp;

    if (!value) {
        PyErr_SetString(PyExc_TypeError, "cannot delete attribute");
        return -1;
    }
    if (value != Py_None && !PyCallable_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "a callable or None is required");
        return -1;
    }
    if (value == Py_None) {
        ev_set_invoke_pending_cb(self->loop, ev_invoke_pending);
    }
    else {
        ev_set_invoke_pending_cb(self->loop, pending_Loop);
    }
    tmp = self->pending_cb;
    Py_INCREF(value);
    self->pending_cb = value;
    Py_XDECREF(tmp);
    return 0;
}


/* LoopType.tp_getsets */
static PyGetSetDef Loop_tp_getsets[] = {
    {"default_loop", (getter)Loop_default_loop_get, NULL,
     Loop_default_loop_doc, NULL},
    {"iteration", (getter)Loop_iteration_get, NULL, Loop_iteration_doc, NULL},
    {"depth", (getter)Loop_depth_get, NULL, Loop_depth_doc, NULL},
    {"backend", (getter)Loop_backend_get, NULL, Loop_backend_doc, NULL},
    {"pending_count", (getter)Loop_pending_count_get, NULL,
     Loop_pending_count_doc, NULL},
    {"pending_cb", (getter)Loop_pending_cb_get, (setter)Loop_pending_cb_set,
     Loop_pending_cb_doc, NULL},
    {NULL}  /* Sentinel */
};


/* LoopType */
static PyTypeObject LoopType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyev.Loop",                              /*tp_name*/
    sizeof(Loop),                             /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    (destructor)Loop_tp_dealloc,              /*tp_dealloc*/
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /*tp_flags*/
    Loop_tp_doc,                              /*tp_doc*/
    (traverseproc)Loop_tp_traverse,           /*tp_traverse*/
    (inquiry)Loop_tp_clear,                   /*tp_clear*/
    0,                                        /*tp_richcompare*/
    0,                                        /*tp_weaklistoffset*/
    0,                                        /*tp_iter*/
    0,                                        /*tp_iternext*/
    Loop_tp_methods,                          /*tp_methods*/
    Loop_tp_members,                          /*tp_members*/
    Loop_tp_getsets,                          /*tp_getsets*/
    0,                                        /*tp_base*/
    0,                                        /*tp_dict*/
    0,                                        /*tp_descr_get*/
    0,                                        /*tp_descr_set*/
    0,                                        /*tp_dictoffset*/
    0,                                        /*tp_init*/
    0,                                        /*tp_alloc*/
    Loop_tp_new,                              /*tp_new*/
};
