#include "rheo/Frontend/Parser.h"
#include "rheo/AST/AST.h"
#include "rheo/Frontend/Token.h"
#include <format>
#include <llvm/ADT/SmallVector.h>
#include <string>
#include <variant>

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
  case TK::ColonEqual:
    return "':='";

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

Stmt *Parser::errorExpectedStmtTerminator(Span StmtSpan) {
  Token Tok = NextToken;

  Diagnostic Diag(Severity::Error);
  Diag.setCode("E1004");

  auto Found = describeToken(Tok);

  Diag.setMessage(
      std::format("expected ';' or newline after statement, found {}", Found));

  Diag.addLabel(Label::primary(
      Tok.Span, File,
      std::format("unexpected {} — statement must end here", Found)));

  Diag.addLabel(Label::secondary(StmtSpan, File, "statement starts here"));

  Diag.setHelp("terminate statements with:\n"
               "  • ';'  explicit terminator\n"
               "  • newline  implicit terminator\n");

  Diags.emit(Diag);

  while (NextToken.Kind != TokenKind::Semicolon &&
         NextToken.Kind != TokenKind::NewLine &&
         NextToken.Kind != TokenKind::Eof) {
    eatNextToken();
  }

  if (NextToken.Kind == TokenKind::Semicolon ||
      NextToken.Kind == TokenKind::NewLine) {
    eatNextToken();
  }

  return nullptr;
}

Stmt *Parser::errorExpectedStmt() {
  Token Tok = NextToken;

  Diagnostic Diag(Severity::Error);
  Diag.setCode("E1005");

  auto Found = describeToken(Tok);

  Diag.setMessage(std::format("expected a statement, found {}", Found));

  Diag.addLabel(Label::primary(
      Tok.Span, File,
      std::format("unexpected {} — statement expected here", Found)));

  Diag.setHelp("statements include:\n"
               "  • variable declarations: let x = 1\n"
               "  • assignments: x = 1\n"
               "  • expression statements: f(x)\n"
               "  • control flow: if, while, for\n");

  Diags.emit(Diag);

  while (NextToken.Kind != TokenKind::NewLine &&
         NextToken.Kind != TokenKind::Semicolon &&
         NextToken.Kind != TokenKind::RBrace &&
         NextToken.Kind != TokenKind::Eof) {
    eatNextToken();
  }

  if (NextToken.Kind == TokenKind::Semicolon ||
      NextToken.Kind == TokenKind::NewLine) {
    eatNextToken();
  }

  return nullptr;
}

Stmt *Parser::errorInvalidAssignmentTarget(Expr *LHS) {
  Diagnostic Diag(Severity::Error);
  Diag.setCode("E1006");

  Diag.setMessage("invalid assignment target");

  Diag.addLabel(
      Label::primary(LHS->Location, File, "cannot assign to this expression"));

  Diag.setHelp("only variables can be assigned to\n"
               "  • valid:   x = 10\n"
               "  • invalid: f() = 10\n"
               "  • invalid: 1 + 2 = 10");

  Diags.emit(Diag);
  return nullptr;
}

Stmt *Parser::errorInvalidDeclTarget(Expr *LHS) {
  Diagnostic Diag(Severity::Error);
  Diag.setCode("E1007");

  Diag.setMessage("invalid declaration target");

  Diag.addLabel(Label::primary(LHS->Location, File,
                               "cannot declare this expression as a variable"));

  Diag.setHelp("only identifiers can be declared\n"
               "  • valid:   x := 10\n"
               "  • invalid: f() := 10");

  Diags.emit(Diag);
  return nullptr;
}

void Parser::skipNewLines() {
  while (NextToken.Kind == TokenKind::NewLine)
    eatNextToken();
}

