#!/usr/bin/env python


# our version
__version__ = "0.1.1"

python_version = (2, 5, 1)
python_error = "Aborting: pyev-%s needs python >= %s" % \
               (__version__, "%s.%s.%s" % python_version)

libev_version = (3, 1)
libev_error = "Aborting: pyev-%s needs libev >= %s installed.\n" \
              "Please see: http://libev.schmorp.de." % \
              (__version__, "%s.%s." % libev_version)


# checking python version
from sys import version_info, exit

py_major, py_minor, py_micro, releaselevel, serial = version_info
if (py_major, py_minor, py_micro) < python_version:
    exit(python_error)


# checking libev installation
from ctypes.util import find_library
from ctypes import CDLL

libev = find_library("ev")
if libev is None:
    exit(libev_error)

try:
    libev = CDLL(libev)
except Exception, exception:
    print exception
    exit(libev_error)

if (libev.ev_version_major(), libev.ev_version_minor()) < libev_version:
    exit(libev_error)


# setup
import ez_setup
ez_setup.use_setuptools()
from setuptools import setup, find_packages, Extension

setup(
      name="ev",
      version=__version__,
      description="ev module",
      long_description="ev is an extension wrapper around libev.",
      author="Malek Hadj-Ali",
      author_email="lekmalek@gmail.com",
      url="http://pyev.googlecode.com/",
      platforms="Linux",
      license="GPLv2",
      packages=find_packages(),
      zip_safe=False,
      ext_modules=[Extension("ev._ev", ["ev/_ev.c"],libraries=["ev"])],
     )

