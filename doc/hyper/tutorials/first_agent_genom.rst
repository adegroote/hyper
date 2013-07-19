Controlling the demo GeNoM module through hyper
===============================================

In the previous tutorials, the movement of the 'robot' is simulated by the
agent, in two different ways. Now, you will see a more "natural" use of
hyper: the control of a functional module, which basically the same kind of
things that the previous agent.

This tutorial introduces how to control a functional component (in this
particular example a GeNoM module).

.. note::

    For this tutorial, you need hyper of course, but also the 
    `hyper_genom interface <git://openrobots.org/robots/hyper_genom>`_ 
    and the GeNoM module demo-genom (compiled with the -x (tclserv_client))
    option.

Describing the interface of the demo agent
------------------------------------------

The ``demo`` interface
++++++++++++++++++++++

Let's describe the ``demo`` agent, which will interface with the ``demo``
GeNoM module.

.. code-block:: ruby
    :linenos:

    demo = ability
    {
        context =
        {
            {
                /* read-write variables */
                double goal;
            }
            {
                /* read-only variables */
                double pos = get_pos();
            }
            {
                /* private variables */
                bool init = false;
                demoModule demoImpl;
            }
        }

        export =
        {
            {
                demoModule = opaquetype;
            }

            {
                /* Here, declare your functions */
                @genom demoModule construct(string host, int port);
                @genom void move(demoModule impl, double goal);
                @genom double read_pos(demoModule impl);
            }
            {
            }

        }
    }


.. warning::

    The naming convention is relatively important here (not mandatory through)
    (``demo``, ``demoModule``, ``demoImpl``). Some code to interface with GeNoM
    module is generated, using the assumption that the ability name is the same
    than the GeNoM module to control. If it is not the case, it must be
    manually adjusted.

Understanding the ``demo`` interface
++++++++++++++++++++++++++++++++++++

The **context** is mostly the same than ``demo_loco_speed`` agent, but all the
variable related to simulated position has disappeared. But, now, you have an
instance of ``demoModule``, which is the handler to control the demo GeNoM
module. This ``demoModule`` is defined as an **opaquetype** (line 23) i.e.
basically a type whose definition is unknown (from the point of view of hyper).

After that, you define three functions, starting by ``@genom`` tag. This tag
means that you need the genom extension to parse and generate the code
associated to this function. This notion of extension plugin allows to generate
specific code for specific functional layer (GeNoM, ROS, ...). Apart from that,
the definition of these functions does not bring anything new:

- line 28: you define the function ``construct`` which permits to create an
  handler ``demoModule`` from an ``host`` and a ``port``.
- line 29: you define the function ``move`` which will asks to the module to
  move the robot it controls.
- line 30: you define the function ``read_pos`` which return the current
  position of the robot.

Implementing these new functions
++++++++++++++++++++++++++++++++

It is now time to implement these functions, which is a bit more complex than 
for ``normal`` functions.

First, you need to create the stubs using::

    hyperc -e genom -i demo


In addition to the flag ``-i`` used to create the different stubs, you must
pass the ``-e genom`` flag which loads the genom extension, allowing to parse
and generate code for GeNoM components.

You will need to implement the different functions in ``user_defined/funcs``,
but first, you need to implement some basic stuff related to the communication
with the GeNoM module.

So, first, edit the ``user_defined/demo_genom_interface.hh``. In this file,
you need to edit ``MODULE_REQUESTS`` and ``MODULES_POSTERS`` you want to
reference for this module (as indicated by ``TODO``). For this tutorial, you
will use the request ``GotoPosition`` (which takes a double in input and
returns nothing)  and will read the poster ``Mobile`` to get the position of
the robot. So you must edit the file to get something like:

.. code-block:: c++

    #define MODULE_REQUESTS \ 
            BOOST_PP_TUPLE_TO_LIST( \ 
                                1,              \ 
                                (               \ 
                                    (GotoPosition, double, void) \ 
                                )                \ 
                        ) 
     
    #define MODULE_POSTERS \ 
            BOOST_PP_TUPLE_TO_LIST( \ 
                                1,              \ 
                                (               \ 
                                    (Mobile, DEMO_STATE_STR) \ 
                                )               \ 
                        )
    

