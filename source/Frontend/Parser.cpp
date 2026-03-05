#include "rheo/Frontend/Parser.h"
#include "rheo/AST/AST.h"
#include "rheo/Frontend/Token.h"
#include <format>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringRef.h>
#include <optional>
#include <string>
#include <variant>

namespace rheo {

static std::string describeToken(const Token &Tok) {
  using TK = TokenKind;

  switch (Tok.Kind) {
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

  case TK::Plus:
  case TK::Minus:
  case TK::Star:
  case TK::Slash:
  case TK::Percent:
  case TK::EqualEqual:
  case TK::BangEqual:
  case TK::Bang:
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

  case TK::IntLiteral:
    return std::format("integer literal {}", Tok.Value.str());

  case TK::FloatLiteral:
    return std::format("floating-point literal {}", Tok.Value.str());

  case TK::True:
  case TK::False:
    return std::format("boolean literal {}", Tok.Value.str());

  case TK::Identifier:
    return std::format("identifier '{}'", Tok.Value.str());

  case TK::Int:
  case TK::Int8:
  case TK::Int16:
  case TK::Int32:
  case TK::Int64:
  case TK::UInt:
  case TK::UInt8:
  case TK::UInt16:
  case TK::UInt32:
  case TK::UInt64:
  case TK::Float32:
  case TK::Float64:
  case TK::Bool:
    return std::format("type '{}'", Tok.Value.str());

  case TK::Def:
  case TK::End:
  case TK::Return:
  case TK::If:
  case TK::Else:
  case TK::While:
  case TK::Continue:
  case TK::Break:
  case TK::Mut:
  case TK::ElseIf:
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
  Diag.setHelp("every '(' must be closed with ')'");
  Diags.emit(Diag);
  return nullptr;
}

Expr *Parser::errorExpectedExpr() {
  Token Tok = NextToken;
  Diagnostic Diag(Severity::Error);
  Diag.setCode("E1002");
  auto Found = describeToken(Tok);
  Diag.setMessage(std::format("expected expression, found {}", Found));
  Diag.addLabel(
      Label::primary(Tok.Span, File, std::format("unexpected {}", Found)));
  Diag.setHelp(
      "expressions include literals, identifiers, or grouped expressions");
  Diags.emit(Diag);
  eatNextToken();
  return nullptr;
}

Expr *Parser::errorExpectedCommaOrRParenInCall(Span OpenParenSpan) {
  Token Tok = NextToken;
  Diagnostic Diag(Severity::Error);
  Diag.setMessage("expected ',' or ')'");
  Diag.setCode("E1003");
  Diag.addLabel(Label::primary(
      Tok.Span, File,
      std::format("found {} instead of ',' or ')'", describeToken(Tok))));
  Diag.addLabel(Label::secondary(OpenParenSpan, File, "call started here"));
  Diag.setHelp("separate arguments with ',' and close with ')'");
  Diags.emit(Diag);
  return nullptr;
}

Stmt *Parser::errorExpectedStmtTerminator(Span StmtSpan) {
  Token Tok = NextToken;
  Diagnostic Diag(Severity::Error);
  Diag.setCode("E1004");
  auto Found = describeToken(Tok);
  Diag.setMessage(
      std::format("expected ';' or newline after statement, found {}", Found));
  Diag.addLabel(
      Label::primary(Tok.Span, File, std::format("unexpected {}", Found)));
  Diag.addLabel(Label::secondary(StmtSpan, File, "statement starts here"));
  Diag.setHelp("terminate statements with ';' or newline");
  Diags.emit(Diag);
  return nullptr;
}

Stmt *Parser::errorExpectedStmt() {
  Token Tok = NextToken;
  Diagnostic Diag(Severity::Error);
  Diag.setCode("E1005");
  auto Found = describeToken(Tok);
  Diag.setMessage(std::format("expected statement, found {}", Found));
  Diag.addLabel(
      Label::primary(Tok.Span, File, std::format("unexpected {}", Found)));
  Diag.setHelp("expected function declaration, variable declaration, "
               "assignment, expression, or control flow");
  Diags.emit(Diag);
  return nullptr;
}

Stmt *Parser::errorInvalidAssignmentTarget(Expr *LHS) {
  Diagnostic Diag(Severity::Error);
  Diag.setCode("E1006");
  Diag.setMessage("invalid assignment target");
  Diag.addLabel(
      Label::primary(LHS->Location, File, "cannot assign to this expression"));
  Diag.setHelp("only variables can be assigned");
  Diags.emit(Diag);
  return nullptr;
}

Stmt *Parser::errorInvalidDeclTarget(Expr *LHS) {
  Diagnostic Diag(Severity::Error);
  Diag.setCode("E1007");
  Diag.setMessage("invalid declaration target");
  Diag.addLabel(
      Label::primary(LHS->Location, File, "cannot declare this expression"));
  Diag.setHelp("only identifiers can be declared");
  Diags.emit(Diag);
  return nullptr;
}

Type *Parser::errorUnexpectedType() {
  Token Tok = NextToken;
  Diagnostic Diag(Severity::Error);
  Diag.setCode("E1008");
  auto Found = describeToken(Tok);
  Diag.setMessage(std::format("expected type, found {}", Found));
  Diag.addLabel(
      Label::primary(Tok.Span, File, std::format("unexpected {}", Found)));
  Diag.setHelp("types include builtins and identifiers");
  Diags.emit(Diag);
  eatNextToken();
  return nullptr;
}

Type *Parser::errorExpectedRParenInType(Span OpenParenSpan) {
  Token Tok = NextToken;
  Diagnostic Diag(Severity::Error);
  Diag.setMessage("expected ')'");
  Diag.setCode("E1009");
  Diag.addLabel(Label::primary(
      Tok.Span, File,
      std::format("found {} instead of ')'", describeToken(Tok))));
  Diag.addLabel(
      Label::secondary(OpenParenSpan, File, "opening '(' in type here"));
  Diag.setHelp("type parentheses must be closed with ')'");
  Diags.emit(Diag);
  eatNextToken();
  return nullptr;
}

Stmt *Parser::errorUnexpectedColonEqualAfterType(Span TypeSpan) {
  Token Tok = NextToken;
  Diagnostic Diag(Severity::Error);
  Diag.setMessage("unexpected ':=' after type");
  Diag.setCode("E1010");
  Diag.addLabel(
      Label::primary(Tok.Span, File, "':=' cannot follow a type annotation"));
  Diag.addLabel(Label::secondary(TypeSpan, File, "type specified here"));
  Diag.setHelp("use '=' to assign a value after a type annotation");
  Diags.emit(Diag);
  eatNextToken();
  return nullptr;
}

Stmt *Parser::errorExpectedIdentifierAfterMut(Span MutSpan) {
  Token Tok = NextToken;
  Diagnostic Diag(Severity::Error);
  Diag.setMessage("expected identifier after 'mut'");
  Diag.setCode("E1011");
  Diag.addLabel(Label::primary(
      Tok.Span, File,
      std::format("found {} instead of identifier", describeToken(Tok))));
  Diag.addLabel(
      Label::secondary(MutSpan, File, "'mut' starts a mutable binding here"));
  Diag.setHelp("write 'mut <name>' to declare a mutable variable");
  Diags.emit(Diag);
  eatNextToken();
  return nullptr;
}

Stmt *Parser::errorInvalidMutInitializer() {
  auto Tok = NextToken;
  Diagnostic Diag(Severity::Error);
  Diag.setMessage("expected ':=' after mutable binding");
  Diag.setCode("E1013");
  Diag.addLabel(Label::primary(Tok.Span, File, "found '=' instead of ':='"));
  Diags.emit(Diag);
  eatNextToken();
  return nullptr;
}

Expr *Parser::errorExpectedThenBlock(Span IfSpan) {
  Token Tok = NextToken;
  Diagnostic Diag(Severity::Error);
  Diag.setMessage("expected newline after 'if' condition");
  Diag.setCode("E1014");
  Diag.addLabel(Label::primary(
      Tok.Span, File,
      std::format("found {} instead of newline", describeToken(Tok))));
  Diag.addLabel(Label::secondary(IfSpan, File, "'if' starts here"));
  Diag.setHelp("put the condition on the same line as 'if', then start the "
               "body on the next line");
  Diags.emit(Diag);
  return nullptr;
}

Expr *Parser::errorExpectedWhileBody(Span WhileSpan) {
  Token Tok = NextToken;
  Diagnostic Diag(Severity::Error);
  Diag.setMessage("expected newline after 'while' condition");
  Diag.setCode("E1015");
  Diag.addLabel(Label::primary(
      Tok.Span, File,
      std::format("found {} instead of newline", describeToken(Tok))));
  Diag.addLabel(Label::secondary(WhileSpan, File, "'while' starts here"));
  Diag.setHelp("put the condition on the same line as 'while', then start the "
               "body on the next line");
  Diags.emit(Diag);
  return nullptr;
}

void Parser::skipNewLines() {
  while (NextToken.Kind == TokenKind::NewLine)
    eatNextToken();
}

Type *Parser::parseType() {
  using TK = TokenKind;

  auto MakeBuiltin = [&](BuiltinKind K) -> Type * {
    auto Tok = NextToken;
    eatNextToken();
    return Context.create<Type>(Tok.Span, BuiltinType{K});
  };

  switch (NextToken.Kind) {
  case TK::Bool:
    return MakeBuiltin(BuiltinKind::Bool);
  case TK::Int:
    return MakeBuiltin(BuiltinKind::Int);
  case TK::Int8:
    return MakeBuiltin(BuiltinKind::I8);
  case TK::Int16:
    return MakeBuiltin(BuiltinKind::I16);
  case TK::Int32:
    return MakeBuiltin(BuiltinKind::I32);
  case TK::Int64:
    return MakeBuiltin(BuiltinKind::I64);
  case TK::UInt:
    return MakeBuiltin(BuiltinKind::UInt);
  case TK::UInt8:
    return MakeBuiltin(BuiltinKind::U8);
  case TK::UInt16:
    return MakeBuiltin(BuiltinKind::U16);
  case TK::UInt32:
    return MakeBuiltin(BuiltinKind::U32);
  case TK::UInt64:
    return MakeBuiltin(BuiltinKind::U64);
  case TK::Float32:
    return MakeBuiltin(BuiltinKind::F32);
  case TK::Float64:
    return MakeBuiltin(BuiltinKind::F64);
  case TK::Bang:
    return MakeBuiltin(BuiltinKind::Never);

  case TK::LParen: {
    auto LTok = NextToken;
    eatNextToken();
    if (NextToken.Kind != TK::RParen)
      return errorExpectedRParenInType(LTok.Span);
    auto RTok = NextToken;
    eatNextToken();
    return Context.create<Type>(LTok.Span.merge(RTok.Span),
                                BuiltinType{BuiltinKind::Unit});
  }

  case TK::Identifier: {
    auto Tok = NextToken;
    eatNextToken();
    return Context.create<Type>(Tok.Span, NamedType{Context.save(Tok.Value)});
  }

  default:
    return errorUnexpectedType();
  }
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
  case TokenKind::Continue: {
    auto Loc = NextToken.Span;
    eatNextToken();
    return Context.create<Expr>(Loc, ContinueExpr{});
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
    if (!Inner)
      return nullptr;
    if (NextToken.Kind != TokenKind::RParen)
      return errorExpectedRParen(LTok.Span);
    eatNextToken();
    return Inner;
  }
  case TokenKind::Break:
    return parseBreakExpr();
  case TokenKind::If:
    return parseIf();
  case TokenKind::While:
    return parseWhile();
  default:
    return errorExpectedExpr();
  }
}

Expr *Parser::parseBreakExpr() {
  auto Loc = NextToken.Span;
  eatNextToken();
  Expr *Value = nullptr;
  if (NextToken.Kind != TokenKind::NewLine &&
      NextToken.Kind != TokenKind::Semicolon &&
      NextToken.Kind != TokenKind::Eof) {
    Value = parseExpr();
    if (!Value)
      return nullptr;
    Loc = Loc.merge(Value->Location);
  }
  return Context.create<Expr>(Loc, BreakExpr{Value});
}

static bool isAtomStart(TokenKind K) {
  switch (K) {
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
      eatNextToken();
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
          if (NextToken.Kind == TokenKind::RParen)
            eatNextToken();
          break;
        }
      }

