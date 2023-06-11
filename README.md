# MATLAB grammar for tree-sitter.

The grammar is now complete, in that it supports every statement/expression I
could find in MATLAB. I'll add more if I ever find something is missing. Since
I'm currently working daily on MATLAB, I'll be fixing errors as I find them in
real-world usage. That said, I got a nice suite of tests here too.

This repo also hosts queries for highlight, fold, context, locals, tags and
textobjects. I'm a Neovim/Doom Emacs user, and textobjects is what made me
consider writing the grammar in the first place. The existing MATLAB grammar
used everywhere seems to be unmaintained and does not have rules or behaves
badly in many cases (no commands, no text objects, no line continuation, no
arguments statement, only integers as numbers, no complex j, etc), so I
preferred to write one from scratch. It was also a good opportunity to learn
how it is done in tree-sitter.

I hope you enjoy it and if you find any broken behaviour or missing
expression/statement, just open an issue.

Things to try:
- Selecting around/inside parameter when in a matrix. Spoiler alert: around
selects the row, inside selects the cell!
- Selecting around/inside function and parameters on a line like this `variable
P(2, 2) semidefinite`. Spoiler again: this is a command and `P(2, 2)` is
actually a single argument to MATLAB, not two, which actually makes sense.

Things that are broken:
- Single-quoted strings won't show escape and formatting options as a
doubled-quoted one. It got impossible to differentiate a string from a
transpose from the scanner.

# Neovim

```lua
local parser_config = require("nvim-treesitter.parsers").get_parser_configs()
parser_config.matlab = {
  install_info = {
    url = "https://github.com/acristoffers/tree-sitter-matlab",
    files = { "src/parser.c", "src/scanner.c" },
    branch= 'main'
  },
  filetype = "matlab", -- if filetype does not agrees with parser name
}
```

# Doom Emacs

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
cp queries/textobjects.scm $STRAIGHT/repos/evil-textobj-tree-sitter/queries/matlab
cp queries/textobjects.scm $STRAIGHT/build-$VERSION/evil-textobj-tree-sitter/queries/matlab
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
