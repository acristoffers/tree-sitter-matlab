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

# Known problems

Newlines, just like whitespaces, are mostly ignored. In the case of spaces, it
allows `abs( a )` and `abs(a)` to be described by the same, simple rule. In the
case of newlines, it allows many multiline constructs (like, `if`, `while`,
`function`) to be expressed the same way.

This creates the undesired side-effect that some constructs, which are not
accepted by MATLAB, are correctly parsed, like:

```matlab
function (
  a
)
end
```

This, however, is hard to fix. The assumption that newlines are ignored by
default is all over the grammar and changing it requires making changes to too
many rules, which also make them all more complex and fragile. Therefore, this
change won't be made.

# Installation

This parser is now the default for the following editors:

- Emacs: Through the `tree-sitter-langs` package.
- Helix: Builtin, now in master and will be available in the next release (whatever comes after 23.05).
- Neovim: Through the `nvim-treesitter` plugin.

# Screenshots

![First Screenshot](https://raw.githubusercontent.com/acristoffers/tree-sitter-matlab/screenshots/s1.png)
![Second Screenshot](https://raw.githubusercontent.com/acristoffers/tree-sitter-matlab/screenshots/s2.png)
![Third Screenshot](https://raw.githubusercontent.com/acristoffers/tree-sitter-matlab/screenshots/s3.png)