      for (Expr *Arg : Args) {
        Span S = ExprNode->Location.merge(Arg->Location);
        ExprNode = Context.create<Expr>(S, CallExpr{ExprNode, Arg});
      }
      continue;
    }

    if (isAtomStart(NextToken.Kind)) {
      Expr *Arg = parsePrimaryExpr();
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
    return Context.create<Expr>(OpTok.Span.merge(Operand->Location),
                                UnaryExpr{Op, Operand});
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

Expr *Parser::parseBinaryExpr(int MinPrec) {
  Expr *Left = parseUnaryExpr();
  if (!Left)
    return nullptr;

  while (true) {
    if (NextToken.Kind == TokenKind::NewLine)
      break;

    auto Op = toBinaryOp(NextToken.Kind);
    if (!Op.has_value())
      break;

    int Prec = getBinOpPrecedence(*Op);
    if (Prec < MinPrec)
      break;

    Token OpTok = NextToken;
    eatNextToken();

    Expr *Right = parseBinaryExpr(Prec + 1);
    if (!Right)
      return Left;

    Span S = Left->Location.merge(Right->Location);
    Left = Context.create<Expr>(S, BinaryExpr{*Op, Left, Right});
  }

  return Left;
}

Expr *Parser::parseExpr() { return parseBinaryExpr(); }

Expr *Parser::parseIf() {
  auto IfTok = NextToken;
  eatNextToken();

  auto *Cond = parseExpr();
  if (!Cond)
    return nullptr;

  if (NextToken.Kind != TokenKind::NewLine &&
      NextToken.Kind != TokenKind::Semicolon)
    return errorExpectedThenBlock(IfTok.Span);
  eatNextToken();

  BlockExpr *Then =
      parseBlock({TokenKind::End, TokenKind::ElseIf, TokenKind::Else});
  BlockExpr *ElseBlock = nullptr;

  if (NextToken.Kind == TokenKind::ElseIf) {
    auto *ElseIf = parseIf();
    if (!ElseIf)
      return nullptr;
    auto StmtsRef = Context.copyArray(llvm::ArrayRef<Stmt *>({}));
    ElseBlock = Context.create<BlockExpr>(StmtsRef, ElseIf);
  } else if (NextToken.Kind == TokenKind::Else) {
    eatNextToken();
    if (NextToken.Kind == TokenKind::NewLine ||
        NextToken.Kind == TokenKind::Semicolon)
      eatNextToken();
    ElseBlock = parseBlock({TokenKind::End});
    if (!ElseBlock)
      return nullptr;
    eatNextToken();
  } else {
    eatNextToken();
  }

  return Context.create<Expr>(IfTok.Span.merge(Cond->Location),
                              IfExpr{Cond, Then, ElseBlock});
}

Expr *Parser::parseWhile() {
  auto WhileTok = NextToken;
  eatNextToken();

  auto *Cond = parseExpr();
  if (!Cond)
    return nullptr;

  if (NextToken.Kind != TokenKind::NewLine &&
      NextToken.Kind != TokenKind::Semicolon)
    return errorExpectedWhileBody(WhileTok.Span);
  eatNextToken();

  auto *Body = parseBlock({TokenKind::End});
  return Context.create<Expr>(WhileTok.Span.merge(Cond->Location),
                              WhileExpr{Cond, Body});
}

Stmt *Parser::parseReturnStmt() {
  auto ReturnLoc = NextToken.Span;
  eatNextToken();

  if (NextToken.Kind == TokenKind::NewLine ||
      NextToken.Kind == TokenKind::Semicolon) {
    eatNextToken();
    return Context.create<Stmt>(ReturnLoc, ReturnStmt{nullptr});
  }

  auto *Expr = parseExpr();
  if (!Expr)
    return nullptr;

  return Context.create<Stmt>(ReturnLoc.merge(Expr->Location),
                              ReturnStmt{Expr});
}

Stmt *Parser::parseExprStmt() {
  auto *Expr = parseExpr();
  if (!Expr)
    return nullptr;
  return Context.create<Stmt>(Expr->Location, ExprStmt{Expr});
}

Stmt *Parser::parseExprOrAssignStmt() {
  bool IsMutable = false;
  std::optional<Span> MutSpan;
  if (NextToken.Kind == TokenKind::Mut) {
    MutSpan = NextToken.Span;
    IsMutable = true;
    eatNextToken();
    if (NextToken.Kind != TokenKind::Identifier)
      return errorExpectedIdentifierAfterMut(*MutSpan);
  }
  Expr *LHS = parseExpr();
  if (!LHS)
    return nullptr;
  if (NextToken.Kind == TokenKind::Equal) {
    if (IsMutable)
      return errorInvalidMutInitializer();
    eatNextToken();
    auto *RHS = parseExpr();
    if (!RHS)
      return nullptr;
    if (!std::holds_alternative<VarRef>(LHS->Kind))
      return errorInvalidAssignmentTarget(LHS);
    return Context.create<Stmt>(LHS->Location.merge(RHS->Location),
                                AssignStmt{LHS, RHS});
  }

  if (NextToken.Kind == TokenKind::ColonEqual ||
      NextToken.Kind == TokenKind::Colon) {
    Type *Ty = nullptr;
    if (NextToken.Kind == TokenKind::Colon) {
      eatNextToken();
      auto TypeTok = NextToken;
      Ty = parseType();
      if (!Ty)
        return nullptr;
      if (NextToken.Kind != TokenKind::ColonEqual)
        return errorUnexpectedColonEqualAfterType(TypeTok.Span);
    }
    eatNextToken();
    auto *RHS = parseExpr();
    if (!RHS)
      return nullptr;
    if (!std::holds_alternative<VarRef>(LHS->Kind))
      return errorInvalidDeclTarget(LHS);
    auto Name = std::get<VarRef>(LHS->Kind).Name;
    return Context.create<Stmt>(LHS->Location.merge(RHS->Location),
                                VarDecl{Name, Ty, RHS, IsMutable});
  }

  return Context.create<Stmt>(LHS->Location, ExprStmt{LHS});
}

BlockExpr *Parser::parseBlock(llvm::ArrayRef<TokenKind> Terminators) {
  llvm::SmallVector<Stmt *, 8> Stmts;

  while (NextToken.Kind != TokenKind::Eof) {
    skipNewLines();
    if (llvm::is_contained(Terminators, NextToken.Kind))
      break;

    auto *S = parseStmt();
    if (!S) {
      while (NextToken.Kind != TokenKind::NewLine &&
             NextToken.Kind != TokenKind::Semicolon &&
             NextToken.Kind != TokenKind::Eof &&
             !llvm::is_contained(Terminators, NextToken.Kind))
        eatNextToken();
      continue;
    }

    Stmts.push_back(S);
  }

  Expr *Tail = nullptr;
  if (!Stmts.empty()) {
    auto *Last = Stmts.back();
    if (std::holds_alternative<ExprStmt>(Last->Kind)) {
      Tail = std::get<ExprStmt>(Last->Kind).Expr;
      Stmts.pop_back();
    }
  }

  return Context.create<BlockExpr>(Context.copyArray(llvm::ArrayRef(Stmts)),
                                   Tail);
}

void Parser::errorExpectedParamName() {
  Token Tok = NextToken;
  Diagnostic Diag(Severity::Error);
  Diag.setMessage("expected parameter name");
  Diag.setCode("E1020");
  Diag.addLabel(Label::primary(
      Tok.Span, File,
      std::format("found {} instead of identifier", describeToken(Tok))));
  Diags.emit(Diag);
  eatNextToken();
}

std::optional<Param> Parser::parseParam() {
  if (NextToken.Kind != TokenKind::Identifier) {
    errorExpectedParamName();
    return std::nullopt;
  }
  auto Name = Context.save(NextToken.Value);
  auto Loc = NextToken.Span;
  eatNextToken();
  Type *Ty = nullptr;
  if (NextToken.Kind == TokenKind::Colon) {
    eatNextToken();
    Ty = parseType();
    if (!Ty)
      return std::nullopt;
    Loc = Loc.merge(Ty->Location);
  }
  return Param{Name, Ty, Loc};
}

void Parser::errorExpectedCommaAfterParam(Span ParamSpan) {
  Token Tok = NextToken;

  Diagnostic Diag(Severity::Error);
  Diag.setMessage("expected ',' after parameter");
  Diag.setCode("E1021");

  Diag.addLabel(Label::primary(
      Tok.Span, File,
      std::format("found {} instead of ','", describeToken(Tok))));

  Diag.addLabel(Label::secondary(ParamSpan, File, "parameter declared here"));

  Diags.emit(Diag);
}

llvm::ArrayRef<Param> Parser::parseParamList() {
  eatNextToken();
  llvm::SmallVector<Param, 8> Params;
  auto SyncToParamBoundary = [&]() {
    while (NextToken.Kind != TokenKind::Eof &&
           NextToken.Kind != TokenKind::Comma &&
           NextToken.Kind != TokenKind::RParen)
      eatNextToken();
    if (NextToken.Kind == TokenKind::Comma)
      eatNextToken();
  };
  while (NextToken.Kind != TokenKind::Eof &&
         NextToken.Kind != TokenKind::RParen) {
    auto Param = parseParam();
    if (!Param.has_value()) {
      SyncToParamBoundary();
      continue;
    }
    Params.push_back(*Param);
    if (NextToken.Kind == TokenKind::Comma) {
      eatNextToken();
      continue;
    }
    if (NextToken.Kind != TokenKind::RParen) {
      errorExpectedCommaAfterParam(Param->Location);
      SyncToParamBoundary();
    }
  }
  if (NextToken.Kind == TokenKind::RParen)
    eatNextToken();
  return Context.copyArray(llvm::ArrayRef(Params));
}

void Parser::errorExpectedFunctionName(Span FnSpan) {
  Token Tok = NextToken;
  Diagnostic Diag(Severity::Error);
  Diag.setMessage("expected function name");
  Diag.setCode("E1022");
  Diag.addLabel(Label::primary(
      Tok.Span, File,
      std::format("found {} instead of identifier", describeToken(Tok))));
  Diag.addLabel(Label::secondary(FnSpan, File, "'fn' declared here"));
  Diags.emit(Diag);
}

void Parser::errorExpectedFunctionBody(Span FnSpan) {
  Token Tok = NextToken;
  Diagnostic Diag(Severity::Error);
  Diag.setMessage("expected newline after function signature");
  Diag.setCode("E1023");
  Diag.addLabel(Label::primary(
      Tok.Span, File,
      std::format("found {} instead of newline", describeToken(Tok))));

  Diag.addLabel(Label::secondary(FnSpan, File, "'def' starts here"));
  Diag.setHelp("put the function signature on one line, "
               "then start the body on the next line");
  Diags.emit(Diag);
}

FunctionDecl *Parser::parseFunc() {
  auto FnSanp = NextToken.Span;
  eatNextToken();
  if (NextToken.Kind != TokenKind::Identifier) {
    errorExpectedFunctionName(FnSanp);
    eatNextToken();
    return nullptr;
  }
  auto Name = Context.save(NextToken.Value);
  auto Loc = NextToken.Span;
  eatNextToken();
  llvm::ArrayRef<Param> Params = {};
  if (NextToken.Kind == TokenKind::LParen)
    Params = parseParamList();
  Type *ReturnType = nullptr;
  if (NextToken.Kind == TokenKind::Arrow) {
    eatNextToken();
    ReturnType = parseType();
    if (!ReturnType)
      return nullptr;
  }
  if (NextToken.Kind != TokenKind::NewLine &&
      NextToken.Kind != TokenKind::Semicolon) {
    errorExpectedFunctionBody(FnSanp);
    eatNextToken();
    return nullptr;
  }
  eatNextToken();
  BlockExpr *Body = parseBlock({TokenKind::End});
  eatNextToken();
  return Context.create<FunctionDecl>(Name, Params, ReturnType, Body, Loc);
}

Stmt *Parser::parseStmt() {
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
  case TokenKind::If:
  case TokenKind::While:
  case TokenKind::Continue:
  case TokenKind::Break:
    S = parseExprStmt();
    break;
  case TokenKind::Identifier:
  case TokenKind::Mut:
    S = parseExprOrAssignStmt();
    break;
  case TokenKind::Def: {
    auto Loc = NextToken.Span;
    FunctionDecl *F = parseFunc();
    if (!F)
      return nullptr;
    return Context.create<Stmt>(Loc, F);
  }
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

Module Parser::parseModule(llvm::StringRef Name) {
  llvm::SmallVector<Stmt *, 8> Stmts;
  while (NextToken.Kind != TokenKind::Eof) {
    skipNewLines();
    if (NextToken.Kind == TokenKind::Eof)
      break;
    Stmt *S = parseStmt();
    if (!S) {
      while (NextToken.Kind != TokenKind::NewLine &&
             NextToken.Kind != TokenKind::Semicolon &&
             NextToken.Kind != TokenKind::Eof)
        eatNextToken();
      continue;
    }
    Stmts.push_back(S);
  }
  return Module{Context.save(Name), Context.copyArray(llvm::ArrayRef(Stmts))};
}

} // namespace rheo
