#include "rheo/AST/AST.h"
#include "rheo/AST/Print.h"
#include "rheo/Diagnostics/DiagnosticEngine.h"
#include "rheo/Diagnostics/SourceManager.h"
#include "rheo/Frontend/Lexer.h"
#include "rheo/Frontend/Parser.h"
#include "rheo/Sema/NameResolver.h"
#include <llvm/ADT/ArrayRef.h>

int main() {
  const auto *Src = R"(
    def sum(x, y) x + y end
    sum(10, 20)
  )";
  rheo::SourceManager Manager;
  auto FileId = Manager.addFile("main.rheo", Src);
  rheo::DiagnosticEngine Engine;
  rheo::Lexer Lexer(FileId, Src, Engine);
  rheo::ASTContext Ctx;
  rheo::Parser Parser(Ctx, Lexer, Engine, FileId);
  auto E = Parser.parseModule("main");
  rheo::NameResolver Resolver(Engine, FileId, Ctx);
  Resolver.analyze(E);
  rheo::ASTPrinter Printer;
  if (Engine.hasError()) {
    auto &Out = llvm::outs();
    for (const auto &Diag : Engine.diagnostics()) {
      Diag.print(Out, Manager);
    }
    return 1;
  }
  Printer.print(E);
  return 0;
}
