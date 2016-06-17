#include <iostream>
#include <string>
#include <string.h>
#include <vector>

#include "common.h"
#include "ast.h"

Driver *Driver::_instance;

/// top ::= definition | external | expression | ';'
static void MainLoop() {
  while (true) {
    Token CurTok = Driver::instance()->CurTok;
    switch (CurTok.type) {
    case tok_eof:
      return;
    default:
      HandleCommand();
      break;
    }
  }
}

int main() {
  const char *test_scm = "(define (square x) (* x x))\n(define (sum-of-squares x y) (+ (square x) (square y)))\n(+ 1 2)\n(square 4)";

  // initialize
  Driver *driver = Driver::instance(test_scm);
  // Prime the first token.
  driver->getNextToken();

  // Run the main "interpreter loop" now.
  MainLoop();

  // Print out all of the generated code.
  (Driver::instance()->TheModule)->dump();

  return 0;
} 
