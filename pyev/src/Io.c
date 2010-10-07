/*******************************************************************************
* utilities
*******************************************************************************/

/* set the Io */
int
set_Io(Io *self, PyObject *fd, int events)
{
    /* fd --> fdnum */
    int fdnum = PyObject_AsFileDescriptor(fd);
    if (fdnum == -1) {
        return -1;
    }
    if (events & ~(EV_READ | EV_WRITE)) {
        PyErr_SetString(Error, "illegal event mask");
        return -1;
    }
    ev_io_set(&self->io, fdnum, events);
    return 0;
}


/*******************************************************************************
* IoType
*******************************************************************************/

/* IoType.tp_new */
static PyObject *
Io_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Io *self = (Io *)WatcherType.tp_new(type, args, kwargs);
    if (!self) {
        return NULL;
    }
    new_Watcher((Watcher *)self, (ev_watcher *)&self->io, EV_IO);
    return (PyObject *)self;
}


/* IoType.tp_init */
static int
Io_tp_init(Io *self, PyObject *args, PyObject *kwargs)
{
    PyObject *fd;
    int events;
    Loop *loop;
    PyObject *callback, *data = NULL;

    static char *kwlist[] = {"fd", "events",
                             "loop", "callback", "data", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OiO!O|O:__init__", kwlist,
            &fd, &events, &LoopType, &loop, &callback, &data)) {
        return -1;
    }
    if (init_Watcher((Watcher *)self, loop, 0, callback, NULL, data)) {
        return -1;
    }
    if (set_Io(self, fd, events)) {
        return -1;
    }
    return 0;
}


/* Io.set(fd, events) */
static PyObject *
Io_set(Io *self, PyObject *args)
{
    PyObject *fd;
    int events;

    if (!PyArg_ParseTuple(args, "Oi:set", &fd, &events)) {
        return NULL;
    }
    if (!inactive_Watcher((Watcher *)self)) {
        return NULL;
    }
    if (set_Io(self, fd, events)) {
        return NULL;
    }
    Py_RETURN_NONE;
}


/* IoType.tp_methods */
static PyMethodDef Io_tp_methods[] = {
    {"set", (PyCFunction)Io_set, METH_VARARGS, Io_set_doc},
    {NULL}  /* Sentinel */
};


/* IoType.tp_members */
static PyMemberDef Io_tp_members[] = {
    {"fd", T_INT, offsetof(Io, io.fd), READONLY, Io_fd_doc},
    {"events", T_INT, offsetof(Io, io.events), READONLY, Io_events_doc},
    {NULL}  /* Sentinel */
};


/* IoType */
static PyTypeObject IoType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyev.Io",                                /*tp_name*/
    sizeof(Io),                               /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    0,                                        /*tp_dealloc*/
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
    Io_tp_doc,                                /*tp_doc*/
    0,                                        /*tp_traverse*/
    0,                                        /*tp_clear*/
    0,                                        /*tp_richcompare*/
    0,                                        /*tp_weaklistoffset*/
    0,                                        /*tp_iter*/
    0,                                        /*tp_iternext*/
    Io_tp_methods,                            /*tp_methods*/
    Io_tp_members,                            /*tp_members*/
    0,                                        /*tp_getsets*/
    0,                                        /*tp_base*/
    0,                                        /*tp_dict*/
    0,                                        /*tp_descr_get*/
    0,                                        /*tp_descr_set*/
    0,                                        /*tp_dictoffset*/
    (initproc)Io_tp_init,                     /*tp_init*/
    0,                                        /*tp_alloc*/
    Io_tp_new,                                /*tp_new*/
};
