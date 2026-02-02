#!/bin/sh

set -eu

ROOT_DIR="fuzzer"

LANG=$1
TIME=$2
CPP=$3
# if scanner = scanner.cc then XFLAG = c++ else XFLAG = c
if [ "$CPP" = "cpp" ]; then
	SCANNER="scanner.cc"
	XFLAG="c++"
else
	SCANNER="scanner.c"
	XFLAG="c"
fi

shift 3

export PATH="/root/.cargo/bin:$PATH"
export CFLAGS="$(pkg-config --cflags --libs tree-sitter) -O0 -g -Wall"

JQ_FILTER='.. | if .type? == "STRING" or (.type? == "ALIAS" and .named? == false) then .value else null end'

build_dict() {
	jq "$JQ_FILTER" <src/grammar.json |
		grep -v "\\\\" | grep -v null >"$ROOT_DIR/dict"
}

build_fuzzer() {
	cat <<END | clang -fsanitize=fuzzer,address $CFLAGS -lstdc++ -g -x $XFLAG - src/$SCANNER src/parser.c $@ -o $ROOT_DIR/fuzzer
#include <stdio.h>
#include <stdlib.h>
#include <tree_sitter/api.h>

#ifdef __cplusplus
extern "C"
#endif
TSLanguage *tree_sitter_$LANG();

#ifdef __cplusplus
extern "C"
#endif
int LLVMFuzzerTestOneInput(const uint8_t * data, const size_t len) {
  // Create a parser.
  TSParser *parser = ts_parser_new();

  // Set the parser's language.
  ts_parser_set_language(parser, tree_sitter_$LANG());

  // Build a syntax tree based on source code stored in a string.
  TSTree *tree = ts_parser_parse_string(
    parser,
    NULL,
    (const char *)data,
    len
  );
  // Free all of the heap-allocated memory.
  ts_tree_delete(tree);
  ts_parser_delete(parser);
  return 0;
}
END
}

generate_fuzzer() {
	tree-sitter generate
}

makedirs() {
	mkdir -p "$ROOT_DIR"
	mkdir -p "$ROOT_DIR/out"
}

makedirs
generate_fuzzer

build_dict
build_fuzzer $@
cd "$ROOT_DIR"
./fuzzer -detect_leaks=0 -dict=dict -timeout=2 -max_total_time=$TIME out/
