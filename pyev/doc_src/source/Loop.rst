:tocdepth: 1


.. _Loop:


.. currentmodule:: pyev


=============
:class:`Loop`
=============

.. class:: Loop([flags=EVFLAG_AUTO, pending_cb=None, data=None, debug=False, io_interval=0.0, timeout_interval=0.0])

    .. seealso::
        `FUNCTIONS CONTROLLING THE EVENT LOOP
        <http://pod.tst.eu/http://cvs.schmorp.de/libev/ev.pod#FUNCTIONS_CONTROLLING_THE_EVENT_LOOP>`_


    .. method:: fork

        TODO.


    .. method:: now() -> float

        TODO.


.. _EVFLAG_constants:

EVFLAG_* constants
=====================

.. data:: EVFLAG_AUTO

    The default *flags* value. This will setup the loop with default behaviour
    and backend.

.. data:: EVFLAG_NOENV

    If this flag bit is or'ed into the *flags* value (or the program runs setuid
    or setgid) then libev will not look at the environment variable
    :envvar:`LIBEV_FLAGS`. Otherwise (the default), :envvar:`LIBEV_FLAGS` will
    override the *flags* completely if it is found in the environment.
    This is useful to try out specific backends to test their performance, or to
    work around bugs.

.. data:: EVFLAG_FORKCHECK

    Instead of calling :meth:`~Loop.fork` manually after a fork, you can also
    make libev check for a fork in each iteration by enabling this flag.
    This works by calling :c:func:`getpid` on every iteration of the loop, and
    thus this might slow down your event loop if you do a lot of loop iterations
    and little real work, but is usually not noticeable.
    The big advantage of this flag is that you can forget about fork (and forget
    about forgetting to tell libev about forking) when you use it.
    This flag setting cannot be overridden or specified in the
    :envvar:`LIBEV_FLAGS` environment variable.

.. data:: EVFLAG_NOINOTIFY

    When this flag is specified, then libev will **not** attempt to use the
    ``inotify`` API for the :class:`Stat` watchers. Apart from debugging and
    testing, this flag can be useful to conserve ``inotify`` file descriptors,
    as otherwise each loop using :class:`Stat` watchers consumes one ``inotify``
    handle.

.. data:: EVFLAG_SIGNALFD

    When this flag is specified, then libev will attempt to use the ``signalfd``
    API for the :class:`Signal` (and :class:`Child`) watchers. This API delivers
    signals synchronously, which makes it both faster and might make it possible
    to get the queued signal data. It can also simplify signal handling with
    threads, as long as you properly block signals in the threads that are not
    interested in handling them.
    ``signalfd`` will not be used by default as this changes your signal mask.


.. _EVBACKEND_constants:

EVBACKEND_* constants
=====================

.. data:: EVBACKEND_SELECT

    TODO.

.. data:: EVBACKEND_POLL

    TODO.

.. data:: EVBACKEND_EPOLL

    TODO.

.. data:: EVBACKEND_KQUEUE

    TODO.

.. data:: EVBACKEND_DEVPOLL

    TODO.

.. data:: EVBACKEND_PORT

    TODO.

.. data:: EVBACKEND_ALL

    TODO.
