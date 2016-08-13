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
#define JIT (Driver::instance()->TheJIT)
#define SCOPE (Driver::instance()->TheScope)
#define FUNCTIONPROTOS (Driver::instance()->FunctionProtos)
#define btpgcstack_var (Driver::instance()->btpgcstack_var)
#define gcframe (Driver::instance()->gcframe)

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
  llvm::Value *LoadV = BUILDER.CreateLoad(V, Name.c_str());

  return LoadV;
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
  std::string bt_unbox_sym("bt_unbox");
  llvm::Function *binOpInt64 = getFunction(bt_binary_int64_sym);
  llvm::Function *box = getFunction(bt_box_sym);
  llvm::Function *unbox = getFunction(bt_unbox_sym);
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
  case tok_unbox:
    ArgsV.push_back( R );
    return BUILDER.CreateCall(unbox, ArgsV, "unboxtmp");
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
  std::string bt_set_box_sym("bt_set_box");
  llvm::Function *binOpInt64 = getFunction(bt_binary_int64_sym);
  llvm::Function *setbox = getFunction(bt_set_box_sym);
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
  case tok_setbox:
    ArgsV.push_back( L );
    ArgsV.push_back( R );
    return BUILDER.CreateCall(setbox, ArgsV, "setboxtmp");
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

llvm::Value *ClosureExprAST::codegen() {
  llvm::Function *CallbackF = getFunction(Callback);
  std::string bt_closure_sym("bt_closure");
  std::vector<llvm::Value *> ArgsV;

  llvm::Value *FP = BUILDER.CreateBitCast(CallbackF, llvm::Type::getInt8PtrTy(LLVM_CONTEXT), "fptr");
  int n = Members.size();

  llvm::ArrayType *AT = llvm::ArrayType::get(llvm::Type::getInt8PtrTy(LLVM_CONTEXT), n);
  llvm::Value *Mems = BUILDER.CreateAlloca(AT);
  // Mems->dump();
  for (int i = 0; i < n; i++) {
    llvm::Value *Ms[] = { llvm::ConstantInt::get(LLVM_CONTEXT, llvm::APInt(32, 0, true)), 
                          llvm::ConstantInt::get(LLVM_CONTEXT, llvm::APInt(32, i, true)) };
    llvm::ArrayRef<llvm::Value *> Ma(Ms);
    auto VI = BUILDER.CreateGEP(Mems, Ma);
    llvm::Value *V = Members[i]->codegen();
    if (!V)
      return LogErrorV("Unknown closure member referenced");
    BUILDER.CreateStore(V, VI);
  }

  // call bt_closure
  llvm::Function *Closure = getFunction(bt_closure_sym);
  ArgsV.push_back(FP);
  ArgsV.push_back(llvm::ConstantInt::get(LLVM_CONTEXT, llvm::APInt(32, n, true)));
  llvm::Value *BI[] = { llvm::ConstantInt::get(LLVM_CONTEXT, llvm::APInt(32, 0, true)), 
                        llvm::ConstantInt::get(LLVM_CONTEXT, llvm::APInt(32, 0, true)) };
  auto MBI = BUILDER.CreateGEP(Mems, BI);
  MBI->dump();
  ArgsV.push_back(MBI);
  return BUILDER.CreateCall(Closure, ArgsV, "clos");
}

llvm::Value *GetFieldExprAST::codegen() {
  std::string bt_getfield_sym("bt_getfield");
  std::vector<llvm::Value *> ArgsV;

  // call bt_getfield
  llvm::Value *CG_Object = Object->codegen();
  llvm::Function *GetField = getFunction(bt_getfield_sym);
  ArgsV.push_back(CG_Object);
  ArgsV.push_back(llvm::ConstantInt::get(LLVM_CONTEXT, llvm::APInt(32, Index, true)));
  return BUILDER.CreateCall(GetField, ArgsV, "getfieldtmp");
}

