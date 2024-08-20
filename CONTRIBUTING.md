# Code style

Follow the code style of the file you are working on. There is a `.clang-format` file at the root of the
repo, use it in both `src/scanner.c` and `grammar.js`. In most Linux distributions `clang-format` is
available in a package named `clang-tools`.

# Commit message

This repo uses [Semantic Commits], all lowercase and with no dot at the end.

# Commit contents

Do not put auto-generated files in the commits, only add the source files (usually `grammar.js` and
`src/scanner.c`). I'll generate the other files after merging.

# Tree-Sitter modifications

This project is considered in production and changes should be minimal. Avoid creating new nodes and
try to keep the changes as minimal as possible.

# Tests

Add or modify tests to cover the changes you a making. There is no reason to go crazy, just add
something so that if it ever breaks for some reason, I know right away that it broke.

[Semantic Commits]: https://gist.github.com/joshbuchea/6f47e86d2510bce28f8e7f42ae84c716
