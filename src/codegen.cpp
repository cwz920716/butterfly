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
#define SCOPE (Driver::instance()->TheScope)
#define FUNCTIONPROTOS (Driver::instance()->FunctionProtos)

llvm::Value *LogErrorV(const char *Str) {
  LogError(Str);
  return nullptr;
}

/// CreateEntryBlockAlloca - Create an alloca instruction in the entry block of
/// the function.  This is used for mutable variables etc.
llvm::AllocaInst *CreateEntryBlockAlloca(llvm::Function *TheFunction,
                                   const std::string &VarName) {
  llvm::IRBuilder<> TmpB(&TheFunction->getEntryBlock(),
                   TheFunction->getEntryBlock().begin());
  return TmpB.CreateAlloca(llvm::Type::getInt8PtrTy(LLVM_CONTEXT), nullptr, VarName);
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
  std::string bt_new_int64_sym("bt_new_int64");
  llvm::Function *newInt64 = getFunction(bt_new_int64_sym);
  std::vector<llvm::Value *> ArgsV;
  ArgsV.push_back( llvm::ConstantInt::get(LLVM_CONTEXT, llvm::APInt(32, Val, true)) );
  return BUILDER.CreateCall(newInt64, ArgsV, "inttmp");
}

llvm::Value *NilExprAST::codegen() {
  return llvm::ConstantPointerNull::get(llvm::Type::getInt8PtrTy(LLVM_CONTEXT));
}

llvm::Value *VariableExprAST::codegen() {
  // check if this varaible is global var
  // check if this variable is a function name
  if (FUNCTIONPROTOS.count(Name) > 0) {
    // this is a global function
    auto F = getFunction(Name);
    std::string bt_new_fptr_sym("bt_new_fptr");
    llvm::Value *FP = BUILDER.CreateBitCast(F, llvm::Type::getInt8PtrTy(LLVM_CONTEXT), "fptr");
    llvm::Value *Nargs = llvm::ConstantInt::get(LLVM_CONTEXT, llvm::APInt(32, (int) F->arg_size(), true));
    llvm::Function *newFPtr = getFunction(bt_new_fptr_sym);
    std::vector<llvm::Value *> ArgsV;
    ArgsV.push_back(FP); 
    ArgsV.push_back(Nargs);
    return BUILDER.CreateCall(newFPtr, ArgsV, "fptmp");
  }

  // Look this variable up in the function.
  llvm::Value *V = SCOPE->NamedValues[Name];
  if (!V)
    return LogErrorV("Unknown variable name");

  // Load the value.
  return BUILDER.CreateLoad(V, Name.c_str());
}

llvm::Value *VarDefinitionExprAST::codegen() {
  // VarDef is a alloca inst which will be allocated at entry block, hence here we only return nil
  // normal scheme code will not rely on the return value of defineVar
  llvm::Value *InitVal = Init->codegen();
  if (!InitVal)
    return LogErrorV("Unknown variable initialization");

  llvm::AllocaInst *Alloca = CreateEntryBlockAlloca(SCOPE->TheFunction, Name);
  BUILDER.CreateStore(InitVal, Alloca);
  SCOPE->NamedValues[Name] = Alloca;
  return llvm::ConstantPointerNull::get(llvm::Type::getInt8PtrTy(LLVM_CONTEXT));
}

llvm::Value *VarSetExprAST::codegen() {
  // VarDef is a alloca inst which will be allocated at entry block, hence here we only return nil
  // normal scheme code will not rely on the return value of defineVar
  llvm::Value *Val = Expr->codegen();
  if (!Val)
    return LogErrorV("Unknown variable assignment");

  llvm::Value *Variable = SCOPE->NamedValues[Name];
  BUILDER.CreateStore(Val, Variable);
  return Val;
}

llvm::Value *UnaryExprAST::codegen() {
  llvm::Value *R = RHS->codegen();
  if (!R)
    return LogErrorV("Unknown RHS.");
  std::string bt_binary_int64_sym("bt_binary_int64");
  std::string bt_box_sym("bt_box");
  llvm::Function *binOpInt64 = getFunction(bt_binary_int64_sym);
  llvm::Function *box = getFunction(bt_box_sym);
  std::vector<llvm::Value *> ArgsV;

  switch (Op) {
  case tok_not:
    ArgsV.push_back( llvm::ConstantInt::get(LLVM_CONTEXT, llvm::APInt(32, Op, true)) );
    ArgsV.push_back( R );
    ArgsV.push_back( R );
    return BUILDER.CreateCall(binOpInt64, ArgsV, "boptmp");
  case tok_box:
    ArgsV.push_back( R );
    return BUILDER.CreateCall(box, ArgsV, "boxtmp");
  default:
    return LogErrorV("invalid binary operator or not implemented yet.");
  }
}

