#ifndef RHEO_AST_H
#define RHEO_AST_H

#include "rheo/Diagnostics/SourceLocation.h"
#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Allocator.h>
#include <llvm/Support/StringSaver.h>
#include <locale>
#include <variant>

namespace rheo {

class ASTContext {
  llvm::BumpPtrAllocator Alloc;

public:
  template <typename T, typename... Args> T *create(Args &&...A) {
    void *Mem = Alloc.Allocate(sizeof(T), alignof(T));
    return new (Mem) T(std::forward<Args>(A)...);
  }
};

struct Expr;
struct Type;
struct Stmt;

// -- Expr --

struct IntLiteral {
  std::uint64_t Value;
  explicit IntLiteral(std::uint64_t Value) : Value(Value) {}
};

struct FloatLiteral {
  double Value;
  explicit FloatLiteral(double Value) : Value(Value) {}
};

struct BoolLiteral {
  bool Value;
  explicit BoolLiteral(bool Value) : Value(Value) {}
};

enum UnaryOp : std::uint8_t { Neg, Not };

struct UnaryExpr {
  UnaryOp Op;
  Expr *Operand;
  UnaryExpr(UnaryOp Op, Expr *Operand) : Op(Op), Operand(Operand) {}
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
  Or,
};

struct BinaryExpr {
  BinaryOp Op;
  Expr *Lhs;
  Expr *Rhs;
  BinaryExpr(BinaryOp Op, Expr *Lhs, Expr *Rhs) : Op(Op), Lhs(Lhs), Rhs(Rhs) {}
};

struct CallExpr {
  Expr *Callee;
  llvm::ArrayRef<Expr *> Args;
  CallExpr(Expr *Callee, llvm::ArrayRef<Expr *> Args)
      : Callee(Callee), Args(Args) {}
};

struct VarRef {
  llvm::StringRef Name;
  explicit VarRef(llvm::StringRef Name) : Name(Name) {}
};

struct IndexExpr {
  Expr *Base;
  Expr *Index;
  IndexExpr(Expr *Base, Expr *Index) : Base(Base), Index(Index) {}
};

struct FieldExpr {
  Expr *Base;
  llvm::StringRef Field;
  FieldExpr(Expr *Base, llvm::StringRef Field) : Base(Base), Field(Field) {}
};

struct BlockExpr {
  llvm::ArrayRef<Stmt *> Stmts;
  Expr *Tail; // nullable
  BlockExpr(llvm::ArrayRef<Stmt *> Stmts, Expr *Tail)
      : Stmts(Stmts), Tail(Tail) {}
};

struct IfExpr {
  Expr *Condition;
  BlockExpr ThenBlock;
  Expr *ElseBranch; // nullable
  IfExpr(Expr *Condition, BlockExpr ThenBlock, Expr *ElseBranch)
      : Condition(Condition), ThenBlock(ThenBlock), ElseBranch(ElseBranch) {}
};

struct WhileExpr {
  Expr *Condition;
  BlockExpr Body;

  WhileExpr(Expr *Cond, BlockExpr Body) : Condition(Cond), Body(Body) {}
};

struct BreakExpr {
  Expr *Value;
  explicit BreakExpr(Expr *Value = nullptr) : Value(Value) {}
};

struct ContinueExpr {};

using ExprKind =
    std::variant<IntLiteral, UnaryExpr, BinaryExpr, CallExpr, VarRef,
                 FloatLiteral, BoolLiteral, IndexExpr, FieldExpr, BlockExpr,
                 IfExpr, WhileExpr, BreakExpr, ContinueExpr>;

struct Expr {
  Span Location;
  ExprKind Kind;
};

// -- Statement --
struct ExprStmt {
  Expr *Expr;
  explicit ExprStmt(struct Expr *Expr) : Expr(Expr) {}
};

struct ReturnStmt {
  Expr *Value;
  explicit ReturnStmt(Expr *Value) : Value(Value) {}
};

struct VarDecl {
  llvm::StringRef Name;
  Type *Type; // nullable if inferred
  Expr *Init; // nullable
  bool IsMut;
};

struct AssignStmt {
  Expr *Target;
  Expr *Value;
  AssignStmt(Expr *Target, Expr *Value) : Target(Target), Value(Value) {}
};

using StmtKind = std::variant<ExprStmt, ReturnStmt, VarDecl, AssignStmt>;

struct Stmt {
  Span Location;
  StmtKind Kind;

  Stmt(Span Location, StmtKind Kind)
      : Location(Location), Kind(std::move(Kind)) {}
};

// -- Type --
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
  Never,
};

struct BuiltinType {
  BuiltinKind Kind;
  explicit BuiltinType(BuiltinKind Kind) : Kind(Kind) {}
};

struct NamedType {
  llvm::StringRef Name;
  explicit NamedType(llvm::StringRef Name) : Name(Name) {}
};

struct PtrType {
  Type *Pointee;
  bool IsMut;
  PtrType(Type *Pointee, bool IsMut) : Pointee(Pointee), IsMut(IsMut) {}
};

struct ArrayType {
  Type *Element;
  Expr *Size; // constant expression
  ArrayType(Type *Element, Expr *Size) : Element(Element), Size(Size) {}
};

struct SliceType {
  Type *Element;
  explicit SliceType(Type *Element) : Element(Element) {}
};

struct FnType {
  llvm::ArrayRef<Type *> Params;
  Type *Ret;
  FnType(llvm::ArrayRef<Type *> Params, Type *Ret) : Params(Params), Ret(Ret) {}
};

struct TupleType {
  llvm::ArrayRef<Type *> Elements;
  explicit TupleType(llvm::ArrayRef<Type *> Elements) : Elements(Elements) {}
};

struct OptionalType {
  Type *Inner;
  explicit OptionalType(Type *Inner) : Inner(Inner) {}
};

using TypeKind = std::variant<BuiltinType, NamedType, PtrType, ArrayType,
                              SliceType, FnType, TupleType, OptionalType>;

struct Type {
  Span Location;
  TypeKind Kind;
  Type(Span Location, TypeKind Kind)
      : Location(Location), Kind(std::move(Kind)) {}
};

struct Param {
  llvm::StringRef Name;
  Type *Ty;
}

} // namespace rheo
#endif
