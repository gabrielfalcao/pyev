.. _Periodic:


.. currentmodule:: pyev


=========================================
:py:class:`Periodic` --- Periodic watcher
=========================================


.. py:class:: Periodic(offset, interval, scheduler, loop, callback[, data=None, priority=0])

    :param float offset: See :ref:`Periodic_modes`.

    :param float interval: See :ref:`Periodic_modes`.

    :type scheduler: callable or None
    :param scheduler: See :ref:`Periodic_modes`.

    :type loop: :py:class:`Loop`
    :param loop: loop object responsible for this watcher (accessible through
        :py:attr:`Watcher.loop`).

    :param callable callback: See :py:attr:`Watcher.callback`.

    :param object data: any Python object you might want to attach to the
        watcher (stored in :py:attr:`Watcher.data`).

    :param int priority: See :py:attr:`Watcher.priority`.

    :py:class:`Periodic` watchers are also timers of a kind, but they are very
    versatile (and unfortunately a bit complex).

    Unlike :py:class:`Timer`, :py:class:`Periodic` watchers are not based on
    real time (or relative time, the physical time that passes) but on wall
    clock time (absolute time, the thing you can read on your calendar or clock).
    The difference is that wall clock time can run faster or slower than real
    time, and time jumps are not uncommon (e.g. when you adjust your wrist-watch).

    You can tell a :py:class:`Periodic` watcher to trigger after some specific
    point in time: for example, if you tell a :py:class:`Periodic` watcher to
    trigger 'in 10 seconds' (by specifying e.g. :py:meth:`Loop.now` + ``10.0``,
    that is, an absolute time not a delay) and then reset your system clock to
    January of the previous year, then it will take a year or more to trigger
    the event (unlike a :py:class:`Timer`, which would still trigger roughly
    ``10`` seconds after starting it, as it uses a relative timeout).

    :py:class:`Periodic` watchers can also be used to implement vastly more
    complex timers, such as triggering an event on each 'midnight, local time',
    or other complicated rules. This cannot be done with :py:class:`Timer`
    watchers, as those cannot react to time jumps.

    As with timers, the callback is guaranteed to be invoked only when the point
    in time where it is supposed to trigger has passed. If multiple timers
    become ready during the same loop iteration then the ones with earlier
    time-out values are invoked before ones with later time-out values (but this
    is no longer true when a callback calls :py:meth:`Loop.start` recursively).

    .. seealso::
        `ev_periodic - to cron or not to cron?
        <http://pod.tst.eu/http://cvs.schmorp.de/libev/ev.pod#code_ev_periodic_code_to_cron_or_not>`_


    .. py:method:: set(offset, interval, scheduler)

        :param float offset: See :ref:`Periodic_modes`.

        :param float interval: See :ref:`Periodic_modes`.

        :type scheduler: callable or None
        :param scheduler: See :ref:`Periodic_modes`.

        Configures the watcher.


    .. py:method:: reset

        Simply stops and restarts the periodic watcher again. This is only
        useful when you changed some attributes or :py:attr:`scheduler` would
        return a different time than the last time it was called (e.g. in a
        crond like program when the crontabs have changed).


    .. py:method:: at() -> float

        When the watcher is active, returns the absolute time that this watcher
        is supposed to trigger next. This is not the same as the offset argument
        to :py:meth:`set` or :py:meth:`__init__`, but indeed works even in
        interval and manual rescheduling modes.


    .. py:attribute:: offset

        When repeating, this contains the offset value, otherwise this is the
        absolute point in time (the offset value passed to :py:meth:`set` or
        :py:meth:`__init__`, although libev might modify this value for better
        numerical stability).

        Can be modified any time, but changes only take effect when the periodic
        timer fires or :py:meth:`reset` is being called.


    .. py:attribute:: interval

        The current interval value. Can be modified any time, but changes only
        take effect when the periodic timer fires or :py:meth:`reset` is being
        called.


    .. py:attribute:: scheduler

        The current reschedule callback, or :py:const:`None`, if this
        functionality is  switched off. Can be changed any time, but changes
        only take effect when the periodic timer fires or :py:meth:`reset` is
        being called.

        If given its signature must be:

        .. py:method:: scheduler(periodic, now) -> float
            :noindex:

            :type periodic: :py:class:`Periodic`
            :param periodic: this watcher.

            :param float now: the current time.

        It must return a :py:class:`float` greater than or equal to the *now*
        argument to indicate the next time the watcher callback should be
        scheduled. It will usually be called just before the callback will be
        triggered, but might be called at other times, too.

        .. warning::
            * This callback **must not** stop or destroy any watcher, ever, or
              make **any** other event loop modifications whatsoever. If you
              need to stop it, return ``now + 1e+30`` and stop it afterwards (e.g.
              by starting a :py:class:`Prepare` watcher, which is the only event
              loop modification you are allowed to do).
            * If the reshedule callback raises an error, or returns anything but
              a :py:class:`float`, pyev will stop this watcher.


.. _Periodic_modes:

:py:class:`Periodic` modes of operation
=======================================


Absolute timer
--------------

This is triggered by setting *scheduler* to :py:const:`None`, *interval* to
``0.0`` and *offset* to a float.

In this configuration the watcher triggers an event after the wall clock
time *offset* has passed. It will not repeat and will not adjust when a time
jump occurs, that is, if it is to be run at January 1st 2012 then it will be
stopped and invoked when the system clock reaches or surpasses this point in
time.

Example, trigger an event on January 1st 2012 00:00:00 UTC::

    Periodic(1325376000.0, 0.0, None, ...)


Repeating interval timer
------------------------

This mode is triggered when *scheduler* is set to :py:const:`None` and
*interval* to a value > ``0.0``.

This can be used to create timers that do not drift with respect to the
system clock, for example, here is a :py:class:`Periodic` that triggers each
hour, on the hour (with respect to UTC)::

    Periodic(0.0, 3600.0, None, ...)

This doesn't mean there will always be ``3600`` seconds in between triggers,
but only that the callback will be called when the system time shows a full
hour (UTC), or more correctly, when the system time is evenly divisible by
``3600``.

In this mode, typical values for *offset* are ``0`` or something between ``0``
and *interval*.

For numerical stability, *interval* should be higher than ``1/8192`` (which
is around 100 microseconds).
Note also that there is an upper limit to how often a timer can fire (CPU
speed for example), so if *interval* is very small then timing stability
will of course deteriorate. libev itself tries to be exact to the
millisecond (if the OS supports it and the machine is fast enough).


Manual reschedule mode
----------------------

This functionality is triggered when *scheduler* is set to a :py:class:`callable`.

In this mode the values for *interval* and *offset* are both ignored.
Instead, each time the :py:class:`Periodic` watcher gets scheduled,
:py:attr:`~Periodic.scheduler` will be called with the watcher as first, and
the current time as second argument::

    def myscheduler(periodic, now):
        return now + 60.0

    Periodic(0.0, 0.0, myscheduler, ...)

This can be used to create very complex timers, such as a timer that
triggers on 'next midnight, local time'. To do this, you would calculate the
next midnight after *now* and return the timestamp value for this.
