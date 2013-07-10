A more "realistic" version of demo_loco agent
=============================================

In the :doc:`previous tutorial <first_agent>`, you write an agent which
controls the position of a robot in 1 dimension. This robot can teleport from
any position to any another solution. In this tutorial, you will write a robot
which moves according a defined speed. 

This tutorial introduces how to define specific types for your application,
how to define functions, and some more complex recipes.

Describing the interface of the demo_loco_speed agent
-----------------------------------------------------

You can start from the previous example. First clean the directory, to remove
hidden generated files, source files, ...::

    hyperc -c demo_loco
    rm -rf build

Now copy the content of this directory into a new directory
``demo_loco_speed``. In addition to the context, you define some new types
concerning speed, and some functions to manipulate them.

The ``demo_loco_speed`` interface
+++++++++++++++++++++++++++++++++

Let complete the previously described agent with the following definitions.

.. code-block:: c
    :linenos:

    demo_loco_speed = ability
    {
        context =
        {
            {
                /* read-write variables */
                double goal;
                double wished_speed = 1.0;
            }
            {
                /* read-only variables */
                double pos = get_pos();
            }
            {
                /* private variables */
                bool init = false;
                double simu_pos;
                hour movement_start;
                double movement_sign;
            }
        }

        export =
        {
            {
                /* Here, define your own types */
                hour = struct { int sec; int usec; };
                speed = newtype double;
            }

            {
                /* Here, declare your functions */
                double distance(hour A, hour B);
                hour now();
                double sign(double d);
            }
            {
                /* Here, some logical relations between these functions */
                distance_symmetry = distance(A, B) |- distance(A, B) == distance(B, A);
                distance_associativity = distance(A, B), distance(B, C) |- distance(A,C) == distance(A, B) + distance(B,C);
            }

        }
    }

Understanding the ``demo_loco_speed`` interface
+++++++++++++++++++++++++++++++++++++++++++++++

The **context** has change a bit. The different positions are now stored as
double, and not int. Two new members appears, ``wished_speed`` which will
contain the desired speed, and the private variable ``movement_start``, with
respectively the type ``speed`` and the type ``hour``. 

These two types are not provided by **hyper**, but are user-defined types.
There definition appears in the **export** block. The **export** block
contains three sub-blocks, potentially empty containing respectively (in
order):

#. definition of user type
#. declaration of functions on these types
#. definition of logical relation between these functions.

Defining new types
******************

A new type can be constructed using three approaches:

- as an alias of an existing type, using the **newtype** word. For example,
  you define line 28 the type ``speed`` as an alias of native type ``double``.
  Contrary to the C keyword **typedef**, you cannot use a double instead of a
  speed (apart the constant case), there are strictly two different types.
  Defining such aliases helps the understanding of the program, and reduce the
  search spaces of the logic solver.
- as a cartesian product type using the **struct** keyword (more or less in
  the same way that struct are defined in C). For example, line 27, you define
  the type ``hour`` which contains two int, representing number of seconds and
  number of microseconds since Epoch. This type hour is equivalent to the
  ``struct timeval`` C type.
- using the keyword **opaquetype** which allows to use a *native type*, not
  representable with the current syntax. See the :doc:`tutorial
  <demo_loco_genom>` for some example of use.

Declaring new functions
***********************

In this block, you can declare new functions. At this point, you only declare
them. Implementation is done at the host level language (C++ in the current
implementation). See :ref:`implementing_funs` to see how to implement them.

In this agent, you define three functions:

- line 33: you define the function ``distance`` which takes two ``hour`` in
  parameters and return a double (how much time elapses between theses two
  times).
- line 34: you define the function ``now`` which takes no arguments, and
  returns the current time.
- line 35: you define the function ``sign`` which takes a double in argument, 
  and returns 1.0 if it is positive, -1.0 if it is negative.

Define logical relation between these functions
***********************************************

The last block of the **export** contains logical relations between functions.
These relations are used only in the deductive part of the agent. A relation
is described in the following way::

    <name> = <premises> |- <conclusions> 

Back to our example, two relations are defined, ``distance_symmetry`` and
``distance_associativity``. These names are only used for debug, so in normal
situation, you never use them. However, it is preferable to give them
meaningful name. The ``distance_symmetry`` rule basically means that, if the
agent have an instance of ``distance(A, B)`` for any ``A`` and any ``B``, then
it can assert that ``distance(A, B) == distance(B, A)`` and so unify the two
expressions. In the same way, ``distance_associativity`` says that for any
``A``, ``B``, ``C`` (of kind ``hour``), the logic engine is able to assume
that ``distance(A, B) + distance(B, C) == distance(A, C)``. 

.. _implementing_funs:

Implementing these new functions
++++++++++++++++++++++++++++++++

You declared ``distance`` and ``now``: it is now time to implement them.
First, you need to create the stubs using::

    hyperc -i demo_loco_speed

