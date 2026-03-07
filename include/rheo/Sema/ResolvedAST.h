#ifndef RHEO_RESOLVED_AST_H
#define RHEO_RESOLVED_AST_H

#include "rheo/AST/AST.h"
#include "rheo/Sema/Symbol.h"

#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/StringRef.h>
#include <variant>

namespace rheo {

struct ResolvedType;

struct ResolvedBuiltinType {
  BuiltinKind Kind;
};

struct ResolvedNamedType {
  SymbolId Id;
};

struct ResolvedFunctionType {
  llvm::ArrayRef<ResolvedType *> Params;
  ResolvedType *Return;
};

using ResolvedTypeKind =
    std::variant<ResolvedBuiltinType, ResolvedNamedType, ResolvedFunctionType>;

struct ResolvedType {
  ResolvedTypeKind Kind;

  ResolvedType(ResolvedTypeKind Kind) : Kind(Kind) {}
};

struct ResolvedExpr;
struct ResolvedStmt;
struct ResolvedBlockExpr;
struct ResolvedFunctionDecl;

struct ResolvedIntLiteral {
  std::uint64_t Value;
};

struct ResolvedFloatLiteral {
  double Value;
};

struct ResolvedBoolLiteral {
  bool Value;
};

struct ResolvedUnitLiteral {};

struct ResolvedUnaryExpr {
  UnaryOp Op;
  ResolvedExpr *Operand;
};

struct ResolvedBinaryExpr {
  BinaryOp Op;
  ResolvedExpr *LHS;
  ResolvedExpr *RHS;
};

struct ResolvedCallExpr {
  ResolvedExpr *Callee;
  llvm::ArrayRef<ResolvedExpr *> Args;
};

struct ResolvedVarRef {
  SymbolId Id;
};

struct ResolvedBlockExpr {
  llvm::ArrayRef<ResolvedStmt *> Stmts;
  ResolvedExpr *Tail; // nullable

  ResolvedBlockExpr(llvm::ArrayRef<ResolvedStmt *> Stmts, ResolvedExpr *Tail)
      : Stmts(Stmts), Tail(Tail) {}
};

struct ResolvedIfExpr {
  ResolvedExpr *Condition;
  ResolvedBlockExpr *ThenBlock;
  ResolvedBlockExpr *ElseBranch; // nullable
};

struct ResolvedWhileExpr {
  ResolvedExpr *Condition;
  ResolvedBlockExpr *Body;
};

struct ResolvedBreakExpr {
  ResolvedExpr *Value; // nullable
};

struct ResolvedContinueExpr {};

using ResolvedExprKind =
    std::variant<ResolvedIntLiteral, ResolvedFloatLiteral, ResolvedBoolLiteral,
                 ResolvedUnitLiteral, ResolvedUnaryExpr, ResolvedBinaryExpr,
                 ResolvedCallExpr, ResolvedVarRef, ResolvedBlockExpr *,
                 ResolvedIfExpr, ResolvedWhileExpr, ResolvedBreakExpr,
                 ResolvedContinueExpr>;

struct ResolvedExpr {
  Span Location;
  ResolvedType *Ty;
  ResolvedExprKind Kind;

  ResolvedExpr(Span Location, ResolvedType *Ty, ResolvedExprKind Kind)
      : Location(Location), Ty(Ty), Kind(Kind) {}
};

struct ResolvedExprStmt {
  ResolvedExpr *Expr;
};

struct ResolvedReturnStmt {
  ResolvedExpr *Value; // nullable
};

struct ResolvedVarDecl {
  SymbolId Id;
  ResolvedType *Ty;   // nullable if inferred
  ResolvedExpr *Init; // nullable
  bool IsMut;
};

struct ResolvedAssignStmt {
  ResolvedExpr *Target;
  ResolvedExpr *Value;
};

struct ResolvedParam {
  SymbolId Id;
  ResolvedType *Ty;
  Span Location;
};

struct ResolvedFunctionDecl {
  SymbolId Id;
  llvm::ArrayRef<ResolvedParam> Params;
  ResolvedType *ReturnType;
  ResolvedBlockExpr *Body;
  Span Location;
};

using ResolvedStmtKind =
    std::variant<ResolvedExprStmt, ResolvedReturnStmt, ResolvedVarDecl,
                 ResolvedAssignStmt, ResolvedFunctionDecl *>;

struct ResolvedStmt {
  Span Location;
  ResolvedStmtKind Kind;

  ResolvedStmt(Span Location, ResolvedStmtKind Kind)
      : Location(Location), Kind(Kind) {}
};

struct ResolvedModule {
  llvm::StringRef Name;
  llvm::ArrayRef<ResolvedStmt *> Stmts;
};

} // namespace rheo

#endif
