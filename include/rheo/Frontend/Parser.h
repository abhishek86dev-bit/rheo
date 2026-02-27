#ifndef RHEO_PARSER_H
#define RHEO_PARSER_H

#include "rheo/AST/AST.h"
#include "rheo/Diagnostics/DiagnosticEngine.h"
#include "rheo/Frontend/Lexer.h"
#include "rheo/Frontend/Token.h"

namespace rheo {

class Parser {
  ASTContext &Context;
  Lexer &Lex;
  Token NextToken;
  bool HasError = false;
  DiagnosticEngine &Diags;

  void eatNextToken() { NextToken = Lex.nextToken(); }
  Expr *parsePrimary();

public:
  Parser(ASTContext &Context, Lexer &Lex, DiagnosticEngine &Diags)
      : Context(Context), Lex(Lex), NextToken(Lex.nextToken()), Diags(Diags) {}
};

}; // namespace rheo

#endif // RHEO_PARSER_H
