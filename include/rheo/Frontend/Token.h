#ifndef RHEO_TOKEN_H
#define RHEO_TOKEN_H

#include "rheo/Diagnostics/SourceLocation.h"
#include <llvm/ADT/StringRef.h>

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
  Bang,
  BangEqual,
  ColonEqual,
  Less,
  LessEqual,
  Greater,
  GreaterEqual,
  Equal,
  Arrow,
  Identifier,
  IntLiteral,
  FloatLiteral,
  Int8,
  Int16,
  Int32,
  Int64,
  UInt8,
  UInt16,
  UInt32,
  UInt64,
  UInt,
  Float32,
  Float64,
  Int,
  True,
  False,
  Bool,
  Fun,
  Return,
  End,
  If,
  Else,
  While,
  Continue,
  Break,
  And,
  Not,
  Or,
  Mut,
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
