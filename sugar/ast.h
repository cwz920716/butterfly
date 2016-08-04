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
  
  virtual ExprAST* closureTransformationPass(FunctionScope *scope, ScopeMap &smap) { return nullptr; }
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

  virtual ExprAST* closureTransformationPass(FunctionScope *scope, ScopeMap &smap) override;
};

/// VarDefinitionExprAST - Expression class for referencing a variable, like "a".
class VarDefinitionExprAST : public ExprAST {
  std::string Name;
  ExprAST* Init;

public:
  VarDefinitionExprAST(const std::string &Name, ExprAST* init) : Name(Name), Init(init) {}
  void print() override { 
    std::cout << "(define " << Name << " ";
    Init->print();
    std::cout << ")";
  }
  int defType() override { return 0; }
  std::string defName() override { return Name; }

  void collectUsedNames(std::unordered_set<std::string> &use) override { Init->collectUsedNames(use); }
  virtual ExprAST* closureTransformationPass(FunctionScope *scope, ScopeMap &smap) override;
};

/// VarSetExprAST - Expression class for referencing a variable, like "a".
class VarSetExprAST : public ExprAST {
  std::string Name;
  ExprAST* Expr;

public:
  VarSetExprAST(const std::string &name, ExprAST* expr) : Name(name), Expr(expr) {}
  void print() override { 
    std::cout << "(set! " << Name << " ";
    Expr->print();
    std::cout << ")";
  }

  void collectUsedNames(std::unordered_set<std::string> &use) override {
    use.insert(Name); 
    Expr->collectUsedNames(use); 
  }
  virtual ExprAST* closureTransformationPass(FunctionScope *scope, ScopeMap &smap) override;
};

/// GetFieldExprAST - Expression class for a binary operator.
class GetFieldExprAST : public ExprAST {
  int idx;
  ExprAST* RHS;

public:
  GetFieldExprAST(int idx, ExprAST* RHS)
    : idx(idx), RHS(RHS) {}

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
  ExprAST* LHS, *RHS;

public:
  BinaryExprAST(token_type op, ExprAST* LHS,
                ExprAST* RHS)
    : Op(op), LHS(LHS), RHS(RHS) {}

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
  virtual ExprAST* closureTransformationPass(FunctionScope *scope, ScopeMap &smap) override;
};

/// UnaryExprAST - Expression class for a binary operator.
class UnaryExprAST : public ExprAST {
  token_type Op;
  ExprAST* RHS;

public:
  UnaryExprAST(token_type op, ExprAST* RHS)
    : Op(op), RHS(RHS) {}

  void print() override { 
    std::cout << "(" << token_desc[Op] << " "; 
    RHS->print();
    std::cout << ")";
  }

  void collectUsedNames(std::unordered_set<std::string> &use) override {
    RHS->collectUsedNames(use); 
  }
  virtual ExprAST* closureTransformationPass(FunctionScope *scope, ScopeMap &smap) override;
};

/// IfExprAST - Expression class for a if statement.
class IfExprAST : public ExprAST {
  ExprAST *Pred, *Then, *Else;

public:
  IfExprAST(ExprAST* _pred,
            ExprAST* _then, 
            ExprAST* _else)
    : Pred(_pred), Then(_then), Else(_else) {}

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
  virtual ExprAST* closureTransformationPass(FunctionScope *scope, ScopeMap &smap) override;
};

class CondExprAST: public ExprAST {
  std::vector<ExprAST*> Preds;
  std::vector<ExprAST*> Exprs;

public:
  CondExprAST(std::vector<ExprAST*> preds,
              std::vector<ExprAST*> exprs)
      : Preds(preds), Exprs(exprs) {}

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
  virtual ExprAST* closureTransformationPass(FunctionScope *scope, ScopeMap &smap) override;
};

class BeginExprAST: public ExprAST {
  std::vector<ExprAST*> Exprs;

public:
  BeginExprAST(std::vector<ExprAST*> exprs)
      : Exprs(exprs) {}

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
  virtual ExprAST* closureTransformationPass(FunctionScope *scope, ScopeMap &smap) override;
};

/// CallExprAST - Expression class for function calls.
class CallExprAST : public ExprAST {
  ExprAST* Callee; // ExprAST* Callee;
  std::vector<ExprAST*> Args;

public:
  CallExprAST(ExprAST* Callee,
              std::vector<ExprAST*> Args)
      : Callee(Callee), Args(Args) {}

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
  virtual ExprAST* closureTransformationPass(FunctionScope *scope, ScopeMap &smap) override;
};

/// ClosureExprAST - Expression class for function calls.
class ClosureExprAST : public ExprAST {
public:
  // std::string Name;                                          // original name of inner function 
  std::string FPtr;                                          // name of flattened global function, should be Name#[0-9]*
  // std::vector<std::string> FieldNames; 
  std::vector<ExprAST*> Fields;              // fields of closure, initialized to 0

public:
  ClosureExprAST(const std::string &FPtr)
      : FPtr(FPtr) {}

  void print() override { 
    std::cout << "(closure " << FPtr; 
    // Callee->print(); std::cout << ": ";
    for (auto &Field : Fields) {
      std::cout << " "; Field->print();
    }
    std::cout << ")";
  }
  virtual ExprAST* closureTransformationPass(FunctionScope *scope, ScopeMap &smap) override;
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
    : Name(name), Args(Args) {}

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
  PrototypeAST* Proto;
  std::vector<ExprAST*> Body;
  friend void HandleCommand();

public:
  FunctionAST(PrototypeAST* Proto,
              std::vector<ExprAST*> Body)
    : Proto(Proto), Body(Body) {}

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

  virtual ExprAST* closureTransformationPass(FunctionScope *scope, ScopeMap &smap) override;
};

ExprAST* LogError(const char *Str);
PrototypeAST* LogErrorP(const char *Str);

class Driver {
public:
  Lexer lex;
  const char *source;
  Token CurTok;
  std::map<std::string, FunctionAST*> Functions; // global function namespace
  std::vector<FunctionAST*> Commands;
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
    return Functions.count(g) > 0 
             || g == std::string("abs")
             || g == std::string("square")
             || g == std::string("average");
  }

private:
  Driver(const char *src): lex(src), source(src) {
  } 

  static Driver *_instance;
};

#endif
