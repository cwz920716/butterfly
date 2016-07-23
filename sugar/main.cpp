#include <iostream>
#include <string>
#include <string.h>
#include <vector>

#include "common.h"
#include "ast.h"

Driver *Driver::_instance;
#define FUNCTIONS (Driver::instance()->Functions)

void Driver::Initialize() {
}

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

static void Transform() {
  for(auto const &fentry : FUNCTIONS) {
    fentry.second->closurePass();
  }
}

int main() {

  const char *test_scm = "(define (foo x) (define y x) (define (withdraw z) (- y z) ) withdraw )\n";
  const char *test_scm2 = "(define (foo x) (define (bar y) (define (baz z) y) (foobar 1 2) baz ) (define (foobar a b) (+ a b)) bar (foobar 1 2) (set! foobar 2) )\n";
  
  // printf(test_scm7);

  // initialize
  Driver *driver = Driver::instance(test_scm2);
  driver->Initialize();

  // Prime the first token.
  driver->getNextToken();

  // Run the main "interpreter loop" now.
  MainLoop();

  // Start transformation
  Transform();

  return 0;
} 
