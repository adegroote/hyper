Hyperc man page
===============

Synopsis
--------

**hyperc** [-c] [-h] [-v <level>] [-I <input_path>] [-e <extension>] \
		   [-i] [-o <output_directory>] agent.


Description
-----------

The Hyper compiler **hyperc(1)** is the tool which transforms hyper
specification into C++ code. The generated files can then be compiled using
cmake and any standard C++ compiler to provide an executable agent.

**hyperc** expects a really strict file hierarchy. The argument passed to
**hyperc** corresponds to the agent description, without its extension (i.e.
*test* and not *test.ability*). Furthermore, it will check for a directory
with this exact name, and a file *test.task* which describes the tasks
available for this agent. After that, it will look for subdirectories, each
one named from the associated task name, and which contains recipe
descriptions for this task. It must look like like the following hierarchy:

.. code-block:: none

	- test.ability
	- test
		- test.task (defining test1 and test2 tasks).
		- test1
			- pipo.recipe
		- test2
			- other.recipe
	- user_defined


Options
-------

:-c, --clean:
        Remove any generated files.
:-h, --help:
		Displays information about **hyperc** use.
:-v <level>:
        Increases the verbosity of the **hyperc** compiler.
:-I, --include-path <input_path>:
		Add *input_dir* in the list of paths **hyperc** searches agent
		description. By default, it only searches in the
		${PREFIX}/share/hyper directory.
:-e, --extension  <extension>:
		Loads the compiler extension *<extension>* before starting the real
		compilation phase. It allows to parse properly tagged functions, and
		generate the associated code.
:-i, --initial:
		Generates the initial files for user_defined functions and types. If
		the directory user_defined already exists, the compiler will raise an
		error.
:-o, --output <directory>:
		Generates C++ files and CMakeFiles in the directory *<directory>*. If
		not present, **hyperc** generates the code in the source directory.

Bugs
----

The documentation is pretty coarse. Diagnostic messages are mostly
inexistent.

