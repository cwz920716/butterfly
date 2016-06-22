#include <iostream>
#include <string>
#include <string.h>
#include <vector>

#include "common.h"
#include "ast.h"

#define LLVM_CONTEXT (Driver::instance()->TheContext)
#define BUILDER (Driver::instance()->Builder)
#define MODULE (Driver::instance()->TheModule)
#define FPM (Driver::instance()->TheFPM)

// for now, we only keep one scope, which is the scope of function local
std::map<std::string, llvm::Value *> NamedValues;

llvm::Value *LogErrorV(const char *Str) {
  LogError(Str);
  return nullptr;
}

llvm::Value *IntExprAST::codegen() {
  return llvm::ConstantInt::get(LLVM_CONTEXT, llvm::APInt(32, Val, true));
}

llvm::Value *VariableExprAST::codegen() {
  // Look this variable up in the function.
  llvm::Value *V = NamedValues[Name];
  if (!V)
    return LogErrorV("Unknown variable name");
  return V;
}

llvm::Value *VarDefinitionExprAST::codegen() {
  return LogErrorV("VarDefinitionExprAST::codegen() not implemented yet.");
}

llvm::Value *BinaryExprAST::codegen() {
  llvm::Value *L = LHS->codegen();
  llvm::Value *R = RHS->codegen();
  if (!L || !R)
    return LogErrorV("Unknown LHS or RHS.");

  switch (Op) {
  case tok_add:
    return BUILDER.CreateAdd(L, R, "addtmp");
  case tok_sub:
    return BUILDER.CreateSub(L, R, "subtmp");
  case tok_mul:
    return BUILDER.CreateMul(L, R, "multmp");
  case tok_div:
    return BUILDER.CreateSDiv(L, R, "multmp");
  default:
    return LogErrorV("invalid binary operator or not implemented yet.");
  }
}

llvm::Value *IfExprAST::codegen() {
  return LogErrorV("IfExprAST::codegen() not implemented yet.");
}

llvm::Value *CallExprAST::codegen() {
  // Look up the name in the global module table.
  llvm::Function *CalleeF = MODULE->getFunction(Callee);
  if (!CalleeF)
    return LogErrorV("Unknown function referenced");

  // If argument mismatch error.
  if (CalleeF->arg_size() != Args.size())
    return LogErrorV("Incorrect # arguments passed");

  std::vector<llvm::Value *> ArgsV;
  for (unsigned i = 0, e = Args.size(); i != e; ++i) {
    ArgsV.push_back(Args[i]->codegen());
    if (!ArgsV.back())
      return nullptr;
  }

  return BUILDER.CreateCall(CalleeF, ArgsV, "calltmp");
}

llvm::Function *PrototypeAST::codegen() {
  // Make the function type:  double(double,double) etc.
  std::vector<llvm::Type *> Int32s(Args.size(), llvm::Type::getInt32Ty(LLVM_CONTEXT));
  llvm::FunctionType *FT =
      llvm::FunctionType::get(llvm::Type::getInt32Ty(LLVM_CONTEXT), Int32s, false);

  llvm::Function *F =
      llvm::Function::Create(FT, llvm::Function::ExternalLinkage, Name, MODULE.get());

  // Set names for all arguments.
  unsigned Idx = 0;
  for (auto &Arg : F->args())
    Arg.setName(Args[Idx++]);

  return F;
}

llvm::Value *FunctionAST::codegen() {
  // First, check for an existing function from a previous 'extern' declaration.
  llvm::Function *TheFunction = MODULE->getFunction(Proto->getName());

  if (!TheFunction)
    TheFunction = Proto->codegen();

  if (!TheFunction)
    return LogErrorV("error when defining function");

  // Create a new basic block to start insertion into.
  llvm::BasicBlock *BB = llvm::BasicBlock::Create(LLVM_CONTEXT, "entry", TheFunction);
  BUILDER.SetInsertPoint(BB);

  // Record the function arguments in the NamedValues map.
  NamedValues.clear();
  for (auto &Arg : TheFunction->args())
    NamedValues[Arg.getName()] = &Arg;

  if (llvm::Value *RetVal = Body->codegen()) {
    // Finish off the function.
    BUILDER.CreateRet(RetVal);
    FPM->run(*TheFunction);

    // Validate the generated code, checking for consistency.
    llvm::verifyFunction(*TheFunction);

    return nullptr;
  }

  // Error reading body, remove function.
  TheFunction->eraseFromParent();
  return nullptr;
}
