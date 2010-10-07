/*******************************************************************************
* utilities
*******************************************************************************/

/* fwd decl */
int
update_Stat(Stat *self);

static int
Watcher_callback_set(Watcher *self, PyObject *value, void *closure);


/* start/stop Watcher */
#define PYEV_WATCHER_START(t, w) t##_start(w->loop->loop, (t *)w->watcher)

#define PYEV_WATCHER_STOP(t, w) t##_stop(w->loop->loop, (t *)w->watcher)

void
start_Watcher(Watcher *self)
{
    switch (self->type) {
        case EV_IO:
            PYEV_WATCHER_START(ev_io, self);
            break;
        case EV_TIMER:
            PYEV_WATCHER_START(ev_timer, self);
            break;
        case EV_PERIODIC:
            PYEV_WATCHER_START(ev_periodic, self);
            break;
        case EV_SIGNAL:
            PYEV_WATCHER_START(ev_signal, self);
            break;
        case EV_CHILD:
            PYEV_WATCHER_START(ev_child, self);
            break;
        case EV_STAT:
            PYEV_WATCHER_START(ev_stat, self);
            break;
        case EV_IDLE:
            PYEV_WATCHER_START(ev_idle, self);
            break;
        case EV_PREPARE:
            PYEV_WATCHER_START(ev_prepare, self);
            break;
        case EV_CHECK:
            PYEV_WATCHER_START(ev_check, self);
            break;
        case EV_EMBED:
            PYEV_WATCHER_START(ev_embed, self);
            break;
        case EV_FORK:
            PYEV_WATCHER_START(ev_fork, self);
            break;
        case EV_ASYNC:
            PYEV_WATCHER_START(ev_async, self);
            break;
        default:
            break;
    }
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
            case EV_PERIODIC:
                PYEV_WATCHER_STOP(ev_periodic, self);
                break;
            case EV_SIGNAL:
                PYEV_WATCHER_STOP(ev_signal, self);
                break;
            case EV_CHILD:
                PYEV_WATCHER_STOP(ev_child, self);
                break;
            case EV_STAT:
                PYEV_WATCHER_STOP(ev_stat, self);
                break;
            case EV_IDLE:
                PYEV_WATCHER_STOP(ev_idle, self);
                break;
            case EV_PREPARE:
                PYEV_WATCHER_STOP(ev_prepare, self);
                break;
            case EV_CHECK:
                PYEV_WATCHER_STOP(ev_check, self);
                break;
            case EV_EMBED:
                PYEV_WATCHER_STOP(ev_embed, self);
                break;
            case EV_FORK:
                PYEV_WATCHER_STOP(ev_fork, self);
                break;
            case EV_ASYNC:
                PYEV_WATCHER_STOP(ev_async, self);
                break;
            default:
                break;
        }
    }
}


