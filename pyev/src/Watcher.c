/*******************************************************************************
* utilities
*******************************************************************************/

/* fwd decl */
static int
Watcher_callback_set(Watcher *self, PyObject *value, void *closure);

#if EV_STAT_ENABLE
int
update_Stat(Stat *self);
#endif


/* start/stop Watcher */
#define PYEV_WATCHER_START(t, w) t##_start(w->loop->loop, (t *)w->watcher)
#define PYEV_WATCHER_STOP(t, w) t##_stop(w->loop->loop, (t *)w->watcher)

int
start_Watcher(Watcher *self)
{
    int result = 0;

    switch (self->type) {
        case EV_IO:
            PYEV_WATCHER_START(ev_io, self);
            break;
        case EV_TIMER:
            PYEV_WATCHER_START(ev_timer, self);
            break;
#if EV_PERIODIC_ENABLE
        case EV_PERIODIC:
            PYEV_WATCHER_START(ev_periodic, self);
            break;
#endif
#if EV_SIGNAL_ENABLE
        case EV_SIGNAL:
            PYEV_WATCHER_START(ev_signal, self);
            break;
#endif
#if EV_CHILD_ENABLE
        case EV_CHILD:
            PYEV_WATCHER_START(ev_child, self);
            break;
#endif
#if EV_STAT_ENABLE
        case EV_STAT:
            PYEV_WATCHER_START(ev_stat, self);
            if (update_Stat((Stat *)self)) {
                PYEV_WATCHER_STOP(ev_stat, self);
                result = -1;
                break;
            }
            break;
#endif
#if EV_IDLE_ENABLE
        case EV_IDLE:
            PYEV_WATCHER_START(ev_idle, self);
            break;
#endif
#if EV_PREPARE_ENABLE
        case EV_PREPARE:
            PYEV_WATCHER_START(ev_prepare, self);
            break;
#endif
#if EV_CHECK_ENABLE
        case EV_CHECK:
            PYEV_WATCHER_START(ev_check, self);
            break;
#endif
#if EV_EMBED_ENABLE
        case EV_EMBED:
            PYEV_WATCHER_START(ev_embed, self);
            break;
#endif
#if EV_FORK_ENABLE
        case EV_FORK:
            PYEV_WATCHER_START(ev_fork, self);
            break;
#endif
#if EV_ASYNC_ENABLE
        case EV_ASYNC:
            PYEV_WATCHER_START(ev_async, self);
            break;
#endif
        default:
            break;
    }
    return result;
}

void
stop_Watcher(Watcher *self)
{
    if (self->loop && self->watcher) {
        switch (self->type) {
            case EV_IO:
                PYEV_WATCHER_STOP(ev_io, self);
                break;
            case EV_TIMER:
                PYEV_WATCHER_STOP(ev_timer, self);
                break;
#if EV_PERIODIC_ENABLE
            case EV_PERIODIC:
                PYEV_WATCHER_STOP(ev_periodic, self);
                break;
#endif
#if EV_SIGNAL_ENABLE
            case EV_SIGNAL:
                PYEV_WATCHER_STOP(ev_signal, self);
                break;
#endif
#if EV_CHILD_ENABLE
            case EV_CHILD:
                PYEV_WATCHER_STOP(ev_child, self);
                break;
#endif
#if EV_STAT_ENABLE
            case EV_STAT:
                PYEV_WATCHER_STOP(ev_stat, self);
                break;
#endif
#if EV_IDLE_ENABLE
            case EV_IDLE:
                PYEV_WATCHER_STOP(ev_idle, self);
                break;
#endif
#if EV_PREPARE_ENABLE
            case EV_PREPARE:
                PYEV_WATCHER_STOP(ev_prepare, self);
                break;
#endif
#if EV_CHECK_ENABLE
            case EV_CHECK:
                PYEV_WATCHER_STOP(ev_check, self);
                break;
#endif
#if EV_EMBED_ENABLE
            case EV_EMBED:
                PYEV_WATCHER_STOP(ev_embed, self);
                break;
#endif
#if EV_FORK_ENABLE
            case EV_FORK:
                PYEV_WATCHER_STOP(ev_fork, self);
                break;
#endif
#if EV_ASYNC_ENABLE
            case EV_ASYNC:
                PYEV_WATCHER_STOP(ev_async, self);
                break;
#endif
            default:
                break;
        }
    }
}


/* watcher callback */
static void
callback_Watcher(ev_loop *loop, ev_watcher *watcher, int revents)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    Watcher *pywatcher = watcher->data;
    PyObject *pyresult, *pyrevents, *pymsg = NULL;

    if (revents & EV_ERROR) {
        stop_Watcher(pywatcher);
        if (errno) { // there's a high probability it is related
            pymsg = PyString_FromFormat("<%s object at %p> has been stopped",
                        Py_TYPE(pywatcher)->tp_name, pywatcher);
            PyErr_SetFromErrnoWithFilenameObject(PyExc_OSError, pymsg);
            Py_XDECREF(pymsg);
        }
        else {
            PyErr_Format(Error, "unspecified libev error: '<%s object at %p> "
                "has been stopped'", Py_TYPE(pywatcher)->tp_name, pywatcher);
        }
        exit_Loop(loop);
    }
