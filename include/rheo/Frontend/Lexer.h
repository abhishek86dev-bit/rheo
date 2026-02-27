#ifndef RHEO_LEXER_H
#define RHEO_LEXER_H

#include "Token.h"
#include "rheo/Diagnostics/DiagnosticEngine.h"
#include "rheo/Diagnostics/SourceLocation.h"
#include <llvm/ADT/StringRef.h>
#include <string>

namespace rheo {

class Lexer {
  FileId File;
  std::string Input;
  std::size_t Pos = 0;
  DiagnosticEngine *Diags;

  [[nodiscard]] char peek() const {
    if (Pos < Input.size()) {
      return Input[Pos];
    }
    return '\0';
  }

  char advance() {
    if (Pos < Input.size()) {
      return Input[Pos++];
    }
    return '\0';
  }

  void makeUnexpectedCharDiag(char Chr, Span Span);
  void makeUnexpectedDoubleDotInFloatDiag(llvm::StringRef FirstPart,
                                          llvm::StringRef SecondPart,
                                          Span Span);

  Token lexNum();
  Token lexKeywordOrIdent();

  void skipWhitespace();

public:
  Lexer(FileId File, llvm::StringRef Input, DiagnosticEngine &Diags)
      : File(File), Input(Input), Diags(&Diags) {}

  Token nextToken();
};

} // namespace rheo

#endif // RHEO_LEXER_H
