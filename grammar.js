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
  power: 21,
  call: 22,
  postfix: 23,
}

module.exports = grammar({
  name: 'matlab',
  extras: ($) => [/\n/, /\s/, $.comment],
  externals: ($) => [
    $.comment,
    $.command_name,
    $.command_argument,
    $.error_sentinel,
  ],
  conflicts: ($) => [[$._expression, $.assignment]],
  word: ($) => $.identifier,
  rules: {
    source_file: ($) => $._block,

    _block: ($) =>
      repeat1(
        seq(
          choice(field('comment', $.comment), $._statement, $._expression),
          optional($._end_of_line)
        )
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

    number: ($) =>
      choice(
        $._integer,
        $._float,
        $._float_alt,
        $._bin_number,
        $._hex_number,
        $._sci_number,
        $._complex_number
      ),
    _integer: ($) => /[+-]?\d+/,
    _float: ($) => /[+-]?\d+\.\d*/,
    _float_alt: ($) => /[+-]?\.\d+/,
    _bin_number: ($) => /0[bB][0-1]+/,
    _hex_number: ($) => /0[xX][\dA-Fa-f]+/,
    // token does no accept $. so we have to inline
    _sci_number: ($) =>
      token(
        seq(
          choice(/[+-]?\d+/, /[+-]?\d+\.\d*/, /[+-]?\.\d+/),
          /[eE][+-]?/,
          /\d+/
        )
      ),
    _complex_number: ($) =>
      token(
        seq(
          choice(
            /[+-]?\d+/,
            /[+-]?\d+\.\d*/,
            /[+-]?\.\d+/,
            seq(
              choice(/[+-]?\d+/, /[+-]?\d+\.\d*/, /[+-]?\.\d+/),
              /[eE][+-]?/,
              /\d+/
            )
          ),
          /[ij]/
        )
      ),

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
        $.identifier,
        $.matrix_definition,
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
              field('left', $._expression),
              field('operator', operator),
              field('right', $._expression)
            )
          )
        )
      )
    },

    unary_operator: ($) =>
      prec(
        PREC.unary,
        seq(
          field('operator', choice('+', '-', '~', '?')),
          field('argument', $._expression)
        )
      ),

    comparison_operator: ($) =>
      prec.left(
        PREC.compare,
        seq(
          $._expression,
          repeat1(
            seq(
              field('operators', choice('<', '<=', '==', '~=', '>=', '>')),
              $._expression
            )
          )
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
          field('argument', $._expression),
          field('operator', choice(".'", "'"))
        )
      ),

    string: ($) =>
      choice(seq('"', /([^"]|(""))*/, '"'), seq("'", /([^']|(''))*/, "'")),

    _expression_sequence: ($) =>
      repeat1(seq(field('argument', $._expression), optional(','))),
    row: ($) =>
      prec.right(
        seq($._expression_sequence, optional(choice(';', '\n', '\r')))
      ),
    matrix_definition: ($) => seq('[', repeat($.row), ']'),
    cell_definition: ($) => seq('{', repeat($.row), '}'),

    ignored_argument: ($) => '~',

    assignment: ($) =>
      choice(
        // A = B
        // A(1) = B
        // A{1} = B
        // A.b = B
        seq(
          field('variable', choice($.identifier, $.function_call, $.struct)),
          '=',
          field('value', $._expression)
        ),
        // [A, B, ~] = C
        seq(
          '[',
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
          ']',
          '=',
          field('value', $._expression)
        ),
      ),

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

    _range_element: ($) => choice($.identifier, $.number, $.function_call),
    range: ($) =>
      seq(
        $._range_element,
        ':',
        $._range_element,
        optional(seq(':', $._range_element))
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
        optional($.elseif_statement),
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
  },
})
