#include <iostream>
#include <string>
#include <string.h>
#include <vector>
#include <queue>
#include <algorithm>

#include "common.h"
#include "ast.h"

#define FUNCTIONS (Driver::instance()->Functions)
#define ISA_GLOBAL_NAME (Driver::instance()->isaGlobalName)

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
      for (auto const &clz : scopes[child]->EnclosedValues) {
        scopes[fn]->EscapedValues.insert(clz);
      }
    }
  }

  // after DFS search
  // all escaped var are collected, push back to used var
  for (auto const &esc : scopes[fn]->EscapedValues) {
    scopes[fn]->UsedValues.insert(esc);
  }
/*
  std::cout << "---" << fn << "---" << std::endl;
  for (auto &n : scopes[fn]->DefinedValues) {
    std::cout << n << ",";
  }
  std::cout << std::endl;
  for (auto &n : scopes[fn]->UsedValues) {
    std::cout << n << ",";
  }
  std::cout << std::endl;
*/
  // an enclosed var is a used but non-defined & non-global var
  // an enclosed var must be contained in obj
  for (auto &n : scopes[fn]->UsedValues) {
    auto &v = scopes[fn]->DefinedValues;
    if ( !ISA_GLOBAL_NAME(n) &&
           std::find(v.begin(), v.end(), n) == v.end() ) {
      scopes[fn]->EnclosedValues.push_back(n);
    }
  }
/*78j
  for (auto &n : scopes[fn]->EscapedValues) {
    std::cout << n << ",";
  }
  std::cout << std::endl;
  for (auto &n : scopes[fn]->EnclosedValues) {
    std::cout << n << ",";
  }
  std::cout << std::endl;
*/
}

// This pass is to replace innder function def with closure statements
void FunctionAST::closurePass() {
  // Do a BFS search on closures, relace inner function with closure ast

  std::queue<FunctionAST *> worklist;
  std::map<std::string, FunctionAST *> innerFunctions;
  std::map<std::string, FunctionScope *> scopes;

  // first handle root of closure tree, i.e., the global function
  scopes[Proto->Name] = new FunctionScope;
  for (unsigned i = 0, e = Body.size(); i != e; ++i) {
    if (Body[i]->isaFunction()) {
      // let's release this function from 
      ExprAST *ast_ptr = Body[i];
      FunctionAST *fn_ast = dynamic_cast<FunctionAST *>(ast_ptr);
      std::string closure_name = fn_ast->Proto->Name;
      std::string closure_fp = genClosureSym(closure_name);
      ExprAST *closure = new ClosureExprAST(closure_fp);
      Body[i] = new VarDefinitionExprAST(closure_name, closure);
      fn_ast->Proto->Name = closure_fp;
      fn_ast->Proto->Args.insert(fn_ast->Proto->Args.begin(), std::string("_obj"));

      // add closure children
      // std::cout << "clos " << Proto->Name << " -> " << fn_ast->Proto->Name << std::endl;
      scopes[Proto->Name]->Closures.insert(fn_ast->Proto->Name);

      worklist.push(fn_ast);
    }
  }
  // collect local use&def names
  scopes[Proto->Name]->DefinedValues.push_back(Proto->Name);
  for (auto const &Arg : Proto->Args) {
    // std::cout << "def " << Arg << std::endl;
    scopes[Proto->Name]->DefinedValues.push_back(Arg);
  }
  for (unsigned i = 0, e = Body.size(); i != e; ++i) {
    if (Body[i]->defType() == 0) {
      // this is a defined var
      // std::cout << "def " << Body[i]->defName() << std::endl;
      scopes[Proto->Name]->DefinedValues.push_back(Body[i]->defName());
    }
  }
  collectUsedNames(scopes[Proto->Name]->UsedValues);
  
  while (!worklist.empty()) {
    FunctionAST *head = worklist.front();
    worklist.pop();
    scopes[head->Proto->Name] = new FunctionScope;

    for (unsigned i = 0, e = head->Body.size(); i != e; ++i) {
      if (head->Body[i]->isaFunction()) {
        // let's release this function from 
        ExprAST *ast_ptr = head->Body[i];
        FunctionAST *fn_ast = dynamic_cast<FunctionAST *>(ast_ptr);
        std::string closure_name = fn_ast->Proto->Name;
        std::string closure_fp = genClosureSym(closure_name);
        ExprAST *closure = new ClosureExprAST(closure_fp);
        head->Body[i] = new VarDefinitionExprAST(closure_name, closure);
        fn_ast->Proto->Name = closure_fp;
        fn_ast->Proto->Args.insert(fn_ast->Proto->Args.begin(), std::string("_obj"));

        // add closure children
        std::cout << "clos " << head->Proto->Name << " -> " << fn_ast->Proto->Name << std::endl;
        scopes[head->Proto->Name]->Closures.insert(fn_ast->Proto->Name);

        worklist.push(fn_ast);
      }
    }
    // head->print();

    // collect local use&def names
    scopes[head->Proto->Name]->DefinedValues.push_back(head->Proto->Name);
    for (auto const &Arg : head->Proto->Args) {
      // std::cout << "def " << Arg << std::endl;
      scopes[head->Proto->Name]->DefinedValues.push_back(Arg);
    }
    for (unsigned i = 0, e = head->Body.size(); i != e; ++i) {
      if (head->Body[i]->defType() == 0) {
        // this is a defined var
        // std::cout << "def " << head->Body[i]->defName() << std::endl;
        scopes[head->Proto->Name]->DefinedValues.push_back(head->Body[i]->defName());
      }
    }
    head->collectUsedNames(scopes[head->Proto->Name]->UsedValues);
    
    innerFunctions[head->Proto->Name] = head;
  }

  // so now that every function is flattented
  // let's collect def/use problem
  scopeDFS(Proto->Name, scopes);

  for(auto const &fentry : innerFunctions) {
    std::string fname = fentry.first;
    innerFunctions[fname]->closureTransformationPass(scopes[fname], scopes);
    // innerFunctions[fname]->print();
    FUNCTIONS[fname] = fentry.second;
  }

  // print before closureTransformationPas
  // print();

  closureTransformationPass(scopes[Proto->Name], scopes);
  // print after closureTransformationPas 
  // print();

  
}

