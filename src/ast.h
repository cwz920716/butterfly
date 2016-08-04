#ifndef _AST_H
#define _AST_H

#include <iostream>
#include "common.h"

/// ExprAST - Base class for all expression nodes.
class ExprAST {
public:
  virtual ~ExprAST() {}
  virtual void print() {}
  virtual bool isaFunction() { return false; }
  virtual llvm::Value *codegen() { return nullptr; }
};

/// IntExprAST - Expression class for numeric literals like "1.0".
class IntExprAST : public ExprAST {
  int Val;

public:
  IntExprAST(int Val) : Val(Val) {}
  void print() override { std::cout << "(Int=" << Val << ")"; }
  llvm::Value *codegen() override;
};

/// IntExprAST - Expression class for numeric literals like "1.0".
class NilExprAST : public ExprAST {
public:
  NilExprAST() {}
  void print() override { std::cout << "nil"; }
  llvm::Value *codegen() override;
};

/// VariableExprAST - Expression class for referencing a variable, like "a".
class VariableExprAST : public ExprAST {
  std::string Name;

public:
  VariableExprAST(const std::string &Name) : Name(Name) {}
  void print() override { std::cout << "(Var=" << Name << ")"; }
  llvm::Value *codegen() override;
};

/// VarDefinitionExprAST - Expression class for referencing a variable, like "a".
class VarDefinitionExprAST : public ExprAST {
  std::string Name;
  std::unique_ptr<ExprAST> Init;

public:
  VarDefinitionExprAST(const std::string &Name, std::unique_ptr<ExprAST> init) : Name(Name), Init(std::move(init)) {}
  void print() override { 
    std::cout << "(var " << Name << " <- ";
    Init->print();
    std::cout << ")";
  }
  llvm::Value *codegen() override;
};

/// VarSetExprAST - Expression class for referencing a variable, like "a".
class VarSetExprAST : public ExprAST {
  std::string Name;
  std::unique_ptr<ExprAST> Expr;

public:
  VarSetExprAST(const std::string &name, std::unique_ptr<ExprAST> expr) : Name(name), Expr(std::move(expr)) {}
  void print() override { 
    std::cout << "(set! " << Name << " ";
    Expr->print();
    std::cout << ")";
  }
  llvm::Value *codegen() override;
};

/// BinaryExprAST - Expression class for a binary operator.
class BinaryExprAST : public ExprAST {
  token_type Op;
  std::unique_ptr<ExprAST> LHS, RHS;

public:
  BinaryExprAST(token_type op, std::unique_ptr<ExprAST> LHS,
                std::unique_ptr<ExprAST> RHS)
    : Op(op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}

  void print() override { 
    std::cout << "(Op=" << token_desc[Op] << ", "; 
    LHS->print(); std::cout << ", "; 
    RHS->print();
    std::cout << ")";
  }
  llvm::Value *codegen() override;
};

/// UnaryExprAST - Expression class for a binary operator.
class UnaryExprAST : public ExprAST {
  token_type Op;
  std::unique_ptr<ExprAST> RHS;

public:
  UnaryExprAST(token_type op, std::unique_ptr<ExprAST> RHS)
    : Op(op), RHS(std::move(RHS)) {}

  void print() override { 
    std::cout << "(Op=" << token_desc[Op] << ", "; 
    RHS->print();
    std::cout << ")";
  }
  llvm::Value *codegen() override;
};

/// IfExprAST - Expression class for a if statement.
class IfExprAST : public ExprAST {
  std::unique_ptr<ExprAST> Pred, Then, Else;

public:
  IfExprAST(std::unique_ptr<ExprAST> _pred,
            std::unique_ptr<ExprAST> _then, 
            std::unique_ptr<ExprAST> _else)
    : Pred(std::move(_pred)), Then(std::move(_then)), Else(std::move(_else)) {}

