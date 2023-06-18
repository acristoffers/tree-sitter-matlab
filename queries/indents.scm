"end" @indent.end @indent.branch

[
  (if_statement)
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

[
  "elseif"
  "else"
  "case"
  "otherwise"
  "catch"
] @indent.branch

((matrix (row) @indent.align)
 (#set! indent.open_delimiter "[")
 (#set! indent.close_delimiter "]"))
((cell_definition (row) @indent.align)
 (#set! indent.open_delimiter "{")
 (#set! indent.close_delimiter "}"))
((parenthesized_expression) @indent.align
 (#set! indent.open_delimiter "(")
 (#set! indent.close_delimiter ")"))

(comment) @indent.auto
