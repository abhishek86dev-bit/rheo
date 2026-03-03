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
  void skipNewLines();

  Type *parseType();

  Expr *parsePrimaryExpr();
  Expr *parseUnaryExpr();
  Expr *parseCallExpr();
  Expr *parseBinaryExpr(int MinOp = 0);
  Expr *parseExpr();

  Stmt *parseReturnStmt();
  Stmt *parseExprStmt();
  Stmt *parseExprOrAssignStmt();

  Type *errorUnexpectedType();
  Type *errorExpectedRParenInType(Span OpenParenSpan);
  Stmt *errorUnexpectedColonEqualAfterType(Span TypeSpan);
  Stmt *errorExpectedStmtTerminator(Span StmtSpan);
  Stmt *errorExpectedStmt();
  Stmt *errorInvalidAssignmentTarget(Expr *LHS);
  Stmt *errorInvalidDeclTarget(Expr *LHS);
  Stmt *errorExpectedIdentifierAfterMut(Span MutSpan);
  Expr *errorExpectedRParen(Span OpenParenSpan);
  Expr *errorExpectedExpr();
  Expr *errorExpectedCommaOrRParenInCall(Span OpenParenSpan);

public:
  Parser(ASTContext &Context, Lexer &Lex, DiagnosticEngine &Diags, FileId File)
      : Context(Context), Lex(Lex), NextToken(Lex.nextToken()), Diags(Diags),
        File(File) {}
  Stmt *parseStmt();
};

}; // namespace rheo

#endif // RHEO_PARSER_H
