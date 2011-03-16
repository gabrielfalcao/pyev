import signal
import pyev


def sig_cb(watcher, events):
    print("got SIGINT")
    # optional - stop all watchers
    if watcher.data:
        print("stopping watchers: {0}".format(watcher.data))
        while watcher.data:
            watcher.data.pop().stop()
    # unloop all nested loop
    print("stopping the loop: {0}".format(watcher.loop))
    watcher.loop.stop(pyev.EVBREAK_ALL)


def timer_cb(watcher, revents):
    watcher.data += 1
    print("timer.data: {0}".format(watcher.data))
    print("timer.loop.iteration: {0}".format(watcher.loop.iteration))
    print("timer.loop.now(): {0}".format(watcher.loop.now()))


if __name__ == "__main__":
    loop = pyev.default_loop()
    # initialise and start a repeating timer
    timer = pyev.Timer(0, 2, loop, timer_cb, data=0)
    timer.start()
    # initialise and start a Signal watcher
    sig = pyev.Signal(signal.SIGINT, loop, sig_cb)
    sig.data = [timer, sig] # optional
    sig.start()
    # now wait for events to arrive
    loop.start()
