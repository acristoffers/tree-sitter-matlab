(function_definition
  (block (_) @context.end)
) @context

(while_statement
  (block (_) @context.end)
) @context

(for_statement
  (block (_) @context.end)
) @context

(if_statement
  (block (_) @context.end)
) @context

(elseif_statement
  (block (_) @context.end)
) @context

(else_statement
  (block (_) @context.end)
) @context

(switch_statement) @context

(case
  (block (_) @context.end)
) @context

(otherwise_clause
  (block (_) @context.end)
) @context

(try_statement
  "try"
  (block (_) @context.end) @context
  "end")
(catch
  "catch"
  (block (_) @context.end) @context)
