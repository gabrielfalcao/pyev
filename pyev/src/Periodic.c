/*******************************************************************************
* utilities
*******************************************************************************/

/* fwd decl */
static int
Periodic_reschedule_cb_set(Periodic *self, PyObject *value, void *closure);


/* Periodic reschedule stop callback */
static void
stop_reschedule_Periodic(struct ev_loop *loop, ev_prepare *prepare, int revents)
{
    PYEV_GIL_ENSURE
    ev_periodic_stop(loop, (ev_periodic *)prepare->data);
    ev_prepare_stop(loop, prepare);
    PyMem_Free((void *)prepare);
    PYEV_GIL_RELEASE
}


/* Periodic reschedule callback */
static double
reschedule_Periodic(ev_periodic *periodic, double now)
{
    double result;
    PYEV_GIL_ENSURE
    Periodic *pyperiodic = periodic->data;
    Loop *loop = ((Watcher *)pyperiodic)->loop;
    PyObject *pynow, *pyresult = NULL;
    ev_prepare *prepare;

    pynow = PyFloat_FromDouble(now);
    if (!pynow) {
        goto error;
    }
    pyresult = PyObject_CallFunctionObjArgs(pyperiodic->reschedule_cb,
                    pyperiodic, pynow, NULL);
    if (!pyresult) {
        goto error;
    }
    result = PyFloat_AsDouble(pyresult);
    if (result == -1 && PyErr_Occurred()) {
        goto error;
    }
    if (result < now) {
        PyErr_SetString(Error, "returned value must be >= 'now' param");
        goto error;
    }
    goto finish;

error:
    report_error_Loop(loop, pyperiodic->reschedule_cb);
    /* warn the user we're going to stop this Periodic */
    PyErr_Format(Error, "due to previous error, <pyev.Periodic object at %p> "
        "will be stopped", pyperiodic);
    report_error_Loop(loop, pyperiodic->reschedule_cb);
    /* start an ev_prepare watcher that will stop this periodic */
    prepare = (ev_prepare *)PyMem_Malloc(sizeof(ev_prepare));
    if (!prepare) {
        Py_FatalError("Memory could not be allocated."); //XXX: hmm, not sure...
    }
    prepare->data = (void *)periodic;
    ev_prepare_init(prepare, stop_reschedule_Periodic);
    ev_prepare_start(loop->loop, prepare);
    result = now + 1e30;

finish:
    Py_XDECREF(pyresult);
    Py_XDECREF(pynow);
    PYEV_GIL_RELEASE
    return result;
}


/* set the Periodic */
int
set_Periodic(Periodic *self, double offset, double interval,
             PyObject *reschedule_cb)
{
    if (!positive_float(interval)) {
        return -1;
    }
    /* self->reschedule_cb */
    if (Periodic_reschedule_cb_set(self, reschedule_cb, (void *)1)) {
        return -1;
    }
    if (reschedule_cb != Py_None) {
        ev_periodic_set(&self->periodic, offset, interval, reschedule_Periodic);
    }
    else{
        ev_periodic_set(&self->periodic, offset, interval, 0);
    }
    return 0;
}


/*******************************************************************************
* PeriodicType
*******************************************************************************/

/* PeriodicType.tp_traverse */
static int
Periodic_tp_traverse(Periodic *self, visitproc visit, void *arg)
{
    Py_VISIT(self->reschedule_cb);
    return 0;
}


/* PeriodicType.tp_clear */
static int
Periodic_tp_clear(Periodic *self)
{
    Py_CLEAR(self->reschedule_cb);
    return 0;
}


/* PeriodicType.tp_dealloc */
static void
Periodic_tp_dealloc(Periodic *self)
{
    Periodic_tp_clear(self);
    WatcherType.tp_dealloc((PyObject *)self);
}


/* PeriodicType.tp_new */
static PyObject *
Periodic_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Periodic *self = (Periodic *)WatcherType.tp_new(type, args, kwargs);
    if (!self) {
        return NULL;
    }
    new_Watcher((Watcher *)self, (ev_watcher *)&self->periodic, EV_PERIODIC);
    return (PyObject *)self;
}


/* PeriodicType.tp_init */
static int
Periodic_tp_init(Periodic *self, PyObject *args, PyObject *kwargs)
{
    double offset, interval;
    PyObject *reschedule_cb;
    Loop *loop;
    PyObject *callback, *data = NULL;

    static char *kwlist[] = {"offset", "interval", "reschedule_cb",
                             "loop", "callback", "data", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ddOO!O|O:__init__", kwlist,
            &offset, &interval, &reschedule_cb, &LoopType, &loop, &callback,
            &data)) {
        return -1;
    }
    if (init_Watcher((Watcher *)self, loop, 0, callback, NULL, data)) {
        return -1;
    }
    if (set_Periodic(self, offset, interval, reschedule_cb)) {
        return -1;
    }
    return 0;
}


