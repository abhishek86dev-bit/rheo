#ifndef RHEO_AST_H
#define RHEO_AST_H

#include "rheo/Diagnostics/SourceLocation.h"
#include <cstdint>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Allocator.h>
#include <llvm/Support/StringSaver.h>
#include <variant>

namespace rheo {

struct Expr;
struct Type;
struct Stmt;
struct BlockExpr;
struct FunctionDecl;
struct Module;
struct VarDecl;

class ASTContext {
  llvm::BumpPtrAllocator Alloc;
  llvm::StringSaver Strings{Alloc};

public:
  template <typename T, typename... Args> T *create(Args &&...A) {
    void *Mem = Alloc.Allocate(sizeof(T), alignof(T));
    return new (Mem) T(std::forward<Args>(A)...);
  }

  llvm::StringRef save(llvm::StringRef S) { return Strings.save(S); }

  template <typename T> llvm::ArrayRef<T> copyArray(llvm::ArrayRef<T> Arr) {
    T *Mem = Alloc.Allocate<T>(Arr.size());
    std::uninitialized_copy(Arr.begin(), Arr.end(), Mem);
    return llvm::ArrayRef<T>(Mem, Arr.size());
  }
};

enum class BuiltinKind : std::uint8_t {
  Int,
  I8,
  I16,
  I32,
  I64,
  U8,
  U16,
  U32,
  U64,
  UInt,
  F32,
  F64,
  Bool,
  Unit,
  Never
};

struct BuiltinType {
  BuiltinKind Kind;
};

struct NamedType {
  llvm::StringRef Name;
};

// Filled by HM inference for type variables during unification
struct TypeVar {
  std::uint32_t Id;
};

using TypeKind = std::variant<BuiltinType, NamedType, TypeVar>;

struct Type {
  Span Location;
  TypeKind Kind;
  Type(Span Location, TypeKind Kind) : Location(Location), Kind(Kind) {}
};

// ─────────────────────────────────────────────
//  Expressions
// ─────────────────────────────────────────────

struct IntLiteral {
  std::uint64_t Value;
};

struct FloatLiteral {
  double Value;
};

struct BoolLiteral {
  bool Value;
};

struct UnitLiteral {};

enum UnaryOp : std::uint8_t { Neg, Not, Plus };

struct UnaryExpr {
  UnaryOp Op;
  Expr *Operand;
};

enum BinaryOp : std::uint8_t {
  Add,
  Sub,
  Mul,
  Div,
  Mod,
  Eq,
  NotEq,
  Lt,
  Le,
  Gt,
  Ge,
  And,
  Or
};

struct BinaryExpr {
  BinaryOp Op;
  Expr *Lhs;
  Expr *Rhs;
};

struct CallExpr {
  Expr *Callee;
  llvm::ArrayRef<Expr *> Args;
  FunctionDecl *Resolved = nullptr;
};

struct VarRef {
  llvm::StringRef Name;
  VarDecl *Resolved = nullptr;
};

// BlockExpr is part of ExprKind via pointer; defined fully below.
struct BlockExpr;

struct IfExpr {
  Expr *Condition;
  BlockExpr *ThenBlock;
  BlockExpr *ElseBranch; // nullable
};

struct WhileExpr {
  Expr *Condition;
  BlockExpr *Body;
};

struct BreakExpr {
  Expr *Value; // nullable
};

struct ContinueExpr {};

using ExprKind =
    std::variant<IntLiteral, FloatLiteral, BoolLiteral, UnitLiteral, UnaryExpr,
                 BinaryExpr, CallExpr, VarRef, BlockExpr *, IfExpr, WhileExpr,
                 BreakExpr, ContinueExpr>;

struct Expr {
  Span Location;
  ExprKind Kind;
  Type *Ty = nullptr;
  Expr(Span Location, ExprKind Kind) : Location(Location), Kind(Kind) {}
};

// ─────────────────────────────────────────────
//  Statements
// ─────────────────────────────────────────────

struct ExprStmt {
  Expr *Expr;
};

struct ReturnStmt {
  Expr *Value; // nullable
};

struct VarDecl {
  llvm::StringRef Name;
  Type *Ty;   // nullable if inferred
  Expr *Init; // nullable
  bool IsMut;
};

struct AssignStmt {
  Expr *Target;
  Expr *Value;
};

// FunctionDecl* (pointer) in StmtKind breaks the BlockExpr→Stmt→FunctionDecl
// cycle; the full definition follows below.
using StmtKind =
    std::variant<ExprStmt, ReturnStmt, VarDecl, AssignStmt, FunctionDecl *>;

struct Stmt {
  Span Location;
  StmtKind Kind;
  Stmt(Span Location, StmtKind Kind) : Location(Location), Kind(Kind) {}
};

struct BlockExpr {
  llvm::ArrayRef<Stmt *> Stmts;
  Expr *Tail; // nullable
  BlockExpr(llvm::ArrayRef<Stmt *> Stmts, Expr *Tail)
      : Stmts(Stmts), Tail(Tail) {}
};

struct Param {
  llvm::StringRef Name;
  Type *Ty;
  Span Location;
};

struct FunctionDecl {
  llvm::StringRef Name;
  llvm::ArrayRef<Param> Params;
  Type *ReturnType; // nullable
  BlockExpr *Body;
  FunctionDecl(llvm::StringRef Name, llvm::ArrayRef<Param> Params,
               Type *ReturnType, BlockExpr *Body)
      : Name(Name), Params(Params), ReturnType(ReturnType), Body(Body) {}
};

struct Module {
  llvm::StringRef Name;
  llvm::ArrayRef<Stmt *> Stmts;
};

} // namespace rheo
#endif // RHEO_AST_H
