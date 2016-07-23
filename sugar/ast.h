#ifndef _AST_H
#define _AST_H

#include <iostream>
#include "common.h"

// forward declaration
class FunctionAST;
class FunctionScope;
typedef std::map<std::string, FunctionScope *> ScopeMap;

void HandleCommand();

/// ExprAST - Base class for all expression nodes.
class ExprAST {
public:
  virtual ~ExprAST() {}
  virtual void print() {}
  virtual bool isaFunction() { return false; }
  virtual int defType() { return -1; }
  virtual std::string defName() { return std::string(""); }

  virtual void collectUsedNames(std::unordered_set<std::string> &use) {  }
  
  virtual std::unique_ptr<ExprAST> closureTransformationPass(FunctionScope *scope, ScopeMap &smap) { return nullptr; }
};

/// IntExprAST - Expression class for numeric literals like "1.0".
class IntExprAST : public ExprAST {
  int Val;

public:
  IntExprAST(int Val) : Val(Val) {}
  void print() override { std::cout << Val; }
};

/// IntExprAST - Expression class for numeric literals like "1.0".
class NilExprAST : public ExprAST {
public:
  NilExprAST() {}
  void print() override { std::cout << "nil"; }
};

/// VariableExprAST - Expression class for referencing a variable, like "a".
class VariableExprAST : public ExprAST {
  std::string Name;

public:
  VariableExprAST(const std::string &Name) : Name(Name) {}
  void print() override { std::cout << Name; }

  void collectUsedNames(std::unordered_set<std::string> &use) override { use.insert(Name); }

  virtual std::unique_ptr<ExprAST> closureTransformationPass(FunctionScope *scope, ScopeMap &smap) override;
};

/// VarDefinitionExprAST - Expression class for referencing a variable, like "a".
class VarDefinitionExprAST : public ExprAST {
  std::string Name;
  std::unique_ptr<ExprAST> Init;

public:
  VarDefinitionExprAST(const std::string &Name, std::unique_ptr<ExprAST> init) : Name(Name), Init(std::move(init)) {}
  void print() override { 
    std::cout << "(define " << Name << " ";
    Init->print();
    std::cout << ")";
  }
  int defType() override { return 0; }
  std::string defName() override { return Name; }

  void collectUsedNames(std::unordered_set<std::string> &use) override { Init->collectUsedNames(use); }
  virtual std::unique_ptr<ExprAST> closureTransformationPass(FunctionScope *scope, ScopeMap &smap) override;
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

  void collectUsedNames(std::unordered_set<std::string> &use) override {
    use.insert(Name); 
    Expr->collectUsedNames(use); 
  }
  virtual std::unique_ptr<ExprAST> closureTransformationPass(FunctionScope *scope, ScopeMap &smap) override;
};

/// GetFieldExprAST - Expression class for a binary operator.
class GetFieldExprAST : public ExprAST {
  int idx;
  std::unique_ptr<ExprAST> RHS;

public:
  GetFieldExprAST(int idx, std::unique_ptr<ExprAST> RHS)
    : idx(idx), RHS(std::move(RHS)) {}

  void print() override { 
    std::cout << "(getfield " << idx << " ";
    RHS->print();
    std::cout << ")";
  }

  void collectUsedNames(std::unordered_set<std::string> &use) override {
    RHS->collectUsedNames(use); 
  }
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
    std::cout << "(" << token_desc[Op] << " "; 
    LHS->print(); std::cout << " "; 
    RHS->print();
    std::cout << ")";
  }

  void collectUsedNames(std::unordered_set<std::string> &use) override {
    LHS->collectUsedNames(use);
    RHS->collectUsedNames(use); 
  }
  virtual std::unique_ptr<ExprAST> closureTransformationPass(FunctionScope *scope, ScopeMap &smap) override;
};

/// UnaryExprAST - Expression class for a binary operator.
class UnaryExprAST : public ExprAST {
  token_type Op;
  std::unique_ptr<ExprAST> RHS;

public:
  UnaryExprAST(token_type op, std::unique_ptr<ExprAST> RHS)
    : Op(op), RHS(std::move(RHS)) {}

  void print() override { 
    std::cout << "(" << token_desc[Op] << " "; 
    RHS->print();
    std::cout << ")";
  }

  void collectUsedNames(std::unordered_set<std::string> &use) override {
    RHS->collectUsedNames(use); 
  }
  virtual std::unique_ptr<ExprAST> closureTransformationPass(FunctionScope *scope, ScopeMap &smap) override;
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
    std::cout << "(if "; 
    Pred->print(); std::cout << " "; 
    Then->print(); std::cout << " "; 
    Else->print();
    std::cout << ")";
  }

  void collectUsedNames(std::unordered_set<std::string> &use) override {
    Pred->collectUsedNames(use);
    Then->collectUsedNames(use);
    Else->collectUsedNames(use); 
  }
  virtual std::unique_ptr<ExprAST> closureTransformationPass(FunctionScope *scope, ScopeMap &smap) override;
};

class CondExprAST: public ExprAST {
  std::vector<std::unique_ptr<ExprAST>> Preds;
  std::vector<std::unique_ptr<ExprAST>> Exprs;

public:
  CondExprAST(std::vector<std::unique_ptr<ExprAST>> preds,
              std::vector<std::unique_ptr<ExprAST>> exprs)
      : Preds(std::move(preds)), Exprs(std::move(exprs)) {}

  void print() override { 
    std::cout << "(cond "; 
    // Callee->print(); std::cout << ": ";

    for (size_t i = 0; i < Preds.size(); i++) {
      std::cout << "( ";

      Preds[i]->print();
      std::cout << " ";
      Exprs[i]->print();

      std::cout << " ) ";
    }

    std::cout << " )";
  }

