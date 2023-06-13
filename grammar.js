const PREC = {
  parenthesized_expression: 1,
  or: 10,
  and: 11,
  not: 12,
  compare: 13,
  bitwise_or: 14,
  bitwise_and: 15,
  xor: 16,
  shift: 17,
  plus: 18,
  times: 19,
  unary: 20,
  postfix: 21,
  power: 22,
  call: 23,
}

module.exports = grammar({
  name: 'matlab',
  extras: ($) => [/\n/, /\s/, $.comment],
  externals: ($) => [
    $.comment,
    $.command_name,
    $.command_argument,
    $.string_open,
    $.string_close,
    $.formatting_sequence,
    $.escape_sequence,
    $._string_text,
    $._multivar_open,
    $._entry_delimiter,
    $.error_sentinel,
  ],
  conflicts: ($) => [
    [$._expression, $._range_element],
    [$._range_element, $._binary_expression],
    [$.range],
    [$._expression, $.dimensions],
    [$._expression, $.validation_functions],
    [$._function_arguments, $.dimensions],
  ],
  word: ($) => $.identifier,
  rules: {
    source_file: ($) =>
      choice(
        seq($._block, repeat($.function_definition)),
        repeat1($.function_definition)
      ),

    _block: ($) =>
      repeat1(
        seq(choice($.comment, $._statement, $._expression), $._end_of_line)
      ),
    block: ($) => $._block,

    identifier: ($) => /[a-zA-Z_][a-zA-Z0-9_]*/,

    _end_of_line: ($) => choice(';', '\n', '\r', ','),

    _operator: ($) =>
      choice($._prefix_operator, $._infix_operator, $._postfix_operator),
    _prefix_operator: ($) => choice('~', '+', '-'),
    _infix_operator: ($) =>
      choice(
        '&',
        '|',
        '&&',
        '||',
        '==',
        '~=',
        '<',
        '>',
        '<=',
        '>=',
        '+',
        '-',
        '.*',
        '*',
        './',
        '/',
        '.\\',
        '\\',
        '.^',
        '^'
      ),
    _postfix_operator: ($) => choice("'", ".'"),

    boolean: ($) => choice('true', 'false'),

    number: ($) => /(\d+|\d+\.\d*|\.\d+)([eE][+-]?\d+)?[ij]?/,

    _statement: ($) =>
      choice(
        $._break_statement,
        $._continue_statement,
        $._return_statement,
        $.assignment,
        $.class_definition,
        $.command,
        $.for_statement,
        $.global_operator,
        $.if_statement,
        $.persistent_operator,
        $.switch_statement,
        $.try_statement,
        $.while_statement
      ),

    _expression: ($) =>
      choice(
        $.binary_operator,
        $.boolean,
        $.boolean_operator,
        $.cell_definition,
        $.comparison_operator,
        $.function_call,
        $.handle_operator,
        $.identifier,
        $.lambda,
        $.matrix_definition,
        $.metaclass_operator,
        $.not_operator,
        $.number,
        $.parenthesized_expression,
        $.postfix_operator,
        $.range,
        $.string,
        $.struct,
        $.unary_operator
      ),

    parenthesized_expression: ($) =>
      prec(PREC.parenthesized_expression, seq('(', $._expression, ')')),

    _binary_expression: ($) =>
      prec(
        1,
        choice(
          $.binary_operator,
          $.boolean,
          $.boolean_operator,
          $.cell_definition,
          $.comparison_operator,
          $.function_call,
          $.identifier,
          $.matrix_definition,
          $.not_operator,
          $.number,
          $.parenthesized_expression,
          $.postfix_operator,
          $.string,
          $.struct,
          $.unary_operator
        )
      ),
    binary_operator: ($) => {
      const table = [
        [prec.left, '+', PREC.plus],
        [prec.left, '.+', PREC.plus],
        [prec.left, '-', PREC.plus],
        [prec.left, '.-', PREC.plus],
        [prec.left, '*', PREC.times],
        [prec.left, '.*', PREC.times],
        [prec.left, '/', PREC.times],
        [prec.left, './', PREC.times],
        [prec.left, '\\', PREC.times],
        [prec.left, '.\\', PREC.times],
        [prec.right, '^', PREC.power],
        [prec.right, '.^', PREC.power],
        [prec.left, '|', PREC.bitwise_or],
        [prec.left, '&', PREC.bitwise_and],
      ]

      return choice(
        ...table.map(([fn, operator, precedence]) =>
          fn(
            precedence,
            seq(
              field('left', $._binary_expression),
              alias(operator, $.operator),
              field('right', $._binary_expression)
            )
          )
        )
      )
    },

    unary_operator: ($) =>
      prec(
        PREC.unary,
        seq(
          alias(choice('+', '-'), $.operator),
          field(
            'argument',
            choice(
              $.boolean,
              $.cell_definition,
              $.function_call,
              $.identifier,
              $.matrix_definition,
              $.not_operator,
              $.number,
              $.parenthesized_expression,
              $.postfix_operator,
              $.struct,
              $.unary_operator
            )
          )
        )
      ),

    not_operator: ($) =>
      prec(
        PREC.not,
        seq(
          alias('~', $.operator),
          choice(
            $.boolean,
            $.function_call,
            $.identifier,
            $.matrix_definition,
            $.not_operator,
            $.number,
            $.parenthesized_expression,
            $.postfix_operator,
            $.struct,
            $.unary_operator
          )
        )
      ),

    metaclass_operator: ($) => seq(alias('?', $.operator), $.identifier),
    handle_operator: ($) => seq(alias('@', $.operator), $.identifier),

    comparison_operator: ($) =>
      prec.left(
        PREC.compare,
        seq(
          $._expression,
          alias(choice('<', '<=', '==', '~=', '>=', '>'), $.operator),
          $._expression
        )
      ),

    boolean_operator: ($) =>
      choice(
        prec.left(
          PREC.and,
          seq(
            field('left', $._expression),
            alias('&&', $.operator),
            field('right', $._expression)
          )
        ),
        prec.left(
          PREC.or,
          seq(
            field('left', $._expression),
            alias('||', $.operator),
            field('right', $._expression)
          )
        )
      ),

    postfix_operator: ($) =>
      prec(
        PREC.postfix,
        seq(
          field(
            'argument',
            choice(
              $.binary_operator,
              $.boolean,
              $.cell_definition,
              $.function_call,
              $.identifier,
              $.matrix_definition,
              $.number,
              $.parenthesized_expression,
              $.postfix_operator,
              $.string,
              $.struct,
              $.unary_operator
            )
          ),
          alias(choice(".'", "'"), $.operator)
        )
      ),

    // Right now the scanner can identify and properly tag escape sequences and
    // formatting options inside strings, however I cannot enable it for single
    // quote strings because it conflicts with the transpose postfix operator.
    // The best solution would be to use the commented version below, but it
    // depends on tree-sitter to offer some function which allows to not have
    // extras inside a rule, as it is wrongly matching the comment right now.
    string: ($) =>
      choice(
        seq(
          $.string_open,
          seq(
            repeat(
              choice($.formatting_sequence, $.escape_sequence, $._string_text)
            )
          ),
          $.string_close
        ),
        seq(/'([^'\n\r]|(''))*'/)
      ),
    // escape_sequence: ($) =>
    //   token.immediate(
    //     seq('\\', choice(/x[a-fA-F\d]+/, /[0-7]+/, /[abfnrtv\\]/))
    //   ),
    // formatting_sequence: ($) =>
    //   token.immediate(
    //     seq('%', choice('%', /\d*[-+ 0#]?\d*(\.\d+)?[bt]?[cdeEfgGosuxX]/))
    //   ),
    // string: ($) =>
    //   choice(
    //     seq(
    //       alias('"', $.string_start),
    //       alias(
    //         repeat(
    //           choice(/([^"]|(""))/, $.escape_sequence, $.formatting_sequence)
    //         ),
    //         $.string_content
    //       ),
    //       alias('"', $.string_end)
    //     ),
    //     seq(
    //       alias("'", $.string_start),
    //       alias(
    //         repeat(
    //           choice(/([^']|(''))/, $.escape_sequence, $.formatting_sequence)
    //         ),
    //         $.string_content
    //       ),
    //       alias("'", $.string_end)
    //     )
    //   ),

    _entry: ($) => field('argument', $._expression),
    row: ($) => seq($._entry, repeat(seq($._entry_delimiter, $._entry))),
    matrix_definition: ($) =>
      seq('[', optional($.row), repeat(seq(/[;\n\r]/, optional($.row))), ']'),
    cell_definition: ($) =>
      seq('{', optional($.row), repeat(seq(/[;\n\r]/, optional($.row))), '}'),

    ignored_argument: ($) => '~',

    // A = B
    // A(1) = B
    // A{1} = B
    // A.b = B
    _variable_assignment: ($) =>
      seq(
        field('variable', choice($.identifier, $.function_call, $.struct)),
        '=',
        field('value', $._expression)
      ),
    // [A, B, ~] = C
    multioutput_variable: ($) =>
      seq(
        $._multivar_open,
        field(
          'variable',
          repeat1(
            seq(
              field(
                'argument',
                choice(
                  $.identifier,
                  $.ignored_argument,
                  $.struct,
                  $.function_call
                )
              ),
              optional(',')
            )
          )
        ),
        ']'
      ),
    _multioutput_assignment: ($) =>
      seq($.multioutput_variable, '=', field('value', $._expression)),
    assignment: ($) =>
      prec.right(choice($._variable_assignment, $._multioutput_assignment)),

    spread_operator: ($) => ':',

    _function_arguments: ($) =>
      seq(
        field('argument', choice($.spread_operator, $._expression)),
        optional(
          repeat(
            seq(
              ',',
              field('argument', choice($.spread_operator, $._expression))
            )
          )
        )
      ),
    _args: ($) =>
      choice(
        seq(
          alias('(', $.func_call_paren),
          field(
            'arguments',
            alias(optional($._function_arguments), $.arguments)
          ),
          alias(')', $.func_call_paren)
        ),
        seq(
          alias('{', $.func_call_paren),
          field(
            'arguments',
            alias(optional($._function_arguments), $.arguments)
          ),
          alias('}', $.func_call_paren)
        )
      ),
    function_call: ($) =>
      prec.right(
        PREC.call,
        seq(
          seq(
            field('name', choice($.identifier, $.function_call)),
            optional(
              seq('@', field('superclass', alias($.identifier, $.superclass)))
            )
          ),
          $._args
        )
      ),

    command: ($) =>
      prec.right(
        seq(
          field('name', $.command_name),
          repeat(choice(field('argument', $.command_argument), $.comment))
        )
      ),

    // Unary operators cannot bind stronger in this case, lest the world falls apart.
    _range_element: ($) =>
      choice(
        prec.dynamic(1, $.binary_operator),
        $.boolean,
        $.function_call,
        $.identifier,
        $.matrix_definition,
        $.not_operator,
        $.number,
        $.parenthesized_expression,
        $.postfix_operator,
        $.struct,
        prec.dynamic(-1, $.unary_operator)
      ),
    range: ($) =>
      prec.right(
        PREC.postfix,
        seq(
          $._range_element,
          ':',
          $._range_element,
          optional(seq(':', $._range_element))
        )
      ),

    _end: ($) => field('end', alias('end', $.keyword)),
    _return_statement: ($) => field('return', alias('return', $.keyword)),
    _continue_statement: ($) => field('continue', alias('continue', $.keyword)),
    _break_statement: ($) => field('break', alias('break', $.keyword)),

    elseif_statement: ($) =>
      seq(
        field('elseif', alias('elseif', $.keyword)),
        alias($._expression, $.condition),
        $._end_of_line,
        optional($.block)
      ),
    else_statement: ($) =>
      seq(field('else', alias('else', $.keyword)), optional($.block)),
    if_statement: ($) =>
      seq(
        field('if', alias('if', $.keyword)),
        field('argument', alias($._expression, $.condition)),
        $._end_of_line,
        optional($.block),
        repeat($.elseif_statement),
        optional($.else_statement),
        $._end
      ),

    iterator: ($) => seq($.identifier, '=', $._expression),
    parfor_options: ($) =>
      choice($.number, $.identifier, $.function_call, $.string),
    for_statement: ($) =>
      choice(
        seq(
          choice(
            field('for', alias('for', $.keyword)),
            field('parfor', alias('parfor', $.keyword))
          ),
          field('argument', $.iterator),
          $._end_of_line,
          optional($.block),
          $._end
        ),
        seq(
          field('parfor', alias('parfor', $.keyword)),
          '(',
          field('argument', $.iterator),
          ',',
          field('argument', $.parfor_options),
          ')',
          $._end_of_line,
          optional($.block),
          $._end
        )
      ),

    while_statement: ($) =>
      seq(
        field('while', alias('while', $.keyword)),
        field('argument', alias($._expression, $.condition)),
        $._end_of_line,
        optional($.block),
        $._end
      ),

    case: ($) =>
      seq(
        field('case', alias('case', $.keyword)),
        // MATLAB says it should be a `switch_expr`, but then accepts any expression
        alias($._expression, $.condition),
        optional($.block)
      ),
    otherwise: ($) =>
      seq(field('otherwise', alias('otherwise', $.keyword)), optional($.block)),
    switch_statement: ($) =>
      seq(
        field('switch', alias('switch', $.keyword)),
        field('argument', alias($._expression, $.condition)),
        repeat($.case),
        optional($.otherwise),
        $._end
      ),

    _struct_element: ($) => choice($.function_call, $.identifier),
    struct: ($) =>
      seq(
        repeat1(seq($._struct_element, choice('.', '.?'))),
        $._struct_element
      ),

    _lambda_arguments: ($) =>
      seq(
        field('argument', choice($.ignored_argument, $.identifier)),
        optional(
          repeat(
            seq(
              ',',
              field('argument', choice($.ignored_argument, $.identifier))
            )
          )
        )
      ),
    lambda: ($) =>
      seq(
        alias('@', $.operator),
        '(',
        field('arguments', alias(optional($._lambda_arguments), $.arguments)),
        ')',
        field('expression', $._expression)
      ),

    global_operator: ($) =>
      seq(
        alias('global', $.keyword),
        field('arguments', repeat(field('argument', $.identifier)))
      ),

    persistent_operator: ($) =>
      seq(
        alias('persistent', $.keyword),
        field('arguments', repeat(field('argument', $.identifier)))
      ),

    _argument_attributes: ($) =>
      seq(
        '(',
        field('argument', $.identifier),
        repeat(seq(',', field('argument', $.identifier))),
        ')'
      ),
    arguments_statement: ($) =>
      seq(
        alias('arguments', $.keyword),
        optional(alias($._argument_attributes, $.attributes)),
        $._end_of_line,
        repeat($.property),
        $._end
      ),

    end_function: ($) =>
      field('end', alias(choice('end', 'endfunction'), $.keyword)),
    function_output: ($) =>
      seq(field('output', choice($.identifier, $.multioutput_variable)), '='),
    function_arguments: ($) =>
      seq('(', field('arguments', optional($._lambda_arguments)), ')'),
    function_definition: ($) =>
      seq(
        alias('function', $.keyword),
        optional($.function_output),
        field(
          'function_name',
          seq(
            optional(seq(alias(choice('get', 'set'), $.keyword), '.')),
            $.identifier
          )
        ),
        optional($.function_arguments),
        $._end_of_line,
        repeat($.arguments_statement),
        $.block,
        optional($.end_function)
      ),
    _function_definition_with_end: ($) =>
      seq(
        alias('function', $.keyword),
        optional($.function_output),
        field(
          'function_name',
          seq(
            optional(seq(alias(choice('get', 'set'), $.keyword), '.')),
            $.identifier
          )
        ),
        optional($.function_arguments),
        $._end_of_line,
        repeat($.arguments_statement),
        $.block,
        $.end_function
      ),

    attribute: ($) => seq($.identifier, optional(seq('=', $._expression))),
    attributes: ($) =>
      seq(
        '(',
        field('argument', $.attribute),
        repeat(seq(',', field('argument', $.attribute))),
        ')'
      ),
    superclasses: ($) =>
      seq(
        '<',
        field('argument', $.identifier),
        repeat(seq('&', field('argument', $.identifier)))
      ),
    dimensions: ($) =>
      seq(
        '(',
        choice($.number, $.spread_operator),
        repeat(seq(',', choice($.number, $.spread_operator))),
        ')'
      ),
    validation_functions: ($) =>
      seq('{', $.identifier, repeat(seq(',', $.identifier)), '}'),
    default_value: ($) => seq('=', field('argument', $._expression)),
    property_name: ($) => seq($.identifier, repeat(seq('.', $.identifier))),
    property: ($) =>
      seq(
        field('argument', $.property_name),
        optional(field('argument', $.dimensions)),
        optional(
          field('argument', alias(choice($.identifier, $.struct), $.class))
        ),
        optional(field('argument', $.validation_functions)),
        optional($.default_value),
        $._end_of_line
      ),
    properties: ($) =>
      seq(
        field('properties', alias('properties', $.keyword)),
        optional($.attributes),
        $._end_of_line,
        repeat($.property),
        $._end
      ),
    function_signature: ($) =>
      seq(
        optional($.function_output),
        field(
          'function_name',
          seq(
            optional(seq(alias(choice('get', 'set'), $.keyword), '.')),
            $.identifier
          )
        ),
        optional($.function_arguments),
        $._end_of_line
      ),
    methods: ($) =>
      seq(
        field('methods', alias('methods', $.keyword)),
        optional($.attributes),
        $._end_of_line,
        repeat(
          choice(
            $.function_signature,
            alias($._function_definition_with_end, $.function_definition)
          )
        ),
        $._end
      ),
    events: ($) =>
      seq(
        field('events', alias('events', $.keyword)),
        optional($.attributes),
        $._end_of_line,
        repeat(seq(field('argument', $.identifier), $._end_of_line)),
        $._end
      ),
    _enum_value: ($) =>
      choice(
        $.boolean,
        $.cell_definition,
        $.function_call,
        $.identifier,
        $.matrix_definition,
        $.not_operator,
        $.number,
        $.postfix_operator,
        $.struct,
        $.unary_operator
      ),
    enum: ($) =>
      seq(
        field('argument', $.identifier),
        optional(
          seq(
            '(',
            field('argument', alias($._enum_value, $.default_value)),
            repeat(
              seq(',', field('argument', alias($._enum_value, $.default_value)))
            ),
            ')'
          )
        )
      ),
    enumeration: ($) =>
      seq(
        field('enumeration', alias('enumeration', $.keyword)),
        optional($.attributes),
        $._end_of_line,
        repeat(seq($.enum, $._end_of_line)),
        $._end
      ),
    class_definition: ($) =>
      seq(
        field('classdef', alias('classdef', $.keyword)),
        optional($.attributes),
        field('class_name', $.identifier),
        optional($.superclasses),
        $._end_of_line,
        repeat(choice($.properties, $.methods, $.events, $.enumeration)),
        $._end
      ),

    catch: ($) =>
      seq(
        field('catch', alias('catch', $.keyword)),
        optional(alias($.identifier, $.captured_exception)),
        $._end_of_line,
        optional($.block)
      ),
    try_statement: ($) =>
      seq(
        field('try', alias('try', $.keyword)),
        $._end_of_line,
        optional($.block),
        optional($.catch),
        $._end
      ),
  },
})
