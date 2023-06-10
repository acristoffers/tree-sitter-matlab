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
    $.error_sentinel,
  ],
  conflicts: ($) => [
    [$._expression, $._range_element],
    [$._expression, $._binary_expression],
    [$._range_element, $._binary_expression],
    [$.range],
  ],
  word: ($) => $.identifier,
  rules: {
    source_file: ($) => $._block,

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
        $.command,
        $.for_statement,
        $.if_statement,
        $.switch_statement,
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
              field('operator', operator),
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
          field('operator', choice('+', '-')),
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
          '~',
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

    metaclass_operator: ($) => seq('?', $.identifier),
    handle_operator: ($) => seq('@', $.identifier),

    comparison_operator: ($) =>
      prec.left(
        PREC.compare,
        seq(
          $._expression,
          field('operators', choice('<', '<=', '==', '~=', '>=', '>')),
          $._expression
        )
      ),

    boolean_operator: ($) =>
      choice(
        prec.left(
          PREC.and,
          seq(
            field('left', $._expression),
            field('operator', '&&'),
            field('right', $._expression)
          )
        ),
        prec.left(
          PREC.or,
          seq(
            field('left', $._expression),
            field('operator', '||'),
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
          field('operator', choice(".'", "'"))
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
        seq(
          alias("'", $.string_open),
          /([^']|(''))*/,
          alias("'", $.string_close)
        )
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

    _expression_sequence: ($) =>
      repeat1(seq(field('argument', $._expression), optional(','))),
    row: ($) =>
      prec.right(
        seq($._expression_sequence, optional(choice(';', '\n', '\r')))
      ),
    matrix_definition: ($) => seq('[', repeat($.row), ']'),
    cell_definition: ($) => seq('{', repeat($.row), '}'),

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
          field('arguments', optional($._function_arguments)),
          alias(')', $.func_call_paren)
        ),
        seq(
          alias('{', $.func_call_paren),
          field('arguments', optional($._function_arguments)),
          alias('}', $.func_call_paren)
        )
      ),
    function_call: ($) =>
      prec.right(PREC.call, seq(field('name', $.identifier), $._args)),

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
          field(
            'argument',
            choice($.number, $.identifier, $.function_call, $.string)
          ),
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

    switch_statement: ($) =>
      seq(
        field('switch', alias('switch', $.keyword)),
        field('argument', alias($._expression, $.condition)),
        repeat(
          seq(
            field('case', alias('case', $.keyword)),
            // MATLAB says it should be a `switch_expr`, but then accepts any expression
            alias($._expression, $.condition),
            optional($.block)
          )
        ),
        optional(
          seq(
            field('otherwise', alias('otherwise', $.keyword)),
            optional($.block)
          )
        ),
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
        field('argument', choice($.ignored_argument, $._expression)),
        optional(
          repeat(
            seq(
              ',',
              field('argument', choice($.ignored_argument, $._expression))
            )
          )
        )
      ),
    lambda: ($) =>
      seq(
        '@',
        '(',
        field('arguments', alias(optional($._lambda_arguments), $.arguments)),
        ')',
        $._expression
      ),
  },
})