/* Periodic.set(offset, interval, reschedule_cb) */
static PyObject *
Periodic_set(Periodic *self, PyObject *args)
{
    double offset, interval;
    PyObject *reschedule_cb;

    if (!PyArg_ParseTuple(args, "ddO:set", &offset, &interval, &reschedule_cb)) {
        return NULL;
    }
    if (!inactive_Watcher((Watcher *)self)) {
        return NULL;
    }
    if (set_Periodic(self, offset, interval, reschedule_cb)) {
        return NULL;
    }
    Py_RETURN_NONE;
}


//XXX: reset?
/* Periodic.reset() */
static PyObject *
Periodic_reset(Periodic *self)
{
    ev_periodic_again(((Watcher *)self)->loop->loop, &self->periodic);
    Py_RETURN_NONE;
}


/* Periodic.at() */
static PyObject *
Periodic_at(Periodic *self)
{
    return PyFloat_FromDouble(ev_periodic_at(&self->periodic));
}


/* PeriodicType.tp_methods */
static PyMethodDef Periodic_tp_methods[] = {
    {"set", (PyCFunction)Periodic_set, METH_VARARGS, Periodic_set_doc},
    {"reset", (PyCFunction)Periodic_reset, METH_NOARGS, Periodic_reset_doc},
    {"at", (PyCFunction)Periodic_at, METH_NOARGS, Periodic_at_doc},
    {NULL}  /* Sentinel */
};


/* PeriodicType.tp_members */
static PyMemberDef Periodic_tp_members[] = {
    {"offset", T_DOUBLE, offsetof(Periodic, periodic.offset), 0,
     Periodic_offset_doc},
    {NULL}  /* Sentinel */
};


/* Periodic.interval */
static PyObject *
Periodic_interval_get(Periodic *self, void *closure)
{
    return PyFloat_FromDouble(self->periodic.interval);
}

static int
Periodic_interval_set(Periodic *self, PyObject *value, void *closure)
{
    double interval;

    if (!value) {
        PyErr_SetString(PyExc_TypeError, "cannot delete attribute");
        return -1;
    }
    interval = PyFloat_AsDouble(value);
    if (interval == -1 && PyErr_Occurred()) {
        return -1;
    }
    if (!positive_float(interval)) {
        return -1;
    }
    self->periodic.interval = interval;
    return 0;
}


/* Periodic.reschedule_cb */
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

    if (!value) {
        PyErr_SetString(PyExc_TypeError, "cannot delete attribute");
        return -1;
    }
    if (value != Py_None && !PyCallable_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "a callable or None is required");
        return -1;
    }
    if (!closure) {
        if (value != Py_None) {
            self->periodic.reschedule_cb = reschedule_Periodic;
        }
        else {
            self->periodic.reschedule_cb = 0;
        }
    }
    tmp = self->reschedule_cb;
    Py_INCREF(value);
    self->reschedule_cb = value;
    Py_XDECREF(tmp);
    return 0;
}


/* PeriodicType.tp_getsets */
static PyGetSetDef Periodic_tp_getsets[] = {
    {"reschedule_cb", (getter)Periodic_reschedule_cb_get,
     (setter)Periodic_reschedule_cb_set, Periodic_reschedule_cb_doc, NULL},
    {"interval", (getter)Periodic_interval_get, (setter)Periodic_interval_set,
     Periodic_interval_doc, NULL},
    {NULL}  /* Sentinel */
};


/* PeriodicType */
static PyTypeObject PeriodicType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyev.Periodic",                          /*tp_name*/
    sizeof(Periodic),                         /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    (destructor)Periodic_tp_dealloc,          /*tp_dealloc*/
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
    Periodic_tp_doc,                          /*tp_doc*/
    (traverseproc)Periodic_tp_traverse,       /*tp_traverse*/
    (inquiry)Periodic_tp_clear,               /*tp_clear*/
    0,                                        /*tp_richcompare*/
    0,                                        /*tp_weaklistoffset*/
    0,                                        /*tp_iter*/
    0,                                        /*tp_iternext*/
    Periodic_tp_methods,                      /*tp_methods*/
    Periodic_tp_members,                      /*tp_members*/
    Periodic_tp_getsets,                      /*tp_getsets*/
    0,                                        /*tp_base*/
    0,                                        /*tp_dict*/
    0,                                        /*tp_descr_get*/
    0,                                        /*tp_descr_set*/
    0,                                        /*tp_dictoffset*/
    (initproc)Periodic_tp_init,               /*tp_init*/
    0,                                        /*tp_alloc*/
    Periodic_tp_new,                          /*tp_new*/
};
