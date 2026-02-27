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
  Int8,
  Int32,
  Int64,
  UInt,
  UInt8,
  UInt32,
  UInt64,
  Float,
  Float32,
  Float64,
  True,
  False,
  Bool,
  Func,
  Let,
  Var,
  Return,
  If,
  Else,
  While,
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
