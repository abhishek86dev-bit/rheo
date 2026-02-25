#ifndef RHEO_LEXER_H
#define RHEO_LEXER_H

#include "Token.h"
#include "rheo/Diagnostics/DiagnosticEngine.h"
#include "rheo/Diagnostics/SourceLocation.h"
#include <cstddef>
#include <llvm/ADT/StringRef.h>
#include <string>

namespace rheo {

class Lexer {
  FileId fileId;
  std::string input;
  std::size_t pos = 0;
  DiagnosticEngine *diags;

  std::unordered_map<std::string, TokenKind> keywords = {
      {"func", TokenKind::Func},
      {"Int", TokenKind::Int},
      {"return", TokenKind::Return},
  };

public:
  Lexer(FileId fileId, llvm::StringRef input, DiagnosticEngine &diags)
      : fileId(fileId), input(input), diags(&diags) {}

  [[nodiscard]] char peek() const {
    if (pos < input.size()) {
      return input[pos];
    }
    return '\0';
  }

  char advance() {
    if (pos < input.size()) {
      return input[pos++];
    }
    return '\0';
  }

  void skipWhitespace();
  void makeUnexpectedCharDiag(char chr, Span span);
  Token nextToken();
};

} // namespace rheo

#endif // RHEO_LEXER_H
