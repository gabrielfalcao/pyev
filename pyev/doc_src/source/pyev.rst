.. _pyev:


***************************************
:mod:`pyev` --- Python libev interface.
***************************************

.. module:: pyev
    :platform: POSIX
    :synopsis: Python libev interface.


.. envvar:: LIBEV_FLAGS

    See :const:`EVFLAG_NOENV`.


Functions
*********

.. seealso::
    `GLOBAL FUNCTIONS
    <http://pod.tst.eu/http://cvs.schmorp.de/libev/ev.pod#GLOBAL_FUNCTIONS>`_


.. function:: version() -> tuple of strings

    Returns a tuple of version strings. The former is pyev version, while the
    latter is the underlying libev version.


.. function:: abi_version() -> tuple of int

    Returns a tuple of major, minor version numbers. These numbers represent the
    libev ABI version that this module is running.

    .. note::
        This is not the same as libev version (although it might coincide).


.. function:: time() -> float

    Returns the current time as libev would use it.

    .. note::
        The :meth:`Loop.now` method is usually faster and also often returns the
        timestamp you actually want to know.


.. function:: sleep(interval)

    Sleep for the given *interval* (in seconds). The current thread will be
    blocked until either it is interrupted or the given time *interval* has
    passed.


.. function:: supported_backends() -> int

    Returns the set of all backends (i.e. their corresponding EVBACKEND_* value)
    compiled into this binary of libev (independent of their availability on the
    system you are running on).

    See :ref:`EVBACKEND_constants` for a description of the set values.


.. function:: recommended_backends() -> int

    Returns the set of all backends compiled into this binary of libev and also
    recommended for this platform. This set is often smaller than the one
    returned by supported_backends(), as for example kqueue is broken on most
    BSDs and will not be auto-detected unless you explicitly request it. This is
    the set of backends that libev will probe for if you specify no backends
    explicitly.

    See :ref:`EVBACKEND_constants` for a description of the set values.


.. function:: embeddable_backends() -> int

    Returns the set of backends that are embeddable in other event loops. This
    is the theoretical, all-platform, value. To find which backends might be
    supported on the current system, you would need to look at::

        embeddable_backends() & supported_backends().

    See :ref:`EVBACKEND_constants` for a description of the set values.



.. function:: default_loop([flags=EVFLAG_AUTO, pending_cb=None, data=None, debug=False, io_interval=0.0, timeout_interval=0.0]) -> the 'default loop'

    TODO.


Types
*****


.. exception:: Error

    Raised when an error specific to pyev happens.


.. toctree::
    :titlesonly:
    :maxdepth: 2

    Loop
    Watchers
