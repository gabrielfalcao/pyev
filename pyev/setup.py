################################################################################
#
# Copyright (c) 2009, Malek Hadj-Ali
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
# 3. Neither the name of the copyright holders nor the names of its contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
# OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
# OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
# IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
# THE POSSIBILITY OF SUCH DAMAGE.
#
#
# Alternatively, the contents of this file may be used under the terms of the
# GNU General Public License (the GNU GPL) version 3 or (at your option) any
# later version, in which case the provisions of the GNU GPL are applicable
# instead of those of the modified BSD license above.
# If you wish to allow use of your version of this file only under the terms
# of the GNU GPL and not to allow others to use your version of this file under
# the modified BSD license above, indicate your decision by deleting
# the provisions above and replace them with the notice and other provisions
# required by the GNU GPL. If you do not delete the provisions above,
# a recipient may use your version of this file under either the modified BSD
# license above or the GNU GPL.
#
################################################################################


from platform import python_version
from os import name as platform_name
from os.path import abspath, join
from re import findall, search
from sys import argv
from subprocess import check_call
from distutils.command.build_ext import build_ext as _build_ext
from distutils.core import setup, Extension


curr_ver = python_version()
min_vers = {2: "2.6.2", 3: "3.1.1"}
if curr_ver < min_vers[int(curr_ver[0])]:
    raise SystemExit("Aborted: pyev requires Python2 >= {0[2]} "
                     "OR Python3 >= {0[3]}".format(min_vers))

if platform_name != "posix":
    raise SystemExit("Aborted: platform '{0}' not supported".format(platform_name))


pyev_version = "0.5.3"
pyev_description = open(abspath("README.txt"), "r").read()
libev_dir = abspath("src/libev")
libev_configure_ac = open(join(libev_dir, "configure.ac"), "r").read()
libev_version = search("AM_INIT_AUTOMAKE\(libev,(\S+)\)",
                       libev_configure_ac).group(1)


class build_ext(_build_ext):
    def finalize_options(self):
        _build_ext.finalize_options(self)
        if "sdist" not in argv:
            check_call(join(libev_dir, "configure"), cwd=libev_dir, shell=True)
            libev_config = open(join(libev_dir, "config.h"), "r").read()
            self.libraries = [l.lower() for l in
                              set(findall("#define HAVE_LIB(\S+) 1",
                                          libev_config))]


setup(
      name="pyev",
      version="-".join((pyev_version, libev_version)),
      url="http://pyev.googlecode.com/",
      description="Python libev interface.",
      long_description=pyev_description,
      author="Malek Hadj-Ali",
      author_email="lekmalek@gmail.com",
      platforms=["POSIX"],
      license="BSD License / GNU General Public License (GPL)",
      cmdclass={"build_ext": build_ext},
      ext_modules=[
                   Extension(
                             "pyev",
                             ["src/pyev.c"],
                             define_macros=[
                                            #("EV_MAXPRI", "5"),
                                            #("EV_MINPRI", "-5"),
                                            ("PYEV_VERSION",
                                             "\"{0}\"".format(pyev_version)),
                                            ("LIBEV_VERSION",
                                             "\"{0}\"".format(libev_version))
                                           ]
                            )
                  ],
      classifiers=[
                   "Development Status :: 4 - Beta",
                   "Intended Audience :: Developers",
                   "License :: OSI Approved :: BSD License",
                   "License :: OSI Approved :: GNU General Public License (GPL)",
                   "Operating System :: POSIX",
                   "Programming Language :: Python :: 2.6",
                   "Programming Language :: Python :: 3.1",
                   "Topic :: Software Development :: Libraries"
                  ]
     )