llvm::Value *BinaryExprAST::codegen() {
  llvm::Value *L = LHS->codegen();
  llvm::Value *R = RHS->codegen();
  if (!L || !R)
    return LogErrorV("Unknown LHS or RHS.");
  std::string bt_binary_int64_sym("bt_binary_int64");
  llvm::Function *binOpInt64 = getFunction(bt_binary_int64_sym);
  std::vector<llvm::Value *> ArgsV;

  switch (Op) {
  case tok_add:
  case tok_sub:
  case tok_mul:
  case tok_div:
  case tok_eq:
  case tok_gt:
  case tok_lt:
  case tok_and:
  case tok_or:
    ArgsV.push_back( llvm::ConstantInt::get(LLVM_CONTEXT, llvm::APInt(32, Op, true)) );
    ArgsV.push_back( L );
    ArgsV.push_back( R );
    return BUILDER.CreateCall(binOpInt64, ArgsV, "boptmp");
  default:
    return LogErrorV("invalid binary operator or not implemented yet.");
  }
}

llvm::Value *IfExprAST::codegen() {
  // return LogErrorV("IfExprAST::codegen() not implemented yet.");
  std::string bt_as_bool_sym("bt_as_bool");
  llvm::Value* cond = Pred->codegen();
  if (!cond)
    return LogErrorV("invalid predicate in If.");
  llvm::Function *test = getFunction(bt_as_bool_sym);
  std::vector<llvm::Value *> ArgsV;
  ArgsV.push_back( cond );
  llvm::Value* pred = BUILDER.CreateCall(test, ArgsV, "predtmp");

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
    BUILDER.CreatePHI(llvm::Type::getInt8PtrTy(LLVM_CONTEXT), 2, "phi");

  PN->addIncoming(thenV, thenBB);
  PN->addIncoming(elseV, elseBB);
  return PN;
}

llvm::Value *BeginExprAST::codegen() {
  llvm::Value *ret = nullptr;
  for (unsigned i = 0, e = Exprs.size(); i != e; ++i) {
    ret = Exprs[i]->codegen();
    if (!ret)
      return LogErrorV("Invalid begin-clause.");
  }

  if (!ret) return LogErrorV("empty begin-clause.");
  return ret;
}

