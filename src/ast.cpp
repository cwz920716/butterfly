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

#include <iostream>
#include <string>
#include <string.h>
#include <vector>

#include "common.h"
#include "ast.h"


/// CurTok/getNextToken - Provide a simple token buffer.  CurTok is the current
/// token the parser is looking at.  getNextToken reads another token from the
/// lexer and updates CurTok with its results.
const char *test_scm = "(define (f x) 1)\n(f 1 2 3)";
Lexer lex(test_scm);
static Token CurTok;
static Token getNextToken() { return CurTok = lex.getNextToken(); }

/// LogError* - These are little helper functions for error handling.
std::unique_ptr<ExprAST> LogError(const char *Str) {
  fprintf(stderr, "Error: %s\n", Str);
  return nullptr;
}

std::unique_ptr<PrototypeAST> LogErrorP(const char *Str) {
  LogError(Str);
  return nullptr;
}

bool expectToken(token_type expect) {
  if (CurTok.type != expect) {
    return false;
  }
  return true;
}

/// int_expr ::= int
static std::unique_ptr<ExprAST> ParseIntExpr() {
  int NumVal = stoi(CurTok.literal);
  auto Result = llvm::make_unique<IntExprAST>(NumVal);
  getNextToken(); // consume the number
  return std::move(Result);
}

/// id_expr
///   ::= identifier
static std::unique_ptr<ExprAST> ParseIdentifierExpr() {
  std::string IdName = CurTok.literal;
  getNextToken(); // eat identifier.
  return llvm::make_unique<VariableExprAST>(IdName);
}

/// primary
///   ::= id_expr
///   ::= int_expr
static std::unique_ptr<ExprAST> ParsePrimary() {
  switch (CurTok.type) {
  case tok_symbol:
    return ParseIdentifierExpr();
  case tok_integer:
    return ParseIntExpr();
  default:
    return LogError("unknown token when expecting a primary");
  }
}

static std::unique_ptr<ExprAST> ParseExpression();

static std::unique_ptr<ExprAST> ParseBinOpExpr() {
  token_type op = CurTok.type;
  getNextToken(); // eat op
  auto LHS = ParseExpression();
  if (!LHS) return LogError("unknown BinOp.LHS when expecting an expr");
  auto RHS = ParseExpression();
  if (!RHS) return LogError("unknown BinOp.RHS when expecting an expr");

  return llvm::make_unique<BinaryExprAST>(op, std::move(LHS), std::move(RHS));
}

static std::unique_ptr<ExprAST> ParseIfExpr() {
  getNextToken(); // eat if
  auto Pred = ParseExpression();
  if (!Pred) return LogError("unknown If.Pred when expecting an expr");
  auto Then = ParseExpression();
  if (!Then) return LogError("unknown If.Then when expecting an expr");
  auto Else = ParseExpression();
  if (!Else) return LogError("unknown If.Else when expecting an expr");

  return llvm::make_unique<IfExprAST>(std::move(Pred), std::move(Then), std::move(Else));
}

/// Definition
///   ::= id expr
///   ::= (id formals) body
/// for now, body is a single expression
static std::unique_ptr<ExprAST> ParseDefExpr() {
  getNextToken(); // eat define
  if (CurTok.type == tok_open) {
    // function definition
    getNextToken(); // eat open
    if (!expectToken(tok_symbol)) return LogError("non ')' at end of expression");
    std::string FunctName = CurTok.literal;
    getNextToken(); // eat fname
    std::vector<std::string> formals;
    while (CurTok.type != tok_close) {
      if (!expectToken(tok_symbol)) return LogError("non ')' at end of expression");
      formals.push_back(CurTok.literal);
      getNextToken(); // eat formal;
    }
    getNextToken(); // eat close;
    auto proto = llvm::make_unique<PrototypeAST>(FunctName, formals);
    auto body= ParseExpression(); // parse body
    return llvm::make_unique<FunctionAST>(std::move(proto), std::move(body));
  } else {
    // variable definition
    std::string IdName = CurTok.literal;
    getNextToken(); // eat identifier
    auto Result = ParseExpression();
    return llvm::make_unique<VarDefinitionExprAST>(IdName, std::move(Result));
  }
}

/// list
///   ::= BinOp expr1 expr2
///   ::= if expr0 expr1 expr2
///   ::= define Definition
///   ::= expr*
static std::unique_ptr<ExprAST> ParseList() {
  switch (CurTok.type) {
  case tok_add:
  case tok_sub:
  case tok_mul:
  case tok_div:
  case tok_gt:
  case tok_lt:
  case tok_eq:
    return ParseBinOpExpr();
  case tok_if:
    return ParseIfExpr();
  case tok_define:
    return ParseDefExpr();
  case tok_close:
    return llvm::make_unique<VariableExprAST>(std::string("nil"));
  default:
    // application expr
    auto Callee = ParseExpression();
    std::vector<std::unique_ptr<ExprAST>> Args;
    while (CurTok.type != tok_close) {
      if (auto Arg = ParseExpression())
        Args.push_back(std::move(Arg));
      else
        return LogError("non expr as arg at proc application");
    }
    return llvm::make_unique<CallExprAST>(std::move(Callee), std::move(Args));
  }
}


/// expression
///   ::= primary
///   ::= ( list )
///
static std::unique_ptr<ExprAST> ParseExpression() {
  if (CurTok.type == tok_open) {
    getNextToken(); // eat open
    auto Result = ParseList();
    if (!expectToken(tok_close)) return LogError("non ')' at end of expression");
    getNextToken(); // eat close
    return Result;
  } else {
    return ParsePrimary();
  }
}

static void HandleCommand() {
  // Evaluate a top-level expression into an anonymous function.
  if (auto ast = ParseExpression()) {
    // dump AST
    ast->print();
    std::cout << std::endl;
  } else {
    // Skip token for error recovery.
    getNextToken();
  }
}

/// top ::= definition | external | expression | ';'
static void MainLoop() {
  while (true) {
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
  // Prime the first token.
  getNextToken();

  // Run the main "interpreter loop" now.
  MainLoop();

  return 0;
}




