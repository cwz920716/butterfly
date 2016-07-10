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
#define FUNCTIONPROTOS (Driver::instance()->FunctionProtos)

// for now, we only keep one scope, which is the scope of function local
std::map<std::string, llvm::Value *> NamedValues;

llvm::Value *LogErrorV(const char *Str) {
  LogError(Str);
  return nullptr;
}

llvm::Function *getFunction(std::string Name) {
  // First, see if the function has already been added to the current module.
  if (auto *F = MODULE->getFunction(Name))
    return F;

  // If not, check whether we can codegen the declaration from some existing
  // prototype.
  auto FI = FUNCTIONPROTOS.find(Name);
  if (FI != FUNCTIONPROTOS.end())
    return FI->second->codegen();

  // If no existing prototype exists, return null.
  return nullptr;
}

llvm::Value *IntExprAST::codegen() {
  std::cout << "int " << Val << std::endl;
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
  // return LogErrorV("IfExprAST::codegen() not implemented yet.");
  llvm::Value* pred = Pred->codegen();
  if (!pred)
    return LogErrorV("invalid predicate in If.");

  // Convert condition to a bool by comparing equal to 0.
  pred = BUILDER.CreateICmpNE(
                  pred, llvm::ConstantInt::get(LLVM_CONTEXT, llvm::APInt(32, 0, true)), "ifcond");

  llvm::Function *TheFunction = BUILDER.GetInsertBlock()->getParent();

  // Create blocks for the then and else cases.  Insert the 'then' block at the
  // end of the function.
  llvm::BasicBlock *thenBB = llvm::BasicBlock::Create(LLVM_CONTEXT, "then", TheFunction);
  llvm::BasicBlock *elseBB = llvm::BasicBlock::Create(LLVM_CONTEXT, "else");
  llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(LLVM_CONTEXT, "ifcont");

  BUILDER.CreateCondBr(pred, thenBB, elseBB);
  // Emit then value.
  BUILDER.SetInsertPoint(thenBB);
  llvm::Value *thenV = Then->codegen();
  if (!thenV)
    return LogErrorV("invalid then in If.");
  BUILDER.CreateBr(mergeBB);
  // Codegen of 'Then' can change the current block, update ThenBB for the PHI.
  thenBB = BUILDER.GetInsertBlock();

  // Emit else block.
  TheFunction->getBasicBlockList().push_back(elseBB);
  BUILDER.SetInsertPoint(elseBB);
  llvm::Value *elseV = Else->codegen();
  if (!elseV)
    return LogErrorV("invalid else in If.");
  BUILDER.CreateBr(mergeBB);
  // codegen of 'Else' can change the current block, update ElseBB for the PHI.
  elseBB = BUILDER.GetInsertBlock();  

  // Emit merge block.
  TheFunction->getBasicBlockList().push_back(mergeBB);
  BUILDER.SetInsertPoint(mergeBB);
  llvm::PHINode *PN =
    BUILDER.CreatePHI(llvm::Type::getInt32Ty(LLVM_CONTEXT), 2, "phi");

  PN->addIncoming(thenV, thenBB);
  PN->addIncoming(elseV, elseBB);
  return PN;
}

llvm::Value *CallExprAST::codegen() {
  // Look up the name in the global module table.
  llvm::Function *CalleeF = getFunction(Callee);
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
  // Transfer ownership of the prototype to the FunctionProtos map, but keep a
  // reference to it for use below.
  auto &P = *Proto;
  FUNCTIONPROTOS[Proto->getName()] = std::move(Proto);
  // First, check for an existing function from a previous 'extern' declaration.
  llvm::Function *TheFunction = getFunction(P.getName());
  if (!TheFunction)
    return nullptr;

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

    return TheFunction;
  }

  // Error reading body, remove function.
  TheFunction->eraseFromParent();
  return nullptr;
}
