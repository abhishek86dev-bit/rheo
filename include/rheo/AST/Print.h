#ifndef RHEO_AST_PRINT_H
#define RHEO_AST_PRINT_H

#include "rheo/AST/AST.h"
#include <llvm/Support/raw_ostream.h>

namespace rheo {

class ASTPrinter {
  llvm::raw_ostream &OS;
  int Indent = 0;

  void indent() { OS.indent(Indent * 2); }
  void push() { ++Indent; }
  void pop() { --Indent; }

  llvm::StringRef unaryOpStr(UnaryOp Op) {
    switch (Op) {
    case Neg:
      return "-";
    case Not:
      return "!";
    }
  }

  llvm::StringRef binaryOpStr(BinaryOp Op) {
    switch (Op) {
    case Add:
      return "+";
    case Sub:
      return "-";
    case Mul:
      return "*";
    case Div:
      return "/";
    case Mod:
      return "%";
    case Eq:
      return "==";
    case NotEq:
      return "!=";
    case Lt:
      return "<";
    case Le:
      return "<=";
    case Gt:
      return ">";
    case Ge:
      return ">=";
    case And:
      return "&&";
    case Or:
      return "||";
    }
  }

  llvm::StringRef builtinKindStr(BuiltinKind K) {
    switch (K) {
    case BuiltinKind::I8:
      return "i8";
    case BuiltinKind::I16:
      return "i16";
    case BuiltinKind::I32:
      return "i32";
    case BuiltinKind::I64:
      return "i64";
    case BuiltinKind::U8:
      return "u8";
    case BuiltinKind::U16:
      return "u16";
    case BuiltinKind::U32:
      return "u32";
    case BuiltinKind::U64:
      return "u64";
    case BuiltinKind::F32:
      return "f32";
    case BuiltinKind::F64:
      return "f64";
    case BuiltinKind::Bool:
      return "bool";
    case BuiltinKind::Void:
      return "void";
    case BuiltinKind::Never:
      return "never";
    }
  }

public:
  ASTPrinter(llvm::raw_ostream &OS = llvm::outs()) : OS(OS) {}

  void print(const Module &M) {
    OS << "Module(" << M.Name << ")\n";
    push();
    for (auto *F : M.Functions)
      print(*F);
    pop();
  }

  void print(const FunctionDecl &F) {
    indent();
    OS << "FunctionDecl(" << F.Name << ")\n";
    push();
    for (auto &P : F.Params) {
      indent();
      OS << "Param(" << P.Name << ": ";
      printType(*P.Ty);
      OS << ")\n";
    }
    if (F.ReturnType) {
      indent();
      OS << "ReturnType: ";
      printType(*F.ReturnType);
      OS << "\n";
    }
    if (F.Body)
      printBlockExpr(*F.Body);
    pop();
  }

  void printType(const Type &T) {
    std::visit([&](auto &K) { printTypeKind(K); }, T.Kind);
  }

  void printTypeKind(const BuiltinType &T) { OS << builtinKindStr(T.Kind); }
  void printTypeKind(const NamedType &T) { OS << T.Name; }
  void printTypeKind(const TupleType &T) {
    OS << "(";
    for (size_t I = 0; I < T.Elements.size(); ++I) {
      if (I)
        OS << ", ";
      printType(*T.Elements[I]);
    }
    OS << ")";
  }

  void printExpr(const Expr &E) {
    std::visit([&](auto &K) { printExprKind(K); }, E.Kind);
  }

