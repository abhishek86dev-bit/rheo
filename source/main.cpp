#include "rheo/AST/AST.h"
#include "rheo/AST/Print.h"
#include "rheo/Diagnostics/DiagnosticEngine.h"
#include "rheo/Diagnostics/SourceManager.h"
#include "rheo/Frontend/Lexer.h"
#include "rheo/Frontend/Parser.h"

int main() {
  const auto *Src = R"(
    f x = 10
  )";
  rheo::SourceManager Manager;
  auto FileId = Manager.addFile("main.rheo", Src);
  rheo::DiagnosticEngine Engine;
  rheo::Lexer Lexer(FileId, Src, Engine);
  rheo::ASTContext Ctx;
  rheo::Parser Parser(Ctx, Lexer, Engine, FileId);
  auto *E = Parser.parseStmt();
  rheo::ASTPrinter Printer;
  if (Engine.hasError()) {
    auto &Out = llvm::outs();
    for (const auto &Diag : Engine.diagnostics()) {
      Diag.print(Out, Manager);
    }
    return 1;
  }
  if (E != nullptr)
    Printer.printStmt(*E);
  return 0;
}
