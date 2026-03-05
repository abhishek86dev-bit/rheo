#ifndef RHEO_PARSER_H
#define RHEO_PARSER_H

#include "rheo/AST/AST.h"
#include "rheo/Diagnostics/DiagnosticEngine.h"
#include "rheo/Diagnostics/SourceLocation.h"
#include "rheo/Frontend/Lexer.h"
#include "rheo/Frontend/Token.h"
#include <llvm/ADT/ArrayRef.h>
#include <optional>

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
  Expr *parseIf();
  Expr *parseWhile();
  Expr *parseBreakExpr();

  Stmt *parseReturnStmt();
  Stmt *parseExprStmt();
  Stmt *parseExprOrAssignStmt();
  Stmt *parseStmt();

  std::optional<Param> parseParam();
  llvm::ArrayRef<Param> parseParamList();
  BlockExpr *parseBlock(llvm::ArrayRef<TokenKind> Terminator);

  Type *errorUnexpectedType();
  Type *errorExpectedRParenInType(Span OpenParenSpan);
  Stmt *errorUnexpectedColonEqualAfterType(Span TypeSpan);
  Stmt *errorExpectedStmtTerminator(Span StmtSpan);
  Stmt *errorExpectedStmt();
  Stmt *errorInvalidMutInitializer();
  Stmt *errorInvalidAssignmentTarget(Expr *LHS);
  Stmt *errorInvalidDeclTarget(Expr *LHS);
  Stmt *errorExpectedIdentifierAfterMut(Span MutSpan);
  Expr *errorExpectedRParen(Span OpenParenSpan);
  Expr *errorExpectedExpr();
  Expr *errorExpectedCommaOrRParenInCall(Span OpenParenSpan);
  Expr *errorExpectedThenBlock(Span IfSpan);
  Expr *errorExpectedWhileBody(Span WhileSpan);
  void errorExpectedParamName();
  void errorExpectedCommaAfterParam(Span ParamSpan);
  void errorExpectedFunctionName(Span FnSpan);
  void errorExpectedFunctionBody(Span FnSpan);

public:
  Parser(ASTContext &Context, Lexer &Lex, DiagnosticEngine &Diags, FileId File)
      : Context(Context), Lex(Lex), NextToken(Lex.nextToken()), Diags(Diags),
        File(File) {}

  FunctionDecl *parseFunc();
};

}; // namespace rheo

#endif // RHEO_PARSER_H
