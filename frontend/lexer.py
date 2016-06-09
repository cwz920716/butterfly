# Copyright 2014-2015, Jay Conrod. All rights reserved.
#
# This file is part of Gypsum. Use of this source code is governed by
# the GPL license that can be found in the LICENSE.txt file.


import re

from utility.errors import LexException
from utility.errors import DEBUG
from utility.location import Location
from tok import *

__opchars = r"[*+\-/<=>]"

__expressions = [
  (r"\r?\n", NEWLINE),
  (r"[\t ]+", SPACE),
  (r"#[^\r\n]*", COMMENT),

  (r"\(",  RESERVED),
  (r"\)",  RESERVED),

  (r"function", KEYWORD),
  (r"end", KEYWORD),
  (r"int", KEYWORD), # optional
  (r"double", KEYWORD), # optional
  (r"if", KEYWORD),
  (r"then", KEYWORD),
  (r"else", KEYWORD),
  (r"elseif", KEYWORD),
  (r"return", KEYWORD), # optional

  (r"[+-]?[0-9]+(?:i[0-9]+)?", INTEGER),
  (r"[+-]?0[xX][0-9A-Fa-f]+(?:i[0-9]+)?", INTEGER),

  (r"[+-]?[0-9]+\.[0-9]*(?:[Ee][+-]?[0-9]+)?(?:f[0-9]+)?", DOUBLE),
  (r"[+-]?\.[0-9]+(?:[Ee][+-]?[0-9]+)?(?:f[0-9]+)?", DOUBLE),
  (r"[+-]?[0-9]+[Ee][+-]?[0-9]+(?:f[0-9]+)?", DOUBLE),
  (r"[+-]?[0-9]+f[0-9]+", DOUBLE),

  (r"[*+\-/<=>]", OPERATOR),
  (r"==", OPERATOR),
  (r"<=", OPERATOR),
  (r">=", OPERATOR),

  (r"[A-Za-z_][A-Za-z0-9_-]*", SYMBOL),
]
__expressions = [(re.compile(expr[0]), expr[1]) for expr in __expressions]


def lex(filename, source):
    tokens = []
    pos = 0
    end = len(source)
    line = 1
    column = 1
    while pos < end:
        token = None
        for expr in __expressions:
            (rx, tag) = expr
            m = rx.match(source, pos)
            if m:
                text = m.group(0)
                if token and len(text) <= len(token.text):
                    continue

                location = Location(filename, line, column, line, column + len(text))
                token = Token(m.group(0), tag, location)
        if not token:
            location = Location(filename, line, column, line, column + 1)
            raise LexException(location, "illegal character: %s" % source[pos:pos+1])
        DEBUG(token)
        tokens.append(token)
        if token.tag is NEWLINE:
            line += 1
            column = 1
        else:
            column += len(token.text)
        pos += len(token.text)

    return tokens

__all__ = ["lex"]
