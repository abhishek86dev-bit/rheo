#ifndef RHEO_RESOLVED_AST_PRINTER_H
#define RHEO_RESOLVED_AST_PRINTER_H

#include "rheo/Sema/ResolvedAST.h"
#include "rheo/Sema/Symbol.h"

#include <llvm/Support/raw_ostream.h>

namespace rheo {

class ResolvedASTPrinter {
  llvm::raw_ostream &OS;
  unsigned Indent = 0;

  void doIndent() { OS.indent(Indent * 2); }

  struct IndentGuard {
    ResolvedASTPrinter &P;
    explicit IndentGuard(ResolvedASTPrinter &P) : P(P) { ++P.Indent; }
    ~IndentGuard() { --P.Indent; }
  };

  static const char *builtinKindStr(BuiltinKind K) {
    switch (K) {
    case BuiltinKind::Int:
      return "int";
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
    case BuiltinKind::UInt:
      return "uint";
    case BuiltinKind::F32:
      return "f32";
    case BuiltinKind::F64:
      return "f64";
    case BuiltinKind::Bool:
      return "bool";
    case BuiltinKind::Unit:
      return "()";
    case BuiltinKind::Never:
      return "!";
    }
    return "?";
  }

  void printType(const ResolvedType *Ty) {
    if (!Ty) {
      OS << "<null-type>";
      return;
    }
    std::visit(
        [&](const auto &K) {
          using T = std::decay_t<decltype(K)>;
          if constexpr (std::is_same_v<T, ResolvedBuiltinType>) {
            OS << builtinKindStr(K.Kind);
          } else if constexpr (std::is_same_v<T, ResolvedNamedType>) {
            OS << "Named(" << K.Id.get() << ")";
          } else if constexpr (std::is_same_v<T, ResolvedFunctionType>) {
            OS << "fn(";
            for (size_t I = 0; I < K.Params.size(); ++I) {
              if (I)
                OS << ", ";
              printType(K.Params[I]);
            }
            OS << ") -> ";
            printType(K.Return);
          }
        },
        Ty->Kind);
  }

  static const char *unaryOpStr(UnaryOp Op) {
    switch (Op) {
    case UnaryOp::Neg:
      return "-";
    case UnaryOp::Not:
      return "!";
    case UnaryOp::Plus:
      return "+";
    }
    return "?";
  }

  static const char *binaryOpStr(BinaryOp Op) {
    switch (Op) {
    case BinaryOp::Add:
      return "+";
    case BinaryOp::Sub:
      return "-";
    case BinaryOp::Mul:
      return "*";
    case BinaryOp::Div:
      return "/";
    case BinaryOp::Mod:
      return "%";
    case BinaryOp::Eq:
      return "==";
    case BinaryOp::NotEq:
      return "!=";
    case BinaryOp::Lt:
      return "<";
    case BinaryOp::Le:
      return "<=";
    case BinaryOp::Gt:
      return ">";
    case BinaryOp::Ge:
      return ">=";
    case BinaryOp::And:
      return "&&";
    case BinaryOp::Or:
      return "||";
    }
    return "?";
  }

  void printTypeSuffix(const ResolvedExpr *E) {
    if (E && E->Ty) {
      OS << " : ";
      printType(E->Ty);
    }
  }

public:
  explicit ResolvedASTPrinter(llvm::raw_ostream &OS) : OS(OS) {}

  void print(const ResolvedModule &M) {
    OS << "Module(" << M.Name << ")\n";
    IndentGuard G(*this);
    for (const ResolvedStmt *S : M.Stmts)
      printStmt(S);
  }

  void printStmt(const ResolvedStmt *S) {
    if (!S) {
      doIndent();
      OS << "<null-stmt>\n";
      return;
    }
    std::visit(
        [&](const auto &K) {
          using T = std::decay_t<decltype(K)>;

          if constexpr (std::is_same_v<T, ResolvedExprStmt>) {
            doIndent();
            OS << "ExprStmt\n";
            IndentGuard G(*this);
            printExpr(K.Expr);

          } else if constexpr (std::is_same_v<T, ResolvedReturnStmt>) {
            doIndent();
            OS << "ReturnStmt\n";
            if (K.Value) {
              IndentGuard G(*this);
              printExpr(K.Value);
            }

          } else if constexpr (std::is_same_v<T, ResolvedVarDecl>) {
            doIndent();
            OS << (K.IsMut ? "LetMut" : "Let") << "(" << K.Id.get() << ")";
            if (K.Ty) {
              OS << " : ";
              printType(K.Ty);
            }
            OS << "\n";
            if (K.Init) {
              IndentGuard G(*this);
              printExpr(K.Init);
            }

          } else if constexpr (std::is_same_v<T, ResolvedAssignStmt>) {
            doIndent();
            OS << "Assign\n";
            IndentGuard G(*this);
            doIndent();
            OS << "Target\n";
            {
              IndentGuard G2(*this);
              printExpr(K.Target);
            }
            doIndent();
            OS << "Value\n";
            {
              IndentGuard G2(*this);
              printExpr(K.Value);
            }

          } else if constexpr (std::is_same_v<T, ResolvedFunctionDecl *>) {
            printFunctionDecl(K);
          }
        },
        S->Kind);
  }