  void printExprKind(const IntLiteral &E) {
    indent();
    OS << "IntLiteral(" << E.Value << ")\n";
  }
  void printExprKind(const FloatLiteral &E) {
    indent();
    OS << "FloatLiteral(" << E.Value << ")\n";
  }
  void printExprKind(const BoolLiteral &E) {
    indent();
    OS << "BoolLiteral(" << (E.Value ? "true" : "false") << ")\n";
  }
  void printExprKind(const VarRef &E) {
    indent();
    OS << "VarRef(" << E.Name << ")\n";
  }
  void printExprKind(const ContinueExpr &) {
    indent();
    OS << "ContinueExpr\n";
  }
  void printExprKind(const UnaryExpr &E) {
    indent();
    OS << "UnaryExpr(" << unaryOpStr(E.Op) << ")\n";
    push();
    printExpr(*E.Operand);
    pop();
  }
  void printExprKind(const BinaryExpr &E) {
    indent();
    OS << "BinaryExpr(" << binaryOpStr(E.Op) << ")\n";
    push();
    printExpr(*E.Lhs);
    printExpr(*E.Rhs);
    pop();
  }
  void printExprKind(const CallExpr &E) {
    indent();
    OS << "CallExpr\n";
    push();
    indent();
    OS << "Callee:\n";
    push();
    printExpr(*E.Callee);
    pop();
    indent();
    OS << "Args:\n";
    push();
    for (auto *A : E.Args)
      printExpr(*A);
    pop();
    pop();
  }
  void printExprKind(BlockExpr *E) { printBlockExpr(*E); }
  void printExprKind(const IfExpr &E) {
    indent();
    OS << "IfExpr\n";
    push();
    indent();
    OS << "Condition:\n";
    push();
    printExpr(*E.Condition);
    pop();
    indent();
    OS << "Then:\n";
    push();
    printBlockExpr(*E.ThenBlock);
    pop();
    if (E.ElseBranch) {
      indent();
      OS << "Else:\n";
      push();
      printExpr(*E.ElseBranch);
      pop();
    }
    pop();
  }
  void printExprKind(const WhileExpr &E) {
    indent();
    OS << "WhileExpr\n";
    push();
    indent();
    OS << "Condition:\n";
    push();
    printExpr(*E.Condition);
    pop();
    indent();
    OS << "Body:\n";
    push();
    printBlockExpr(*E.Body);
    pop();
    pop();
  }
  void printExprKind(const BreakExpr &E) {
    indent();
    OS << "BreakExpr\n";
    if (E.Value) {
      push();
      printExpr(*E.Value);
      pop();
    }
  }

  void printBlockExpr(const BlockExpr &B) {
    indent();
    OS << "BlockExpr\n";
    push();
    for (auto *S : B.Stmts)
      printStmt(*S);
    if (B.Tail) {
      indent();
      OS << "Tail:\n";
      push();
      printExpr(*B.Tail);
      pop();
    }
    pop();
  }

  void printStmt(const Stmt &S) {
    std::visit([&](auto &K) { printStmtKind(K); }, S.Kind);
  }

  void printStmtKind(const ExprStmt &S) {
    indent();
    OS << "ExprStmt\n";
    push();
    printExpr(*S.Expr);
    pop();
  }
  void printStmtKind(const ReturnStmt &S) {
    indent();
    OS << "ReturnStmt\n";
    if (S.Value) {
      push();
      printExpr(*S.Value);
      pop();
    }
  }
  void printStmtKind(const VarDecl &S) {
    indent();
    OS << "VarDecl(" << (S.IsMut ? "mut " : "") << S.Name;
    if (S.Ty) {
      OS << ": ";
      printType(*S.Ty);
    }
    OS << ")\n";
    if (S.Init) {
      push();
      printExpr(*S.Init);
      pop();
    }
  }
  void printStmtKind(const AssignStmt &S) {
    indent();
    OS << "AssignStmt\n";
    push();
    indent();
    OS << "Target:\n";
    push();
    printExpr(*S.Target);
    pop();
    indent();
    OS << "Value:\n";
    push();
    printExpr(*S.Value);
    pop();
    pop();
  }
};

inline void printAST(const Module &M, llvm::raw_ostream &OS = llvm::outs()) {
  ASTPrinter(OS).print(M);
}
inline void printAST(const FunctionDecl &F,
                     llvm::raw_ostream &OS = llvm::outs()) {
  ASTPrinter(OS).print(F);
}
inline void printAST(const Expr &E, llvm::raw_ostream &OS = llvm::outs()) {
  ASTPrinter(OS).printExpr(E);
}

} // namespace rheo
#endif
