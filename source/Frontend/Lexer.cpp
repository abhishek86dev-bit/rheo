#include "rheo/Frontend/Lexer.h"
#include "rheo/Diagnostics/SourceLocation.h"
#include "rheo/Frontend/Token.h"
#include <format>
#include <llvm/ADT/StringRef.h>
#include <string>

namespace rheo {

void Lexer::skipWhitespace() {
  while (true) {
    char CH = peek();
    if (CH == '\n' || CH == '\0') {
      return;
    }
    if (std::isspace(static_cast<unsigned char>(CH)) == 0) {
      return;
    }
    advance();
  }
}

void Lexer::makeUnexpectedCharDiag(char Chr, Span Span) {
  Diagnostic Diag(Severity::Error);
  Diag.setMessage("unexpected character '" + std::string(1, Chr) + "'");
  Diag.setCode("E0001");
  Diag.addLabel(Label::primary(Span, File, "unexpected character"));
  switch (Chr) {
  case '@':
    Diag.setHelp("remove '@' or replace it with a valid identifier character");
    break;
  case '#':
    Diag.setHelp("remove '#' or start a preprocessor directive if supported");
    break;
  default:
    break;
  }
  Diags->emit(Diag);
}

void Lexer::makeUnexpectedDoubleDotInFloatDiag(llvm::StringRef FirstPart,
                                               llvm::StringRef SecondPart,
                                               Span Span) {
  Diagnostic Diag(Severity::Error);
  Diag.setMessage("unexpected '.' in floating point literal");
  Diag.setCode("E0002");
  Diag.addLabel(Label::primary(Span, File, "second '.' here"));
  Diag.setHelp(std::format("floating point literals can only have one decimal "
                           "point — did you mean '{}' or '{}{}'?",
                           FirstPart, FirstPart, SecondPart));
  Diags->emit(Diag);
}

Token Lexer::lexNum() {
  auto Start = Pos;
  while (Pos < Input.size() && std::isdigit(peek()) != 0)
    advance();

  if (Pos < Input.size() && peek() == '.') {
    advance();
    while (Pos < Input.size() && std::isdigit(peek()) != 0)
      advance();

    llvm::StringRef FirstPart(Input.data() + Start, Pos - Start);

    if (Pos < Input.size() && peek() == '.') {
      auto SecondDotPos = Pos;
      advance();
      auto SecondPartStart = Pos - 1;
      while (Pos < Input.size() && std::isdigit(peek()) != 0)
        advance();

      llvm::StringRef SecondPart(Input.data() + SecondPartStart,
                                 Pos - SecondPartStart);
      makeUnexpectedDoubleDotInFloatDiag(FirstPart, SecondPart,
                                         Span(SecondDotPos, Pos));
    }

    return {.Span = Span(Start, Pos),
            .Kind = TokenKind::FloatLiteral,
            .Value = llvm::StringRef(Input.data() + Start, Pos - Start)};
  }

  return {.Span = Span(Start, Pos),
          .Kind = TokenKind::IntLiteral,
          .Value = llvm::StringRef(Input.data() + Start, Pos - Start)};
}

#define KW(str, kind)                                                          \
  if (Keyword == str)                                                          \
    return kind;

static TokenKind classifyIdent(llvm::StringRef Keyword) {
  switch (Keyword.size()) {
  case 2:
    KW("if", TokenKind::If)
    KW("in", TokenKind::In)
    break;
  case 3:
    KW("let", TokenKind::Let)
    KW("var", TokenKind::Var)
    KW("for", TokenKind::For)
    KW("Int", TokenKind::Int)
    break;
  case 4:
    KW("func", TokenKind::Func)
    KW("true", TokenKind::True)
    KW("Bool", TokenKind::Bool)
    break;
  case 5:
    KW("false", TokenKind::False)
    KW("while", TokenKind::While)
    KW("Float", TokenKind::Float)
    break;
  case 6:
    KW("struct", TokenKind::Struct)
    KW("return", TokenKind::Return)
    break;
  case 8:
    KW("continue", TokenKind::Continue)
    break;
  }
  return TokenKind::Identifier;
}

Token Lexer::lexKeywordOrIdent() {
  auto Start = Pos;
  while (Pos < Input.size() && ((std::isalpha(peek()) != 0) || peek() == '_'))
    advance();
  auto Ident = llvm::StringRef(Input.data() + Start, Pos - Start);
  return {
      .Span = Span(Start, Pos), .Kind = classifyIdent(Ident), .Value = Ident};
}

Token Lexer::nextToken() {
  skipWhitespace();

  auto Start = Pos;

  if (Pos >= Input.size()) {
    return {.Span = Span(Start, Start), .Kind = TokenKind::Eof};
  }

  auto Ch = peek();

  if (std::isdigit(Ch) != 0) {
    return lexNum();
  }

  if ((std::isalpha(Ch) != 0) || Ch == '_') {
    return lexKeywordOrIdent();
  }

  advance();

  switch (Ch) {
  case '(':
    return {.Span = Span(Start, Pos), .Kind = TokenKind::LParen, .Value = "("};

  case ')':
    return {.Span = Span(Start, Pos), .Kind = TokenKind::RParen, .Value = ")"};

  case '{':
    return {.Span = Span(Start, Pos), .Kind = TokenKind::LBrace, .Value = "{"};

  case '}':
    return {.Span = Span(Start, Pos), .Kind = TokenKind::RBrace, .Value = "}"};

  case '[':
    return {
        .Span = Span(Start, Pos), .Kind = TokenKind::LBracket, .Value = "["};

  case ']':
    return {
        .Span = Span(Start, Pos), .Kind = TokenKind::RBracket, .Value = "]"};

  case '\n':
    return {
        .Span = Span(Start, Pos), .Kind = TokenKind::NewLine, .Value = "\n"};

  case ',':
    return {.Span = Span(Start, Pos), .Kind = TokenKind::Comma, .Value = ","};

  case ':':
    return {.Span = Span(Start, Pos), .Kind = TokenKind::Colon, .Value = ":"};

  case ';':
    return {
        .Span = Span(Start, Pos), .Kind = TokenKind::Semicolon, .Value = ";"};

  case '.':
    return {.Span = Span(Start, Pos), .Kind = TokenKind::Dot, .Value = "."};

  case '+':
    return {.Span = Span(Start, Pos), .Kind = TokenKind::Plus, .Value = "+"};

  case '-':
    if (Pos < Input.size() && peek() == '>') {
      advance();
      return {
          .Span = Span(Start, Pos), .Kind = TokenKind::Arrow, .Value = "->"};
    }
    return {.Span = Span(Start, Pos), .Kind = TokenKind::Minus, .Value = "-"};

  case '*':
    return {.Span = Span(Start, Pos), .Kind = TokenKind::Star, .Value = "*"};

  case '/':
    return {.Span = Span(Start, Pos), .Kind = TokenKind::Slash, .Value = "/"};

  case '%':
    return {.Span = Span(Start, Pos), .Kind = TokenKind::Percent, .Value = "%"};

  case '=':
    if (Pos < Input.size() && peek() == '=') {
      advance();
      return {.Span = Span(Start, Pos),
              .Kind = TokenKind::EqualEqual,
              .Value = "=="};
    }
    return {.Span = Span(Start, Pos), .Kind = TokenKind::Equal, .Value = "="};

  case '!':
    if (Pos < Input.size() && peek() == '=') {
      advance();
      return {.Span = Span(Start, Pos),
              .Kind = TokenKind::BangEqual,
              .Value = "!="};
    }
    return {.Span = Span(Start, Pos), .Kind = TokenKind::Bang, .Value = "!"};

  case '<':
    if (Pos < Input.size() && peek() == '=') {
      advance();
      return {.Span = Span(Start, Pos),
              .Kind = TokenKind::LessEqual,
              .Value = "<="};
    }
    return {.Span = Span(Start, Pos), .Kind = TokenKind::Less, .Value = "<"};

  case '>':
    if (Pos < Input.size() && peek() == '=') {
      advance();
      return {.Span = Span(Start, Pos),
              .Kind = TokenKind::GreaterEqual,
              .Value = ">="};
    }
    return {.Span = Span(Start, Pos), .Kind = TokenKind::Greater, .Value = ">"};

  case '&':
    if (Pos < Input.size() && peek() == '&') {
      advance();
      return {
          .Span = Span(Start, Pos), .Kind = TokenKind::AndAnd, .Value = "&&"};
    }
    break;

  case '|':
    if (Pos < Input.size() && peek() == '|') {
      advance();
      return {.Span = Span(Start, Pos), .Kind = TokenKind::OrOr, .Value = "||"};
    }
    break;

  default:
    break;
  }

  makeUnexpectedCharDiag(Ch, Span(Start, Pos));
  return {.Span = Span(Start, Pos),
          .Kind = TokenKind::Error,
          .Value = llvm::StringRef(Input.data() + 1, 1)};
}

} // namespace rheo
