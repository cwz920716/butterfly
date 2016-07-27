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
  const char *test_scm3 = "(define (sqrt x) \
                                   (define (good-enough? guess) \
                                           (< (abs (- (square guess) x)) 1)) \
                                   (define (improve guess) \
                                           (average guess (/ x guess))) \
                                   (define (sqrt-iter guess) \
                                           (if (good-enough? guess) \
                                               guess \
                                               (sqrt-iter (improve guess)))) \
                                   (sqrt-iter 1))";
  const char *test_scm4 = "(define (foo x) (define (fib x) (if (> x 0) 1 (+ (fib (+ x 1)) (fib (- x 1))))) fib )";

  // printf(test_scm7);

  // initialize
  Driver *driver = Driver::instance(test_scm4);
  driver->Initialize();

  // Prime the first token.
  driver->getNextToken();

  // Run the main "interpreter loop" now.
  MainLoop();

  // Start transformation
  Transform();

  return 0;
} 
