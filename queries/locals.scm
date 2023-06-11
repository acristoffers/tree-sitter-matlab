(function_definition
  (function_output
    [(identifier) @local.definition
     (multioutput_variable
       (identifier) @local.definition
       ("," (identifier) @local.definition))])?
  (function_arguments
    (identifier)* @local.definition
    ("," (identifier) @local.definition)*)?) @local.scope

(assignment
  variable: (_) @local.definition)
(assignment
  (multioutput_variable
    (_) @local.definition))

(unary_operator (identifier) @local.reference)
(binary_operator (identifier) @local.reference)
(comparison_operator (identifier) @local.reference)
(boolean_operator (identifier) @local.reference)
(postfix_operator (identifier) @local.reference)
(not_operator (identifier) @local.reference)
(metaclass_operator (identifier) @local.reference)
(handle_operator (identifier) @local.reference)

(function_call
    name: (identifier) @local.reference)

(struct . [(function_call
             name: (identifier) @local.reference)
           (identifier) @local.reference])

(struct . [(function_call
             name: (identifier) @local.definition)
           (identifier) @local.definition])

(range (identifier) @local.reference)
(condition (identifier) @local.reference)
(iterator . (identifier) @local.definition)
(iterator (identifier) @local.reference)
(parfor_options (identifier) @local.reference)
(lambda (arguments (identifier) @local.definition))
(global_operator (identifier) @local.definition)
(persistent_operator (identifier) @local.definition)
(try_statement (captured_exception) @local.definition)
