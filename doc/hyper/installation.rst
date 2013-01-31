HYPER installation
==================

Requirements
------------

Supported platforms
+++++++++++++++++++

Only Linux (x86, x86_64) is currently officially supported. The code has been
written with portability in mind, using portable libraries, so the code
probably runs on every Unix system (including MacOSX). There is probably some
glitches to fix to make it work properly on Windows.

Dependencies
++++++++++++

In order to build hyper, you need the following softwares:

	- `cmake <http://www.cmake.org>`_ (>= 2.6.4)
	- `boost libraries <http://www.boost.org>`_ (>= 1.46.1)

Moreover, if you want to build the documentation, you additionally need:

	- `sphinx <http://http://sphinx-doc.org/>`_


Building Hyper
--------------

You can fetch the code using git::

  $ git clone https://github.com/adegroote/hyper.git

or grab a complete archive using::
	
  $ wget https://github.com/adegroote/hyper/archive/master.zip

Once downloaded, go the hyper directory and create a build directory::

  $ mkdir build && cd build

Now, you can call cmake to generate the build infrastructure. You can pass
several parameter to cmake to influence the build of hyper.

- ``CMAKE_INSTALL_PREFIX`` controls where will be installed hyper.
- ``CMAKE_BUILD_TYPE`` controls the optimization level, between Release,
  RelWithDebInfo, or Debug. Release is a good choice if you not develop in
  hyper.
- ``BUILD_DOC`` controls the build of the documentation.
- ``CMAKE_CXX_COMPILER`` permits to control which compiler will be used to
  compiler hyper. The default on Unix is g++. clang++ is another good choice.
- ``CMAKE_CXX_FLAGS`` allows to control some build variable using in hyper.
  The default choice is sane, so don't try to change the default value if you
  do not understand well their impacts. You can specify the following
  variables:

  - ``AGENT_TIMEOUT`` is the time in millisecond between each ping
  - ``HYPER_SERIALIZATION`` is the method used to serialize message between
	different agents.

For example, constructing hyper in release mode, with the doc, and
installing it in ``/opt`` will lead to the following command::

  $ cmake -DCMAKE_INSTALL_PREFIX=/opt -DCMAKE_BUILD_TYPE=Release -DBUILD_DOC=ON ..

You can furthermore change your settings, rerunning ``cmake`` or using the
``ccmake`` to view and change interactively the different settings. Once
done, you can really build hyper, using ``make``.

The target ``make test`` runs the regression test of Hyper. They must all
pass. If it is not the case, generated code probably won't work as expected,
so start a bug report if the detected regression.



