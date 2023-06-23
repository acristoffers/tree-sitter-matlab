; Errors

(ERROR) @error

; Constants

(events (identifier) @constant)
(attribute (identifier) @constant)

"~" @constant.builtin

; Fields/Properties

(superclass "." (identifier) @property)

(property_name "." (identifier) @property)

(property name: (identifier) @property)

; Types

(class_definition name: (identifier) @type)

(attributes (identifier) @constant)

(enum . (identifier) @type)

; Functions

(function_definition
  "function" @keyword
  name: (identifier) @function
  [ "end" "endfunction" ]? @keyword)

(function_signature name: (identifier) @function)

(function_call
  name: (identifier) @function.call)

(handle_operator (identifier) @function)

(validation_functions (identifier) @function)

(command (command_name) @function.call)
(command_argument) @string

(return_statement) @keyword

; Assignments

(assignment left: (_) @variable)
(multioutput_variable (_) @variable)

; Parameters

(function_arguments (identifier) @variable.parameter)

; Operators

[
  "+"
  ".+"
  "-"
  ".*"
  "*"
  ".*"
  "/"
  "./"
  "\\"
  ".\\"
  "^"
  ".^"
  "'"
  ".'"
  "|"
  "&"
  "?"
  "@"
  "<"
  "<="
  ">"
  ">="
  "=="
  "~="
  "="
  "&&"
  "||"
  ":"
] @operator

; Conditionals

(if_statement [ "if" "end" ] @keyword)
(elseif_clause "elseif" @keyword)
(else_clause "else" @keyword)
(switch_statement [ "switch" "end" ] @keyword)
(case_clause "case" @keyword)
(otherwise_clause "otherwise" @keyword)
(break_statement) @keyword

; Repeats

(for_statement [ "for" "parfor" "end" ] @keyword)
(while_statement [ "while" "end" ] @keyword)
(continue_statement) @keyword

; Exceptions

(try_statement [ "try" "end" ] @keyword)
(catch_clause "catch" @keyword)

; Punctuation

[ ";" "," "." ] @punctuation.delimiter

[ "(" ")" "[" "]" "{" "}" ] @punctuation.bracket

; Literals

(escape_sequence) @escape
(formatting_sequence) @escape
(string) @string
(number) @number
(boolean) @keyword.builtin

; Comments

[ (comment) (line_continuation) ] @comment @spell

; Keywords

[
  "arguments"
  "classdef"
  "end"
  "enumeration"
  "events"
  "global"
  "methods"
  "persistent"
  "properties"
] @keyword
