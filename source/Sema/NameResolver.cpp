#include "rheo/Sema/NameResolver.h"
#include "rheo/AST/AST.h"
#include "rheo/Common.h"
#include <format>
#include <variant>

namespace rheo {

void NameResolver::declare(llvm::StringRef Name, Symbol S) {
  Scopes.back().insert_or_assign(Name, S);
}

const Symbol *NameResolver::lookup(llvm::StringRef Name) const {
  for (auto It = Scopes.rbegin(); It != Scopes.rend(); ++It) {
    auto Found = It->find(Name);
    if (Found != It->end())
      return &Found->second;
  }
  return nullptr;
}

void NameResolver::errorSymbolRedeclared(llvm::StringRef Name, Span NewSpan,
                                         Span OldSpan) {
  Diagnostic Diag(Severity::Error);
  Diag.setMessage(std::format("symbol '{}' redeclared", Name.str()));
  Diag.setCode("E2001");

  Diag.addLabel(Label::primary(NewSpan, File, "redeclaration occurs here"));

  Diag.addLabel(
      Label::secondary(OldSpan, File, "previous declaration is here"));

  Diag.setHelp("rename the symbol or remove the previous declaration");

  Diags.emit(Diag);
}

void NameResolver::errorCalleeUndefined(llvm::StringRef Name, Span CallSpan) {
  Diagnostic Diag(Severity::Error);
  Diag.setMessage(std::format("undefined function '{}'", Name.str()));
  Diag.setCode("E2002");
  Diag.addLabel(Label::primary(CallSpan, File, "not found in this scope"));
  Diag.setHelp("ensure the function is declared before this call");
  Diags.emit(Diag);
}

void NameResolver::errorCalleeNotCallable(llvm::StringRef Name, Span CallSpan,
                                          Span DeclSpan) {
  Diagnostic Diag(Severity::Error);
  Diag.setMessage(std::format("'{}' is not a function", Name.str()));
  Diag.setCode("E2003");
  Diag.addLabel(Label::primary(CallSpan, File, "called here"));
  Diag.addLabel(
      Label::secondary(DeclSpan, File, "declared as non-function here"));
  Diag.setHelp("only functions and closures can be called");
  Diags.emit(Diag);
}

void NameResolver::errorCalleeArityMismatch(llvm::StringRef Name, Span CallSpan,
                                            Span DeclSpan, size_t Expected,
                                            size_t Got) {
  Diagnostic Diag(Severity::Error);
  Diag.setMessage(std::format("'{}' expects {} argument{}, got {}", Name.str(),
                              Expected, Expected == 1 ? "" : "s", Got));
  Diag.setCode("E2004");
  Diag.addLabel(Label::primary(
      CallSpan, File,
      std::format("{} argument{} provided", Got, Got == 1 ? "" : "s")));
  Diag.addLabel(
      Label::secondary(DeclSpan, File,
                       std::format("defined with {} parameter{}", Expected,
                                   Expected == 1 ? "" : "s")));
  Diag.setHelp("check the function signature and adjust the call");
  Diags.emit(Diag);
}

void NameResolver::errorCalleeExprNotCallable(Span CallSpan) {
  Diagnostic Diag(Severity::Error);
  Diag.setMessage("expression is not callable");
  Diag.setCode("E2005");
  Diag.addLabel(
      Label::primary(CallSpan, File, "this expression cannot be called"));
  Diag.setHelp("only functions and closures can be called with '()'");
  Diags.emit(Diag);
}

void NameResolver::errorUndefinedDecl(llvm::StringRef Name, Span UseSpan) {
  Diagnostic Diag(Severity::Error);
  Diag.setMessage(std::format("undefined symbol '{}'", Name.str()));
  Diag.setCode("E2006");
  Diag.addLabel(Label::primary(UseSpan, File, "not found in this scope"));
  Diag.setHelp(std::format("ensure '{}' is declared before use", Name));
  Diags.emit(Diag);
}

void NameResolver::errorVarRefNotAVariable(llvm::StringRef Name, Span UseSpan,
                                           Span DeclSpan) {
  Diagnostic Diag(Severity::Error);
  Diag.setMessage(std::format("'{}' is not a variable", Name.str()));
  Diag.setCode("E2007");
  Diag.addLabel(Label::primary(UseSpan, File, "used as a variable here"));
  Diag.addLabel(
      Label::secondary(DeclSpan, File, "declared as non-variable here"));
  Diag.setHelp(
      std::format("'{}' refers to a function or type, not a variable", Name));
  Diags.emit(Diag);
}

void NameResolver::errorAssignToImmutable(llvm::StringRef Name, Span AssignSpan,
                                          Span DeclSpan) {
  Diagnostic Diag(Severity::Error);
  Diag.setMessage(
      std::format("cannot assign to immutable variable '{}'", Name.str()));
  Diag.setCode("E2008");
  Diag.addLabel(Label::primary(AssignSpan, File, "assignment here"));
  Diag.addLabel(Label::secondary(DeclSpan, File, "declared without '~' here"));
  Diag.setHelp(
      std::format("declare '{}' as mutable with '~{} := ...'", Name, Name));
  Diags.emit(Diag);
}

void NameResolver::analyzeBlock(BlockExpr &B) {
  ScopeGuard BSCope(&Scopes);
  for (auto *S : B.Stmts) {
    if (!std::holds_alternative<FunctionDecl *>(S->Kind))
      continue;
    auto *Fn = std::get<FunctionDecl *>(S->Kind);
    declare(Fn->Name, Symbol(S->Location, Fn));
  }
  for (auto *S : B.Stmts) {
    analyzeStmt(*S);
  }
  if (B.Tail)
    analyzeExpr(*B.Tail);
}

void NameResolver::analyzeExpr(Expr &E) {
  std::visit(
      Overloaded{[&](UnaryExpr &Node) { analyzeExpr(*Node.Operand); },

                 [&](BinaryExpr &Node) {
                   analyzeExpr(*Node.Lhs);
                   analyzeExpr(*Node.Rhs);
                 },

                 [&](CallExpr &Node) {
                   if (!std::holds_alternative<VarRef>(Node.Callee->Kind))
                     return errorCalleeExprNotCallable(Node.Callee->Location);
                   auto VRef = std::get<VarRef>(Node.Callee->Kind);
                   auto *Sym = lookup(VRef.Name);
                   if (!Sym)
                     return errorCalleeUndefined(VRef.Name,
                                                 Node.Callee->Location);
                   if (!Sym->is<FunctionDecl *>())
                     return errorCalleeNotCallable(
                         VRef.Name, Node.Callee->Location, Sym->Location);
                   auto *FDecl = Sym->get<FunctionDecl *>();
                   if (FDecl->Params.size() != Node.Args.size())
                     return errorCalleeArityMismatch(
                         VRef.Name, E.Location, Sym->Location,
                         FDecl->Params.size(), Node.Args.size());
                   for (auto *Arg : Node.Args)
                     analyzeExpr(*Arg);
                   Node.Resolved = FDecl;
                 },

                 [&](VarRef &Node) {
                   auto *Sym = lookup(Node.Name);
                   if (!Sym)
                     return errorUndefinedDecl(Node.Name, E.Location);
                   if (!Sym->is<VarDecl *>())
                     return errorVarRefNotAVariable(Node.Name, E.Location,
                                                    Sym->Location);
                   Node.Resolved = Sym->get<VarDecl *>();
                 },

                 [&](IfExpr &Node) {
                   analyzeExpr(*Node.Condition);
                   analyzeBlock(*Node.ThenBlock);
                   if (Node.ElseBranch)
                     analyzeBlock(*Node.ElseBranch);
                 },

                 [&](WhileExpr &Node) {
                   analyzeExpr(*Node.Condition);
                   analyzeBlock(*Node.Body);
                 },

                 [&](BlockExpr &Node) { analyzeBlock(Node); },

                 [&](BreakExpr &Node) {
                   if (Node.Value)
                     analyzeExpr(*Node.Value);
                 },

                 [](auto &&) { return; }},
      E.Kind);
}

void NameResolver::analyzeStmt(Stmt &S) {
  std::visit(
      Overloaded{
          [&](ExprStmt &Node) { analyzeExpr(*Node.Expr); },

          [&](ReturnStmt &Node) {
            if (Node.Value)
              analyzeExpr(*Node.Value);
          },

          [&](VarDecl &Node) {
            if (Node.Init)
              analyzeExpr(*Node.Init);
            declare(Node.Name, Symbol(S.Location, &Node));
          },

          [&](AssignStmt &Node) {
            if (!std::holds_alternative<VarRef>(Node.Target->Kind)) {
              errorCalleeExprNotCallable(Node.Target->Location);
              return analyzeExpr(*Node.Value);
            }

            auto &VRef = std::get<VarRef>(Node.Target->Kind);
            auto *Sym = lookup(VRef.Name);
            if (!Sym)
              return errorUndefinedDecl(VRef.Name, Node.Target->Location);
            if (!Sym->is<VarDecl *>())
              return errorVarRefNotAVariable(VRef.Name, Node.Target->Location,
                                             Sym->Location);
            if (!Sym)
              return errorUndefinedDecl(VRef.Name, Node.Target->Location);
            if (!Sym->is<VarDecl *>())
              return errorVarRefNotAVariable(VRef.Name, Node.Target->Location,
                                             Sym->Location);
            auto *Decl = Sym->get<VarDecl *>();
            if (!Decl->IsMut)
              errorAssignToImmutable(VRef.Name, Node.Target->Location,
                                     Sym->Location);
            VRef.Resolved = Decl;
            analyzeExpr(*Node.Value);
          },

          [&](FunctionDecl *Node) {
            ScopeGuard FnScope(&Scopes);
            for (auto &P : Node->Params)
              declare(P.Name,
                      Symbol(P.Location, Ctx.create<VarDecl>(VarDecl{
                                             P.Name, P.Ty, nullptr, false})));
          }},
      S.Kind);
}

void NameResolver::analyze(Module &M) {
  ScopeGuard Global(&Scopes);
  for (auto *S : M.Stmts) {
    if (!std::holds_alternative<FunctionDecl *>(S->Kind))
      continue;
    auto *Fn = std::get<FunctionDecl *>(S->Kind);
    if (auto *Existing = lookup(Fn->Name))
      errorSymbolRedeclared(Fn->Name, S->Location, Existing->Location);
    else
      declare(Fn->Name, Symbol(S->Location, Fn));
  }
  for (auto *S : M.Stmts)
    analyzeStmt(*S);
}

} // namespace rheo