  void print() override { 
    std::cout << "(If "; 
    Pred->print(); std::cout << ", "; 
    Then->print(); std::cout << ", "; 
    Else->print();
    std::cout << ")";
  }
  llvm::Value *codegen() override;
};

class BeginExprAST: public ExprAST {
  std::vector<std::unique_ptr<ExprAST>> Exprs;

public:
  BeginExprAST(std::vector<std::unique_ptr<ExprAST>> exprs)
      : Exprs(std::move(exprs)) {}

  void print() override { 
    std::cout << "(Begin " << "Exprs: {"; 
    // Callee->print(); std::cout << ": ";
    for (auto &e : Exprs) {
      e->print(); std::cout << ", "; 
    }
    std::cout << "})" << std::endl;
  }
  llvm::Value *codegen() override;
};

/// CallExprAST - Expression class for function calls.
/// TODO: add short-cut support for static dispatch
class CallExprAST : public ExprAST {
  std::unique_ptr<ExprAST> Callee;
  std::vector<std::unique_ptr<ExprAST>> Args;
  std::string Symbol_; // candidate for static callee

public:
/*
  CallExprAST(std::unique_ptr<ExprAST> Callee,
              std::vector<std::unique_ptr<ExprAST>> Args)
    : Callee(std::move(Callee)), Args(std::move(Args)) {}
*/
  CallExprAST(std::unique_ptr<ExprAST> Callee,
              std::vector<std::unique_ptr<ExprAST>> Args,
              std::string symbol)
      : Callee(std::move(Callee)), Args(std::move(Args)), Symbol_(symbol) {}

  void print() override { 
    std::cout << "(apply "; 
    Callee->print(); std::cout << ": ";
    for (auto &Arg : Args) {
      Arg->print(); std::cout << ", "; 
    }
    std::cout << ")";
  }
  llvm::Value *codegen() override;
};

/// ClosureExprAST - Expression class for new closure.
class ClosureExprAST : public ExprAST {
  std::string Callback; // std::unique_ptr<ExprAST> Callee;
  std::vector<std::unique_ptr<ExprAST>> Members;

public:
  ClosureExprAST(const std::string &Callback,
                 std::vector<std::unique_ptr<ExprAST>> Members)
      : Callback(Callback), Members(std::move(Members)) {}

  void print() override { 
    std::cout << "(clos " << Callback << ": "; 
    // Callee->print(); std::cout << ": ";
    for (auto &m : Members) {
      m->print(); std::cout << ", ";  
    }
    std::cout << ")";
  }
  llvm::Value *codegen() override;
};

/// GetFieldExprAST - Expression class for get field.
class GetFieldExprAST : public ExprAST {
  int Index; // std::unique_ptr<ExprAST> Callee;
  std::unique_ptr<ExprAST> Object;

public:
  GetFieldExprAST(const int index,
                  std::unique_ptr<ExprAST> object)
      : Index(index), Object(std::move(object)) {}

  void print() override { 
    std::cout << "(getfield " << Index << " "; 
    Object->print();
    std::cout << ")";
  }
  llvm::Value *codegen() override;
};

/// PrototypeAST - This class represents the "prototype" for a function,
/// which captures its name, and its argument names (thus implicitly the number
/// of arguments the function takes).
class PrototypeAST {
  std::string Name;
  std::vector<std::string> Args;
  friend class FunctionAST;

public:
  PrototypeAST(const std::string &name, std::vector<std::string> Args)
    : Name(name), Args(std::move(Args)) {}

  void print() { 
    std::cout << Name << "("; 
    for (auto &i : Args)
      std::cout << i << ", ";
    std::cout << ")";
  }
  const std::string &getName() const { return Name; }
  int nargs() { return Args.size(); }
  llvm::Function *codegen();
};

class FunctionScope {
public:
  // scope of function local
  std::map<std::string, llvm::AllocaInst *> NamedValues;
  llvm::Function *TheFunction;
  FunctionScope() {}
};

