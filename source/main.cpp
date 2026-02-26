#include "rheo/Diagnostics/DiagnosticEngine.h"
#include "rheo/Diagnostics/SourceManager.h"
#include "rheo/Frontend/Lexer.h"
#include "rheo/Frontend/Token.h"
#include "llvm/Support/raw_ostream.h"
#include <iostream>

int main() {
  const auto *Src = "func main() -> Int { return 1399 }";
  rheo::DiagnosticEngine Engine;
  rheo::SourceManager Manager;
  auto FileId = Manager.addFile("main.rheo", Src);
  rheo::Lexer Lexer(FileId, Src, Engine);
  while (true) {
    auto Tok = Lexer.nextToken();
    std::cout << "Token: " << Tok.Value.str()
              << ", Location: " << Tok.Span.getStart() << ".."
              << Tok.Span.getEnd() << "\n";

    if (Tok.Kind == rheo::TokenKind::Eof) {
      break;
    }
  }
  if (Engine.hasError()) {
    auto &Out = llvm::outs();
    auto Diags = Engine.diagnostics();
    for (const auto &Diag : Diags) {
      Diag.print(Out, Manager);
    }
  }
  return 0;
}
