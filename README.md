# MATLAB grammar for tree-sitter.

There are screenshots at the end of this README :)

This parser has the objective of generating a tree that is as correct as
possible (but sometimes just convenient) with what MATLAB itself executes. It
is not intended only for syntax highlight, but also to be used by scripts to
whatever it may be needed. In fact, I wrote it because I'm a Neovim/Doom Emacs
user and love having text-objects, and was really missing a text object for
matrices rows/cells.

Being as correct as possible means that some things are done correctly, for
example:

- Commands are parsed the same way MATLAB does it, by treating arguments as
literals, grouping them correctly and only starting comments when allowed. It
should perfectly match what MATLAB does.

- Assignment has its own token, and multiple-variable assignment is NOT an
assignment to a matrix (and returning an error is the correct thing to do, as
it allows the user to see that something is off with the highlight, meaning
something is probably off with the code):

```matlab
% (assignment (multioutput_variable (identifier) (identifier)) (identifier))
[a,b] = d

% this is WRONG:
[a;b] = d
```

- Inside a matrix, `1 + 1` and `1 +1` are different things:

```matlab
a = 1 + 1 % 2
a = 1 +1 %2
[1 + 1] == [2]
[1 +1]  == [1 1]
```

Being convenient means that sometimes the difference between what is acceptable
and what is not acceptable lives in the semantics, so we can't know. In such
cases I just accept semantically wrong but syntax correct things and group them
in the same token (first example). I do the same when the overhead of
generating a specific token would not really pay off (second example).

- Function calls and Matrix Indexing are the same in MATLAB: `A(1)` can be any
of them and you cannot tell them apart unless you know for sure what `A` is
referring to. So for convenience I just generate a `function_call` for them and
also for cell indexing `A{1}`. The "problem" with that is that this is a valid
indexing but an invalid function call: `A(:)`. However I don't distinguish at
all and say that all of them are `function_call`.

- Function definitions, when inside a class, accepts a special syntax for the
name of the function, allowing it to be preceded by either `get.` or `set.`,
like `function get.name()`. I could have a `method_definition` that would allow
that to only be valid in the class context, but I doubt that would be worth it.
So any function anywhere can have those and be recognize as correct still.
Given the existence of external method definition, maybe that is even the
correct thing to do, since we don't know if the current file is inside a
special class folder.

# Known Limitations

While this parser aims to be as correct as possible, there are some inherent
limitations in MATLAB's syntax that make certain constructs difficult or
impossible to parse correctly. These are documented here based on discussions in
closed issues:

## Context-Sensitive Parsing Issues

### String vs. Transpose Operator Ambiguity (Issue #51, #113)

The single quote `'` can be either a string delimiter or the transpose operator.
This creates ambiguity in certain contexts:

```matlab
% This can fail to parse correctly:
m1 = [a.'; b.'];

% In if-statement conditions:
if a' < 'a'  % ambiguous: is a' an identifier+transpose or start of string?
```

**Workaround**: Use explicit `.'` transpose syntax or avoid these patterns.

### Dynamic Field Reference with Expressions (Issue #151, #152)

Dynamic field reference syntax `obj.(expr)` fails when the expression contains
certain operators due to precedence issues:

```matlab
T.(~a + 1)  % fails
T.(1 + ~a)  % works
```

## Edge Case Patterns

### Nested Multiline Comments (Issue #136)

MATLAB allows nested block comments, but the parser doesn't support this:

```matlab
%{
  code
  %{ nested comment %}
  more code
%}
```

### Matrix/Cell Delimiter Edge Cases (Issue #129)

MATLAB accepts matrices and cells with unusual delimiter patterns that the
parser doesn't handle:

```matlab
[;,; 1, 2]
{1,;,;;,2}
```

**Impact**: Low—affects only edge cases with unnecessary delimiters.

### Function on Same Line as Methods Keyword (Issue #150)

Placing a function definition on the same line as `methods` causes parse errors:

```matlab
classdef myClass
  methods function b = foo(a)  % fails
```

**Workaround**: Place the function on the next line.

### Ellipsis Between Class Definition Blocks (Issue #134)

Line continuation (`...`) between class blocks (properties/methods/events) is
valid MATLAB but causes parser errors:

```matlab
properties
end
...  % causes error
methods
```

## Structural Issues

### Comments in Class Properties Block (Issue #137, #138, #139)

Comments between class properties can cause parse errors or incorrect scope
attribution. Trailing newlines after statements may be included in the parent
node's scope, affecting tools that rely on accurate node ranges.

### Catch Exception Identifier with Comment (Issue #148)

`catch` clause identifier followed by a comment (with a space) causes the
identifier to be parsed incorrectly:

```matlab
catch err % comment  % fails
catch err% comment   % works (no space)
```

### Line Continuation with Missing Commas (Issue #118)

Line continuations in matrices/cells without commas are not parsed correctly:

```matlab
[a() ...  % needs comma before continuation
 b()]
```

**Workaround**: Add explicit commas: `[a(), ... b()]`

### Function Handles in Cells with Line Continuations (Issue #124)

Function handles (anonymous functions) in cells fail to parse correctly when
combined with line continuations.

### Uppercase Integer Size Specifiers (Issue #133)

Uppercase size specifiers in binary/hex literals cause parse errors, despite
MATLAB accepting them:

```matlab
0b100000s16  % works
0B100000S16  % fails
```

### Enumeration Keyword Without Space (Issue #149)

`enumeration...` (no space before `...`) is parsed as an identifier instead of
keyword + line continuation.

**Workaround**: Add a space: `enumeration ...`

Most users can work around these limitations by adjusting code formatting
slightly or avoiding these uncommon patterns. These edge cases are documented
here for completeness and to help users understand why certain valid MATLAB code
might not parse correctly.

# Installation

This parser is now the default for the following editors:

- Emacs: Through the `tree-sitter-langs` package.
- Helix: Builtin.
- Neovim: Through the `nvim-treesitter` plugin.

# Screenshots

![First Screenshot](https://raw.githubusercontent.com/acristoffers/tree-sitter-matlab/screenshots/s1.png)
![Second Screenshot](https://raw.githubusercontent.com/acristoffers/tree-sitter-matlab/screenshots/s2.png)
![Third Screenshot](https://raw.githubusercontent.com/acristoffers/tree-sitter-matlab/screenshots/s3.png)
