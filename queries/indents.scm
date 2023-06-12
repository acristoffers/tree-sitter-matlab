((keyword) @indent.end @indent.branch
  (#eq? @indent.end "end")
  (#eq? @indent.branch "end"))

[(if_statement)
 (for_statement)
 (while_statement)
 (switch_statement)
 (try_statement)
 (function_definition)
 (class_definition)
 (enumeration)
 (events)
 (methods)
 (properties)
 ] @indent.begin

[(elseif_statement)
 (else_statement)
 (case)
 (otherwise)
 (catch)
 ] @indent.branch

((matrix_definition (row) @indent.align)
 (#set! indent.open_delimiter "[")
 (#set! indent.close_delimiter "]"))
((cell_definition (row) @indent.align)
 (#set! indent.open_delimiter "{")
 (#set! indent.close_delimiter "}"))
((parenthesized_expression) @indent.align
 (#set! indent.open_delimiter "(")
 (#set! indent.close_delimiter ")"))

(comment) @indent.auto
