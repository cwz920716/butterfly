#ifndef _AST_H
#define _AST_H

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
  std::unique_ptr<ExprAST> Value;

public:
  VarDefinitionExprAST(const std::string &Name, std::unique_ptr<ExprAST> value) : Name(Name), Value(std::move(value)) {}
  void print() override { 
    std::cout << "(var " << Name << " <- ";
    Value->print();
    std::cout << std::endl; 
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

/// CallExprAST - Expression class for function calls.
class CallExprAST : public ExprAST {
  std::string Callee; // std::unique_ptr<ExprAST> Callee;
  std::vector<std::unique_ptr<ExprAST>> Args;

public:
/*
  CallExprAST(std::unique_ptr<ExprAST> Callee,
              std::vector<std::unique_ptr<ExprAST>> Args)
    : Callee(std::move(Callee)), Args(std::move(Args)) {}
*/
  CallExprAST(const std::string &Callee,
              std::vector<std::unique_ptr<ExprAST>> Args)
      : Callee(Callee), Args(std::move(Args)) {}

  void print() override { 
    std::cout << "(apply " << Callee << ": "; 
    // Callee->print(); std::cout << ": ";
    for (auto &Arg : Args) {
      Arg->print(); std::cout << ", "; 
    }
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
  llvm::Function *codegen();
};

/// FunctionAST - This class represents a function definition itself.
class FunctionAST : public ExprAST {
  std::unique_ptr<PrototypeAST> Proto;
  std::unique_ptr<ExprAST> Body;

public:
  FunctionAST(std::unique_ptr<PrototypeAST> Proto,
              std::unique_ptr<ExprAST> Body)
    : Proto(std::move(Proto)), Body(std::move(Body)) {}

  bool isaFunction() override { return true; }
  void print() override { 
    std::cout << "(function "; 
    Proto->print(); std::cout << ", "; 
    Body->print();
    std::cout << ")" << std::endl;
  }
  llvm::Value *codegen() override;
};

std::unique_ptr<ExprAST> LogError(const char *Str);
std::unique_ptr<PrototypeAST> LogErrorP(const char *Str);

void HandleCommand();

#endif