The ``-i`` flags create the stubs for the different functions in the directory
``user_defined/funcs``. If the directory already exists, it does overwrite
files, but you can find the generated files in ``.hyper/user_defined/funcs/``
and copy the new generated files at the right place.

Now, open the file ``user_defined/funcs/distance.cc``. It must look like

.. code-block:: c++

    #include <demo_loco_speed/funcs.hh>

    namespace hyper {
        namespace demo_loco_speed {
            double distance::apply(demo_loco_speed::hour const & v0, demo_loco_speed::hour const & v1 )
            {
    #error distance not implemented !!
            }
        }
    }

So your job is simply to replace "#error ...." by something useful. One
possible implementation here is

.. code-block:: c++

    #include <demo_loco_speed/funcs.hh> 
  
    namespace hyper {
        namespace demo_loco_speed {
            double distance::apply(demo_loco_speed::hour const & v0, demo_loco_speed::hour const & v1 )
            {
                return (double(v1.sec - v0.sec) * 1000 + double(v1.usec - v0.usec) / 1000) + 1;
            }
        }  
    }

The function ``now`` can be implemented in the following way

.. code-block:: c++

    #include <demo_loco_speed/funcs.hh>
    #include <sys/time.h>

    namespace hyper {
        namespace demo_loco_speed {

            demo_loco_speed::hour now::apply( )
            {
                struct timeval tv;
                gettimeofday(&tv, NULL);
                return demo_loco_speed::hour(tv.tv_sec, tv.tv_usec);
            }

        }
    }

The function ``sign`` can be implemeed as

.. code-block:: c++

    #include <demo_loco/funcs.hh>

    namespace hyper {
        namespace demo_loco {
            double sign::apply(double v0 )
            {
                if (v0 < 0.0) return -1.0;
                return 1.0;
            }
        }
    }

.. warning::

    Do not forget to put under your chosen version control system files from
    ``user_defined/funcs``, they are necessary to build properly the agent.

.. note::

    If you add new functions after first generation, you cannot call again
    ``hyperc -i``. But you can retrieve the template file in
    ``.hyper/demo_loco_speed/funcs/``.

Building the agent
++++++++++++++++++

You can now build the agent as usual::

    hyperc demo_loco_speed
    mkdir build
    cd build
    cmake ../
    make

A more realistic behaviour
--------------------------

You have defined some new functions, it is now time to use it to implement a
more realistic behaviour. The task interface is the same, so no need to make
any chance at this point.

The type of ``simu_pos`` has changed, so now you need to initialize it with 0.0
in the recipe **init_r** recipe. The only interesting change is in the
**move_r** recipe. It is now implemented as

.. code-block:: c
    :linenos:

    move = recipe {
        pre = {}
        post = {}
        body = {
            set movement_start now()
            let move_distance goal - simu_pos
            set movement_sign sign(move_distance)
            let time_needed movement_sign * move_distance / wished_speed
            wait(distance(movement_start, now()) > time_needed)
        }

        end = {
            let current_time now()
            let diff distance(current_time, movement_start)
            let dist diff * wished_speed * movement_sign
            set simu_pos simu_pos + dist
        }
    }

Whoot! It seems a lot more complex!

One new block appears: the **end** block, which is called at the end of the
body, whether after normal termination or after a cancellation. In this block,
we are computing the position of the robot after a certain delay at speed
``wished_speed``. For that purpose, we are computing several intermediate
variable, introduced by the **let** keyword. The type of this variable is
deduced from the returned type of the associated expression. Thus,
``current_time`` is of type ``hour`` (line 13) and ``diff`` is of type
``double`` (line 14). The **end** block does not accept the full **ROAR
language**, see the TODO page. 

Another keyword introduced is the **wait** keyword. Its effects is to block the
recipe until the condition is reached. In this case, the recipe **move_r**
waits until a certain delay has elapsed.


Using the agent
+++++++++++++++

The agent has more or less the same interface than the previous one, the main
difference is the use of double instead of int for ``goal`` and ``pos``.

.. warning::

    hyper does not promote constant int in constant double. In particular 1 is
    not a valid entry for a double. Use 1.0.

Let's try to send a constraint to our agent::

    hyper_demo_loco_speed_test make "demo_loco_speed::pos == demo_loco_speed::goal where demo_loco_speed::goal == 10.0"


Contrary to previous call, it does not return instantaneously. It takes
approximatively 10 sec. You can verify using the following command::

    time hyper_demo_loco_speed_test make "demo_loco_speed::pos == demo_loco_speed::goal where demo_loco_speed::goal == 0.0"

You can check the position of the agent using the same command than previously.
Normally, it must be around 0.0 (but not exactly 0.0, due to the method used to
compute the simulated position).

You can try also to play on the wished_speed::

    time hyper_demo_loco_speed_test make "demo_loco_speed::pos == demo_loco_speed::goal where demo_loco_speed::goal == 20.0 and demo_loco_speed::wished_speed=10.0"

must only takes approximatively 2 seconds.
