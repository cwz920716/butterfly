#include <iostream>
#include <string>
#include <string.h>
#include <vector>

#include "common.h"
#include "ast.h"

/// top ::= <expr>+
/// expr ::= <int> | <symbol> | <definition> | <application>
/// definition ::= (define <symbol> <expr>)
///              | (define (<symbol+>) <expr>+)
/// <application> ::= (<op> <expr> <expr>)
static void MainLoop() {
  while (true) {
    fprintf(stderr, "ready> ");
    switch (CurTok) {
    case tok_eof:
      return;
    case tok_def:
      HandleDefinition();
      break;
    case tok_extern:
      HandleExtern();
      break;
    default:
      HandleTopLevelExpression();
      break;
    }
  }
}