llvm::Value *CallExprAST::codegen() {
  // Look up the name in the global module table.
  std::string bt_get_callable_sym("bt_get_callable");
  std::string bt_typeof_sym("bt_typeof");
  std::string bt_error_sym("bt_error");
  std::vector<llvm::Value *> ArgsV;

  bool is_static = false;
  llvm::Function *CalleeF = getFunction(Symbol_);
  if (CalleeF) {
    is_static = true;
  }

  if (!is_static) {
    // if not a static dispatch, then test if it is fptr or closure
    llvm::Value *V = Callee->codegen();
    if (!V)
      return LogErrorV("Unknown function referenced");
    llvm::Value *Callable = V;
    // test callable...
    llvm::Function *getCallable = getFunction(bt_get_callable_sym);
    ArgsV.push_back(Callable);
    llvm::Value *maybeFP = BUILDER.CreateCall(getCallable, ArgsV, "callable");
    ArgsV.clear();
    
    llvm::Value *pred = BUILDER.CreateICmpEQ(maybeFP, 
                                             llvm::ConstantPointerNull::get(llvm::Type::getInt8PtrTy(LLVM_CONTEXT)), "calltest");

    llvm::Function *TheFunction = BUILDER.GetInsertBlock()->getParent();
    llvm::BasicBlock *badBB = llvm::BasicBlock::Create(LLVM_CONTEXT, "badcall", TheFunction);
    llvm::BasicBlock *passBB = llvm::BasicBlock::Create(LLVM_CONTEXT, "pass");
    llvm::BasicBlock *fptrBB = llvm::BasicBlock::Create(LLVM_CONTEXT, "functptr");
    llvm::BasicBlock *closBB = llvm::BasicBlock::Create(LLVM_CONTEXT, "closure");
    llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(LLVM_CONTEXT, "merge");

    BUILDER.CreateCondBr(pred, badBB, passBB);
    BUILDER.SetInsertPoint(badBB);
    llvm::Function *error = getFunction(bt_error_sym);
    BUILDER.CreateCall(error, ArgsV, "error");
    ArgsV.clear();
    BUILDER.CreateBr(passBB);

    // Emit pass block.
    TheFunction->getBasicBlockList().push_back(passBB);
    BUILDER.SetInsertPoint(passBB);
    // Now test if it is a functin ptr
    llvm::Function *typeOf = getFunction(bt_typeof_sym);
    ArgsV.push_back(Callable);
    llvm::Value *type = BUILDER.CreateCall(typeOf, ArgsV, "type");
    ArgsV.clear();
    llvm::Value *pred2 = BUILDER.CreateICmpEQ(type,
                                              llvm::ConstantInt::get(LLVM_CONTEXT, llvm::APInt(32, FunctionRefTy, true)), "fptest");

    std::vector<llvm::Value *> ArgValues;
    for (unsigned i = 0, e = Args.size(); i != e; ++i) {
      ArgValues.push_back(Args[i]->codegen());
    }

    BUILDER.CreateCondBr(pred2, fptrBB, closBB);

    TheFunction->getBasicBlockList().push_back(fptrBB);
    BUILDER.SetInsertPoint(fptrBB);
    // Make the function type:  double(double,double) etc.
    std::vector<llvm::Type *> I8Ptrs(Args.size(), llvm::Type::getInt8PtrTy(LLVM_CONTEXT));
    llvm::FunctionType *FT =
        llvm::FunctionType::get(llvm::Type::getInt8PtrTy(LLVM_CONTEXT), I8Ptrs, false);
    llvm::Value *FP = BUILDER.CreateBitCast(maybeFP, llvm::PointerType::get(FT, 0), "fptr");

    for (unsigned i = 0, e = Args.size(); i != e; ++i) {
      ArgsV.push_back(ArgValues[i]);
    }
    llvm::Value *call1 = BUILDER.CreateCall(FP, ArgsV, "calltmp");
    ArgsV.clear();
    BUILDER.CreateBr(mergeBB);

    TheFunction->getBasicBlockList().push_back(closBB);
    BUILDER.SetInsertPoint(closBB);
    std::vector<llvm::Type *> I8Ptrs_Clos(Args.size() + 1, llvm::Type::getInt8PtrTy(LLVM_CONTEXT));
    llvm::FunctionType *FT_Clos =
        llvm::FunctionType::get(llvm::Type::getInt8PtrTy(LLVM_CONTEXT), I8Ptrs_Clos, false);
    llvm::Value *FP_Clos = BUILDER.CreateBitCast(maybeFP, llvm::PointerType::get(FT_Clos, 0), "fptr");
 
    // push the closure object first
    ArgsV.push_back(Callable);
    for (unsigned i = 0, e = Args.size(); i != e; ++i) {
      ArgsV.push_back(ArgValues[i]);
    }
    llvm::Value *call2 = BUILDER.CreateCall(FP_Clos, ArgsV, "calltmp");
    ArgsV.clear();
    BUILDER.CreateBr(mergeBB);

    // Emit merge value
    TheFunction->getBasicBlockList().push_back(mergeBB);
    BUILDER.SetInsertPoint(mergeBB);
    llvm::PHINode *PN =
      BUILDER.CreatePHI(llvm::Type::getInt8PtrTy(LLVM_CONTEXT), 2, "phi");

    PN->addIncoming(call1, fptrBB);
    PN->addIncoming(call2, closBB);
    return PN;
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

  llvm::Type *T_pvalue = llvm::Type::getInt8PtrTy(LLVM_CONTEXT);
  llvm::Type *T_ppvalue = llvm::PointerType::get(T_pvalue, 0);
  llvm::Type *T_int32 = llvm::Type::getInt32Ty(LLVM_CONTEXT);
  llvm::Type *T_int64 = llvm::Type::getInt64Ty(LLVM_CONTEXT);
  llvm::Type *T_size = T_int64;
  llvm::Type *T_psize = llvm::PointerType::get(T_size, 0);

  int n_roots = TheFunction->arg_size();
  llvm::Value *argTemp = BUILDER.CreateAlloca(T_pvalue,
                                      llvm::ConstantInt::get(T_int32, n_roots+2));
  gcframe = argTemp;
  argTemp = BUILDER.CreateConstGEP1_32(argTemp, 2);
  BUILDER.CreateStore(llvm::ConstantInt::get(T_size, n_roots<<1),
                      BUILDER.CreateBitCast(BUILDER.CreateConstGEP1_32(gcframe, 0), T_psize));
  BUILDER.CreateStore(BUILDER.CreateLoad(btpgcstack_var),
                      BUILDER.CreateBitCast(BUILDER.CreateConstGEP1_32(gcframe, 1), 
                                            llvm::PointerType::get(T_ppvalue, 0)));
  BUILDER.CreateStore(gcframe, btpgcstack_var);

  int varnum = 0;
  for (auto &Arg : TheFunction->args()) {
    llvm::Value *arg = &Arg;

    // Create an alloca for this variable.
    auto Alloca = BUILDER.CreateConstGEP1_32(argTemp, varnum++); // CreateEntryBlockAlloca(TheFunction, Arg.getName());
    // Store the initial value into the alloca.
    BUILDER.CreateStore(arg, Alloca);
    // Add arguments to variable symbol table.
    Scope.NamedValues[Arg.getName()] = Alloca;
  }
}

void FunctionAST::registerMe() {
  name = Proto->getName();
  FUNCTIONPROTOS[Proto->getName()] = std::move(Proto);
}

llvm::Value *FunctionAST::codegen() {
  // Transfer ownership of the prototype to the FunctionProtos map, but keep a
  // reference to it for use below.
  // auto &P = *Proto;
  // FUNCTIONPROTOS[Proto->getName()] = std::move(Proto);
  // First, check for an existing function from a previous 'extern' declaration.
  llvm::Function *TheFunction = getFunction(name);
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
    // pop the gc frame
    llvm::Type *T_pvalue = llvm::Type::getInt8PtrTy(LLVM_CONTEXT);
    llvm::Type *T_ppvalue = llvm::PointerType::get(T_pvalue, 0);
    llvm::Value *gcpop = BUILDER.CreateConstGEP1_32(gcframe, 1);
    BUILDER.CreateStore(BUILDER.CreateBitCast(BUILDER.CreateLoad(gcpop, false), T_ppvalue),
                        btpgcstack_var);

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

  std::string bt_typeof_sym("bt_typeof");
  std::string bt_new_int64_sym("bt_new_int64"), num_sym("num");
  std::string bt_new_fptr_sym("bt_new_fptr"), fp_sym("fp"), nargs_sym("nargs");
  std::string bt_box_sym("bt_box");
  std::string bt_unbox_sym("bt_unbox"), box_sym("box");
  std::string bt_set_box_sym("bt_set_box"), new_val_sym("new_val");
  std::string bt_closure_sym("bt_closure"), n_sym("n"), members_sym("members"); 
  std::string bt_getfield_sym("bt_getfield"), object_sym("object"); 
  std::string bt_get_callable_sym("bt_get_callable"), val_sym("val");
  std::string bt_binary_int64_sym("bt_binary_int64"), op_sym("op"), lhs_sym("lhs"), rhs_sym("rhs");
  std::string bt_as_bool_sym("bt_as_bool"), cond_sym("cond");
  std::string bt_error_sym("bt_error");
  llvm::FunctionType *FT = nullptr;
  llvm::Function *F = nullptr;
  unsigned Idx = 0;

  std::vector<std::string> formals_name;
  std::vector<llvm::Type *> formals_type;
  
  // initialize bt_typeof
  formals_name.push_back(val_sym);
  formals_type.push_back( llvm::Type::getInt8PtrTy(LLVM_CONTEXT) );
  FT = llvm::FunctionType::get(llvm::Type::getInt32Ty(LLVM_CONTEXT), formals_type, false);
  F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, bt_typeof_sym, MODULE.get());
  // Set names for all arguments.
  Idx = 0;
  for (auto &Arg : F->args())
    Arg.setName(formals_name[Idx++]);
  // cleanup 
  formals_name.clear();
  formals_type.clear();

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

  // initialize bt_unbox
  formals_name.push_back(box_sym);
  formals_type.push_back( llvm::Type::getInt8PtrTy(LLVM_CONTEXT) );
  FT = llvm::FunctionType::get(llvm::Type::getInt8PtrTy(LLVM_CONTEXT), formals_type, false);
  F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, bt_unbox_sym, MODULE.get());
  // Set names for all arguments.
  Idx = 0;
  for (auto &Arg : F->args())
    Arg.setName(formals_name[Idx++]);
  // cleanup 
  formals_name.clear();
  formals_type.clear();

  // initialize bt_set_box
  formals_name.push_back(box_sym);
  formals_name.push_back(new_val_sym);
  formals_type.push_back( llvm::Type::getInt8PtrTy(LLVM_CONTEXT) );
  formals_type.push_back( llvm::Type::getInt8PtrTy(LLVM_CONTEXT) );
  FT = llvm::FunctionType::get(llvm::Type::getInt8PtrTy(LLVM_CONTEXT), formals_type, false);
  F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, bt_set_box_sym, MODULE.get());
  // Set names for all arguments.
  Idx = 0;
  for (auto &Arg : F->args())
    Arg.setName(formals_name[Idx++]);
  // cleanup 
  formals_name.clear();
  formals_type.clear();

  // initialize bt_closure
  formals_name.push_back(fp_sym);
  formals_name.push_back(n_sym);
  formals_name.push_back(members_sym);
  formals_type.push_back( llvm::Type::getInt8PtrTy(LLVM_CONTEXT) );
  formals_type.push_back( llvm::Type::getInt32Ty(LLVM_CONTEXT) );
  formals_type.push_back( llvm::PointerType::get(llvm::Type::getInt8PtrTy(LLVM_CONTEXT), 0) );
  FT = llvm::FunctionType::get(llvm::Type::getInt8PtrTy(LLVM_CONTEXT), formals_type, false);
  F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, bt_closure_sym, MODULE.get());
  // Set names for all arguments.
  Idx = 0;
  for (auto &Arg : F->args())
    Arg.setName(formals_name[Idx++]);
  // cleanup 
  formals_name.clear();
  formals_type.clear();

  // initialize bt_closure
  formals_name.push_back(object_sym);
  formals_name.push_back(n_sym);
  formals_type.push_back( llvm::Type::getInt8PtrTy(LLVM_CONTEXT) );
  formals_type.push_back( llvm::Type::getInt32Ty(LLVM_CONTEXT) );
  FT = llvm::FunctionType::get(llvm::Type::getInt8PtrTy(LLVM_CONTEXT), formals_type, false);
  F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, bt_getfield_sym, MODULE.get());
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

  llvm::Type *T_pvalue = llvm::Type::getInt8PtrTy(LLVM_CONTEXT);
  llvm::Type *T_ppvalue = llvm::PointerType::get(T_pvalue, 0);
  btpgcstack_var =
        new llvm::GlobalVariable(*MODULE, T_ppvalue,
                           true, llvm::GlobalVariable::ExternalLinkage,
                           NULL, "bt_pgcstack");
  // Should I add global mapping?
  JIT->addGlobalMapping("bt_pgcstack", (char *)&bt_pgcstack);

  return;
}
