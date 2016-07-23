#include <iostream>
#include <string>
#include <string.h>
#include <vector>

#include "common.h"
#include "ast.h"

#define CUR_TOK (Driver::instance()->CurTok)
#define FUNCTIONS (Driver::instance()->Functions)

static Token getNextToken() { return Driver::instance()->getNextToken(); }

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
  if (CUR_TOK.type != expect) {
    return false;
  }
  return true;
}

/// int_expr ::= int
static std::unique_ptr<ExprAST> ParseIntExpr() {
  int NumVal = stoi(CUR_TOK.literal);
  auto Result = helper::make_unique<IntExprAST>(NumVal);
  getNextToken(); // consume the number
  return std::move(Result);
}

/// id_expr
///   ::= identifier
static std::unique_ptr<ExprAST> ParseIdentifierExpr() {
  std::string IdName = CUR_TOK.literal;
  getNextToken(); // eat identifier.
  return helper::make_unique<VariableExprAST>(IdName);
}

/// primary
///   ::= id_expr
///   ::= int_expr
///   ::= nil
static std::unique_ptr<ExprAST> ParsePrimary() {
  switch (CUR_TOK.type) {
  case tok_symbol:
    return ParseIdentifierExpr();
  case tok_integer:
    return ParseIntExpr();
  case tok_nil:
    getNextToken(); // eat nil
    return helper::make_unique<NilExprAST>();
  default:
    std::cout << "when parsing " << CUR_TOK.literal;
    return LogError("unknown token when expecting a primary");
  }
}

static std::unique_ptr<ExprAST> ParseExpression();

static std::unique_ptr<ExprAST> ParseUnaryOpExpr() {
  token_type op = CUR_TOK.type;
  getNextToken(); // eat op
  auto RHS = ParseExpression();
  if (!RHS) return LogError("unknown UnaryOp.RHS when expecting an expr");

  return helper::make_unique<UnaryExprAST>(op, std::move(RHS));
}

static std::unique_ptr<ExprAST> ParseBinOpExpr() {
  token_type op = CUR_TOK.type;
  getNextToken(); // eat op
  auto LHS = ParseExpression();
  if (!LHS) return LogError("unknown BinOp.LHS when expecting an expr");

  // handle a special case of (- x)
  if (op == tok_sub && expectToken(tok_close)) {
    std::unique_ptr<ExprAST> zero = helper::make_unique<IntExprAST>(0);
    return helper::make_unique<BinaryExprAST>(op, std::move(zero), std::move(LHS));
  } else {
    auto RHS = ParseExpression();
    if (!RHS) return LogError("unknown BinOp.RHS when expecting an expr");

    return helper::make_unique<BinaryExprAST>(op, std::move(LHS), std::move(RHS));
  }
}

static std::unique_ptr<ExprAST> ParseIfExpr() {
  getNextToken(); // eat if
  auto Pred = ParseExpression();
  if (!Pred) return LogError("unknown If.Pred when expecting an expr");
  auto Then = ParseExpression();
  if (!Then) return LogError("unknown If.Then when expecting an expr");
  auto Else = ParseExpression();
  if (!Else) return LogError("unknown If.Else when expecting an expr");

  return helper::make_unique<IfExprAST>(std::move(Pred), std::move(Then), std::move(Else));
}

static std::unique_ptr<ExprAST> ParseCondExpr() {
  getNextToken(); // eat cond
  std::vector<std::unique_ptr<ExprAST>> preds;
  std::vector<std::unique_ptr<ExprAST>> exprs;
  while (CUR_TOK.type != tok_close) {
    if (!expectToken(tok_open)) return LogError("non '(' at begin of sub cond expression");
    getNextToken(); // eat open
    if (auto pred = ParseExpression()) {
      if (auto expr = ParseExpression()) {  
        preds.push_back(std::move(pred));
        exprs.push_back(std::move(expr));
      } else
        return LogError("non expr as expr at cond-pred");
    } else
      return LogError("non expr as preds at cond");
    if (!expectToken(tok_close)) return LogError("non ')' at end of sub cond expression");
    getNextToken(); // eat close
  }

  return helper::make_unique<CondExprAST>(std::move(preds), std::move(exprs));
}

static std::unique_ptr<ExprAST> ParseBeginExpr() {
  getNextToken(); // eat begin
  std::vector<std::unique_ptr<ExprAST>> exprs;
  while (CUR_TOK.type != tok_close) {
    if (auto expr = ParseExpression()) {
      exprs.push_back(std::move(expr));
    } else
      return LogError("non expr as expr at begin");
  }

  return helper::make_unique<BeginExprAST>(std::move(exprs));
}