Expr *Parser::parsePrimaryExpr() {
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

static bool startsWithAtom(TokenKind Tok) {
  switch (Tok) {
  case TokenKind::Identifier:
  case TokenKind::IntLiteral:
  case TokenKind::FloatLiteral:
  case TokenKind::LParen:
  case TokenKind::True:
  case TokenKind::False:
    return true;
  default:
    return false;
  }
}

Expr *Parser::parseCallExpr() {
  Expr *ExprNode = parsePrimaryExpr();
  if (!ExprNode)
    return nullptr;
  while (true) {
    if (NextToken.Kind == TokenKind::LParen) {
      Token LParen = NextToken;
      eatNextToken(); // (
      skipNewLines();
      llvm::SmallVector<Expr *, 8> Args;
      if (NextToken.Kind == TokenKind::RParen) {
        eatNextToken();
      } else {
        while (true) {
          skipNewLines();
          Expr *Arg = parseExpr();
          if (!Arg)
            break;
          Args.push_back(Arg);
          skipNewLines();
          if (NextToken.Kind == TokenKind::Comma) {
            eatNextToken();
            skipNewLines();
            continue;
          }
          if (NextToken.Kind == TokenKind::RParen) {
            eatNextToken();
            break;
          }
          errorExpectedCommaOrRParenInCall(LParen.Span);
          if (NextToken.Kind == TokenKind::Comma) {
            eatNextToken();
            skipNewLines();
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
    if (startsWithAtom(NextToken.Kind)) {
      skipNewLines();
      Expr *Arg = parseUnaryExpr();
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

Expr *Parser::parseUnaryExpr() {
  switch (NextToken.Kind) {
  case TokenKind::Plus:
  case TokenKind::Not:
  case TokenKind::Minus: {
    Token OpTok = NextToken;
    eatNextToken();
    Expr *Operand = parseUnaryExpr();
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
    return parseCallExpr();
  }
}

static std::optional<BinaryOp> toBinaryOp(TokenKind K) {
  switch (K) {
  case TokenKind::Plus:
    return BinaryOp::Add;
  case TokenKind::Minus:
    return BinaryOp::Sub;
  case TokenKind::Star:
    return BinaryOp::Mul;
  case TokenKind::Slash:
    return BinaryOp::Div;
  case TokenKind::Percent:
    return BinaryOp::Mod;

  case TokenKind::EqualEqual:
    return BinaryOp::Eq;
  case TokenKind::BangEqual:
    return BinaryOp::NotEq;

  case TokenKind::Less:
    return BinaryOp::Lt;
  case TokenKind::LessEqual:
    return BinaryOp::Le;
  case TokenKind::Greater:
    return BinaryOp::Gt;
  case TokenKind::GreaterEqual:
    return BinaryOp::Ge;

  case TokenKind::And:
    return BinaryOp::And;
  case TokenKind::Or:
    return BinaryOp::Or;

  default:
    return std::nullopt;
  }
}

static int getBinOpPrecedence(BinaryOp Op) {
  switch (Op) {
  case BinaryOp::Or:
    return 1;
  case BinaryOp::And:
    return 2;

  case BinaryOp::Eq:
  case BinaryOp::NotEq:
    return 3;

  case BinaryOp::Lt:
  case BinaryOp::Le:
  case BinaryOp::Gt:
  case BinaryOp::Ge:
    return 4;

  case BinaryOp::Add:
  case BinaryOp::Sub:
    return 5;

  case BinaryOp::Mul:
  case BinaryOp::Div:
  case BinaryOp::Mod:
    return 6;
  }
  return -1;
}

Expr *Parser::parseBinaryExpr(int MinOp) {
  auto *Left = parseUnaryExpr();
  while (true) {
    skipNewLines();
    auto Tok = NextToken;
    auto Op = toBinaryOp(Tok.Kind);
    if (!Op.has_value()) {
      break;
    }
    auto Bp = getBinOpPrecedence(*Op);
    if (Bp < MinOp)
      break;
    eatNextToken();
    auto *Right = parseBinaryExpr(Bp + 1);
    if (!Right)
      return Left;
    auto S = Tok.Span.merge(Right->Location);
    Left = Context.create<Expr>(Expr(S, BinaryExpr{*Op, Left, Right}));
  }
  return Left;
}

Expr *Parser::parseExpr() { return parseBinaryExpr(); }

Stmt *Parser::parseReturnStmt() {
  auto ReturnLoc = NextToken.Span;
  eatNextToken(); // return
  if (NextToken.Kind == TokenKind::NewLine ||
      NextToken.Kind == TokenKind::Semicolon) {
    eatNextToken();
    return Context.create<Stmt>(ReturnLoc, ReturnStmt{nullptr});
  }
  auto *Expr = parseExpr();
  if (!Expr)
    return nullptr;
  auto Loc = ReturnLoc.merge(Expr->Location);
  return Context.create<Stmt>(Loc, ReturnStmt{Expr});
}

Stmt *Parser::parseExprStmt() {
  auto *Expr = parseExpr();
  if (!Expr)
    return nullptr;
  return Context.create<Stmt>(Expr->Location, ExprStmt{Expr});
}

Stmt *Parser::parseExprOrAssignStmt() {
  Expr *LHS = parseExpr();
  if (!LHS)
    return nullptr;
  if (NextToken.Kind == TokenKind::Equal) {
    eatNextToken();
    auto *RHS = parseExpr();
    if (!RHS)
      return nullptr;
    if (!std::holds_alternative<VarRef>(LHS->Kind))
      return errorInvalidAssignmentTarget(LHS);
    auto Loc = LHS->Location.merge(RHS->Location);
    return Context.create<Stmt>(Loc, AssignStmt{LHS, RHS});
  }
  return Context.create<Stmt>(LHS->Location, ExprStmt{LHS});
}

Stmt *Parser::parseStmt() {
  skipNewLines();
  auto Start = NextToken.Span;
  Stmt *S = nullptr;
  switch (NextToken.Kind) {
  case TokenKind::Return:
    S = parseReturnStmt();
    break;
  case TokenKind::True:
  case TokenKind::False:
  case TokenKind::IntLiteral:
  case TokenKind::FloatLiteral:
  case TokenKind::Minus:
  case TokenKind::Plus:
  case TokenKind::Not:
  case TokenKind::LParen:
    S = parseExprStmt();
    break;
  case TokenKind::Identifier:
    S = parseExprOrAssignStmt();
    break;
  default:
    return errorExpectedStmt();
  }
  if (!S)
    return nullptr;
  if (NextToken.Kind == TokenKind::NewLine ||
      NextToken.Kind == TokenKind::Semicolon ||
      NextToken.Kind == TokenKind::Eof) {
    if (NextToken.Kind != TokenKind::Eof)
      eatNextToken();
    return S;
  }
  return errorExpectedStmtTerminator(Start);
}

} // namespace rheo
