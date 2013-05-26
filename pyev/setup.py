#
# Copyright (c) 2009 - 2013 Malek Hadj-Ali
# All rights reserved.
#
# This file is part of pyev.
#
# pyev is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3
# as published by the Free Software Foundation.
#
# pyev is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with pyev.  If not, see <http://www.gnu.org/licenses/>.
#


from distutils.version import StrictVersion
from distutils.core import setup, Extension

from ctypes.util import find_library
from ctypes import cdll

from sys import argv
from platform import python_version
from os.path import abspath


pyev_version = "0.9.0"
min_python_versions = {"2": "2.7.0", "3": "3.3.0"}
min_libev_version = "4.15"


version_error = "Aborted: pyev-{0} requires {{0}} >= {{1}}".format(pyev_version)

def check_version(current_version, minimum_version, name):
    if StrictVersion(current_version) < StrictVersion(minimum_version):
        raise SystemExit(version_error.format(name, minimum_version))


def libev_version():
    libev_dll_name = find_library("ev")
    if not libev_dll_name:
        raise SystemExit(version_error.format("libev", min_libev_version))
    libev_dll = cdll.LoadLibrary(libev_dll_name)
    return "{0}.{1}".format(libev_dll.ev_version_major(),
                            libev_dll.ev_version_minor())


if "sdist" not in argv:
    python_version = python_version()
    major = python_version[0]
    check_version(python_version, min_python_versions[major], "Python{0}".format(major))
    check_version(libev_version(), min_libev_version, "libev")


PYEV_VERSION = "\"{0}\"".format(pyev_version)

setup(
      name="pyev",
      version=pyev_version,
      url="http://code.google.com/p/pyev/",
      download_url="http://code.google.com/p/pyev/downloads/list",
      description="Python libev interface.",
      long_description=open(abspath("README.txt"), "r").read(),
      author="Malek Hadj-Ali",
      author_email="lekmalek@gmail.com",
      license="GNU General Public License v3 (GPLv3)",
      platforms=["POSIX"],
      ext_modules=[Extension("pyev", ["src/pyev.c"], libraries=["ev"],
                             define_macros=[("PYEV_VERSION", PYEV_VERSION)])],
      classifiers=[
                   "Development Status :: 5 - Production/Stable",
                   "Intended Audience :: Developers",
                   "Intended Audience :: System Administrators",
                   "License :: OSI Approved :: GNU General Public License v3 (GPLv3)",
                   "Operating System :: POSIX",
                   "Programming Language :: Python :: 3.3",
                   "Programming Language :: Python :: 2.7",
                   "Programming Language :: Python :: Implementation :: CPython"
                  ]
     )
