Writing your first **Hyper** agent
==================================

Let's write a simple agent which controls the position of a robot in one
dimension. Normally, such agent must control one (or more) processes of the
functional layer, but here, it will simulate the whole movement.

Describing the interface of the demo_loco agent
-----------------------------------------------

First, you need to declare the interface of your agent. The interface is
composed of two things, its context i.e. the variable it exposes, and the
exported interface (optional) (structures and functions it exposes). This
second part is optional. 

The ``demo_loco`` interface
+++++++++++++++++++++++++++

First, create a directory ``demo_loco`` and create a file
``demo_loco.ability`` containing:

.. code-block:: c
    :linenos:

    demo_loco = ability 
    {
        context = 
        {
            {
                /* read-write variables */
                int goal;
            }
            {
                /* read-only variables */
                int pos = get_pos();
            }
            {
                /* private variables */
                bool init = false;
                int simu_pos;
            }
        }
    }

Understanding the ``demo_loco`` interface
+++++++++++++++++++++++++++++++++++++++++

Line 1 begins the description of the **agent** (or **ability**) ``demo_loco``.
The definition ends line 19, with the closing curly bracket. Inside, it is
possible to add some comments between ``/*``  and ``*/``. Line 3, the
description of context starts. A context must be composed of three blocks,
containing respectively the read-write variables (or controllable variables),
the read-only variables, and the private variables (internal to the agent).
**The order IS important here.**

Each block contains the definition of several variables (possibly 0). A
variable definition is composed of:

- its type
- its name
- possibly an initializer. If there is no initialiser, the value of variable
  is undefined until something give it a value. If the initializer is a
  *function* (like ``get_pos()``), the *function* is called at each access of
  this variable.

Back to the example, you define here a variable ``goal`` which can be
controlled of type **int**, the variable ``pos`` which represent the real
position of the agent, and can retrieved with ``get_pos``. Lastly, you define
two private variables, **init** which tell if the agent is properly
initialized or not, and the **simu_pos** which represents the simulated
position.

.. note::

    On a real implementation, you must rely on the functional layer to get
    access to the real position of the robot, and so **simu_pos** is
    meaningless.


Generating and build this simple agent
++++++++++++++++++++++++++++++++++++++

Now, you can try to generate and compile the described agent. First, you need
to generate the C++ code of this agent from the described specification::

    hyperc demo_loco

It must create a directory ``src/`` containing various C++ files, and a file
``CMakeLists.txt`` to compile them. To build them, you can use the following
command::

    mkdir build
    cd build
    cmake ../
    make

It must produce an executable file ``hyper_demo_loco``. If you start it, it
must fails with the following error message::

    Catched exception : Connection refused

You need to start the **hyper runtime master** before starting other agents.
In another terminal, you can start::

    hyperruntime

Hyper runtime maser communicates with the other hyper agents through sockets.
The default port is ``4242``, but you may specify another using the ``-p``
option.
In order to join the master, we have to specify to hyper agents like
``hyper_demo_loco`` what are the host and port to use. One way is to use the
environment::

    export HYPER_ROOT_ADDR="localhost:4242"

.. note::

    You probably want to integrate this into your .bashrc directly. Another way
    to configure hyper is to use the ``~/.hyper/config`` file::
    
        mkdir -p ~/.hyper/
        echo "localhost 4242" > ~/.hyper/config

Then, starting ``hyper_demo_loco`` must produce::

    discover localhost 4242
    Succesfully registring demo_loco on 127.0.0.1:50147 140.93.65.74:50147 

which means that the agent has properly started and found the runtime master.
At this point, you have a working agent, but it cannot do anything useful.


Describing tasks of your agent
------------------------------

For the moment, you define some interface, but there is no relation between
the different variables. How the fact that another agent set **goal**
influence the behaviour of ``demo_loco`` agent. The tasks definition describes
, in an abstract way, the different behaviours of your agent, and the effects
of these behaviour on the agent state.

Tasks description of ``demo_loco``
++++++++++++++++++++++++++++++++++

First, create a subdirectory ``demo_loco`` (**the name is important here**).
In this directory, you can create one or more files with ``task`` extension
which describes the task of your agent. Let's create ``loco.task`` with the
following content

