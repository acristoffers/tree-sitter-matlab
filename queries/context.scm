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

(otherwise
  (block (_) @context.end)
) @context

(try_statement
  try: (keyword)
  (block (_) @context.end) @context
  catch: (keyword)
  (block (_) @context.end) @context
  end: (keyword))
