; References

(identifier) @reference

; Definitions

(function_definition
  (function_output
    [(identifier) @definition.var
                  (matrix (row
                    (identifier) @definition.var))])?
  name: (identifier) @definition.function
  (function_arguments
    (identifier)* @definition.parameter
    ("," (identifier) @definition.parameter)*)?) @scope

(assignment
  left: (identifier) @definition.var)
(assignment
  left: (multioutput_variable
    (identifier) @definition.var))

(iterator . (identifier) @definition.var)
(lambda (arguments (identifier) @definition.parameter))
(global_operator (identifier) @definition.var)
(persistent_operator (identifier) @definition.var)
(catch (identifier) @definition)