  void printExpr(const ResolvedExpr *E) {
    if (!E) {
      doIndent();
      OS << "<null-expr>\n";
      return;
    }
    std::visit(
        [&](const auto &K) {
          using T = std::decay_t<decltype(K)>;

          if constexpr (std::is_same_v<T, ResolvedIntLiteral>) {
            doIndent();
            OS << "IntLiteral(" << K.Value << ")";
            printTypeSuffix(E);
            OS << "\n";

          } else if constexpr (std::is_same_v<T, ResolvedFloatLiteral>) {
            doIndent();
            OS << "FloatLiteral(" << K.Value << ")";
            printTypeSuffix(E);
            OS << "\n";

          } else if constexpr (std::is_same_v<T, ResolvedBoolLiteral>) {
            doIndent();
            OS << "BoolLiteral(" << (K.Value ? "true" : "false") << ")";
            printTypeSuffix(E);
            OS << "\n";

          } else if constexpr (std::is_same_v<T, ResolvedUnitLiteral>) {
            doIndent();
            OS << "UnitLiteral";
            printTypeSuffix(E);
            OS << "\n";

          } else if constexpr (std::is_same_v<T, ResolvedUnaryExpr>) {
            doIndent();
            OS << "UnaryExpr(" << unaryOpStr(K.Op) << ")";
            printTypeSuffix(E);
            OS << "\n";
            IndentGuard G(*this);
            printExpr(K.Operand);

          } else if constexpr (std::is_same_v<T, ResolvedBinaryExpr>) {
            doIndent();
            OS << "BinaryExpr(" << binaryOpStr(K.Op) << ")";
            printTypeSuffix(E);
            OS << "\n";
            IndentGuard G(*this);
            printExpr(K.LHS);
            printExpr(K.RHS);

          } else if constexpr (std::is_same_v<T, ResolvedCallExpr>) {
            doIndent();
            OS << "CallExpr";
            printTypeSuffix(E);
            OS << "\n";
            IndentGuard G(*this);
            doIndent();
            OS << "Callee\n";
            {
              IndentGuard G2(*this);
              printExpr(K.Callee);
            }
            doIndent();
            OS << "Args\n";
            IndentGuard G2(*this);
            for (const ResolvedExpr *A : K.Args)
              printExpr(A);

          } else if constexpr (std::is_same_v<T, ResolvedVarRef>) {
            doIndent();
            OS << "VarRef(" << K.Id.get() << ")";
            printTypeSuffix(E);
            OS << "\n";

          } else if constexpr (std::is_same_v<T, ResolvedBlockExpr *>) {
            printBlockExpr(K);

          } else if constexpr (std::is_same_v<T, ResolvedIfExpr>) {
            doIndent();
            OS << "IfExpr";
            printTypeSuffix(E);
            OS << "\n";
            IndentGuard G(*this);
            doIndent();
            OS << "Condition\n";
            {
              IndentGuard G2(*this);
              printExpr(K.Condition);
            }
            doIndent();
            OS << "Then\n";
            {
              IndentGuard G2(*this);
              printBlockExpr(K.ThenBlock);
            }
            if (K.ElseBranch) {
              doIndent();
              OS << "Else\n";
              IndentGuard G2(*this);
              printBlockExpr(K.ElseBranch);
            }

          } else if constexpr (std::is_same_v<T, ResolvedWhileExpr>) {
            doIndent();
            OS << "WhileExpr";
            printTypeSuffix(E);
            OS << "\n";
            IndentGuard G(*this);
            doIndent();
            OS << "Condition\n";
            {
              IndentGuard G2(*this);
              printExpr(K.Condition);
            }
            doIndent();
            OS << "Body\n";
            {
              IndentGuard G2(*this);
              printBlockExpr(K.Body);
            }

          } else if constexpr (std::is_same_v<T, ResolvedBreakExpr>) {
            doIndent();
            OS << "BreakExpr";
            printTypeSuffix(E);
            OS << "\n";
            if (K.Value) {
              IndentGuard G(*this);
              printExpr(K.Value);
            }

          } else if constexpr (std::is_same_v<T, ResolvedContinueExpr>) {
            doIndent();
            OS << "ContinueExpr";
            printTypeSuffix(E);
            OS << "\n";
          }
        },
        E->Kind);
  }

  void printBlockExpr(const ResolvedBlockExpr *B) {
    if (!B) {
      doIndent();
      OS << "<null-block>\n";
      return;
    }
    doIndent();
    OS << "BlockExpr\n";
    IndentGuard G(*this);
    for (const ResolvedStmt *S : B->Stmts)
      printStmt(S);
    if (B->Tail) {
      doIndent();
      OS << "Tail\n";
      IndentGuard G2(*this);
      printExpr(B->Tail);
    }
  }

  void printFunctionDecl(const ResolvedFunctionDecl *F) {
    if (!F) {
      doIndent();
      OS << "<null-fn>\n";
      return;
    }
    doIndent();
    OS << "FunctionDecl(" << F->Id.get() << ") -> ";
    printType(F->ReturnType);
    OS << "\n";
    IndentGuard G(*this);
    for (const ResolvedParam &P : F->Params) {
      doIndent();
      OS << "Param(" << P.Id.get() << " : ";
      printType(P.Ty);
      OS << ")\n";
    }
    printBlockExpr(F->Body);
  }
};

} // namespace rheo

#endif // RHEO_RESOLVED_AST_PRINTER_H
