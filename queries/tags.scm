 (class_definition
   class_name: (identifier) @name) @definition.class

(function_definition
  function_name: (identifier) @name) @definition.function

(function_call
  name: (identifier) @name) @reference.call

(command
  name: (command_name) @name) @reference.call
