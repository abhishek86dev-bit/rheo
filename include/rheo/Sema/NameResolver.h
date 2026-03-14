#ifndef RHEO_SCOPE_H
#define RHEO_SCOPE_H

#include "rheo/AST/AST.h"
#include "rheo/Diagnostics/DiagnosticEngine.h"
#include "rheo/Diagnostics/SourceLocation.h"
#include <llvm/ADT/StringMap.h>
#include <llvm/ADT/StringRef.h>
namespace rheo {

using SymbolKind = std::variant<FunctionDecl *, VarDecl *>;

struct Symbol {
  Span Location;
  SymbolKind Kind;

  Symbol(Span Location, SymbolKind Kind) : Location(Location), Kind(Kind) {}

  template <typename T> bool is() const {
    return std::holds_alternative<T>(Kind);
  }
  template <typename T> T get() const { return std::get<T>(Kind); }
  template <typename T> T *getIf() const { return std::get_if<T>(&Kind); }
};

using Scope = llvm::StringMap<Symbol>;

struct ScopeGuard {
  llvm::SmallVector<Scope, 8> *Scopes;
  explicit ScopeGuard(llvm::SmallVector<Scope, 8> *Scopes) : Scopes(Scopes) {
    Scopes->emplace_back();
  }
  ~ScopeGuard() { Scopes->pop_back(); }
  ScopeGuard(const ScopeGuard &) = delete;
  ScopeGuard &operator=(const ScopeGuard &) = delete;
};

class NameResolver {
  llvm::SmallVector<Scope, 8> Scopes;
  DiagnosticEngine &Diags;
  FileId File;
  ASTContext &Ctx;

  void declare(llvm::StringRef Name, Symbol S);
  const Symbol *lookup(llvm::StringRef Name) const;

  void errorSymbolRedeclared(llvm::StringRef Name, Span NewSpan, Span OldSpan);
  void errorCalleeUndefined(llvm::StringRef Name, Span CallSpan);
  void errorCalleeNotCallable(llvm::StringRef Name, Span CallSpan,
                              Span DeclSpan);
  void errorCalleeArityMismatch(llvm::StringRef Name, Span CallSpan,
                                Span DeclSpan, size_t Expected, size_t Got);
  void errorCalleeExprNotCallable(Span CallSpan);
  void errorUndefinedDecl(llvm::StringRef Name, Span UseSpan);
  void errorVarRefNotAVariable(llvm::StringRef Name, Span UseSpan,
                               Span DeclSpan);
  void errorAssignToImmutable(llvm::StringRef Name, Span AssignSpan,
                              Span DeclSpan);

  void analyzeStmt(Stmt &S);
  void analyzeExpr(Expr &E);
  void analyzeBlock(BlockExpr &B);

public:
  NameResolver(DiagnosticEngine &Diags, FileId File, ASTContext &Ctx)
      : Diags(Diags), File(File), Ctx(Ctx) {}
  void analyze(Module &M);
};

} // namespace rheo

#endif // !RHEO_SCOPE_H
