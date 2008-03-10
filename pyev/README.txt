pyev is an extension wrapper around libev.


At the time of this writing pyev needs a patched cvs version of libev.
The patch does two things:
1. bump the minor abi version, to avoid confusion with the latest release.
2. enable support for large files, needed by python.

install:

~$: mkdir /tmp/pyev
~$: cd /tmp/pyev/
/tmp/pyev$: cvs -z3 -d :pserver:anonymous@cvs.schmorp.de/schmorpforge co libev
/tmp/pyev$: wget http://pyev.googlecode.com/files/libev_cvs_python.patch
/tmp/pyev$: cd libev/
/tmp/pyev/libev$: patch -p0 < ../libev_cvs_python.patch
/tmp/pyev/libev$: autoreconf -f -i
/tmp/pyev/libev$: ./configure
/tmp/pyev/libev$: make
/tmp/pyev/libev$: sudo make install
/tmp/pyev/libev$: cd ..
/tmp/pyev$: wget http://pyev.googlecode.com/files/ev-0.1.1.tar.gz
/tmp/pyev$: tar -zxf ev-0.1.1.tar.gz
/tmp/pyev$: cd ev-0.1.1/
/tmp/pyev$: sudo python setup.py install
/tmp/pyev$: cd ~
~$: sudo rm -rf /tmp/pyev


example:

>>> import signal
>>> import ev
>>>
>>> def signal_cb(watcher, events):
...     print watcher.data
...     watcher.stop()
...     watcher.loop.unloop()
...
>>> def timer_cb(watcher, events):
...     print watcher.data
...
>>> l = ev.default_loop()
>>> t = ev.Timer(0, 3, l, timer_cb, "hello world")
>>> t.start()
>>> s = ev.Signal(signal.SIGINT, l, signal_cb, (1,2,3))
>>> s.start()
>>> l.loop()
hello world
hello world
hello world
hello world
[hit Ctrl + C]
(1, 2, 3)
>>>

