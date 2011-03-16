/*******************************************************************************
* utilities
*******************************************************************************/

#define PYEV_MININTERVAL (double)1/8192


/* fwd decl */
static int
Periodic_scheduler_set(Periodic *self, PyObject *value, void *closure);


/* Periodic scheduler stop callback */
static void
stop_scheduler_Periodic(ev_loop *loop, ev_prepare *prepare, int revents)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    Periodic *pyperiodic = prepare->data;
    ev_periodic_stop(loop, &pyperiodic->periodic);
    ev_prepare_stop(loop, prepare);
    PyErr_Restore(pyperiodic->err_type, pyperiodic->err_value,
                  pyperiodic->err_traceback);
    if (pyperiodic->err_fatal) {
        exit_Loop(loop);
    }
    else {
        report_error_Loop(ev_userdata(loop), pyperiodic->scheduler);
    }
    PyMem_Free((void *)prepare);
    PyGILState_Release(gstate);
}


/* Periodic scheduler callback */
static double
scheduler_Periodic(ev_periodic *periodic, double now)
{
    PyGILState_STATE gstate = PyGILState_Ensure();
    double result;
    Periodic *pyperiodic = periodic->data;
    PyObject *pynow, *pyresult = NULL;
    ev_prepare *prepare;

    pynow = PyFloat_FromDouble(now);
    if (!pynow) {
        pyperiodic->err_fatal = 1;
        goto error;
    }
    pyresult = PyObject_CallFunctionObjArgs(pyperiodic->scheduler, pyperiodic,
                                            pynow, NULL);
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
    prepare = (ev_prepare *)PyMem_Malloc(sizeof(ev_prepare));
    if (!prepare) {
        Py_FatalError("failed to allocate memory.");
    }
    PyErr_Fetch(&pyperiodic->err_type, &pyperiodic->err_value,
                &pyperiodic->err_traceback);
    prepare->data = (void *)pyperiodic;
    ev_prepare_init(prepare, stop_scheduler_Periodic);
    ev_prepare_start(((Watcher *)pyperiodic)->loop->loop, prepare);
    result = now + 1e30;

finish:
    Py_XDECREF(pyresult);
    Py_XDECREF(pynow);
    PyGILState_Release(gstate);
    return result;
}


/* set the Periodic */
int
set_Periodic(Periodic *self, double offset, double interval,
             PyObject *scheduler)
{
    if (!positive_float(interval)) {
        return -1;
    }
    if (interval > 0.0) {
        if (interval < PYEV_MININTERVAL) {
            PyErr_SetString(PyExc_ValueError, "'interval' too small");
            return -1;
        }
        if (!positive_float(offset)) {
            return -1;
        }
    }
    /* self->scheduler */
    if (Periodic_scheduler_set(self, scheduler, (void *)1)) {
        return -1;
    }
    if (scheduler != Py_None) {
        ev_periodic_set(&self->periodic, offset, interval, scheduler_Periodic);
    }
    else{
        ev_periodic_set(&self->periodic, offset, interval, 0);
    }
    return 0;
}


/*******************************************************************************
* PeriodicType
*******************************************************************************/

/* PeriodicType.tp_doc */
PyDoc_STRVAR(Periodic_tp_doc,
"Periodic(offset, interval, scheduler, loop, callback[, data=None, priority=0])");


/* PeriodicType.tp_traverse */
static int
Periodic_tp_traverse(Periodic *self, visitproc visit, void *arg)
{
    Py_VISIT(self->err_traceback);
    Py_VISIT(self->err_value);
    Py_VISIT(self->err_type);
    Py_VISIT(self->scheduler);
    return 0;
}


