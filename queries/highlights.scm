; highlights.scm

(string) @string @spell
(formatting_sequence) @string.special
(escape_sequence) @string.escape

(number) @number
(boolean) @boolean
(comment) @comment @spell
(operator) @operator
(keyword) @keyword
(ERROR) @error

[";" "," "." ":"] @punctuation.delimiter
["(" ")" "[" "]" "{" "}" ] @punctuation.bracket

(unary_operator
  (operator) @number
  (number))

(metaclass_operator (identifier) @variable)
(handle_operator (identifier) @function)

(assignment variable: (_) @variable)
(multioutput_variable (_) @variable)

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

(range ":" @operator)

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

(arguments_statement (_) @variable)
(global_operator (identifier) @variable)
(persistent_operator (identifier) @variable)

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

(enum argument: (identifier) @constant)
(events (identifier) @constant)
(validation_functions (identifier) @variable)
(attribute (identifier) @constant)

(function_signature function_name: (identifier) @function)

(property
  (property_name) @constant
  (class)? @type)

((keyword) @keyword.return
  (#eq? @keyword.return "return"))
