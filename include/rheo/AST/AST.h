#ifndef RHEO_AST_H
#define RHEO_AST_H

#include "rheo/Diagnostics/SourceLocation.h"
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

struct IntLiteral {
  std::uint64_t Value;
};

struct FloatLiteral {
  double Value;
};

struct BoolLiteral {
  bool Value;
};

enum UnaryOp : std::uint8_t { Neg, Not };

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
};

struct VarRef {
  llvm::StringRef Name;
};

struct BlockExpr {
  llvm::ArrayRef<Stmt *> Stmts;
  Expr *Tail; // nullable
};

struct IfExpr {
  Expr *Condition;
  BlockExpr *ThenBlock;
  Expr *ElseBranch; // nullable
};

struct WhileExpr {
  Expr *Condition;
  BlockExpr *Body;
};

struct BreakExpr {
  Expr *Value; // nullable
};

struct ContinueExpr {};

using ExprKind = std::variant<IntLiteral, FloatLiteral, BoolLiteral, UnaryExpr,
                              BinaryExpr, CallExpr, VarRef, BlockExpr *, IfExpr,
                              WhileExpr, BreakExpr, ContinueExpr>;

struct Expr {
  Span Location;
  ExprKind Kind;
  Expr(Span Location, ExprKind Kind) : Location(Location), Kind(Kind) {}
};

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

using StmtKind = std::variant<ExprStmt, ReturnStmt, VarDecl, AssignStmt>;

struct Stmt {
  Span Location;
  StmtKind Kind;
  Stmt(Span Location, StmtKind Kind) : Location(Location), Kind(Kind) {}
};

enum class BuiltinKind : std::uint8_t {
  I8,
  I16,
  I32,
  I64,
  U8,
  U16,
  U32,
  U64,
  F32,
  F64,
  Bool,
  Void,
  Never
};

struct BuiltinType {
  BuiltinKind Kind;
};

struct NamedType {
  llvm::StringRef Name;
};

struct TupleType {
  llvm::ArrayRef<Type *> Elements;
};

using TypeKind = std::variant<BuiltinType, NamedType, TupleType>;

struct Type {
  Span Location;
  TypeKind Kind;
};

struct Param {
  llvm::StringRef Name;
  Type *Ty;
};

struct FunctionDecl {
  llvm::StringRef Name;
  llvm::ArrayRef<Param> Params;
  Type *ReturnType; // nullable = void
  BlockExpr *Body;  // nullable for extern
  Span Location;
  FunctionDecl(llvm::StringRef Name, llvm::ArrayRef<Param> Params,
               Type *ReturnType, BlockExpr *Body, Span Location)
      : Name(Name), Params(Params), ReturnType(ReturnType), Body(Body),
        Location(Location) {}
};

struct Module {
  llvm::StringRef Name;
  llvm::ArrayRef<FunctionDecl *> Functions;
};

} // namespace rheo

#endif
