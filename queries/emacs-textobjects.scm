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
((arguments ","? @parameter.outer._start . (_) @parameter.outer._end @parameter.inner . ))
((arguments (_) @parameter.outer._start @parameter.inner . "," @parameter.outer._end))

(command) @call.outer
(command (command_argument) @parameter.inner @parameter.outer)
(command
  (command_argument) @call.inner._start (command_argument)* @call.inner._end .)

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

((multioutput_variable ","? @parameter.outer._start . (_) @parameter.outer._end @parameter.inner . ))
((multioutput_variable (_) @parameter.inner @parameter.outer._start . "," @parameter.outer._end))

((function_arguments ","? @parameter.outer._start . (_) @parameter.inner._end @parameter.inner . ))
((function_arguments (_) @parameter.outer._start @parameter.inner . "," @parameter.outer._end))

(try_statement
  (block) @conditional.inner) @conditional.outer
(catch
  (captured_exception) @parameter.inner @parameter.outer)
(catch
  (block) @conditional.inner)

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

((superclasses "&"? @_start . (_) @parameter.inner . )
 (#make-range! "parameter.outer" @_start @parameter.inner))
((superclasses (_) @parameter.inner . "&" @_end)
 (#make-range! "parameter.outer" @parameter.inner @_end))

(enum argument: (identifier) @parameter.inner @parameter.outer)

(property argument: (_) @parameter.outer @parameter.inner)

((enum ","? @_start . (_) @parameter.inner . )
 (#make-range! "parameter.outer" @_start @parameter.inner))
((enum (_) @parameter.inner . "," @_end)
 (#make-range! "parameter.outer" @parameter.inner @_end))

((validation_functions ","? @_start . (_) @parameter.inner . )
 (#make-range! "parameter.outer" @_start @parameter.inner))
((validation_functions (_) @parameter.inner . "," @_end)
 (#make-range! "parameter.outer" @parameter.inner @_end))

((dimensions ","? @_start . (_) @parameter.inner . )
 (#make-range! "parameter.outer" @_start @parameter.inner))
((dimensions (_) @parameter.inner . "," @_end)
 (#make-range! "parameter.outer" @parameter.inner @_end))

((attributes ","? @_start . (_) @parameter.inner . )
 (#make-range! "parameter.outer" @_start @parameter.inner))
((attributes (_) @parameter.inner . "," @_end)
 (#make-range! "parameter.outer" @parameter.inner @_end))
