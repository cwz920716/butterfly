#ifndef _COMMON_H
#define _COMMON_H

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "../include/KaleidoscopeJIT.h"

#include "objects.h"

// The lexer returns tokens [0-255] if it is an unknown character, otherwise one
// of these for known things.
enum token_type {
  tok_error,
  tok_eof,

  // commands
  tok_define,
  tok_lambda,
  tok_if,
  tok_cond,

  // operators
  tok_open,
  tok_close,
  tok_add,
  tok_sub,
  tok_mul,
  tok_div,
  tok_gt,
  tok_lt,
  tok_eq,
  tok_and,
  tok_or,
  tok_not,

  // primitive types
  tok_symbol,
  tok_integer,

  // whitespaces
  tok_space,
  tok_newline
};

extern std::string token_desc[];

struct Token {
  token_type type;
  std::string literal;
};

token_type getToken(const char *text, int *len);

class Lexer {
public:
  const char *text;
  int next;
  
  Lexer(const char *_text) : text(_text), next(0) {}

  Token getNextToken() {
    token_type tok;
    Token token;
    const char* src = text + next;

    do {
      int tok_len = 0;
      tok = getToken(src, &tok_len);
      std::string lit(src, tok_len);
      token.type = tok;
      token.literal = lit;
      src += tok_len;
      next += tok_len;
    } while (tok == tok_space || tok == tok_newline);

    return token;
  }
};

#endif