ExprAST *
VariableExprAST::closureTransformationPass(FunctionScope *scope, ScopeMap &smap) {
  auto escape = scope->EscapedValues.find(Name);
  auto enclose = find(scope->EnclosedValues.begin(), scope->EnclosedValues.end(), Name);

  if ( escape != scope->EscapedValues.end() ) {
    ExprAST *var = new VariableExprAST(Name);
    ExprAST *unbox = new UnaryExprAST(tok_unbox, var);
    return unbox;
  } else if ( enclose != scope->EnclosedValues.end() ) {
    int pos = enclose - scope->EnclosedValues.begin();
    ExprAST *target = new VariableExprAST(std::string("_obj"));
    ExprAST *var = new GetFieldExprAST(pos + 1, target);
    ExprAST *unbox = new UnaryExprAST(tok_unbox, var);
    return unbox;
  }
  
  return nullptr;
}

ExprAST *
VarDefinitionExprAST::closureTransformationPass(FunctionScope *scope, ScopeMap &smap) {
  auto new_init = Init->closureTransformationPass(scope, smap);
  if (new_init) {
    delete Init;
    Init = new_init;
  }

  auto escape = scope->EscapedValues.find(Name);
  // a defined var can't be enclosed: auto enclose = find(scope->EnclosedValues.begin(), scope->EnclosedValues.end(), Name);
  if ( escape != scope->EscapedValues.end() ) {
    ExprAST *box = new UnaryExprAST(tok_box, Init);
    Init = box;
  }

  return nullptr;
}

ExprAST *
ClosureExprAST::closureTransformationPass(FunctionScope *scope, ScopeMap &smap) {
  FunctionScope *target = smap[FPtr];
  
  // first push a few fields in
  for (auto farg : target->EnclosedValues) {
    auto enclose = find(scope->EnclosedValues.begin(), scope->EnclosedValues.end(), farg);
    if (enclose == scope->EnclosedValues.end()) {
      // this is not a enclosed var
      ExprAST *farg_var = new VariableExprAST(farg);
      Fields.push_back(farg_var);
    } else {
      int pos = enclose - scope->EnclosedValues.begin();
      ExprAST *target = new VariableExprAST(std::string("_obj"));
      ExprAST *farg_var = new GetFieldExprAST(pos + 1, target);
      Fields.push_back(farg_var);
    }
  }

  return nullptr;
}

