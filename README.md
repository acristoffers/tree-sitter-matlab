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

# Installation

## Neovim

Theoretically this should work:

```lua
local parser_config = require("nvim-treesitter.parsers").get_parser_configs()
parser_config.matlab = {
  install_info = {
    url = 'https://github.com/acristoffers/tree-sitter-matlab',
    files = { 'src/parser.c', 'src/scanner.c' },
    branch = 'main',
    generate_requires_npm = false,
    requires_generate_from_grammar = false,
  },
  maintainers = { '@acristoffers' },
}
```

But I had to clone the repo locally and put the path to it in `url` for it to
work, otherwise it keeps complaining about something no being a valid tarball.

## Doom Emacs

You have to manually compile and copy the files to the appropriate folders. Pay
attention to the STRAIGHT variable value, as yours may be different.

```zsh
tree-sitter generate --abi 13
gcc src/*.c -I./src --shared -fPIC -Os -o matlab.so 

VERSION=28.2
STRAIGHT=~/.config/emacs/.local/straight

mkdir -p $STRAIGHT/{build-$VERSION,repos}/tree-sitter-langs/bin
mkdir -p $STRAIGHT/{build-$VERSION,repos}/tree-sitter-langs/queries/matlab
mkdir -p $STRAIGHT/{build-$VERSION,repos}/evil-textobj-tree-sitter/queries/matlab
mkdir -p $STRAIGHT/repos/elisp-tree-sitter/langs/queries/matlab

cp matlab.so $STRAIGHT/repos/tree-sitter-langs/bin
cp matlab.so $STRAIGHT/build-$VERSION/tree-sitter-langs/bin

cp queries/*               $STRAIGHT/repos/tree-sitter-langs/queries/matlab/
cp queries/*               $STRAIGHT/build-$VERSION/tree-sitter-langs/queries/matlab/
cp queries/*               $STRAIGHT/repos/elisp-tree-sitter/langs/queries/matlab
cp queries/emacs-textobjects.scm $STRAIGHT/repos/evil-textobj-tree-sitter/queries/matlab/textobjects.scm
cp queries/emacs-textobjects.scm $STRAIGHT/build-$VERSION/evil-textobj-tree-sitter/queries/matlab/textobjects.scm

mv $STRAIGHT/repos/tree-sitter-langs/queries/matlab/emacs-highlights.scm $STRAIGHT/repos/tree-sitter-langs/queries/matlab/highlights.scm
mv $STRAIGHT/build-$VERSION/tree-sitter-langs/queries/matlab/emacs-highlights.scm $STRAIGHT/build-$VERSION/tree-sitter-langs/queries/matlab/highlights.scm
mv $STRAIGHT/repos/elisp-tree-sitter/langs/queries/matlab/emacs-highlights.scm $STRAIGHT/repos/elisp-tree-sitter/langs/queries/matlab/highlights.scm
```

- packages.el

```elisp
(package! matlab-mode)
```

- config.el

```elisp
(use-package! matlab-mode :defer t)
(add-hook! 'matlab-mode-hook
           #'display-line-numbers-mode
           #'matlab-toggle-show-mlint-warnings
           (setq! matlab-file-font-lock-keywords matlab-file-basic-font-lock-keywords)
           (tree-sitter-hl-mode 1))
```

You may need to add MATLAB to `tree-sitter-major-mode-language-alist` and
`evil-textobj-tree-sitter-major-mode-language-alist`.

# Screen Shots

![First Screenshot](https://raw.githubusercontent.com/acristoffers/tree-sitter-matlab/screenshots/s1.png)
![Second Screenshot](https://raw.githubusercontent.com/acristoffers/tree-sitter-matlab/screenshots/s2.png)
![Third Screenshot](https://raw.githubusercontent.com/acristoffers/tree-sitter-matlab/screenshots/s3.png)
