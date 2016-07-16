#include <iostream>
#include <string>
#include <string.h>
#include <vector>

#include "common.h"
#include "ast.h"

Driver *Driver::_instance;

void Driver::Initialize() {
  // Open a new module.
  TheModule = llvm::make_unique<llvm::Module>("my cool jit", TheContext);
  TheModule->setDataLayout(TheJIT->getTargetMachine().createDataLayout());

  // Create a new pass manager attached to it.
  TheFPM = llvm::make_unique<llvm::legacy::FunctionPassManager>(TheModule.get());

  // Do simple "peephole" optimizations and bit-twiddling optzns.
  // TheFPM->add(llvm::createInstructionCombiningPass());
  // Reassociate expressions.
  // TheFPM->add(llvm::createReassociatePass());
  // Eliminate Common SubExpressions.
  // TheFPM->add(llvm::createGVNPass());
  // Simplify the control flow graph (deleting unreachable blocks, etc).
  // TheFPM->add(llvm::createCFGSimplificationPass());

  TheFPM->doInitialization();

  init_butterfly_per_module();
}

/// top ::= definition | external | expression | ';'
static void MainLoop() {
  while (true) {
    Token CurTok = Driver::instance()->CurTok;
    switch (CurTok.type) {
    case tok_eof:
      return;
    default:
      HandleCommand();
      break;
    }
  }
}

int main() {
  init_butterfly();
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  llvm::InitializeNativeTargetAsmParser();
  llvm::sys::DynamicLibrary::LoadLibraryPermanently(nullptr);

  const char *test_scm = "(define (square x) (* x x))\n(define (sum-of-squares x y) (+ (square x) (square y)))\n(square 4)\n(sum-of-squares 2 4)\n(if 1 2 3)";
  const char *test_scm2 = "1\n2\n3\n123\n(* 2 3)\n(and 1 2)\n(not 0)\n(not (and (+ 1 2) (* 4 5)))\n(if (> 2 12) 1 -1)";

  // initialize
  Driver *driver = Driver::instance(test_scm);
  driver->Initialize();

  // Prime the first token.
  driver->getNextToken();

  // Run the main "interpreter loop" now.
  MainLoop();

  // Print out all of the generated code.
  // (Driver::instance()->TheModule)->dump();

  return 0;
} 
