from pathlib import Path
from unittest import TestCase

import tree_sitter
import tree_sitter_matlab


class TestLanguage(TestCase):
    def test_can_load_grammar(self):
        try:
            tree_sitter.Language(tree_sitter_matlab.language())
        except Exception:
            self.fail("Error loading Matlab grammar")

    def test_crlf_corpus_preserves_line_endings(self):
        corpus = Path(__file__).resolve().parents[3] / "test" / "corpus" / "crlf.txt"
        source = corpus.read_bytes()
        self.assertGreater(source.count(b"\r\n"), 0)
        self.assertEqual(source.count(b"\n"), source.count(b"\r\n"))

    def test_embedded_nul_is_not_eof(self):
        parser = tree_sitter.Parser(tree_sitter.Language(tree_sitter_matlab.language()))
        for source in (b"pwd\x00\n", b"x = 1;\x00\ny = 2;"):
            with self.subTest(source=source):
                self.assertTrue(parser.parse(source).root_node.has_error)

    def test_crlf_comment_ranges(self):
        parser = tree_sitter.Parser(tree_sitter.Language(tree_sitter_matlab.language()))
        lines = [
            b"% leading one",
            b"    % leading two",
            b"",
            b"% separate",
            b"function f",
            b"% help one",
            b"    % help two",
            b"",
            b"% body comment",
            b"x = 1;",
            b"end",
            b"",
        ]
        comment_ranges = []
        for newline in (b"\n", b"\r\n"):
            with self.subTest(newline=newline):
                source = newline.join(lines)
                root = parser.parse(source).root_node
                self.assertFalse(root.has_error)
                pending = [root]
                comments = []
                while pending:
                    node = pending.pop()
                    if node.type == "comment":
                        comments.append(node)
                    pending.extend(reversed(node.children))
                self.assertEqual(len(comments), 4)
                self.assertEqual(comments[0].text, newline.join(lines[:2]))
                self.assertEqual(comments[1].text, lines[3])
                self.assertEqual(comments[2].text, newline.join(lines[5:7]))
                self.assertEqual(comments[3].text, lines[8])
                for comment in comments:
                    self.assertEqual(
                        source[comment.start_byte : comment.end_byte], comment.text
                    )
                comment_ranges.append(
                    [(comment.start_point, comment.end_point) for comment in comments]
                )
        self.assertEqual(comment_ranges[0], comment_ranges[1])
        self.assertEqual(
            comment_ranges[0],
            [
                ((0, 0), (1, 17)),
                ((3, 0), (3, 10)),
                ((5, 0), (6, 14)),
                ((8, 0), (8, 14)),
            ],
        )
