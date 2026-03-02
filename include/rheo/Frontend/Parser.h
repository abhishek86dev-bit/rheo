#ifndef RHEO_PARSER_H
#define RHEO_PARSER_H

#include "rheo/AST/AST.h"
#include "rheo/Diagnostics/DiagnosticEngine.h"
#include "rheo/Diagnostics/SourceLocation.h"
#include "rheo/Frontend/Lexer.h"
#include "rheo/Frontend/Token.h"

namespace rheo {

class Parser {
  ASTContext &Context;
  Lexer &Lex;
  Token NextToken;
  DiagnosticEngine &Diags;
  FileId File;

  void eatNextToken() { NextToken = Lex.nextToken(); }
  Expr *parsePrimary();
  Expr *parseUnary();
  Expr *parseCall();
  Expr *parseBinary(int MinOp = 0);

  Expr *errorExpectedRParen(Span OpenParenSpan);
  Expr *errorExpectedExpr();
  Expr *errorExpectedCommaOrRParenInCall(Span OpenParenSpan);

public:
  Expr *parseExpr();

  Parser(ASTContext &Context, Lexer &Lex, DiagnosticEngine &Diags, FileId File)
      : Context(Context), Lex(Lex), NextToken(Lex.nextToken()), Diags(Diags),
        File(File) {}
};

}; // namespace rheo

#endif // RHEO_PARSER_H
