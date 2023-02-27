#pragma once

#include <map>
#include <string>
// Declares clang::SyntaxOnlyAction.
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
// Declares llvm::cl::extrahelp.
#include "llvm/Support/CommandLine.h"

#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"

bool areSameVariable(const clang::ValueDecl *First,
                            const clang::ValueDecl *Second);

/**
 * Gets the portion of the code that corresponds to given SourceRange, including
 * the last token. Returns expanded macros. Ref:
 * https://stackoverflow.com/questions/11083066/getting-the-source-behind-clangs-ast
 *
 * @see get_source_text_raw()
 */
std::string_view get_source_text(clang::SourceRange range,
                                 const clang::SourceManager &sm);

/**
 * Gets the portion of the code that corresponds to given SourceRange exactly as
 * the range is given.
 *
 * @warning The end location of the SourceRange returned by some Clang functions
 * (such as clang::Expr::getSourceRange) might actually point to the first
 * character (the "location") of the last token of the expression, rather than
 * the character past-the-end of the expression like clang::Lexer::getSourceText
 * expects. get_source_text_raw() does not take this into account. Use
 * get_source_text() instead if you want to get the source text including the
 * last token.
 *
 * @warning This function does not obtain the source of a macro/preprocessor
 * expansion. Use get_source_text() for that.
 */
std::string_view get_source_text_raw(clang::SourceRange range,
                                     const clang::SourceManager &sm);

class CodeNode {
protected:
  std::string_view *code;
  unsigned long long offset;
public:
  unsigned long long start_wd;
  unsigned long long end_wd;

  /**
   * @brief store its childs
   * @details key is the star_wd of the node, and its value is the node. Since
   * AST and all codes are some kind of tree structure, this will work well.
   *
   */
  std::map<unsigned long long, CodeNode *> childs;

  int level;
  CodeNode(unsigned long long _start_wd, unsigned long long _end_wd);
  CodeNode();
  virtual ~CodeNode();

  void get_source(std::string_view* _code, unsigned long long offset);


  /**
   * @brief this function will print the code it covers, will be overwritten.
   *
   */
  virtual void printCode() = 0;

  /**
   * @brief check the relative position of two nodes
   * @details return -1 if that node is prior to this, 1 if that node is
   * posterior, 0 if this node contains that node(or vice versa)
   * @warning this will only work when the AST input is a tree structure,
   * otherwise its behavior will be undefined
   * @param node
   * @return int
   */
  int CompareNode(CodeNode *node);
  /**
   * @brief is that node included in this node
   * @warning should only be used when AST input is a tree structure, otherwise
   * its behavior will be undefined
   * @param node
   * @return true
   * @return false
   */
  bool isInclude(CodeNode *node);

  /**
   * @brief insert a node into the tree, return the new root
   * @warning must ensure that the node is valid and that this node either
   * include the range of the inserted node or just the opposite
   * @param node
   * @return CodeNode*
   */
  CodeNode *Insert(CodeNode *node);


};



class OtherNode: public CodeNode
{
public:
  OtherNode(unsigned long long _start_wd, unsigned long long _end_wd);
  virtual void printCode() override;
};


std::map<unsigned long long, std::string_view> remove_outer_bracklets(std::map<unsigned long long, std::string_view> code_block);


std::map<unsigned long long, std::string_view> parse_lines(std::string_view code);

