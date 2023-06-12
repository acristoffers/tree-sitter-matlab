; @assignment.inner
; @assignment.lhs
; @assignment.outer
; @assignment.rhs
; @attribute.inner
; @attribute.outer
; @block.inner
; @block.outer
; @call.inner
; @call.outer
; @class.inner
; @class.outer
; @comment.inner
; @comment.outer
; @conditional.inner
; @conditional.outer
; @frame.inner
; @frame.outer
; @function.inner
; @function.outer
; @loop.inner
; @loop.outer
; @number.inner
; @parameter.inner
; @parameter.outer
; @regex.inner
; @regex.outer
; @return.inner
; @return.outer
; @scopename.inner
; @statement.outer

(_ (block) @block.inner) @block.outer
(block (_) @statement.outer)
(source_file (_) @statement.outer)

(function_call
  (arguments)? @call.inner) @call.outer
((arguments ","? @_start . (_) @parameter.inner . )
 (#make-range! "parameter.outer" @_start @parameter.inner))
((arguments (_) @parameter.inner . "," @_end)
 (#make-range! "parameter.outer" @parameter.inner @_end))

(command) @call.outer
(command (command_argument) @parameter.inner @parameter.outer)
(command
  (command_argument) @_start (command_argument)* @_end .
  (#make-range! "call.inner" @_start @_end))

(if_statement
  (block) @conditional.inner) @conditional.outer
(if_statement
  (elseif_statement
    (block) @conditional.inner))
(if_statement
  (else_statement
    (block) @conditional.inner))

(switch_statement
  (case (block) @conditional.inner)) @conditional.outer

(switch_statement
  (otherwise (block) @conditional.inner))

(for_statement
  (block) @loop.inner) @loop.outer
(while_statement
  (block) @loop.inner) @loop.outer

(lambda
  expression: (_) @function.inner) @function.outer

(global_operator
  (identifier) @parameter.inner)

(persistent_operator
  (identifier) @parameter.inner)

(function_definition
  (block) @function.inner) @function.outer

(function_output (identifier) @parameter.inner @parameter.outer)

((multioutput_variable ","? @_start . (_) @parameter.inner . )
 (#make-range! "parameter.outer" @_start @parameter.inner))
((multioutput_variable (_) @parameter.inner . "," @_end)
 (#make-range! "parameter.outer" @parameter.inner @_end))

((function_arguments ","? @_start . (_) @parameter.inner . )
 (#make-range! "parameter.outer" @_start @parameter.inner))
((function_arguments (_) @parameter.inner . "," @_end)
 (#make-range! "parameter.outer" @parameter.inner @_end))

(try_statement
  (block) @conditional.inner) @conditional.outer
(try_statement
  (captured_exception) @parameter.inner @parameter.outer)
(try_statement
  (block) @conditional.inner . )

(class_definition) @class.outer

(number) @number.inner
(_ return: (keyword) @return.inner @return.outer)
(comment) @comment.outer

(matrix_definition (row) @parameter.outer)
(row (_) @parameter.inner)

(matrix_definition (row) @parameter.outer)
(cell_definition (row) @parameter.outer)
(row (_) @parameter.inner)

(assignment
  variable: (_) @assignment.lhs
  (_) @assignment.rhs) @assignment.outer
(assignment
  (multioutput_variable) @assignment.lhs
  (_) @assignment.rhs) @assignment.outer
