#include "rheo/Frontend/Parser.h"
#include "rheo/AST/AST.h"
#include "rheo/Frontend/Token.h"
#include <string>

namespace rheo {

Expr *Parser::parsePrimary() {
  auto Location = NextToken.Span;
  auto Value = NextToken.Value;
  switch (NextToken.Kind) {
  case TokenKind::True:
    eatNextToken();
    return Context.create<Expr>(Location, BoolLiteral{true});
  case TokenKind::False:
    eatNextToken();
    return Context.create<Expr>(Location, BoolLiteral{false});
  case TokenKind::FloatLiteral:
    eatNextToken();
    return Context.create<Expr>(Location, FloatLiteral{std::stof(Value.str())});
  case TokenKind::IntLiteral:
    eatNextToken();
    return Context.create<Expr>(Location, IntLiteral{static_cast<std::uint64_t>(
                                              std::stoll(Value.str()))});
  case TokenKind::LParen:

  default:
    llvm_unreachable("TODO: not implemented");
  }
}

} // namespace rheo