#if EV_STAT_ENABLE
    else if ((revents & EV_STAT) && update_Stat((Stat *)pywatcher)) {
        exit_Loop(loop);
    }
#endif
    else if (pywatcher->callback != Py_None) {
        pyrevents = PyInt_FromLong(revents);
        if (!pyrevents) {
            exit_Loop(loop);
        }
        else {
            pyresult = PyObject_CallFunctionObjArgs(pywatcher->callback,
                            pywatcher, pyrevents, NULL);
            if (!pyresult) {
                report_error_Loop(ev_userdata(loop), pywatcher->callback);
            }
            else {
                Py_DECREF(pyresult);
            }
            Py_DECREF(pyrevents);
        }
    }
#if EV_EMBED_ENABLE
    else if (revents & EV_EMBED) {
        ev_embed_sweep(loop, (ev_embed *)watcher);
    }
#endif
    PyGILState_Release(gstate);
}


/* called by subtypes before calling ev_TYPE_set */
int
inactive_Watcher(Watcher *self)
{
    if (ev_is_active(self->watcher)) {
        PyErr_SetString(Error, "you cannot set a watcher while it is active");
        return 0;
    }
    return 1;
}


/* instanciate (sort of) the Watcher - called by subtypes tp_new */
void
new_Watcher(Watcher *self, ev_watcher *watcher, int type)
{
    /* self->watcher */
    self->watcher = watcher;
    /* self->type */
    self->type = type;
    /* self->watcher->data */
    self->watcher->data = (void *)self;
    /* init the watcher*/
    ev_init(self->watcher, callback_Watcher);
}


/* init the Watcher - called by subtypes tp_init */
int
init_Watcher(Watcher *self, Loop *loop, char default_loop,
             PyObject *callback, void *cb_closure, PyObject *data, int priority)
{
    PyObject *tmp;

    if (!inactive_Watcher(self)) {
        return -1;
    }
    /* self->loop */
    if (default_loop && !ev_is_default_loop(loop->loop)) {
        PyErr_SetString(Error, "loop must be the 'default loop'");
        return -1;
    }
    tmp = (PyObject *)self->loop;
    Py_INCREF(loop);
    self->loop = loop;
    Py_XDECREF(tmp);
    /* self->callback */
    if (Watcher_callback_set(self, callback, cb_closure)) {
        return -1;
    }
    /* self->data */
    if (data) {
        tmp = self->data;
        Py_INCREF(data);
        self->data = data;
        Py_XDECREF(tmp);
    }
    /* priority */
    if (priority) {
        ev_set_priority(self->watcher, priority);
    }
    return 0;
}


/* Py[Int/Long] -> int */
int
pyvalue_as_int(PyObject *pyvalue)
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


/*******************************************************************************
* WatcherType
*******************************************************************************/

/* WatcherType.tp_traverse */
static int
Watcher_tp_traverse(Watcher *self, visitproc visit, void *arg)
{
    Py_VISIT(self->data);
    Py_VISIT(self->callback);
    Py_VISIT(self->loop);
    return 0;
}


/* WatcherType.tp_clear */
static int
Watcher_tp_clear(Watcher *self)
{
    Py_CLEAR(self->data);
    Py_CLEAR(self->callback);
    Py_CLEAR(self->loop);
    return 0;
}


/* WatcherType.tp_dealloc */
static void
Watcher_tp_dealloc(Watcher *self)
{
    stop_Watcher(self);
    Watcher_tp_clear(self);
    Py_TYPE(self)->tp_free((PyObject *)self);
}


/* Watcher.start() */
PyDoc_STRVAR(Watcher_start_doc,
"start()");

static PyObject *
Watcher_start(Watcher *self)
{
    if (start_Watcher(self)) {
        return NULL;
    }
    Py_RETURN_NONE;
}


/* Watcher.stop() */
PyDoc_STRVAR(Watcher_stop_doc,
"stop()");

static PyObject *
Watcher_stop(Watcher *self)
{
    stop_Watcher(self);
    Py_RETURN_NONE;
}


/* Watcher.clear() -> int */
PyDoc_STRVAR(Watcher_clear_doc,
"clear() -> int");

static PyObject *
Watcher_clear(Watcher *self)
{
    return PyInt_FromLong(ev_clear_pending(self->loop->loop, self->watcher));
}


/* Watcher.invoke(revents) */
PyDoc_STRVAR(Watcher_invoke_doc,
"invoke(revents)");

static PyObject *
Watcher_invoke(Watcher *self, PyObject *args)
{
    int revents;

    if (!PyArg_ParseTuple(args, "i:invoke", &revents)) {
        return NULL;
    }
    ev_invoke(self->loop->loop, self->watcher, revents);
    Py_RETURN_NONE;
}


/* Watcher.feed(revents) */
PyDoc_STRVAR(Watcher_feed_doc,
"feed(revents) ");

