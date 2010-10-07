/*******************************************************************************
* ChildType
*******************************************************************************/

/* ChildType.tp_new */
static PyObject *
Child_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Child *self = (Child *)WatcherType.tp_new(type, args, kwargs);
    if (!self) {
        return NULL;
    }
    new_Watcher((Watcher *)self, (ev_watcher *)&self->child, EV_CHILD);
    return (PyObject *)self;
}


/* ChildType.tp_init */
static int
Child_tp_init(Child *self, PyObject *args, PyObject *kwargs)
{
    int pid;
    PyObject *trace;
    Loop *loop;
    PyObject *callback, *data = NULL;

    static char *kwlist[] = {"pid", "trace",
                             "loop", "callback", "data", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "iO!O!O|O:__init__", kwlist,
            &pid, &PyBool_Type, &trace, &LoopType, &loop, &callback, &data)) {
        return -1;
    }
    if (init_Watcher((Watcher *)self, loop, 1, callback, NULL, data)) {
        return -1;
    }
    ev_child_set(&self->child, pid, (trace == Py_True) ? 1 : 0);
    return 0;
}


/* Child.set(pid, trace) */
static PyObject *
Child_set(Child *self, PyObject *args)
{
    int pid;
    PyObject *trace;

    if (!PyArg_ParseTuple(args, "iO!:set", &pid, &PyBool_Type, &trace)) {
        return NULL;
    }
    if (!inactive_Watcher((Watcher *)self)) {
        return NULL;
    }
    ev_child_set(&self->child, pid, (trace == Py_True) ? 1 : 0);
    Py_RETURN_NONE;
}


/* ChildType.tp_methods */
static PyMethodDef Child_tp_methods[] = {
    {"set", (PyCFunction)Child_set, METH_VARARGS, Child_set_doc},
    {NULL}  /* Sentinel */
};


/* ChildType.tp_members */
static PyMemberDef Child_tp_members[] = {
    {"pid", T_INT, offsetof(Child, child.pid), READONLY, Child_pid_doc},
    {"rpid", T_INT, offsetof(Child, child.rpid), 0, Child_rpid_doc},
    {"rstatus", T_INT, offsetof(Child, child.rstatus), 0, Child_rstatus_doc},
    {NULL}  /* Sentinel */
};


/* ChildType */
static PyTypeObject ChildType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyev.Child",                             /*tp_name*/
    sizeof(Child),                            /*tp_basicsize*/
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
    Child_tp_doc,                             /*tp_doc*/
    0,                                        /*tp_traverse*/
    0,                                        /*tp_clear*/
    0,                                        /*tp_richcompare*/
    0,                                        /*tp_weaklistoffset*/
    0,                                        /*tp_iter*/
    0,                                        /*tp_iternext*/
    Child_tp_methods,                         /*tp_methods*/
    Child_tp_members,                         /*tp_members*/
    0,                                        /*tp_getsets*/
    0,                                        /*tp_base*/
    0,                                        /*tp_dict*/
    0,                                        /*tp_descr_get*/
    0,                                        /*tp_descr_set*/
    0,                                        /*tp_dictoffset*/
    (initproc)Child_tp_init,                  /*tp_init*/
    0,                                        /*tp_alloc*/
    Child_tp_new,                             /*tp_new*/
};