/* PeriodicType.tp_clear */
static int
Periodic_tp_clear(Periodic *self)
{
    Py_CLEAR(self->err_traceback);
    Py_CLEAR(self->err_value);
    Py_CLEAR(self->err_type);
    Py_CLEAR(self->scheduler);
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
    PyObject *scheduler;
    Loop *loop;
    PyObject *callback, *data = NULL;
    int priority = 0;

    static char *kwlist[] = {"offset", "interval", "scheduler",
                             "loop", "callback", "data", "priority", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ddOO!O|Oi:__init__", kwlist,
            &offset, &interval, &scheduler,
            &LoopType, &loop, &callback, &data, &priority)) {
        return -1;
    }
    if (init_Watcher((Watcher *)self, loop, 0,
                     callback, NULL, data, priority)) {
        return -1;
    }
    if (set_Periodic(self, offset, interval, scheduler)) {
        return -1;
    }
    return 0;
}


/* Periodic.set(offset, interval, scheduler) */
PyDoc_STRVAR(Periodic_set_doc,
"set(offset, interval, scheduler)");

static PyObject *
Periodic_set(Periodic *self, PyObject *args)
{
    double offset, interval;
    PyObject *scheduler;

    if (!PyArg_ParseTuple(args, "ddO:set", &offset, &interval, &scheduler)) {
        return NULL;
    }
    if (!inactive_Watcher((Watcher *)self)) {
        return NULL;
    }
    if (set_Periodic(self, offset, interval, scheduler)) {
        return NULL;
    }
    Py_RETURN_NONE;
}


/* Periodic.reset() */
PyDoc_STRVAR(Periodic_reset_doc,
"reset()");

static PyObject *
Periodic_reset(Periodic *self)
{
    ev_periodic_again(((Watcher *)self)->loop->loop, &self->periodic);
    Py_RETURN_NONE;
}


/* Periodic.at() -> float */
PyDoc_STRVAR(Periodic_at_doc,
"at() -> float");

static PyObject *
Periodic_at(Periodic *self)
{
    return PyFloat_FromDouble(ev_periodic_at(&self->periodic));
}


/* PeriodicType.tp_methods */
static PyMethodDef Periodic_tp_methods[] = {
    {"set", (PyCFunction)Periodic_set,
     METH_VARARGS, Periodic_set_doc},
    {"reset", (PyCFunction)Periodic_reset,
     METH_NOARGS, Periodic_reset_doc},
    {"at", (PyCFunction)Periodic_at,
     METH_NOARGS, Periodic_at_doc},
    {NULL}  /* Sentinel */
};


/* Periodic.offset */
PyDoc_STRVAR(Periodic_offset_doc,
"offset");


/* PeriodicType.tp_members */
static PyMemberDef Periodic_tp_members[] = {
    {"offset", T_DOUBLE, offsetof(Periodic, periodic.offset),
     0, Periodic_offset_doc},
    {NULL}  /* Sentinel */
};


/* Periodic.interval */
PyDoc_STRVAR(Periodic_interval_doc,
"interval");

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


/* Periodic.scheduler */
PyDoc_STRVAR(Periodic_scheduler_doc,
"scheduler");

static PyObject *
Periodic_scheduler_get(Periodic *self, void *closure)
{
    Py_INCREF(self->scheduler);
    return self->scheduler;
}

static int
Periodic_scheduler_set(Periodic *self, PyObject *value, void *closure)
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
            self->periodic.reschedule_cb = scheduler_Periodic;
        }
        else {
            self->periodic.reschedule_cb = 0;
        }
    }
    tmp = self->scheduler;
    Py_INCREF(value);
    self->scheduler = value;
    Py_XDECREF(tmp);
    return 0;
}


/* PeriodicType.tp_getsets */
static PyGetSetDef Periodic_tp_getsets[] = {
    {"interval", (getter)Periodic_interval_get, (setter)Periodic_interval_set,
     Periodic_interval_doc, NULL},
    {"scheduler", (getter)Periodic_scheduler_get, (setter)Periodic_scheduler_set,
     Periodic_scheduler_doc, NULL},
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
