# Copyright 2014, Jay Conrod. All rights reserved.
#
# This file is part of Gypsum. Use of this source code is governed by
# the GPL license that can be found in the LICENSE.txt file.


from utility.data import Data

KEYWORD = "keyword"
RESERVED = "reserved"
SYMBOL = "symbol"
OPERATOR = "operator"
INTEGER = "integer"
DOUBLE = "double"

NEWLINE = "newline"
SPACE = "space"
COMMENT = "comment"

INTERNAL = "internal"

class Token(Data):
    propertyNames = ["text", "tag", "location"]

    def __str__(self):
        return '("%s", %s) @ %s' % (self.text, self.tag, str(self.location))

    def isPrintable(self):
        return self.tag not in [NEWLINE, SPACE, COMMENT]

__all__ = ["Token", "KEYWORD", "RESERVED", "SYMBOL", "OPERATOR",
           "INTEGER", "DOUBLE", "NEWLINE", "SPACE",
           "COMMENT", "INTERNAL"]