ExprAST *
CallExprAST::closureTransformationPass(FunctionScope *scope, ScopeMap &smap) {
  auto new_callee = Callee->closureTransformationPass(scope, smap);
  if (new_callee) {
    delete Callee;
    Callee = new_callee;
  }
  
  // first push a few fields in
  for (unsigned i = 0, e = Args.size(); i != e; ++i) {
    auto new_arg = Args[i]->closureTransformationPass(scope, smap);
    if (new_arg) {
      delete Args[i];
      Args[i] = new_arg;
    }
  }

  return nullptr;
}

ExprAST *
BinaryExprAST::closureTransformationPass(FunctionScope *scope, ScopeMap &smap) {
  auto new_lhs = LHS->closureTransformationPass(scope, smap);
  if (new_lhs) {
    delete LHS;
    LHS = new_lhs;
  }
  
  auto new_rhs = RHS->closureTransformationPass(scope, smap);
  if (new_rhs) {
    delete RHS;
    RHS = new_rhs;
  }

  return nullptr;
}

ExprAST *
IfExprAST::closureTransformationPass(FunctionScope *scope, ScopeMap &smap) {
  auto new_pred = Pred->closureTransformationPass(scope, smap);
  if (new_pred) {
    delete Pred;
    Pred = new_pred;
  }
  auto new_then = Then->closureTransformationPass(scope, smap);
  if (new_then) {
    delete Then;
    Then = new_then;
  }
  auto new_else = Else->closureTransformationPass(scope, smap);
  if (new_else) {
    delete Else;
    Else = new_else;
  }

  return nullptr;
}

ExprAST *
UnaryExprAST::closureTransformationPass(FunctionScope *scope, ScopeMap &smap) {
  auto new_rhs = RHS->closureTransformationPass(scope, smap);
  if (new_rhs) {
    delete RHS;
    RHS = new_rhs;
  }

  return nullptr;
}

ExprAST *
BeginExprAST::closureTransformationPass(FunctionScope *scope, ScopeMap &smap) {
  // first push a few fields in
  for (unsigned i = 0, e = Exprs.size(); i != e; ++i) {
    auto new_expr = Exprs[i]->closureTransformationPass(scope, smap);
    if (new_expr) {
      delete Exprs[i];
      Exprs[i] = new_expr;
    }
  }

  return nullptr;
}

ExprAST *
CondExprAST::closureTransformationPass(FunctionScope *scope, ScopeMap &smap) {
  // first push a few fields in
  for (unsigned i = 0, e = Preds.size(); i != e; ++i) {
    auto new_pred = Preds[i]->closureTransformationPass(scope, smap);
    if (new_pred) {
      delete Preds[i];
      Preds[i] = new_pred;
    }
  }
  for (unsigned i = 0, e = Exprs.size(); i != e; ++i) {
    auto new_expr = Exprs[i]->closureTransformationPass(scope, smap);
    if (new_expr) {
      delete Exprs[i];
      Exprs[i] = new_expr;
    }
  }

  return nullptr;
}

ExprAST *
VarSetExprAST::closureTransformationPass(FunctionScope *scope, ScopeMap &smap) {
  auto new_expr = Expr->closureTransformationPass(scope, smap);
  if (new_expr) {
    delete Expr;
    Expr = new_expr;
  }

  auto escape = scope->EscapedValues.find(Name);
  auto enclose = find(scope->EnclosedValues.begin(), scope->EnclosedValues.end(), Name);

  if ( escape != scope->EscapedValues.end() ) {
    ExprAST *var = new VariableExprAST(Name);
    ExprAST *setbox = new BinaryExprAST(tok_setbox, var, Expr);
    return setbox;
  } else if ( enclose != scope->EnclosedValues.end() ) {
    int pos = enclose - scope->EnclosedValues.begin();
    ExprAST *target = new VariableExprAST(std::string("_obj"));
    ExprAST *var = new GetFieldExprAST(pos + 1, target);
    ExprAST *setbox = new BinaryExprAST(tok_setbox, var, Expr);
    return setbox;
  }
  
  return nullptr;
}

ExprAST *
FunctionAST::closureTransformationPass(FunctionScope *scope, ScopeMap &smap) {
  for (unsigned i = 0, e = Body.size(); i != e; ++i) {
    // std::cout << "before: ";
    // Body[i]->print();
    // std::cout << std::endl;
    auto new_body_expr = Body[i]->closureTransformationPass(scope, smap);
    if (new_body_expr) {
      // new_body_expr->print();
      delete Body[i];
      Body[i] = new_body_expr;
    }
    // std::cout << "after: ";
    // Body[i]->print();
    // std::cout << std::endl;
  }

  return nullptr;
}
