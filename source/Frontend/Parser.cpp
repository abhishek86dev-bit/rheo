#include "rheo/Frontend/Parser.h"
#include "rheo/AST/AST.h"
#include "rheo/Frontend/Token.h"
#include <format>
#include <llvm/ADT/SmallVector.h>
#include <string>

namespace rheo {

static std::string describeToken(const Token &Tok) {
  using TK = TokenKind;

  switch (Tok.Kind) {

  // punctuation
  case TK::LParen:
    return "'('";
  case TK::RParen:
    return "')'";
  case TK::LBrace:
    return "'{'";
  case TK::RBrace:
    return "'}'";
  case TK::LBracket:
    return "'['";
  case TK::RBracket:
    return "']'";
  case TK::Comma:
    return "','";
  case TK::Colon:
    return "':'";
  case TK::Semicolon:
    return "';'";
  case TK::Dot:
    return "'.'";

  // operators
  case TK::Plus:
  case TK::Minus:
  case TK::Star:
  case TK::Slash:
  case TK::Percent:
  case TK::EqualEqual:
  case TK::BangEqual:
  case TK::Less:
  case TK::LessEqual:
  case TK::Greater:
  case TK::GreaterEqual:
  case TK::Equal:
  case TK::Arrow:
  case TK::Not:
  case TK::And:
  case TK::Or:
    return std::format("operator '{}'", Tok.Value.str());

  // literals
  case TK::IntLiteral:
    return std::format("integer literal {}", Tok.Value.str());
  case TK::FloatLiteral:
    return std::format("floating-point literal {}", Tok.Value.str());
  case TK::True:
  case TK::False:
    return std::format("boolean literal {}", Tok.Value.str());

  // identifier
  case TK::Identifier:
    return std::format("identifier '{}'", Tok.Value.str());

  // type keywords
  case TK::Int:
  case TK::Int8:
  case TK::Int32:
  case TK::Int64:
  case TK::UInt:
  case TK::UInt8:
  case TK::UInt32:
  case TK::UInt64:
  case TK::Float:
  case TK::Float32:
  case TK::Float64:
  case TK::Bool:
    return std::format("type '{}'", Tok.Value.str());

  // language keywords
  case TK::Fun:
  case TK::End:
  case TK::Return:
  case TK::If:
  case TK::Else:
  case TK::While:
  case TK::Continue:
  case TK::Break:
    return std::format("keyword '{}'", Tok.Value.str());

  case TK::NewLine:
    return "newline";

  case TK::Eof:
    return "end of file";

  case TK::Error:
    return "invalid token";
  }

  return "token";
}

Expr *Parser::errorExpectedRParen(Span OpenParenSpan) {
  Token Tok = NextToken;

  Diagnostic Diag(Severity::Error);
  Diag.setMessage("expected ')'");
  Diag.setCode("E1001");

  Diag.addLabel(Label::primary(
      Tok.Span, File,
      std::format("found {} instead of ')'", describeToken(Tok))));

  Diag.addLabel(Label::secondary(OpenParenSpan, File, "opening '(' here"));

  Diag.setHelp("every '(' must be closed with ')'\n"
               "  • unit value: ()\n"
               "  • grouping: (expression)");

  Diags.emit(Diag);

  if (Tok.Kind != TokenKind::RParen)
    eatNextToken();

  return nullptr;
}

Expr *Parser::errorExpectedExpr() {
  Token Tok = NextToken;

  Diagnostic Diag(Severity::Error);
  Diag.setCode("E1002");

  auto Found = describeToken(Tok);

  Diag.setMessage(std::format("expected an expression, found {}", Found));

  Diag.addLabel(
      Label::primary(Tok.Span, File, std::format("unexpected {}", Found)));

  Diag.setHelp("expressions include:\n"
               "  • literals: 42, 3.14, true, false, ()\n"
               "  • identifiers: x, value\n"
               "  • grouped: (expression)");

  Diags.emit(Diag);

  eatNextToken();
  return nullptr;
}

Expr *Parser::errorExpectedCommaOrRParenInCall(Span OpenParenSpan) {
  Token Tok = NextToken;

  Diagnostic Diag(Severity::Error);
  Diag.setMessage("expected ',' or ')' in call");
  Diag.setCode("E1003");

  Diag.addLabel(Label::primary(
      Tok.Span, File,
      std::format("found {} instead of ',' or ')'", describeToken(Tok))));

  Diag.addLabel(Label::secondary(OpenParenSpan, File, "call started here"));

  Diag.setHelp(
      "function call arguments must be separated by ',' and closed with ')'\n"
      "  • foo(a, b)\n"
      "  • foo()\n"
      "  • foo(x + y, z)");

  Diags.emit(Diag);

  while (Tok.Kind != TokenKind::Comma && Tok.Kind != TokenKind::RParen &&
         Tok.Kind != TokenKind::Eof) {
    eatNextToken();
    Tok = NextToken;
  }

  return nullptr;
}

Expr *Parser::parsePrimary() {
  switch (NextToken.Kind) {
  case TokenKind::True: {
    auto Tok = NextToken;
    eatNextToken();
    return Context.create<Expr>(Tok.Span, BoolLiteral{true});
  }
  case TokenKind::False: {
    auto Tok = NextToken;
    eatNextToken();
    return Context.create<Expr>(Tok.Span, BoolLiteral{false});
  }
  case TokenKind::FloatLiteral: {
    auto Tok = NextToken;
    eatNextToken();
    float V = 0;
    std::from_chars(Tok.Value.begin(), Tok.Value.end(), V);
    return Context.create<Expr>(Tok.Span, FloatLiteral{V});
  }
  case TokenKind::IntLiteral: {
    auto Tok = NextToken;
    eatNextToken();
    std::uint64_t V = 0;
    std::from_chars(Tok.Value.begin(), Tok.Value.end(), V);
    return Context.create<Expr>(Tok.Span, IntLiteral{V});
  }
  case TokenKind::Identifier: {
    auto Tok = NextToken;
    eatNextToken();
    return Context.create<Expr>(Tok.Span, VarRef{Context.save(Tok.Value)});
  }
  case TokenKind::LParen: {
    auto LTok = NextToken;
    eatNextToken();
    if (NextToken.Kind == TokenKind::RParen) {
      auto RTok = NextToken;
      eatNextToken();
      return Context.create<Expr>(LTok.Span.merge(RTok.Span), UnitLiteral{});
    }
    Expr *Inner = parseExpr();
    if (NextToken.Kind != TokenKind::RParen)
      return errorExpectedRParen(LTok.Span);
    auto RTok = NextToken;
    eatNextToken();
    return Inner;
  }
  default:
    return errorExpectedExpr();
  }
}

static bool startsExpr(TokenKind Tok) {
  switch (Tok) {
  case TokenKind::Identifier:
  case TokenKind::IntLiteral:
  case TokenKind::FloatLiteral:
  case TokenKind::LParen:
  case TokenKind::True:
  case TokenKind::False:
  case TokenKind::Minus:
  case TokenKind::Not:
    return true;
  default:
    return false;
  }
}

Expr *Parser::parseCall() {
  Expr *ExprNode = parsePrimary();
  if (!ExprNode)
    return nullptr;
  while (true) {
    if (NextToken.Kind == TokenKind::LParen) {
      Token LParen = NextToken;
      eatNextToken(); // (
      llvm::SmallVector<Expr *, 8> Args;
      if (NextToken.Kind == TokenKind::RParen) {
        eatNextToken();
      } else {
        while (true) {
          Expr *Arg = parseExpr();
          if (!Arg)
            break;
          Args.push_back(Arg);
          if (NextToken.Kind == TokenKind::Comma) {
            eatNextToken();
            continue;
          }
          if (NextToken.Kind == TokenKind::RParen) {
            eatNextToken();
            break;
          }
          errorExpectedCommaOrRParenInCall(LParen.Span);
          if (NextToken.Kind == TokenKind::Comma) {
            eatNextToken();
            continue;
          }
          if (NextToken.Kind == TokenKind::RParen) {
            eatNextToken();
          }
          break;
        }
      }
      for (Expr *Arg : Args) {
        Span S = ExprNode->Location.merge(Arg->Location);
        ExprNode = Context.create<Expr>(S, CallExpr{ExprNode, Arg});
      }
      continue;
    }
    if (startsExpr(NextToken.Kind)) {
      Expr *Arg = parseUnary();
      if (!Arg)
        break;
      Span S = ExprNode->Location.merge(Arg->Location);
      ExprNode = Context.create<Expr>(S, CallExpr{ExprNode, Arg});
      continue;
    }
    break;
  }
  return ExprNode;
}

Expr *Parser::parseUnary() {
  switch (NextToken.Kind) {
  case TokenKind::Plus:
  case TokenKind::Not:
  case TokenKind::Minus: {
    Token OpTok = NextToken;
    eatNextToken();
    Expr *Operand = parseUnary();
    if (!Operand)
      return nullptr;
    UnaryOp Op;
    if (OpTok.Kind == TokenKind::Not)
      Op = UnaryOp::Not;
    else if (OpTok.Kind == TokenKind::Minus)
      Op = UnaryOp::Neg;
    else
      Op = UnaryOp::Plus;
    Span S = OpTok.Span.merge(Operand->Location);
    return Context.create<Expr>(S, UnaryExpr{Op, Operand});
  }
  default:
    return parseCall();
  }
}

Expr *Parser::parseExpr() { return parseUnary(); }
} // namespace rheo
