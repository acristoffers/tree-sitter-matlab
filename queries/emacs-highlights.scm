; highlights.scm

;; Emacs uses some different conventions for highlights.

(unary_operator
  (operator) @number
  (number))

(unary_operator
  (operator) @operator
  (_))
(binary_operator
  (operator) @operator)
(comparison_operator
  (operator) @operator)
(boolean_operator
  (operator) @operator)
(postfix_operator
  (operator) @operator)
(not_operator
  (operator) @operator)
(metaclass_operator
  (operator) @operator)
(handle_operator
  (operator) @operator)

(assignment
  variable: (_) @variable)
(assignment
  (multioutput_variable
    (_) @variable))

(struct "." @operator)

(function_call
  name: (identifier) @function.call
  ("@" @operator (superclass) @type)?)

(command
  (command_name) @function.call
  (command_argument)* @string)

(spread_operator) @constant

(range
  ":" @operator)

(if_statement
  if: (keyword) @conditional
  (elseif_statement
    elseif: (keyword) @conditional)*
  (else_statement
    else: (keyword) @conditional)*
  end: (keyword) @conditional)

(for_statement
  (keyword) @repeat
  (identifier)? @variable
  (block)
  end: (keyword) @repeat)

(while_statement
  while: (keyword) @repeat
  end: (keyword) @repeat)

(switch_statement
  switch: (keyword) @conditional
  (case
    case: (keyword) @conditional)+
  (otherwise
    otherwise: (keyword) @conditional)+
  end: (keyword) @conditional)

(lambda
  (operator) @operator
  (arguments
    (_) @variable)?)

(global_operator
  (keyword) @keyword
  (identifier) @variable)

(persistent_operator
  (keyword) @keyword
  (identifier) @variable)

(function_definition
  (keyword) @keyword.function
  (identifier) @function
  (end_function
    (keyword) @keyword.function)?)

(function_output
  [(identifier) @variable
                (multioutput_variable
                  (identifier) @variable
                  ("," (identifier) @variable))])

(function_arguments
  (identifier)* @variable.parameter
  ("," (identifier) @variable.parameter)*)

(try_statement
   try: (keyword) @exception
   end: (keyword) @exception)
(catch
   catch: (keyword) @exception
   (captured_exception) @variable)

(class_definition
  classdef: (keyword) @keyword.function
  (attributes
    (identifier) @constant)?
  class_name: (identifier) @type.definition
  (superclasses
    (identifier) @type)?
  end: (keyword) @keyword.function)

(enumeration
  enumeration: (keyword) @structure
  (enum
    argument: (identifier) @constant)
  end: (keyword) @structure)

(events
  events: (keyword) @structure
  (identifier) @constant
  end: (keyword) @structure)

(methods
  methods: (keyword) @structure
  end: (keyword) @structure)

(function_signature
  (function_output)?
  function_name: (identifier) @function
  (function_arguments)?)

(properties
  properties: (keyword) @structure
  end: (keyword) @structure)

(property
  (property_name) @constant
  (class)? @type)

(validation_functions
  (identifier) @type)

(attribute
  (identifier) @constant)

(keyword) @keyword

(string) @string
(string) @spell
(formatting_sequence) @string.special
(escape_sequence) @string.escape

(number) @number
(boolean) @constant.builtin
(comment) @comment
(comment) @spell

[";" "," "." ":"] @punctuation.delimiter

["(" ")" "[" "]" "{" "}" ] @punctuation.bracket