/// Definition
///   ::= id expr
///   ::= (id formals) body
static std::unique_ptr<ExprAST> ParseDefExpr() {
  getNextToken(); // eat define
  if (CUR_TOK.type == tok_open) {
    // function definition
    getNextToken(); // eat open
    if (!expectToken(tok_symbol)) return LogError("non ')' at end of expression");
    std::string FunctName = CUR_TOK.literal;
    getNextToken(); // eat fname
    std::vector<std::string> formals;
    while (CUR_TOK.type != tok_close) {
      if (!expectToken(tok_symbol)) return LogError("non ')' at end of expression");
      formals.push_back(CUR_TOK.literal);
      getNextToken(); // eat formal;
    }
    getNextToken(); // eat close;
    auto proto = helper::make_unique<PrototypeAST>(FunctName, formals);
    std::vector<std::unique_ptr<ExprAST>> body; // parse body
    while (CUR_TOK.type != tok_close) {
      if (auto expr = ParseExpression()) {
        body.push_back(std::move(expr));
      } else
        return LogError("non expr as expr at function body");
    }
    return helper::make_unique<FunctionAST>(std::move(proto), std::move(body));
  } else {
    // variable definition
    std::string IdName = CUR_TOK.literal;
    getNextToken(); // eat identifier
    auto Result = ParseExpression();
    return helper::make_unique<VarDefinitionExprAST>(IdName, std::move(Result));
  }
}

static std::unique_ptr<ExprAST> ParseSetExpr() {
  getNextToken(); // eat set!
  std::string IdName = CUR_TOK.literal;
  getNextToken(); // eat identifier
  auto Result = ParseExpression();
  return helper::make_unique<VarSetExprAST>(IdName, std::move(Result));
}

/// list
///   ::= BinOp expr1 expr2
///   ::= if expr0 expr1 expr2
///   ::= define Definition
///   ::= set! id expr
///   ::= symbol expr*  (* function call: could be direct function call or fptr/closure application *)
///   ::= (list) expr*  (* function apply: closure or function ptr *)
///   ::= cond ((pred1) (expr1))+
///   ::= begin expr*
static std::unique_ptr<ExprAST> ParseList() {
  switch (CUR_TOK.type) {
  case tok_add:
  case tok_sub:
  case tok_mul:
  case tok_div:
  case tok_gt:
  case tok_lt:
  case tok_eq:
  case tok_and:
  case tok_or:
    return ParseBinOpExpr();
  case tok_not:
  case tok_box:
    return ParseUnaryOpExpr();
  case tok_if:
    return ParseIfExpr();
  case tok_define:
    return ParseDefExpr();
  case tok_set:
    return ParseSetExpr();
  case tok_close:
    return helper::make_unique<VariableExprAST>(std::string("nil"));
  case tok_cond:
    return ParseCondExpr();
  case tok_begin:
    return ParseBeginExpr();
  default:
    // application expr
    // auto Callee = ParseExpression();
    auto Callee = ParseExpression(); // eat function identifier.
    std::vector<std::unique_ptr<ExprAST>> Args;
    while (CUR_TOK.type != tok_close) {
      if (auto Arg = ParseExpression())
        Args.push_back(std::move(Arg));
      else
        return LogError("non expr as arg at proc application");
    }
    // return helper::make_unique<CallExprAST>(std::move(Callee), std::move(Args));
    return helper::make_unique<CallExprAST>(std::move(Callee), std::move(Args));
  }
}

static std::string gensym() {
  static int id = 0;
  static std::string sym = "_gensym";
  return sym + std::to_string(id++);
}

static std::string gensym(std::string prefix) {
  static int id = 0;
  static std::string sym = "_gensym";
  return prefix + sym + std::to_string(id++);
}

/// expression
///   ::= primary
///   ::= ( list )
///
static std::unique_ptr<ExprAST> ParseExpression() {
  if (CUR_TOK.type == tok_open) {
    getNextToken(); // eat open
    auto Result = ParseList();
    if (!expectToken(tok_close)) return LogError("non ')' at end of expression");
    getNextToken(); // eat close
    return Result;
  } else {
    return ParsePrimary();
  }
}

void HandleCommand() {
  // Evaluate a top-level expression into an anonymous function.
  if (auto ast = ParseExpression()) {
    if (ast->isaFunction()) {
      ast->print();
      ExprAST *ast_ptr = ast.release();
      FunctionAST *fn_ptr = dynamic_cast<FunctionAST *>(ast_ptr);
      auto fn = helper::make_unique<FunctionAST>(std::move(fn_ptr->Proto), std::move(fn_ptr->Body));
      FUNCTIONS[fn->defName()] = std::move(fn);
    } else {
      // Make an anonymous proto.
      auto proto = helper::make_unique<PrototypeAST>(gensym("__anon_expr"),
                                                   std::vector<std::string>());
      std::vector<std::unique_ptr<ExprAST>> body;
      body.push_back(std::move(ast));
      auto fn = helper::make_unique<FunctionAST>(std::move(proto), std::move(body));
      fn->print();
      FUNCTIONS[fn->defName()] = std::move(fn);
    }
  } else {
    // Skip token for error recovery.
    getNextToken();
  }
}
