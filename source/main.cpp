#include "rheo/Diagnostics/DiagnosticEngine.h"
#include "rheo/Diagnostics/SourceManager.h"
#include "rheo/Frontend/Lexer.h"
#include "rheo/Frontend/Token.h"
#include "llvm/Support/raw_ostream.h"
#include <iostream>

int main() {
  const auto *src = "func main() -> Int { return 10 }";
  rheo::DiagnosticEngine engine;
  rheo::SourceManager manager;
  auto fileId = manager.addFile("main.rheo", src);
  rheo::Lexer lexer(fileId, src, engine);
  while (true) {
    auto tok = lexer.nextToken();
    std::cout << "Token: " << tok.value << ", Location: " << tok.span.getStart()
              << ".." << tok.span.getEnd() << "\n";

    if (tok.kind == rheo::TokenKind::Eof) {
      break;
    }
  }
  if (engine.hasError()) {
    auto &out = llvm::outs();
    auto diags = engine.diagnostics();
    for (const auto &diag : diags) {
      diag.print(out, manager);
    }
  }
  return 0;
}
