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
  TheFPM->add(llvm::createInstructionCombiningPass());
  // Promote allocas to registers.
  TheFPM->add(llvm::createPromoteMemoryToRegisterPass());
  // Do simple "peephole" optimizations and bit-twiddling optzns.
  TheFPM->add(llvm::createInstructionCombiningPass());
  // Reassociate expressions.
  TheFPM->add(llvm::createReassociatePass());
  // Eliminate Common SubExpressions.
  TheFPM->add(llvm::createGVNPass());
  // Simplify the control flow graph (deleting unreachable blocks, etc).
  TheFPM->add(llvm::createCFGSimplificationPass());

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

  const char *test_scm = "(define (square x) (* x x))\n(define (sum-of-squares x y) (+ (square x) (square y)))\n(square 4)\n(sum-of-squares 2 4)";
  const char *test_scm2 = "(begin nil 1 (/ 3 2) (and (> 1 0) (< 1 0)) nil)\n";
  const char *test_scm3 = "(define (pos-neg x) ( cond ((= x 0) 1) ((> x 0) (- x)) ))\n(pos-neg -12)\n(pos-neg 102)\n(pos-neg 0)";
  const char *test_scm4 = "(define x 0)\n(define (quad-square x) (define y (* x x)) (* y y))\n(quad-square 3)\n(quad-square -3)";
  const char *test_scm5 = "(define (square-1 x) (define y (- x 1)) (define r y) (set! y (+ x 1)) (set! r (* r y)) r )\n(square-1 1)\n(square-1 2)\n(square-1 3)";

  // initialize
  Driver *driver = Driver::instance(test_scm5);
  driver->Initialize();

  // Prime the first token.
  driver->getNextToken();

  // Run the main "interpreter loop" now.
  MainLoop();

  // Print out all of the generated code.
  // (Driver::instance()->TheModule)->dump();

  return 0;
} 
