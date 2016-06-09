# Copyright 2014, Jay Conrod. All rights reserved.
#
# This file is part of Gypsum. Use of this source code is governed by
# the GPL license that can be found in the LICENSE.txt file.


import unittest

from lexer import *
from utility.errors import LexException
from utility.location import Location
from tok import *

class TestLexer(unittest.TestCase):
    def stripSpace(self, tokens):
        toks = []
        for tok in tokens:
           if tok.tag not in [SPACE, NEWLINE, COMMENT]:
               toks.append(tok)
        return toks

    def checkTexts(self, expected, text):
        tokens = lex("test", text)
        texts = [t.text for t in self.stripSpace(tokens)]
        self.assertEquals(len(expected), len(texts))
        for i in range(len(expected)):
            self.assertEquals(expected[i], texts[i])

    def checkTags(self, expected, text):
        tokens = lex("test", text)
        tags = [t.tag for t in self.stripSpace(tokens)]
        self.assertEquals(len(expected), len(tags))
        for i in range(len(expected)):
            self.assertIs(expected[i], tags[i])

    def checkAll(self, text, expected_tags, expected_texts):
        self.checkTexts(expected_texts, text)
        self.checkTags(expected_tags, text)

    def testEmpty(self):
        self.checkAll("", [], [])

    def testLiterals(self):
        self.checkAll("1.22324324 1 \n 12 22.3 0x45", [DOUBLE, INTEGER, INTEGER, DOUBLE, INTEGER], ["1.22324324", "1", "12", "22.3", "0x45"])

    def testKeywords(self):
        self.checkAll("function f(x) 1 end", [KEYWORD, SYMBOL, RESERVED, SYMBOL, RESERVED, INTEGER, KEYWORD], ["function", "f", "(", "x", ")", "1", "end"])

if __name__ == '__main__':
    unittest.main()