/// FunctionAST - This class represents a function definition itself.
class FunctionAST : public ExprAST {
  std::unique_ptr<PrototypeAST> Proto;
  std::vector<std::unique_ptr<ExprAST>> Body;
  FunctionScope Scope;
  std::string name;

public:
  FunctionAST(std::unique_ptr<PrototypeAST> Proto,
              std::vector<std::unique_ptr<ExprAST>> Body)
    : Proto(std::move(Proto)), Body(std::move(Body)), Scope() {}

  bool isaFunction() override { return true; }
  void print() override { 
    std::cout << "(function "; 
    Proto->print(); std::cout << ", "; 
    for (auto &e : Body) {
      e->print(); std::cout << ", "; 
    }
    std::cout << ")" << std::endl;
  }
  llvm::Value *codegen() override;

  void registerMe();

  // before generating function def, a few codegen pass have to be invoked
  // including but not limited to:
  //   lambda, let, processing
  //   closure reducing
  //   local var allocation
  //   gc frame setup

  void allocaArgPass(void);
};

/**************************************************************************************************
 *
 * Here starts high level syntax, a.k.a, PseudoAST
 * e.g. function definiton (not yet supported), lambda, let
 * high level AST cannot do codegen, but can be lowered to a low level AST
 *
 **************************************************************************************************/

/// ExprAST - Base class for all expression nodes.
class PseudoAST {
public:
  virtual ~PseudoAST() {}
  virtual void print() {}
  virtual std::unique_ptr<ExprAST> lower() = 0;
};

class CondExprAST: public PseudoAST {
  std::vector<std::unique_ptr<ExprAST>> Preds;
  std::vector<std::unique_ptr<ExprAST>> Exprs;

public:
  CondExprAST(std::vector<std::unique_ptr<ExprAST>> preds,
              std::vector<std::unique_ptr<ExprAST>> exprs)
      : Preds(std::move(preds)), Exprs(std::move(exprs)) {}

  void print() override { 
    std::cout << "(Cond " << "Preds: {"; 
    // Callee->print(); std::cout << ": ";
    for (auto &p : Preds) {
      p->print(); std::cout << ", "; 
    }
    std::cout << "}\n\tExprs: {"; 
    // Callee->print(); std::cout << ": ";
    for (auto &e : Exprs) {
      e->print(); std::cout << ", "; 
    }
    std::cout << "})" << std::endl;
  }
  
  virtual std::unique_ptr<ExprAST> lower() override;
};

std::unique_ptr<ExprAST> LogError(const char *Str);
std::unique_ptr<PrototypeAST> LogErrorP(const char *Str);

void HandleCommand();

class Driver {
public:
  Lexer lex;
  const char *source;
  llvm::IRBuilder<> Builder;
  Token CurTok;
  llvm::LLVMContext TheContext;
  std::unique_ptr<llvm::Module> TheModule;
  std::unique_ptr<llvm::legacy::FunctionPassManager> TheFPM;
  std::unique_ptr<llvm::orc::KaleidoscopeJIT> TheJIT;
  std::map<std::string, std::unique_ptr<PrototypeAST>> FunctionProtos; // global function namespace
  std::vector<std::unique_ptr<FunctionAST>> BufferedFunctions;
  FunctionScope *TheScope;

  Token getNextToken() { return CurTok = lex.getNextToken(); } 

  static Driver *instance(const char *src) {
    if (!_instance)
      _instance = new Driver(src);
    return _instance;
  }

  static Driver *instance() {
    return _instance;
  }

  void Initialize(void);

private:
  Driver(const char *src): lex(src), source(src), Builder(TheContext) {
    // TheModule = llvm::make_unique<llvm::Module>("my cool jit", TheContext);
    TheJIT = llvm::make_unique<llvm::orc::KaleidoscopeJIT>();
  } 

  static Driver *_instance;
};

#endif
