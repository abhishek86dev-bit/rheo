#include "rheo/Frontend/Lexer.h"
#include "rheo/Diagnostics/SourceLocation.h"
#include "rheo/Frontend/Token.h"
#include <cctype>
#include <llvm/ADT/StringRef.h>
#include <string>

namespace rheo {

void Lexer::skipWhitespace() {
  while (true) {
    char pchar = peek();
    if (pchar == '\n' || pchar == '\0') {
      return;
    }
    if (std::isspace(static_cast<unsigned char>(pchar)) == 0) {
      return;
    }
    advance();
  }
}

void Lexer::makeUnexpectedCharDiag(char chr, Span span) {
  Diagnostic diag(Severity::Error);
  diag.setMessage("unexpected character '" + std::string(1, chr) + "'");
  diag.setCode("E0001");
  diag.addLabel(Label::primary(span, fileId, "unexpected character"));
  switch (chr) {
  case '@':
    diag.setHelp("remove '@' or replace it with a valid identifier character");
    break;
  case '#':
    diag.setHelp("remove '#' or start a preprocessor directive if supported");
    break;
  default:
    break;
  }
  diags->emit(diag);
}

Token Lexer::nextToken() {
  skipWhitespace();

  auto start = pos;

  if (pos >= input.size()) {
    return {.span = Span(start, start), .kind = TokenKind::Eof};
  }

  auto chr = advance();

  if (std::isdigit(chr) != 0) {
    std::string value{chr};
    while (pos < input.size() && std::isdigit(peek()) != 0) {
      value += advance();
    }
    return {.span = Span(start, pos),
            .kind = TokenKind::IntLiteral,
            .value = value};
  }

  if ((std::isalpha(chr) != 0) || chr == '_') {
    std::string value{chr};
    while (pos < input.size() &&
           ((std::isalpha(peek()) != 0) || peek() == '_')) {
      value += advance();
    }
    auto kind = TokenKind::Identifier;
    if (keywords.contains(value)) {
      kind = keywords[value];
    }
    return {.span = Span(start, pos), .kind = kind, .value = value};
  }

  switch (chr) {
  case '(':
    return {.span = Span(start, pos), .kind = TokenKind::LParen, .value = "("};
  case ')':
    return {.span = Span(start, pos), .kind = TokenKind::RParen, .value = ")"};
  case '{':
    return {
        .span = Span(start, pos), .kind = TokenKind::LBracket, .value = "{"};
  case '}':
    return {
        .span = Span(start, pos), .kind = TokenKind::RBracket, .value = "}"};
  case '\n':
    return {
        .span = Span(start, pos), .kind = TokenKind::NewLine, .value = "\n"};
  case '-':
    if (pos < input.size() && peek() == '>') {
      advance();
      return {
          .span = Span(start, pos), .kind = TokenKind::Arrow, .value = "->"};
    }
    break;
  default:
  }

  makeUnexpectedCharDiag(chr, Span(start, pos));
  return {.span = Span(start, pos),
          .kind = TokenKind::Error,
          .value = std::string(1, chr)};
}

} // namespace rheo
