(function_definition
  (function_output
    [(identifier) @definition.var
                  (multioutput_variable
                    (identifier) @definition.var
                    ("," (identifier) @definition.var))])?
  function_name: (identifier) @definition.function
  (function_arguments
    (identifier)* @definition.parameter
    ("," (identifier) @definition.parameter)*)?) @scope

(assignment
  variable: (_) @definition.var)
(assignment
  (multioutput_variable
    (_) @definition.var))

(unary_operator (identifier) @reference)
(binary_operator (identifier) @reference)
(comparison_operator (identifier) @reference)
(boolean_operator (identifier) @reference)
(postfix_operator (identifier) @reference)
(not_operator (identifier) @reference)
(metaclass_operator (identifier) @reference)
(handle_operator (identifier) @reference)

(function_call (identifier) @reference)
(arguments (identifier) @reference)

(struct . [(function_call
             name: (identifier) @reference)
           (identifier) @reference])

(range (identifier) @reference)
(condition (identifier) @reference)
(iterator . (identifier) @definition.var)
(iterator (identifier) @reference)
(parfor_options (identifier) @reference)
(lambda (arguments (identifier) @definition.parameter))
(global_operator (identifier) @definition.var)
(persistent_operator (identifier) @definition.var)
(catch (captured_exception) @definition.var)
