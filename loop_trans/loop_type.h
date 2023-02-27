#pragma once

// Declares clang::SyntaxOnlyAction.
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
// Declares llvm::cl::extrahelp.
#include "llvm/Support/CommandLine.h"

#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"

#include "code_reprocess.h"

using namespace clang;
using namespace clang::ast_matchers;

/**
 * @brief this type is the middle form between AST Matcher format and codegen
 * @details A for-loop AST node is first converted to this class, then after
 * some format preprocess, will be sent to Output layer.
 * Also, this demo should tolerate some performance lost as it will eventually
 * introduce some manual check after the procedure
 * @property eventually this will look like: for (int i=0;i<100;i++)
 */
class loop_type_org {
public:
  bool isInitValid;    /** is the initializer's expr valid*/
  bool isCompValid;    /** is the compare expr valid*/
  bool isIncValid;     /** is the increment expr valid*/
  bool isRef;          /** is for-loop-variant a reference or a lambda variant*/
  bool isBinaryIncOp;  /**in the inc part, whether the Operator is binary*/
  std::string VarName; /** the for-loop-variant*/
  std::string InitialVal; /** the initial value of for-loop-variant*/
  std::string CompOP;  /** the Operator of the condition expr*/
  std::string CompLhsVar; /** the variant or whatever on the left hand side of
                             the condition expr, would be the same as
                             for-loop-variant after conversion */
  std::string CompRhsVar; /** the variant or whatever on the right hand side of
                             the conditionexpr*/
  std::string IncOp;      /** the Operator of the increment expr*/
  std::string IncLhsVar;  /** the variant or whatever on the left hand side of
                             the increment expr*/
  std::string IncRhsVar;  /** the variant or whatever on the right hand side of
                             the incremet expr(if exists)*/
    void dump();
  /* data */
};

/**
 * @brief this will convert the information of a for-loop AST node to a
 * loop_type_org object
 *
 *
 * @return loop_type_org
 */
loop_type_org loop_classify(const MatchFinder::MatchResult &Result,
                            const clang::ForStmt *FS);

// as it means
// inline auto hasIgnoringBraces = [](auto const &Matcher) {
//   return anyOf(Matcher, compoundStmt(has(Matcher)));
// };
// as it means
inline auto matchoptional = [](auto const &Matcher) { return anyOf(Matcher, anything()); };

/**
 * @brief this is the matcher for for-loop, integer variants limited.
 *
 */
const clang::ast_matchers::StatementMatcher LoopMatcher =
    forStmt(
        matchoptional(anyOf(
            hasLoopInit(declStmt(hasSingleDecl(
                varDecl(hasInitializer(integerLiteral(/*equals(0)*/).bind("InitVal")))
                    .bind("initVarName")))),
            hasLoopInit(binaryOperator(
                hasOperatorName("="),
                hasLHS(ignoringParenImpCasts(declRefExpr(
                    to(varDecl(hasType(isInteger())).bind("initrefVarName"))))),
                hasRHS(integerLiteral(/*equals(0)*/).bind("InitVal")))))),
        matchoptional(anyOf(
            hasIncrement(
                unaryOperator(
                    anyOf(hasOperatorName("++"), hasOperatorName("--")),
                    hasUnaryOperand(declRefExpr(
                        to(varDecl(hasType(isInteger())).bind("incVarName")))))
                    .bind("incUnaryOperator")),
            hasIncrement(
                binaryOperator(
                    anyOf(hasOperatorName("+="), hasOperatorName("*="),
                          hasOperatorName("-=")),
                    hasLHS(ignoringParenImpCasts(declRefExpr(
                        to(varDecl(hasType(isInteger()))
                               .bind("incVarName"))))), // need modification to
                                                        // diverge them into
                                                        // different types
                    hasRHS(integerLiteral(/*equals(0)*/).bind("IncValR")))
                    .bind("incBinaryOperator")))),
        matchoptional(hasCondition(
            binaryOperator(
                anyOf(hasOperatorName("<"), hasOperatorName(">"),
                      hasOperatorName("=="), hasOperatorName(">="),
                      hasOperatorName("<=")),
                hasLHS(ignoringParenImpCasts(declRefExpr(
                    to(varDecl(hasType(isInteger())).bind("condVarNameL"))))),
                hasRHS(anyOf(
                    integerLiteral(/*equals(0)*/).bind("CondValR"),
                    ignoringParenImpCasts(declRefExpr(to(
                        varDecl(hasType(isInteger())).bind("condVarNameR")))))))
                .bind("condBinaryOperator"))))
        .bind("forLoop");


class ForLoopNode: public CodeNode
{
public:
  loop_type_org type;
  ForLoopNode(const MatchFinder::MatchResult &Result, const ForStmt *FS);
  virtual void printCode() override;
};