static PyObject *
Watcher_feed(Watcher *self, PyObject *args)
{
    int revents;

    if (!PyArg_ParseTuple(args, "i:feed", &revents)) {
        return NULL;
    }
    ev_feed_event(self->loop->loop, self->watcher, revents);
    Py_RETURN_NONE;
}


/* WatcherType.tp_methods */
static PyMethodDef Watcher_tp_methods[] = {
    {"start", (PyCFunction)Watcher_start,
     METH_NOARGS, Watcher_start_doc},
    {"stop", (PyCFunction)Watcher_stop,
     METH_NOARGS, Watcher_stop_doc},
    {"clear", (PyCFunction)Watcher_clear,
     METH_NOARGS, Watcher_clear_doc},
    {"invoke", (PyCFunction)Watcher_invoke,
     METH_VARARGS, Watcher_invoke_doc},
    {"feed", (PyCFunction)Watcher_feed,
     METH_VARARGS, Watcher_feed_doc},
    {NULL}  /* Sentinel */
};


/* Watcher.loop */
PyDoc_STRVAR(Watcher_loop_doc,
"loop");


/* Watcher.data */
PyDoc_STRVAR(Watcher_data_doc,
"data");


/* WatcherType.tp_members */
static PyMemberDef Watcher_tp_members[] = {
    {"loop", T_OBJECT_EX, offsetof(Watcher, loop),
     READONLY, Watcher_loop_doc},
    {"data", T_OBJECT, offsetof(Watcher, data),
     0, Watcher_data_doc},
    {NULL}  /* Sentinel */
};


/* Watcher.active */
PyDoc_STRVAR(Watcher_active_doc,
"active");

static PyObject *
Watcher_active_get(Watcher *self, void *closure)
{
    if (ev_is_active(self->watcher)) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}


/* Watcher.pending */
PyDoc_STRVAR(Watcher_pending_doc,
"pending");

static PyObject *
Watcher_pending_get(Watcher *self, void *closure)
{
    if (ev_is_pending(self->watcher)) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}


/* Watcher.callback */
PyDoc_STRVAR(Watcher_callback_doc,
"callback");

static PyObject *
Watcher_callback_get(Watcher *self, void *closure)
{
    Py_INCREF(self->callback);
    return self->callback;
}

static int
Watcher_callback_set(Watcher *self, PyObject *value, void *closure)
{
    PyObject *tmp;

    if (!value) {
        PyErr_SetString(PyExc_TypeError, "cannot delete attribute");
        return -1;
    }
    if (closure) {
        if (value != Py_None && !PyCallable_Check(value)) {
            PyErr_SetString(PyExc_TypeError, "a callable or None is required");
            return -1;
        }
    }
    else {
        if (!PyCallable_Check(value)) {
            PyErr_SetString(PyExc_TypeError, "a callable is required");
            return -1;
        }
    }
    tmp = self->callback;
    Py_INCREF(value);
    self->callback = value;
    Py_XDECREF(tmp);
    return 0;
}


/* Watcher.priority */
PyDoc_STRVAR(Watcher_priority_doc,
"priority");

static PyObject *
Watcher_priority_get(Watcher *self, void *closure)
{
    return PyInt_FromLong(ev_priority(self->watcher));
}

static int
Watcher_priority_set(Watcher *self, PyObject *value, void *closure)
{
    int priority;

    if (ev_is_active(self->watcher) || ev_is_pending(self->watcher)) {
        PyErr_SetString(Error, "you cannot change the 'priority' of a watcher "
            "while it is active or pending.");
        return -1;
    }
    if (!value) {
        PyErr_SetString(PyExc_TypeError, "cannot delete attribute");
        return -1;
    }
    priority = pyvalue_as_int(value);
    if (priority == -1 && PyErr_Occurred()) {
        return -1;
    }
    ev_set_priority(self->watcher, priority);
    return 0;
}


/* WatcherType.tp_getsets */
static PyGetSetDef Watcher_tp_getsets[] = {
    {"active", (getter)Watcher_active_get, NULL,
     Watcher_active_doc, NULL},
    {"pending", (getter)Watcher_pending_get, NULL,
     Watcher_pending_doc, NULL},
    {"callback", (getter)Watcher_callback_get, (setter)Watcher_callback_set,
     Watcher_callback_doc, NULL},
    {"priority", (getter)Watcher_priority_get, (setter)Watcher_priority_set,
     Watcher_priority_doc, NULL},
    {NULL}  /* Sentinel */
};


/* WatcherType */
static PyTypeObject WatcherType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyev.Watcher",                           /*tp_name*/
    sizeof(Watcher),                          /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    (destructor)Watcher_tp_dealloc,           /*tp_dealloc*/
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
    (traverseproc)Watcher_tp_traverse,        /*tp_traverse*/
    (inquiry)Watcher_tp_clear,                /*tp_clear*/
    0,                                        /*tp_richcompare*/
    0,                                        /*tp_weaklistoffset*/
    0,                                        /*tp_iter*/
    0,                                        /*tp_iternext*/
    Watcher_tp_methods,                       /*tp_methods*/
    Watcher_tp_members,                       /*tp_members*/
    Watcher_tp_getsets,                       /*tp_getsets*/
};