  void collectUsedNames(std::unordered_set<std::string> &use) override {
    for (auto &e : Preds) {
      e->collectUsedNames(use);
    } 
    for (auto &e : Exprs) {
      e->collectUsedNames(use);
    } 
  }
  virtual std::unique_ptr<ExprAST> closureTransformationPass(FunctionScope *scope, ScopeMap &smap) override;
};

class BeginExprAST: public ExprAST {
  std::vector<std::unique_ptr<ExprAST>> Exprs;

public:
  BeginExprAST(std::vector<std::unique_ptr<ExprAST>> exprs)
      : Exprs(std::move(exprs)) {}

  void print() override { 
    std::cout << "(begin " << " "; 
    // Callee->print(); std::cout << ": ";
    for (auto &e : Exprs) {
      e->print(); std::cout << " "; 
    }
    std::cout << ")";
  }

  void collectUsedNames(std::unordered_set<std::string> &use) override {
    for (auto &e : Exprs) {
      e->collectUsedNames(use);
    } 
  }
  virtual std::unique_ptr<ExprAST> closureTransformationPass(FunctionScope *scope, ScopeMap &smap) override;
};

/// CallExprAST - Expression class for function calls.
class CallExprAST : public ExprAST {
  std::unique_ptr<ExprAST> Callee; // std::unique_ptr<ExprAST> Callee;
  std::vector<std::unique_ptr<ExprAST>> Args;

public:
/*
  CallExprAST(std::unique_ptr<ExprAST> Callee,
              std::vector<std::unique_ptr<ExprAST>> Args)
    : Callee(std::move(Callee)), Args(std::move(Args)) {}
*/
  CallExprAST(std::unique_ptr<ExprAST> Callee,
              std::vector<std::unique_ptr<ExprAST>> Args)
      : Callee(std::move(Callee)), Args(std::move(Args)) {}

  void print() override { 
    std::cout << "(";
    Callee->print();  
    std::cout << " ";
    // Callee->print(); std::cout << ": ";
    for (auto &Arg : Args) {
      Arg->print(); std::cout << " "; 
    }
    std::cout << ")";
  }

  void collectUsedNames(std::unordered_set<std::string> &use) override {
    Callee->collectUsedNames(use);
    for (auto &e : Args) {
      e->collectUsedNames(use);
    } 
  }
  virtual std::unique_ptr<ExprAST> closureTransformationPass(FunctionScope *scope, ScopeMap &smap) override;
};

/// ClosureExprAST - Expression class for function calls.
class ClosureExprAST : public ExprAST {
public:
  // std::string Name;                                          // original name of inner function 
  std::string FPtr;                                          // name of flattened global function, should be Name#[0-9]*
  // std::vector<std::string> FieldNames; 
  std::vector<std::unique_ptr<ExprAST>> Fields;              // fields of closure, initialized to 0

public:
  ClosureExprAST(const std::string &FPtr)
      : FPtr(std::move(FPtr)) {}

  void print() override { 
    std::cout << "(new-closure " << FPtr; 
    // Callee->print(); std::cout << ": ";
    for (auto &Field : Fields) {
      std::cout << " "; Field->print();
    }
    std::cout << ")";
  }
  virtual std::unique_ptr<ExprAST> closureTransformationPass(FunctionScope *scope, ScopeMap &smap) override;
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
    std::cout << "( " << Name << " "; 
    for (auto &i : Args)
      std::cout << i << " ";
    std::cout << ")";
  }
  const std::string &getName() const { return Name; }
  int nargs() { return Args.size(); }
};

class FunctionScope {
public:
  // function local defined&used var
  std::vector<std::string> DefinedValues;
  std::unordered_set<std::string> UsedValues;

  std::unordered_set<std::string> Closures; // inner function namespace

  // escaped & enclosed var: a var cannot be both escaped and enclosed
  // enclosed var will be pushed to closure-obj
  // escaped var will be boxed and moved to new-closure
  std::unordered_set<std::string> EscapedValues;
  std::vector<std::string> EnclosedValues;

  FunctionScope() {}
};

/// FunctionAST - This class represents a function definition itself.
class FunctionAST : public ExprAST {
  std::unique_ptr<PrototypeAST> Proto;
  std::vector<std::unique_ptr<ExprAST>> Body;
  friend void HandleCommand();

public:
  FunctionAST(std::unique_ptr<PrototypeAST> Proto,
              std::vector<std::unique_ptr<ExprAST>> Body)
    : Proto(std::move(Proto)), Body(std::move(Body)) {}

  bool isaFunction() override { return true; }
  void print() override { 
    std::cout << "(define "; 
    Proto->print(); std::cout << " "; 
    for (auto &e : Body) {
      e->print(); std::cout << " "; 
    }
    std::cout << ")" << std::endl;
  }
  int defType() override { return 1; }
  std::string defName() override { return Proto->Name; }

  void collectUsedNames(std::unordered_set<std::string> &use) override {
    for (auto &e : Body) {
      e->collectUsedNames(use);
    } 
  }

  void closurePass(void);

  virtual std::unique_ptr<ExprAST> closureTransformationPass(FunctionScope *scope, ScopeMap &smap) override;
};

std::unique_ptr<ExprAST> LogError(const char *Str);
std::unique_ptr<PrototypeAST> LogErrorP(const char *Str);

class Driver {
public:
  Lexer lex;
  const char *source;
  Token CurTok;
  std::map<std::string, std::unique_ptr<FunctionAST>> Functions; // global function namespace
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

  bool isaGlobalName(std::string g) {
    return Functions.count(g) > 0;
  }

private:
  Driver(const char *src): lex(src), source(src) {
  } 

  static Driver *_instance;
};

#endif
