Semantic of the Hyper language
==============================

In this page, we describe precisely the semantic of the Hyper language, used
to describe recipes. The grammar is itself described :doc:`here <grammar>`. 

.. note::

    In the following part, we often refer to ``well-typed`` expression. The
    type system of Hyper is relatively simple (similar to typed lambda
    calculus). The implementation of the type checking is in
    ``src/compiler/expression_ast_validate.cc`` and
    ``src/compiler/recipe_expression.cc``.

Variables management
--------------------

let <identifier> <expression>
+++++++++++++++++++++++++++++

The ``let`` keyword allows to create new local variable ``identifier`` in the
recipe scope. ``expression`` must be well-typed. The type of ``identifier`` is
automatically deduced from the result type from ``expression``. The scope of
the local variable is the recipe, i.e. it disappears at the end of the recipe.
If ``expression`` fails to be evaluated (because some variables cannot be
evaluated or the function fails), the recipe fails completely (and so the
variable is not created).

set <identifier> <expression>
+++++++++++++++++++++++++++++

The ``set`` keyword allows to modify the value of a variable referenced by
``identifier`` (it can be a local variable, or an ability variable).
``Expression`` must be well-typed, and the result of ``expression`` must match
the type of ``identifier``. If ``expression`` fails to evaluate, the recipe
fails completely (and in this case, the variable is not modified).

Observers
---------

The following recipe_expression do not modify the agent, nor external agents.
They only observe the world, in a periodic fashion, and react when some
condition are met.

wait(block [, delay])
+++++++++++++++++++++

The ``wait`` keyword represents a blocking action, which waits until the
condition represented by ``block`` is verified. ``block`` is a sequence of
well-typed ``recipe_expression``, which last expression must return a boolean
variable. If at some point, one element of the block fails to evaluate, the
``wait`` instruction ends with failure, which terminate the whole recipe.
``delay`` is the quantity of time (in ms) between two ``block`` execution, the
default value is 50 ms. If the dynamic of your system, considering the
condition verified in ``block`` changes slowly, it is a good idea to set
higher delay (to reduce the load associated to this condition checking).

assert(block [, delay])
+++++++++++++++++++++++

The ``assert`` keyword allows to represent a continuous verification of some
invariant. Contrary to ``wait``, ``assert`` returns immediately an identifier,
which may be used for ``abort`` to suppress the periodic check. As for
``wait``, ``block`` is a sequence of ``recipe_expression``, which last
expression must return a boolean variable. ``delay`` is the quantity of time
(in ms) between two ``block`` execution, the default value is 50 ms. If at
some point, one element of the ``block`` fails to evaluate, the whole recipe
finishes with an ``execution error``. If at some point, ``block`` returns
false, the whole recipe finishes with an ``assertion failure``. 

Constraints maker
-----------------

The following keywords introduce the most important primitives of the
language, as they allow to constraint remote agents, i.e. dynamically interact
with the other part of the system.

make(logic_expression)
++++++++++++++++++++++

The  ``make`` keyword represents a single discrete constraint.
``logic_expression`` is composed of two parts, an ``expression``, and an
``unification list``. Both must be well-typed. In particular, for the second
one, for each pair of the list, the compiler verifies that both part of the
pair are of the same kind. From the ``expression``, the target of the make is
automatically deduced, as the first agent which appears in the expression.
``make`` is a blocking operation, the recipe execution is suspended until the
``constraint`` is verified by the remote agent. If it successes, the recipe
continues normally, otherwise, the whole recipe fails with a ``constraint
failure``.

ensure(logic_expression [, delay])
++++++++++++++++++++++++++++++++++

The ``ensure`` keyword represents a "continuous", or more properly "periodic"
constraint. The ``logic_expression`` follows the same rule than make.
``delay`` is an optional delay (in ms) between two verification of the
constraint, the default value is 50ms. Depending the dynamic of the system and
the verified constraint, it may be or not a good default. The ``ensure``
expression returns an identifier after the first "verification" of the
``constraint``. After that, it periodically checks if the constraint is still
verified, otherwise, it tries to execute some recipes to make it valid again.
If at some point, the remote agent fails to find a solution, the ``ensure``
expression fails with a ``constraint failure``. The process is repeated until
the end of the recipe, or the execution of ``abort`` with the returned
identifier.

abort id
++++++++

The ``abort`` expression allows to finish gracefully a periodic operation such
as ``ensure`` or ``assert``.  It cannot fail. The ``abort`` expression returns
when the aborted operation really terminates.