/* watcher callback */
static void
callback_Watcher(struct ev_loop *loop, ev_watcher *watcher, int revents)
{
    PYEV_GIL_ENSURE
    Watcher *pywatcher = watcher->data;
    PyObject *pyresult, *pyrevents, *pymsg = NULL;

    if (revents & EV_ERROR) {
        stop_Watcher(pywatcher);
        if (errno) {
            // there's a high probability it is related
            pymsg = PyString_FromFormat("<%s object at %p> has been stopped",
                        Py_TYPE(pywatcher)->tp_name, pywatcher);
            PyErr_SetFromErrnoWithFilenameObject(PyExc_OSError, pymsg);
            Py_XDECREF(pymsg);
        }
        else {
            PyErr_Format(Error, "unspecified libev error: '<%s object at %p> "
                "has been stopped'", Py_TYPE(pywatcher)->tp_name, pywatcher);
        }
        set_error_Loop(ev_userdata(loop), NULL, 1); //XXX: should we really bail out here?
    }
    else if ((revents & EV_STAT) && update_Stat((Stat *)pywatcher)) {
        set_error_Loop(ev_userdata(loop), NULL, 1); //XXX: should we really bail out here?
    }
    else if (pywatcher->callback != Py_None) {
        pyrevents = PyInt_FromLong(revents);
        if (!pyrevents) {
            set_error_Loop(ev_userdata(loop), NULL, 1); //XXX: should we really bail out here?
        }
        else {
            pyresult = PyObject_CallFunctionObjArgs(pywatcher->callback,
                            pywatcher, pyrevents, NULL);
            if (!pyresult) {
                set_error_Loop(ev_userdata(loop), pywatcher->callback, 0);
            }
            else {
                Py_DECREF(pyresult);
            }
            Py_DECREF(pyrevents);
        }
    }
    else if (revents & EV_EMBED) {
        ev_embed_sweep(loop, (ev_embed *)watcher);
    }
    PYEV_GIL_RELEASE
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
             PyObject *callback, void *cb_closure, PyObject *data)
{
    PyObject *tmp;

    if (!inactive_Watcher(self)) {
        return -1;
    }
    /* self->loop */
    if (default_loop && !ev_is_default_loop(loop->loop)) {
        PyErr_SetString(Error, "loop must be the 'default_loop'");
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
    return 0;
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
static PyObject *
Watcher_start(Watcher *self)
{
    start_Watcher(self);
    if ((self->type == EV_STAT) && update_Stat((Stat *)self)) {
        return NULL;
    }
    Py_RETURN_NONE;
}


/* Watcher.stop() */
static PyObject *
Watcher_stop(Watcher *self)
{
    stop_Watcher(self);
    Py_RETURN_NONE;
}


/* Watcher.invoke(revents) */
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


/* Watcher.clear_pending() -> int */
static PyObject *
Watcher_clear_pending(Watcher *self)
{
    return PyInt_FromLong(ev_clear_pending(self->loop->loop, self->watcher));
}


/* Watcher.feed_event(revents) */
static PyObject *
Watcher_feed_event(Watcher *self, PyObject *args)
{
    int revents;

    if (!PyArg_ParseTuple(args, "i:feed_event", &revents)) {
        return NULL;
    }
    ev_feed_event(self->loop->loop, self->watcher, revents);
    Py_RETURN_NONE;
}


/* WatcherType.tp_methods */
static PyMethodDef Watcher_tp_methods[] = {
    {"start", (PyCFunction)Watcher_start, METH_NOARGS, Watcher_start_doc},
    {"stop", (PyCFunction)Watcher_stop, METH_NOARGS, Watcher_stop_doc},
    {"invoke", (PyCFunction)Watcher_invoke, METH_VARARGS, Watcher_invoke_doc},
    {"clear_pending", (PyCFunction)Watcher_clear_pending, METH_NOARGS,
     Watcher_clear_pending_doc},
    {"feed_event", (PyCFunction)Watcher_feed_event, METH_VARARGS,
     Watcher_feed_event_doc},
    {NULL}  /* Sentinel */
};


/* WatcherType.tp_members */
static PyMemberDef Watcher_tp_members[] = {
    {"loop", T_OBJECT_EX, offsetof(Watcher, loop), READONLY, Watcher_loop_doc},
    {"data", T_OBJECT, offsetof(Watcher, data), 0, Watcher_data_doc},
    {NULL}  /* Sentinel */
};


/* Watcher.callback */
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


/* Watcher.active */
static PyObject *
Watcher_active_get(Watcher *self, void *closure)
{
    if (ev_is_active(self->watcher)) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}


/* Watcher.pending */
static PyObject *
Watcher_pending_get(Watcher *self, void *closure)
{
    if (ev_is_pending(self->watcher)) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}


/* Watcher.priority */
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
    priority = PyNum_AsInt(value);
    if (priority == -1 && PyErr_Occurred()) {
        return -1;
    }
    ev_set_priority(self->watcher, priority);
    return 0;
}


/* WatcherType.tp_getsets */
static PyGetSetDef Watcher_tp_getsets[] = {
    {"callback", (getter)Watcher_callback_get, (setter)Watcher_callback_set,
     Watcher_callback_doc, NULL},
    {"active", (getter)Watcher_active_get, NULL, Watcher_active_doc, NULL},
    {"pending", (getter)Watcher_pending_get, NULL, Watcher_pending_doc, NULL},
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