llvm::Value *CallExprAST::codegen() {
  // Look up the name in the global module table.
  llvm::Function *CalleeF = getFunction(Callee);
  std::string bt_get_callable_sym("bt_get_callable");
  std::string bt_error_sym("bt_error");
  std::vector<llvm::Value *> ArgsV;

  if (!CalleeF) {
    // if not in global module, then test if it is a local var, which could be fptr or closure
    llvm::Value *V = SCOPE->NamedValues[Callee];
    if (!V)
      return LogErrorV("Unknown function referenced");
    llvm::Value *Callable = BUILDER.CreateLoad(V, Callee.c_str());
    // test callable...
    llvm::Function *getCallable = getFunction(bt_get_callable_sym);
    ArgsV.push_back(Callable);
    llvm::Value *maybeFP = BUILDER.CreateCall(getCallable, ArgsV, "fptr");
    ArgsV.clear();
    llvm::Value *pred = BUILDER.CreateICmpEQ(maybeFP, 
                                             llvm::ConstantPointerNull::get(llvm::Type::getInt8PtrTy(LLVM_CONTEXT)), "fptest");

    llvm::Function *TheFunction = BUILDER.GetInsertBlock()->getParent();
    llvm::BasicBlock *badBB = llvm::BasicBlock::Create(LLVM_CONTEXT, "badFP", TheFunction);
    llvm::BasicBlock *afterBB = llvm::BasicBlock::Create(LLVM_CONTEXT, "after");

    BUILDER.CreateCondBr(pred, badBB, afterBB);
    // Emit then value.
    BUILDER.SetInsertPoint(badBB);
    llvm::Function *error = getFunction(bt_error_sym);
    BUILDER.CreateCall(error, ArgsV, "error");
    BUILDER.CreateBr(afterBB);

    // Emit merge block.
    TheFunction->getBasicBlockList().push_back(afterBB);
    BUILDER.SetInsertPoint(afterBB);
    // Make the function type:  double(double,double) etc.
    std::vector<llvm::Type *> I8Ptrs(Args.size(), llvm::Type::getInt8PtrTy(LLVM_CONTEXT));
    llvm::FunctionType *FT =
        llvm::FunctionType::get(llvm::Type::getInt8PtrTy(LLVM_CONTEXT), I8Ptrs, false);
    llvm::Value *FP = BUILDER.CreateBitCast(maybeFP, llvm::PointerType::get(FT, 0), "fptr");

    for (unsigned i = 0, e = Args.size(); i != e; ++i) {
      ArgsV.push_back(Args[i]->codegen());
      if (!ArgsV.back())
        return nullptr;
    }

    return BUILDER.CreateCall(FP, ArgsV, "calltmp");
  } else {
    // If argument mismatch error.
    if (CalleeF->arg_size() != Args.size())
      return LogErrorV("Incorrect # arguments passed");

    for (unsigned i = 0, e = Args.size(); i != e; ++i) {
      ArgsV.push_back(Args[i]->codegen());
      if (!ArgsV.back())
        return nullptr;
    }

    return BUILDER.CreateCall(CalleeF, ArgsV, "calltmp");
  }
}

llvm::Function *PrototypeAST::codegen() {
  // Make the function type:  double(double,double) etc.
  std::vector<llvm::Type *> I8Ptrs(Args.size(), llvm::Type::getInt8PtrTy(LLVM_CONTEXT));
  llvm::FunctionType *FT =
      llvm::FunctionType::get(llvm::Type::getInt8PtrTy(LLVM_CONTEXT), I8Ptrs, false);

  llvm::Function *F =
      llvm::Function::Create(FT, llvm::Function::ExternalLinkage, Name, MODULE.get());

  // Set names for all arguments.
  unsigned Idx = 0;
  for (auto &Arg : F->args())
    Arg.setName(Args[Idx++]);

  return F;
}

