#ifndef _AST_H
#define _AST_H

#include "common.h"

/// ExprAST - Base class for all expression nodes.
class ExprAST {
public:
  virtual ~ExprAST() {}
  virtual void print() {}
};

/// IntExprAST - Expression class for numeric literals like "1.0".
class IntExprAST : public ExprAST {
  int Val;

public:
  IntExprAST(int Val) : Val(Val) {}
  void print() { std::cout << "( Int=" << Val << ")" << std::endl; }
};

/// VariableExprAST - Expression class for referencing a variable, like "a".
class VariableExprAST : public ExprAST {
  std::string Name;

public:
  VariableExprAST(const std::string &Name) : Name(Name) {}
  void print() { std::cout << "( Var=" << Name << ")" << std::endl; }
};

/// BinaryExprAST - Expression class for a binary operator.
class BinaryExprAST : public ExprAST {
  token_type Op;
  std::unique_ptr<ExprAST> LHS, RHS;

public:
  BinaryExprAST(token_type op, std::unique_ptr<ExprAST> LHS,
                std::unique_ptr<ExprAST> RHS)
    : Op(op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}

  void print() { 
    std::cout << "( Op=" << token_desc[Op] << ", "; 
    LHS->print(); std::cout << ", "; 
    RHS->print(); std::cout << ", ";
    std::cout << ")" << std::endl;
  }
};

/// IfExprAST - Expression class for a if statement.
class IfExprAST : public ExprAST {
  std::unique_ptr<ExprAST> Pred, Then, Else;

public:
  IfExprAST(std::unique_ptr<ExprAST> _pred,
            std::unique_ptr<ExprAST> _then, 
            std::unique_ptr<ExprAST> _else)
    : Pred(std::move(_pred)), Then(std::move(_then)), Else(std::move(_else)) {}

  void print() { 
    std::cout << "( If "; 
    Pred->print(); std::cout << ", "; 
    Then->print(); std::cout << ", "; 
    Else->print(); std::cout << ", ";
    std::cout << ")" << std::endl;
  }
};

/// CallExprAST - Expression class for function calls.
class CallExprAST : public ExprAST {
  std::string Callee;
  std::vector<std::unique_ptr<ExprAST>> Args;

public:
  CallExprAST(const std::string &Callee,
              std::vector<std::unique_ptr<ExprAST>> Args)
    : Callee(Callee), Args(std::move(Args)) {}
};

/// PrototypeAST - This class represents the "prototype" for a function,
/// which captures its name, and its argument names (thus implicitly the number
/// of arguments the function takes).
class PrototypeAST {
  std::string Name;
  std::vector<std::string> Args;

public:
  PrototypeAST(const std::string &name, std::vector<std::string> Args)
    : Name(name), Args(std::move(Args)) {}
};

/// FunctionAST - This class represents a function definition itself.
class FunctionAST {
  std::unique_ptr<PrototypeAST> Proto;
  std::unique_ptr<ExprAST> Body;

public:
  FunctionAST(std::unique_ptr<PrototypeAST> Proto,
              std::unique_ptr<ExprAST> Body)
    : Proto(std::move(Proto)), Body(std::move(Body)) {}
};

#endif
