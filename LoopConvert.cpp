// Declares clang::SyntaxOnlyAction.
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
// Declares llvm::cl::extrahelp.
#include "llvm/Support/CommandLine.h"

#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/AST/ASTContext.h"

/*
this project starts with clang document
*/
#include <iostream>

// the following are user predefined libs
#include "./loop_trans/loop_type.h"
#include "./loop_trans/code_reprocess.h"

using namespace clang;
using namespace clang::ast_matchers;


class LoopPrinter : public MatchFinder::MatchCallback {
public :
  virtual void run(const MatchFinder::MatchResult &Result) ;
};

CodeNode* root_node=nullptr;
std::string_view * code_str = nullptr;

void LoopPrinter::run(const MatchFinder::MatchResult &Result) {   //need simplify later
  ASTContext *Context = Result.Context;
  static clang::SourceManager &smng = Context->getSourceManager();
  const ForStmt *FS = Result.Nodes.getNodeAs<ForStmt>("forLoop");
  if (!root_node)
  {
    auto start_pos = smng.getLocForStartOfFile(smng.getFileID(FS->getSourceRange().getBegin())).getRawEncoding();
    auto end_pos = smng.getLocForEndOfFile(smng.getFileID(FS->getSourceRange().getEnd())).getRawEncoding();
    auto startpos = SourceLocation::getFromRawEncoding(start_pos);
    auto endpos = SourceLocation::getFromRawEncoding(end_pos);
    code_str = new std::string_view(get_source_text(SourceRange(startpos, endpos), smng));   
    root_node = new OtherNode(start_pos, end_pos);
    root_node->get_source(code_str, start_pos);
  }
  
  // do not convert header files!
  if (!FS || !Context->getSourceManager().isWrittenInMainFile(FS->getForLoc()))
    return;
  root_node->Insert(new ForLoopNode(Result, FS));
  // FS->dump();
  

} 


using namespace clang::tooling;
using namespace llvm;

// Apply a custom category to all command-line options so that they are the
// only ones displayed.
static llvm::cl::OptionCategory MyToolCategory("my-tool options");

// CommonOptionsParser declares HelpMessage with a description of the common
// command-line options related to the compilation database and input files.
// It's nice to have this help message in all tools.
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);

// A help message for this specific tool can be added afterwards.
static cl::extrahelp MoreHelp("\nMore help text...\n");

int main(int argc, const char **argv) {
  auto ExpectedParser = CommonOptionsParser::create(argc, argv, MyToolCategory);
  if (!ExpectedParser) {
    // Fail gracefully for unsupported options.
    llvm::errs() << ExpectedParser.takeError();
    return 1;
  }
  CommonOptionsParser& OptionsParser = ExpectedParser.get();
  ClangTool Tool(OptionsParser.getCompilations(),
                 OptionsParser.getSourcePathList());

  LoopPrinter Printer;
  MatchFinder Finder;
  Finder.addMatcher(LoopMatcher, &Printer);
  auto tmp = Tool.run(newFrontendActionFactory(&Finder).get());
  root_node->printCode();
  return tmp;
}


