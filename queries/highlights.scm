; highlights.scm

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
  (operator) @operator
  (identifier) @variable)
(handle_operator
  (operator) @operator
  (identifier) @function)

(assignment
  variable: (_) @variable)
(assignment
  (multioutput_variable
    (_) @variable))

(struct "." @operator)
(struct . [(function_call
             name: (identifier) @variable)
           (identifier) @variable])
(struct
  [(function_call
     name: (identifier) @field)
   (identifier) @field])

(function_call
  name: (identifier) @function.call
  ("@" @operator (superclass) @type)?)

(command
  (command_name) @function.call
  (command_argument)* @text.literal)

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
  (identifier)* @variable
  ("," (identifier) @variable)*)

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
  enumeration: (keyword) @keyword
  (enum
    argument: (identifier) @constant)
  end: (keyword) @keyword)

(events
  events: (keyword) @keyword
  (identifier) @constant
  end: (keyword) @keyword)

(methods
  methods: (keyword) @keyword
  end: (keyword) @keyword)

(function_signature
  (function_output)?
  function_name: (identifier) @function
  (function_arguments)?)

(properties
  properties: (keyword) @keyword
  end: (keyword) @keyword)

(property
  (property_name) @constant
  (class)? @type)

(validation_functions
  (identifier) @variable)

(attributes
  (identifier) @constant)

((keyword) @keyword.return
  (#eq? @keyword.return "return"))
(keyword) @keyword

(string) @string @spell
(formatting_sequence) @string.special
(escape_sequence) @string.escape

(number) @number
(boolean) @boolean
(comment) @comment @spell
(ERROR) @error

[";" "," "." ":"] @punctuation.delimiter
["(" ")" "[" "]" "{" "}" ] @punctuation.bracket
