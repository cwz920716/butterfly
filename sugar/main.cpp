#include <iostream>
#include <string>
#include <string.h>
#include <vector>

#include "common.h"
#include "ast.h"
#include "../lib/shared.h"

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

#define MAX_FLEN (1024 * 10)
static char test_scm[MAX_FLEN];

int main(int argc, char **argv) {

  int n = readFile(argv[1], test_scm, MAX_FLEN);
  if (n > 0) test_scm[n] = '\0';
  // printf(test_scm7);

  // initialize
  Driver *driver = Driver::instance(test_scm);
  driver->Initialize();

  // Prime the first token.
  driver->getNextToken();

  // Run the main "interpreter loop" now.
  MainLoop();

  // Start transformation
  Transform();

  std::cerr << "Sugar Output:" << std::endl;
  for(auto const &fentry : FUNCTIONS) {
    std::string fname = fentry.first;
    std::cerr << fname << ":" << std::endl;
    fentry.second->print();
  }

  return 0;
} 