void FunctionAST::allocaArgPass() {
  llvm::Function *TheFunction = Scope.TheFunction;

  for (auto &Arg : TheFunction->args()) {
    // Create an alloca for this variable.
    llvm::AllocaInst *Alloca = CreateEntryBlockAlloca(TheFunction, Arg.getName());

    // Store the initial value into the alloca.
    BUILDER.CreateStore(&Arg, Alloca);

    // Add arguments to variable symbol table.
    Scope.NamedValues[Arg.getName()] = Alloca;
  }
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

  // Setup local function scope, make sure it is visible from anywhere
  Scope.TheFunction = TheFunction;
  SCOPE = &Scope;

  // Create a new basic block to start insertion into.
  llvm::BasicBlock *BB = llvm::BasicBlock::Create(LLVM_CONTEXT, "entry", TheFunction);
  BUILDER.SetInsertPoint(BB);

  // Record the function arguments in the NamedValues map.
  allocaArgPass();

  llvm::Value *RetVal = nullptr;
  for (unsigned i = 0, e = Body.size(); i != e; ++i) {
    RetVal = Body[i]->codegen();
    if (!RetVal)
      return LogErrorV("Invalid body expr.");
  }

  if (RetVal) {
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

void init_butterfly_per_module(void) {
  // printf("init start...\n");

  std::string bt_new_int64_sym("bt_new_int64"), num_sym("num");
  std::string bt_new_fptr_sym("bt_new_fptr"), fp_sym("fp"), nargs_sym("nargs");
  std::string bt_box_sym("bt_box");
  std::string bt_get_callable_sym("bt_get_callable"), val_sym("val");
  std::string bt_binary_int64_sym("bt_binary_int64"), op_sym("op"), lhs_sym("lhs"), rhs_sym("rhs");
  std::string bt_as_bool_sym("bt_as_bool"), cond_sym("cond");
  std::string bt_error_sym("bt_error");
  llvm::FunctionType *FT = nullptr;
  llvm::Function *F = nullptr;
  unsigned Idx = 0;

  std::vector<std::string> formals_name;
  std::vector<llvm::Type *> formals_type;

  // initialize bt_new_int64
  formals_name.push_back(num_sym);
  formals_type.push_back( llvm::Type::getInt32Ty(LLVM_CONTEXT) );
  FT = llvm::FunctionType::get(llvm::Type::getInt8PtrTy(LLVM_CONTEXT), formals_type, false);
  F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, bt_new_int64_sym, MODULE.get());
  // Set names for all arguments.
  Idx = 0;
  for (auto &Arg : F->args())
    Arg.setName(formals_name[Idx++]);
  // cleanup 
  formals_name.clear();
  formals_type.clear();

  // initialize bt_new_fptr
  formals_name.push_back(fp_sym);
  formals_name.push_back(nargs_sym);
  formals_type.push_back( llvm::Type::getInt8PtrTy(LLVM_CONTEXT) );
  formals_type.push_back( llvm::Type::getInt32Ty(LLVM_CONTEXT) );
  FT = llvm::FunctionType::get(llvm::Type::getInt8PtrTy(LLVM_CONTEXT), formals_type, false);
  F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, bt_new_fptr_sym, MODULE.get());
  // Set names for all arguments.
  Idx = 0;
  for (auto &Arg : F->args())
    Arg.setName(formals_name[Idx++]);
  // cleanup 
  formals_name.clear();
  formals_type.clear();

  // initialize bt_box
  formals_name.push_back(val_sym);
  formals_type.push_back( llvm::Type::getInt8PtrTy(LLVM_CONTEXT) );
  FT = llvm::FunctionType::get(llvm::Type::getInt8PtrTy(LLVM_CONTEXT), formals_type, false);
  F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, bt_box_sym, MODULE.get());
  // Set names for all arguments.
  Idx = 0;
  for (auto &Arg : F->args())
    Arg.setName(formals_name[Idx++]);
  // cleanup 
  formals_name.clear();
  formals_type.clear();

  // initialize bt_get_callable
  formals_name.push_back(val_sym);
  formals_type.push_back( llvm::Type::getInt8PtrTy(LLVM_CONTEXT) );
  FT = llvm::FunctionType::get(llvm::Type::getInt8PtrTy(LLVM_CONTEXT), formals_type, false);
  F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, bt_get_callable_sym, MODULE.get());
  // Set names for all arguments.
  Idx = 0;
  for (auto &Arg : F->args())
    Arg.setName(formals_name[Idx++]);
  // cleanup 
  formals_name.clear();
  formals_type.clear();

  // initialize bt_binary_int64
  formals_name.push_back(op_sym);
  formals_name.push_back(lhs_sym);
  formals_name.push_back(rhs_sym);
  formals_type.push_back( llvm::Type::getInt32Ty(LLVM_CONTEXT) );
  formals_type.push_back( llvm::Type::getInt8PtrTy(LLVM_CONTEXT) );
  formals_type.push_back( llvm::Type::getInt8PtrTy(LLVM_CONTEXT) );
  FT = llvm::FunctionType::get(llvm::Type::getInt8PtrTy(LLVM_CONTEXT), formals_type, false);
  F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, bt_binary_int64_sym, MODULE.get());
  // Set names for all arguments.
  Idx = 0;
  for (auto &Arg : F->args())
    Arg.setName(formals_name[Idx++]);
  // cleanup 
  formals_name.clear();
  formals_type.clear();

  // initialize bt_new_int64
  formals_name.push_back(cond_sym);
  formals_type.push_back( llvm::Type::getInt8PtrTy(LLVM_CONTEXT) );
  FT = llvm::FunctionType::get(llvm::Type::getInt32Ty(LLVM_CONTEXT), formals_type, false);
  F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, bt_as_bool_sym, MODULE.get());
  // Set names for all arguments.
  Idx = 0;
  for (auto &Arg : F->args())
    Arg.setName(formals_name[Idx++]);
  // cleanup 
  formals_name.clear();
  formals_type.clear();

  // initialize bt_error
  FT = llvm::FunctionType::get(llvm::Type::getInt8PtrTy(LLVM_CONTEXT), formals_type, false);
  F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, bt_error_sym, MODULE.get());

  // printf("init successful!\n");
  // MODULE->dump();

  return;
}
