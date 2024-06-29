package tree_sitter_matlab_test

import (
	"testing"

	tree_sitter "github.com/smacker/go-tree-sitter"
	"github.com/tree-sitter/tree-sitter-matlab"
)

func TestCanLoadGrammar(t *testing.T) {
	language := tree_sitter.NewLanguage(tree_sitter_matlab.Language())
	if language == nil {
		t.Errorf("Error loading Matlab grammar")
	}
}
