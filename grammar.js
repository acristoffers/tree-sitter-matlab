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

    _statement: ($) => choice($.assignment, $.command, $.if_statement),

    _expression: ($) =>
      choice(
        $.binary_operator,
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
        seq(
          field('variable', $.identifier),
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
                field('argument', choice($.identifier, $.ignored_argument)),
                optional(',')
              )
            )
          ),
          ']',
          '=',
          field('value', $._expression)
        ),
        // A(1) = B
        seq(
          field('variable', $.function_call),
          '=',
          field('value', $._expression)
        )
      ),

    _function_arguments: ($) =>
      seq(
        field('argument', $._expression),
        optional(repeat(seq(',', field('argument', $._expression))))
      ),
    _args: ($) =>
      seq(
        alias('(', $.func_call_paren),
        field('arguments', optional($._function_arguments)),
        alias(')', $.func_call_paren)
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

    end: ($) => field('end', 'end'),

    elseif_statement: ($) =>
      seq('elseif', alias($._expression, $.condition), optional($.block)),
    else_statement: ($) => seq('else', optional($.block)),
    if_statement: ($) =>
      seq(
        'if',
        alias($._expression, $.condition),
        optional($.block),
        optional($.elseif_statement),
        optional($.else_statement),
        $.end
      ),
  },
})