.. code-block:: c
    :linenos:

    init = task {
       pre = {{ init == false }}
       post = {{ init == true }}
    }

    move = task {
       pre = {
              {init == true}
              { goal != pos }
             }
       post = {{ goal == pos }}
    }

    get_pos = task {
       pre = {{init == true}}
       post = {}
    }

Understanding ``demo_loco`` tasks
+++++++++++++++++++++++++++++++++

The file ``loco.task`` contains the definition of three tasks. On line 1, you
start the definition of task **init**, definition  which ends line 4. A task
defines a contract, i.e. a set of precondition, postconditions, which must
respectively be true before the execution, and after the execution.
Preconditions are block starting prefixed by **pre**, while postconditions
start with **post**. These blocks can be empty (for example, line 16).

A condition can be any expression returning a boolean value, i.e. it can
includes function call, access to variables, use of standard operators. 

You can show here that the previously used ``get_pos()`` is not a function in
fact, but a task. 

Rebuilding the agent
++++++++++++++++++++

It is simply a matter of calling again the ``hyperc`` compiler from the root
directory::

    hyperc demo_loco

Then, you compile again using standard command::

    cd build
    make rebuild_cache
    make

.. warning::

    Do not forget to call ``make rebuild_cache`` to let cmake searches for new
    source files in the ``src`` directory.


Implementing real behaviours for ``demo_loco`` agent
----------------------------------------------------

Previously, you define **tasks**, which are only abstract behaviours, with
contracts. It is now time to implement real strategies for each behaviour. It
is done through the implementation of **recipes**. You must implement at least
one recipe for each task.

Implementing recipes
++++++++++++++++++++

First, you need to move in subdirectory ``demo_loco``, and create one
subdirectory for each **task**, so::

    mkdir init
    mkdir move
    mkdir get_pos

and in each subdirectory, you must create a **recipe** file (with the
extension ``.recipe``). Lets implement some recipes now:

.. code-block:: c
    :linenos:

    init_r = recipe {
        pre = {}
        post = {}
        body = {
            set init true
            set simu_pos 0
        }   
    }   

.. code-block:: c
    :linenos:

    get_pos_r = recipe {
        pre = {}
        post = {}
        body = {
            set pos simu_pos
        }
    }

.. code-block:: c
    :linenos:

    move_r = recipe {
        pre = {}
        post = {}
        body = {
            set simu_pos goal
        }
    }

Understand recipes
++++++++++++++++++

The ``init.recipe`` contains the implementation of the recipe  **init_r**, for
the task **init**. On line 1, you use the keyword **recipe** to start the
definition of a recipe which ends line 8. For each recipe, you can define some
preconditions, some postconditions (in the same way than for tasks). Moreover,
you must define a block **body** which contains the real behaviour of the recipe.

The body must be implemented with the **ROAR language** which is described, in a
comprehensive way in this page (TODO). Here, you are just using the keyword **set**
which takes a variable name in parameter and an expression, and affects the
result of this expression to this variable.

The recipe **init_r** just sets the variable ``init`` to true, and ``simu_pos`` to 0.
The recipe **get_pos_r** copies the value of ``simu_pos`` in ``pos``. Last, the
**move_r** makes the robot changes instantaneously the real position of the
robot ``simu_pos`` to the ``goal``.

Using the agent
+++++++++++++++

In addition of the ``hyper_demo_loco`` agent, the build generates a test
program called ``hyper_demo_loco_test``. This test program allows to access to
the different values of agent variables (including private one), and permits
to add some constraint on this agent.

We now restart the ``demo_loco`` agent::
    
    hyper_demo_loco

and then try to access the ``init`` value of it::

    hyper_demo_loco_test init

It normally answers::

    Get "init": false

We can now try to get the value of ``pos`` using::

    hyper_demo_loco_test pos

It normally answers 0. But lot of things happen in fact. If you check the
value of the variable ``init`` again, you will discover that it is now true,
meaning that the recipe ``init_r`` has been called (and ``get_pos_r`` too).

Let's try to give some goal to the agent::

    hyper_demo_loco_test make "demo_loco::pos == demo_loco::goal where demo_loco::goal == 5"

Basically, we ask the agent to make ``pos`` equivalent to ``goal`` after
assigning to ``goal`` the value 5. The contrary is not possible because of the
permission on these two variables.

Almost instantaneously, the program returns::

    "Successfully enforcing equal_int(demo_loco::pos,demo_loco::goal)"
    
If you ask for the ``pos`` of the agent, it must be 5 now.




