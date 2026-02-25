#ifndef RHEO_TOKEN_H
#define RHEO_TOKEN_H

#include "rheo/Diagnostics/SourceLocation.h"
#include <cstdint>
#include <string>

namespace rheo {

enum class TokenKind : std::uint8_t {
  LParen,
  RParen,
  RBracket,
  LBracket,
  Arrow,
  Identifier,
  IntLiteral,
  Int,
  Func,
  Let,
  Return,
  Eof,
  NewLine,
  Error,
};

struct Token {
  Span span;
  TokenKind kind;
  std::string value;
};

} // namespace rheo

#endif // RHEO_TOKEN_H
