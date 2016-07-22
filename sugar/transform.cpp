#include <iostream>
#include <string>
#include <string.h>
#include <vector>
#include <queue>

#include "common.h"
#include "ast.h"

#define FUNCTIONS (Driver::instance()->Functions)

static std::string genClosureSym(std::string fname) {
  static int id = 0;
  static std::string sym = "#";
  return fname + sym + std::to_string(id++);
}

// pre: closures are pre-computed
// post: all other fields
static void scopeDFS(std::string fn, std::map<std::string, FunctionScope *> &scopes) {
  if (scopes[fn]->Closures.size() > 0) {
    for (auto const &child : scopes[fn]->Closures) {
      scopeDFS(child, scopes);
    }
  }

  // after DFS search
  
}

// This pass is to replace innder function def with closure statements
void FunctionAST::declosurePass() {
  // Do a BFS search on closures, relace inner function with closure ast

  std::queue<std::unique_ptr<FunctionAST>> worklist;
  std::map<std::string, std::unique_ptr<FunctionAST>> innerFunctions;
  std::map<std::string, FunctionScope *> scopes;

  // first handle root of closure tree, i.e., the global function
  scopes[Proto->Name] = new FunctionScope;
  for (unsigned i = 0, e = Body.size(); i != e; ++i) {
    if (Body[i]->isaFunction()) {
      // let's release this function from 
      ExprAST *ast_ptr = Body[i].release();
      FunctionAST *fn_ast_ptr = dynamic_cast<FunctionAST *>(ast_ptr);
      auto fn_ast = helper::make_unique<FunctionAST>(std::move(fn_ast_ptr->Proto), std::move(fn_ast_ptr->Body));
      std::string closure_name = fn_ast->Proto->Name;
      std::string closure_fp = genClosureSym(closure_name);
      Body[i] = helper::make_unique<ClosureExprAST>(closure_name, closure_fp);
      fn_ast->Proto->Name = closure_fp;

      // add closure children
      std::cout << "clos " << Proto->Name << " -> " << fn_ast->Proto->Name << std::endl;
      scopes[Proto->Name]->Closures.insert(fn_ast->Proto->Name);

      worklist.push(std::move(fn_ast));
    }
  }
  // collect local use&def names
  for (auto const &Arg : Proto->Args) {
    std::cout << "def " << Arg << std::endl;
    scopes[Proto->Name]->DefinedValues.push_back(Arg);
  }
  for (unsigned i = 0, e = Body.size(); i != e; ++i) {
    if (Body[i]->defType() == 0) {
      // this is a defined var
      std::cout << "def " << Body[i]->defName() << std::endl;
      scopes[Proto->Name]->DefinedValues.push_back(Body[i]->defName());
    }
  }
  
  while (!worklist.empty()) {
    std::unique_ptr<FunctionAST> head = std::move(worklist.front());
    worklist.pop();
    scopes[head->Proto->Name] = new FunctionScope;

    for (unsigned i = 0, e = head->Body.size(); i != e; ++i) {
      if (head->Body[i]->isaFunction()) {
        // let's release this function from 
        ExprAST *ast_ptr = head->Body[i].release();
        FunctionAST *fn_ast_ptr = dynamic_cast<FunctionAST *>(ast_ptr);
        auto fn_ast = helper::make_unique<FunctionAST>(std::move(fn_ast_ptr->Proto), std::move(fn_ast_ptr->Body));
        std::string closure_name = fn_ast->Proto->Name;
        std::string closure_fp = genClosureSym(closure_name);
        head->Body[i] = helper::make_unique<ClosureExprAST>(closure_name, closure_fp);
        fn_ast->Proto->Name = closure_fp;

        // add closure children
        std::cout << "clos " << head->Proto->Name << " -> " << fn_ast->Proto->Name << std::endl;
        scopes[head->Proto->Name]->Closures.insert(fn_ast->Proto->Name);

        worklist.push(std::move(fn_ast));
      }
    }
    head->print();

    // collect local use&def names
    for (auto const &Arg : head->Proto->Args) {
      std::cout << "def " << Arg << std::endl;
      scopes[head->Proto->Name]->DefinedValues.push_back(Arg);
    }
    for (unsigned i = 0, e = head->Body.size(); i != e; ++i) {
      if (head->Body[i]->defType() == 0) {
        // this is a defined var
        std::cout << "def " << head->Body[i]->defName() << std::endl;
        scopes[head->Proto->Name]->DefinedValues.push_back(head->Body[i]->defName());
      }
    }
    
    innerFunctions[head->Proto->Name] = std::move(head);
  }

  // so now that every function is flattented
  // let's collect def/use problem
  

  // print after declosure
  print();
}
