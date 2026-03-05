#include "rheo/AST/AST.h"
#include "rheo/AST/Print.h"
#include "rheo/Diagnostics/DiagnosticEngine.h"
#include "rheo/Diagnostics/SourceManager.h"
#include "rheo/Frontend/Lexer.h"
#include "rheo/Frontend/Parser.h"
#include <llvm/ADT/ArrayRef.h>

int main() {
  const auto *Src = R"(def main(args)
      x := 10
    end
  )";
  rheo::SourceManager Manager;
  auto FileId = Manager.addFile("main.rheo", Src);
  rheo::DiagnosticEngine Engine;
  rheo::Lexer Lexer(FileId, Src, Engine);
  rheo::ASTContext Ctx;
  rheo::Parser Parser(Ctx, Lexer, Engine, FileId);
  auto *E = Parser.parseFunc();
  rheo::ASTPrinter Printer;
  if (Engine.hasError()) {
    auto &Out = llvm::outs();
    for (const auto &Diag : Engine.diagnostics()) {
      Diag.print(Out, Manager);
    }
    return 1;
  }
  if (E != nullptr)
    Printer.print(*E);
  return 0;
}
