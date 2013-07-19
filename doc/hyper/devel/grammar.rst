Grammar of Hyper
================

In this part, we present the current grammar of Hyper. For readability, and
consistency with the implementation, the whole grammar is splited in several
sub-grammar.

From an implementation point of view, the grammar of Hyper is implemented
through the help of `Boost::Spirit <http://www.boost.org/doc/libs/1_54_0/libs/spirit/doc/html/index.html>`_
which uses a syntax relatively similar to plain BNF, using advanced template
method.

.. productionlist::
    hyper: import* (ability | task | recipe_def)*

The following part of the grammar is implemented in
``compiler/base_parser.hh`` through the class ``atom_grammar``,
``identifier_grammar`` and ``scoped_identifier_grammar``.

.. productionlist::
    atom: const_bool | const_int | const_double | const_string
    const_string: """ .* """
    const_int: -?[0-9]+
    const_double: -?[0-9]+ "." [0-9]+
    identifier: [a_zA_Z_][a_zA_Z0-9_]*
    scoped_identifier: (identifier "::")?identifier

This part is implemented in class ``import_grammar`` in
``compiler/import_parser.hh``.

.. productionlist::
    import: "import" const_string

The ``grammar_expression`` is implemented in ``compiler/base_parser.hh``.

.. productionlist::
    expression: binary_expr
    binary_expr: primary_expr ("+" | "-" | "*" | "/" | "==" | "!=" | "<=" | "=>" | ">" | "<" | "&&" | "||") primary_expr
    primary_expr: function_call | atom | "(" expression ")"
    function_call: scoped_identifier "(" (expression ("," expression)*)? ")"

The whole ability grammar is implemented in ``src/compiler/parser.cc``. In the

.. productionlist::
    ability: identifier "=" "ability" "{" ability_context export_context? "}"
    ability_context: "context" "=" "{" block_variable block_variable block_variable "}"
    block_variable: "{" variable_decl* "}#
    variable_decl: scoped_identifier identifier ("=" variable_initializer)? ";"
    variable_initializer: atom | identifier "(" ")"
    export_context: "export" "=" "{" type_block fun_block logic_block "}"
    type_block: "{" type_decl* "}"
    type_decl: newtype_decl | opaquedecl | struct_decl
    newtype_decl: identifier "=" "newtype" scoped_identifier ";"
    opaque_decl: identifier "=" "opaquetype" ";"
    struct_decl: identifier "=" "struct" "{" (scoped_identifier identifier ";")* "}" ";"
    fun_block: "{" fun_decl* "}"
    tag: "@" identifier
    fun_decl: tag? scoped_identifier identifier "(" (scoped_identifier identifier? (", " scoped_identifier identifier?)*)? ")"
    logic_block: "{" logic_decl* "}"
    logic_decl: identifier "=" (expression ("," expression*))? "|-" (expression ("," expression*))? ";"

The task grammar (``grammar_task``) is implemented in
``src/compiler/task_parser.cc``.
The ``cond_block`` part is implemented separately in
``compiler/cond_block_parser.hh``, as it is shared with recipe grammar.

.. productionlist:: 
    task: identifier "=" "task" "{" con_block "}" "}" ";"
    cond_block: pre_cond post_cond
    pre_cond: "pre" "=" cond
    post_cond: "post" "=" cond
    cond: "{" ("{" expression "}")* "} ";"? 

Last, the recipe grammar is implemented in ``src/compiler/recipe_parser.cc``.
As it is a bit complex, the implementation uses several classes to implement
the full grammar recipe, including ``recipe_grammar``, ``recipe_context``,
``body_block_grammar``  and ``end_block_grammar``.
    
.. productionlist::
    recipe_def: (letname | letfun | recipe)*
    letname: "letname" identifier expression
    letfun: "let" identifier+ "=" "fun" body_block
    recipe: identifier "=" "recipe" "{" cond_block prefer_decl? "body" "=" body_block end_block? "}" ";"?
    prefer_decl: "prefer" "=" const_int ";"?
    body_block: "{" recipe_expression* "}"
    recipe_expression: (let_decl | set_decl | make_decl | ensure_decl | wait_decl | assert_decl | abort_decl | expression);
    let_decl: "let" identifier recipe_expression
    set_decl: "set" identifier expression
    make_decl: "make" "(" logic_expression ")"
    ensure_decl: "ensure" "(" logic_expression ("," const_double)? ")"
    logic_expression: expression ("where" atom "==" atom)*
    wait_decl: "wait" "(" recipe_expression* ("," const_double)? ")"
    assert_decl: "assert" "(" recipe_expression* ("," const_double)? ")"
    abort_decl: "abort" identifier
    end_block: "end" "{" (let_decl | set_decl | expression) "}"