Now, you must edit the ``user_defined/genom_funcs.hh``. This file contains lots
of code, but the part we must potentially edited  are marked by some `` XXX ``
annotation. Basically, you must define for each declared function if it uses
some GeNoM call, and which input and output are needed. In this case, you only
need to change the ``genom_input_type`` for the ``move`` structure from
``void`` to ``double`` (as it will be implemented by the ``GotoPosition`` request).

.. note::

    The manual definition of ``genom_{input, output}_type`` is needed because
    the framework must store some instance of it (due to the callback chain
    used to implement hyper).

Once you have modified these two files, you can edit the different
``user_defined/funcs/*`` file. Now open the file
``user_defined/funcs/move.cc``. The file must look like this:

.. code-block:: c++
    :linenos:

    #include <demo/genom_funcs.hh>

    namespace hyper {
        namespace demo {
    
            void move::callback(const boost::system::error_code& e,
                            genom_model::reference<genom_output_type>::type genom_output,
                            genom_model::reference<output_type>::type output,
                            hyper::genom_model::cb_type cb)
    
            {
                if (e)
                    return cb(e);
    
                // convert genom_output to ouput if necessary
    
                cb(e);
            }
    
            ssize_t move::apply(demo::demoModule const & v0, double v1,
                            boost::asio::io_service& io_s,
                            genom_model::reference<genom_input_type>::type genom_input,
                            genom_model::reference<genom_output_type>::type genom_output,
                            genom_model::reference<output_type>::type output,
                            hyper::genom_model::cb_type cb)
            {
                hyper::genom_model::cb_type local_cb =
                        boost::bind(move::callback,
                            boost::asio::placeholders::error,
                            boost::ref(genom_output), boost::ref(output), cb);
            }
    
        }
    }


So there is two functions: the ``apply`` one (line 20) which is the main part
(where you must call some GeNoM request), and an optional ``callback`` where
you can process the output of the GeNoM request to adapt from GeNoM semantic to
hyper semantic. If there is no output (which is the general case), you don't
need to implement anything in this method. Now, let implement this function:

.. code-block:: c++
    :linenos:
    
        ssize_t move::apply(demo::demoModule const & v0, double v1,
                        boost::asio::io_service& io_s,
                        genom_model::reference<genom_input_type>::type genom_input,
                        genom_model::reference<genom_output_type>::type genom_output,
                        genom_model::reference<output_type>::type output,
                        hyper::genom_model::cb_type cb)
        {
            hyper::genom_model::cb_type local_cb =
                    boost::bind(move::callback,
                        boost::asio::placeholders::error,
                        boost::ref(genom_output), boost::ref(output), cb);

            genom_input = v1;
            return v0.impl->async_GotoPosition(genom_input, cb);
        }


What is new ? Line 14, you fill genom_input correctly. Then, you call the
request ``GotoPosition`` through the method ``async_GotoPosition`` with
``genom_input`` and the callback ``cb``. Here, there is no output, so we can
return directly the callback ``cb``, otherwise, you must pass the automatically
generated ``local_cb``.

The two other functions are a bit different, as they use some synchronous call.
For the ``read_pos`` method, you can edit like:

.. code-block:: c++
    :linenos:

        ssize_t read_pos::apply(demo::demoModule const & v0,
                    boost::asio::io_service& io_s,
                    genom_model::reference<genom_input_type>::type genom_input,
                    genom_model::reference<genom_output_type>::type genom_output,
                    genom_model::reference<output_type>::type output,
                    hyper::genom_model::cb_type cb)
        {
            hyper::genom_model::cb_type local_cb =
                    boost::bind(read_pos::callback,
                        boost::asio::placeholders::error,
                        boost::ref(genom_output), boost::ref(output), cb);

            const DEMO_STATE_STR& str = v0.impl->Mobile();
            output = str.position;

            cb(boost::system::error_code());
            return -1;
        }


Line 13, you read the ``Mobile`` poster. Line 14, you fill properly the output
of the function (here, you are only interested by the position of the robot).
Line 16 is important, you must call the callback, even if it looks synchronous.
As it is success, you can just use the default constructor. Last line, you
return -1, an invalid genom identifier, so the request cannot be aborted.

Finally, let edit the ``construct.cc`` file.

.. code-block:: c++
    :linenos:

     ssize_t construct::apply(std::string const & v0, int v1,
                        boost::asio::io_service& io_s,
                        genom_model::reference<genom_input_type>::type genom_input,
                        genom_model::reference<genom_output_type>::type genom_output,
                        genom_model::reference<output_type>::type output,
                        hyper::genom_model::cb_type cb)
        {
            hyper::genom_model::cb_type local_cb =
                    boost::bind(construct::callback,
                        boost::asio::placeholders::error,
                        boost::ref(genom_output), boost::ref(output), cb);

            try {
                output = demoModule(io_s, v0, v1, cb);
            } catch (const genom_model::genom_process_exception&){
                cb(make_error_code(boost::system::errc::invalid_argument));
            }
            return -1;
        }

Basically, you construct an handler line 14. The constructor will call the
callback ``cb`` if everything goes fine. Otherwise, it may throw an exception,
that must be catched. Last, your return -1.


Building the agent
++++++++++++++++++

You can now build the agent as usual::

    hyperc demo
    mkdir build
    cd build
    cmake ../
    make

Implementing the agent behaviour
--------------------------------

The task interface is the same, so no need to make any chance at this point.
You just need to implement the different recipes differently, but the
implementation is really easy.

.. code-block:: c
    :linenos:

    init = recipe {
        pre = {}
        post = {}
        body = {
            set demoImpl construct("localhost", 9472)
            set init true
        }
    }

.. code-block:: c
    :linenos:

    get_pos = recipe {
        pre = {}
        post = {}
        body = {
            set pos read_pos(demoImpl)
        }
    }

.. code-block:: c
    :linenos:

    move = recipe {
        pre = {}
        post = {}
        body = {
            move(demoImpl, goal)
        }
    }

That is quite easy. In ``init``, you just construct an handler, and assign it
to ``demoImpl``. The two arguments are the default value for the GeNoM /
tclserv environment. For the two other recipes, you just call the previously
defined function. Note that ``move`` only returns when the robot has finished
its movement (i.e it reaches its final destination).


Using the agent
+++++++++++++++

The agent has the same interface than previously, so we can use the same kind
of requests.

.. warning:: 

    To make these examples working, you must start the standard GeNoM
    environment, i.e. h2 and tclserv

Let's try to get the position of the robot::

    hyper_demo_test get pos

It takes some time, because on the first call, it must start the real module to
get an handle on it. Next call are instantaneous.

Now, you can try to move the robot::

    hyper_demo_test make "demo::pos == demo::goal where demo::goal == 0.5"

It takes some time, but it finishes. If you ask for the position of the robot
in parallel, you will see that it slowly changes. At the end, the position of
the robot is 0.5.

Now, try to send the robot further::

    hyper_demo_test make "demo::pos == demo::goal where demo::goal == 2.5"

It fails mostly instantaneously with the following error message::

    Failed to enforce equal_double(demo::pos,demo::goal)
    <constraint_failure equal_double(demo::pos,demo::goal) > 
        <execution_failure demo::move(demo::demoImpl,demo::goal) with S_demo_TOO_FAR_AWAY > in demo#move#move


It provides some kind of backtrace, quite simple in this case, meaning it fails
to enforce the constraint demo::pos == demo::goal (what we ask) because there
is some failure in the execution of move(impl, goal) in the call of
demo#move#move (the ``move`` recipe of the ``move`` task of the ``demo``
agent). The error is named ``S_demo_TOO_FAR_AWAY``. The error is due to the
fact that the ``demo`` module accept by default only request between -1.0 and
1.0. 

More generally, the backtrace shows all the different try before the failure of
the constraint. It can be further analysed by another program to find a
different solution, or in last resort, by the human operator, which can try to
fix the situation.

You must now be able to construct an agent to control one GeNoM module, or more
generally speaking one component of some functional layers. It is the basis to
control a robot system. You must then write agents to coordinate these
low-level agent in order to have a global consistent behaviour.
