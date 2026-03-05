#ifndef RHEO_AST_PRINT_H
#define RHEO_AST_PRINT_H

#include "rheo/AST/AST.h"
#include <llvm/Support/raw_ostream.h>

namespace rheo {

class ASTPrinter {
  llvm::raw_ostream &OS;
  int Indent = 0;
  const Span *CurrentLoc = nullptr; // set before dispatching into kind printers

  void indent() { OS.indent(Indent * 2); }
  void push() { ++Indent; }
  void pop() { --Indent; }

  void printLoc(const Span &S) {
    OS << " [" << S.getStart() << ":" << S.getEnd() << "]";
  }

  llvm::StringRef unaryOpStr(UnaryOp Op) {
    switch (Op) {
    case Neg:
      return "-";
    case Plus:
      return "+";
    case Not:
      return "not";
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
      return "and";
    case Or:
      return "or";
    }
  }

  llvm::StringRef builtinKindStr(BuiltinKind K) {
    switch (K) {
    case BuiltinKind::UInt:
      return "UInt";
    case BuiltinKind::Int:
      return "Int";
    case BuiltinKind::I8:
      return "I8";
    case BuiltinKind::I16:
      return "I16";
    case BuiltinKind::I32:
      return "I32";
    case BuiltinKind::I64:
      return "I64";
    case BuiltinKind::U8:
      return "U8";
    case BuiltinKind::U16:
      return "U16";
    case BuiltinKind::U32:
      return "U32";
    case BuiltinKind::U64:
      return "U64";
    case BuiltinKind::F32:
      return "F32";
    case BuiltinKind::F64:
      return "F64";
    case BuiltinKind::Bool:
      return "Bool";
    case BuiltinKind::Unit:
      return "Unit";
    case BuiltinKind::Never:
      return "Never";
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
    OS << "FunctionDecl(" << F.Name << ")";
    printLoc(F.Location);
    OS << "\n";
    push();
    for (auto &P : F.Params) {
      indent();
      OS << "Param(" << P.Name;
      if (P.Ty) {
        OS << ": ";
        printType(*P.Ty);
      }
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

  void printExpr(const Expr &E) {
    CurrentLoc = &E.Location;
    std::visit([&](auto &K) { printExprKind(K); }, E.Kind);
    CurrentLoc = nullptr;
  }

  void printExprKind(const IntLiteral &E) {
    indent();
    OS << "IntLiteral(" << E.Value << ")";
    if (CurrentLoc)
      printLoc(*CurrentLoc);
    OS << "\n";
  }
  void printExprKind(const FloatLiteral &E) {
    indent();
    OS << "FloatLiteral(" << E.Value << ")";
    if (CurrentLoc)
      printLoc(*CurrentLoc);
    OS << "\n";
  }
  void printExprKind(const BoolLiteral &E) {
    indent();
    OS << "BoolLiteral(" << (E.Value ? "true" : "false") << ")";
    if (CurrentLoc)
      printLoc(*CurrentLoc);
    OS << "\n";
  }
  void printExprKind(const UnitLiteral &) {
    indent();
    OS << "UnitLiteral";
    if (CurrentLoc)
      printLoc(*CurrentLoc);
    OS << "\n";
  }
  void printExprKind(const VarRef &E) {
    indent();
    OS << "VarRef(" << E.Name << ")";
    if (CurrentLoc)
      printLoc(*CurrentLoc);
    OS << "\n";
  }
  void printExprKind(const ContinueExpr &) {
    indent();
    OS << "ContinueExpr";
    if (CurrentLoc)
      printLoc(*CurrentLoc);
    OS << "\n";
  }
  void printExprKind(const UnaryExpr &E) {
    indent();
    OS << "UnaryExpr(" << unaryOpStr(E.Op) << ")";
    if (CurrentLoc)
      printLoc(*CurrentLoc);
    OS << "\n";
    push();
    printExpr(*E.Operand);
    pop();
  }
  void printExprKind(const BinaryExpr &E) {
    indent();
    OS << "BinaryExpr(" << binaryOpStr(E.Op) << ")";
    if (CurrentLoc)
      printLoc(*CurrentLoc);
    OS << "\n";
    push();
    printExpr(*E.Lhs);
    printExpr(*E.Rhs);
    pop();
  }
  void printExprKind(const CallExpr &E) {
    indent();
    OS << "CallExpr";
    if (CurrentLoc)
      printLoc(*CurrentLoc);
    OS << "\n";
    push();
    indent();
    OS << "Callee:\n";
    push();
    printExpr(*E.Callee);
    pop();
    indent();
    OS << "Arg:\n";
    push();
    printExpr(*E.Arg);
    pop();
    pop();
  }
  void printExprKind(BlockExpr *E) { printBlockExpr(*E); }
  void printExprKind(const IfExpr &E) {
    indent();
    OS << "IfExpr";
    if (CurrentLoc)
      printLoc(*CurrentLoc);
    OS << "\n";
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
      printBlockExpr(*E.ElseBranch);
      pop();
    }
    pop();
  }
  void printExprKind(const WhileExpr &E) {
    indent();
    OS << "WhileExpr";
    if (CurrentLoc)
      printLoc(*CurrentLoc);
    OS << "\n";
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
    OS << "BreakExpr";
    if (CurrentLoc)
      printLoc(*CurrentLoc);
    OS << "\n";
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
    CurrentLoc = &S.Location;
    std::visit([&](auto &K) { printStmtKind(K); }, S.Kind);
    CurrentLoc = nullptr;
  }

  void printStmtKind(const ExprStmt &S) {
    indent();
    OS << "ExprStmt";
    if (CurrentLoc)
      printLoc(*CurrentLoc);
    OS << "\n";
    push();
    printExpr(*S.Expr);
    pop();
  }
  void printStmtKind(const ReturnStmt &S) {
    indent();
    OS << "ReturnStmt";
    if (CurrentLoc)
      printLoc(*CurrentLoc);
    OS << "\n";
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
    OS << ")";
    if (CurrentLoc)
      printLoc(*CurrentLoc);
    OS << "\n";
    if (S.Init) {
      push();
      printExpr(*S.Init);
      pop();
    }
  }
  void printStmtKind(const AssignStmt &S) {
    indent();
    OS << "AssignStmt";
    if (CurrentLoc)
      printLoc(*CurrentLoc);
    OS << "\n";
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
