#ifndef RHEO_TOKEN_H
#define RHEO_TOKEN_H

#include "rheo/Diagnostics/SourceLocation.h"
#include <cstdint>
#include <llvm/ADT/StringRef.h>
#include <string>

namespace rheo {

enum class TokenKind : std::uint8_t {
  LParen,
  RParen,
  LBrace,
  RBrace,
  LBracket,
  RBracket,
  Comma,
  Colon,
  Semicolon,
  Dot,
  Plus,
  Minus,
  Star,
  Slash,
  Percent,
  EqualEqual,
  BangEqual,
  Less,
  LessEqual,
  Greater,
  GreaterEqual,
  AndAnd,
  OrOr,
  Bang,
  Equal,
  Arrow,
  Identifier,
  IntLiteral,
  FloatLiteral,
  Int,
  Float,
  True,
  False,
  Bool,
  Func,
  Let,
  Var,
  Return,
  If,
  Else,
  For,
  In,
  While,
  Struct,
  Continue,
  Break,
  NewLine,
  Eof,
  Error,
};

struct Token {
  Span Span;
  TokenKind Kind;
  llvm::StringRef Value;
};

} // namespace rheo

#endif // RHEO_TOKEN_H
