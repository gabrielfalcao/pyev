pyev is Python `libev <http://libev.schmorp.de/>`_ interface.

libev is an event loop: you register interest in certain events (such as a file
descriptor being readable or a timeout occurring), and it will manage these event
sources and provide your program with events.
To do this, it must take more or less complete control over your process
(or thread) by executing the event loop handler, and will then communicate
events via a callback mechanism.
You register interest in certain events by registering so-called event watchers,
which you initialise with the details of the event, and then hand it over to
libev by starting the watcher.

libev supports ``select``, ``poll``, the Linux-specific ``epoll``,
the BSD-specific ``kqueue`` and the Solaris-specific ``event port`` mechanisms
for file descriptor events (:class:`~pyev.Io`), the Linux ``inotify`` interface
(for :class:`~pyev.Stat`), Linux ``eventfd``/``signalfd`` (for faster and
cleaner inter-thread wakeup (:class:`~pyev.Async`) and signal handling
(:class:`~pyev.Signal`)), relative timers (:class:`~pyev.Timer`), absolute
timers with customised rescheduling (:class:`~pyev.Periodic`), synchronous
signals (:class:`~pyev.Signal`), process status change events
(:class:`~pyev.Child`), and event watchers dealing with the event loop mechanism
itself (:class:`~pyev.Idle`, :class:`~pyev.Embed`, :class:`~pyev.Prepare` and
:class:`~pyev.Check` watchers) as well as file watchers (:class:`~pyev.Stat`)
and even limited support for fork events (:class:`~pyev.Fork`).

libev is written and maintained by Marc Lehmann.

.. seealso::
    `libev's documentation
    <http://pod.tst.eu/http://cvs.schmorp.de/libev/ev.pod>`_.


Useful links:

- `Latest release <http://pypi.python.org/pypi/pyev/>`_
- `Documentation <http://packages.python.org/pyev/>`_
- `Bug reports and feature requests <http://code.google.com/p/pyev/issues/list>`_


`pyev's source code <http://pyev.googlecode.com/>`_ is currently hosted by
`Google code <http://code.google.com/>`_ and kept in a `Subversion
<http://subversion.apache.org/>`_ repository.

- `Subversion instructions <http://code.google.com/p/pyev/source/checkout>`_
- `Subversion browser <http://code.google.com/p/pyev/source/browse/>`_
