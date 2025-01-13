import XCTest
import SwiftTreeSitter
import TreeSitterMatlab

final class TreeSitterMatlabTests: XCTestCase {
    func testCanLoadGrammar() throws {
        let parser = Parser()
        let language = Language(language: tree_sitter_matlab())
        XCTAssertNoThrow(try parser.setLanguage(language),
                         "Error loading Matlab grammar")
    }
}
