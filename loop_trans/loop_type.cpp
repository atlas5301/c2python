#include "loop_type.h"

void loop_type_org::dump()
    {
        if (isInitValid)
        {
            llvm::outs() << "var: " << VarName << " val: " << InitialVal << " isref: " << isRef << "\n";
        } 
        if (isCompValid)
        {
            llvm::outs() << CompLhsVar << " " << CompOP << " " << CompRhsVar << "\n";
        }
        if (isIncValid)
        {
            llvm::outs() << IncLhsVar << " " <<IncOp;
            if (isBinaryIncOp)
            {
                llvm::outs() << " " << IncRhsVar;
            }
            llvm::outs() << "\n";
        }
    }


std::string getValuefromIntegerLiterral(const IntegerLiteral *InitVal) {
  llvm::SmallString<50> my_dump_buff;
  InitVal->getValue().toStringSigned(my_dump_buff);
  return (std::string)((std::string_view)(llvm::StringRef)my_dump_buff);
}

const VarDecl *checkLoopInit(const VarDecl *InitVar, const VarDecl *InitrefVar,
                             const IntegerLiteral *InitVal,
                             loop_type_org &loop_node) {
  if (InitVar) {
    // check if it is not a reference but a definition
    loop_node.isRef = false;
    loop_node.isInitValid = true;
    loop_node.VarName = InitVar->getNameAsString();
    loop_node.InitialVal = getValuefromIntegerLiterral(InitVal);
    return InitVar;
  } else if (InitrefVar) {
    // in this case, the initializer use a reference instead of a definition
    loop_node.isRef = true;
    loop_node.isInitValid = true;
    loop_node.VarName = InitrefVar->getNameAsString();
    loop_node.InitialVal = getValuefromIntegerLiterral(InitVal);
    return InitrefVar;
  } else { // in this case, initializer does not appear or is invalid
    loop_node.isInitValid = false;
    return nullptr;
  }
}

bool operatorFlip(const std::string_view &src, std::string &target) {
  if (src == std::string("<")) {
    target = ">";
    return true;
  }
  if (src == std::string("<=")) {
    target = ">=";
    return true;
  }
  if (src == std::string("==")) {
    target = "==";
    return true;
  }
  if (src == std::string(">=")) {
    target = "<=";
    return true;
  }
  if (src == std::string(">")) {
    target = "<";
    return true;
  }
  return false;
}

void checkLoopCond(const VarDecl *CondVarL, const VarDecl *CondVarR,
                   const IntegerLiteral *CondValR, const VarDecl *InitVarpt,
                   const BinaryOperator *CondBinaryOperator,
                   loop_type_org &loop_node) {
  if (!CondBinaryOperator) { // in this case, there's no valid match
    loop_node.isCompValid = false;
    return;
  }
  if (areSameVariable(CondVarL, InitVarpt)) { // in this case, the loop variant
                                              // is exactly on the left side of
    // the expr, which is the same as the standard format
    loop_node.CompLhsVar = loop_node.VarName;
    if (CondVarR)
      loop_node.CompRhsVar = CondVarR->getNameAsString();
    else
      loop_node.CompRhsVar = getValuefromIntegerLiterral(CondValR);

    loop_node.CompOP =
        CondBinaryOperator->getOpcodeStr(CondBinaryOperator->getOpcode());
    loop_node.isCompValid = true;
  } else { // in this case, the loop variant is on the right side of the expr,
           // but the standard format is on left side, so the expr will be
           // converted
    loop_node.CompLhsVar = loop_node.VarName;
    loop_node.CompRhsVar = CondVarL->getNameAsString();
    loop_node.isCompValid = operatorFlip(
        CondBinaryOperator->getOpcodeStr(CondBinaryOperator->getOpcode()),
        loop_node.CompOP);
  }
}

void checkLoopInc(const VarDecl *IncVar, const IntegerLiteral *IncValR,
                  const UnaryOperator *IncUnaryOperator,
                  const BinaryOperator *IncBinaryOperator,
                  loop_type_org &loop_node) {
  if (IncUnaryOperator) {
    loop_node.isIncValid = true;
    loop_node.isBinaryIncOp = false;
    loop_node.IncOp =
        IncUnaryOperator->getOpcodeStr(IncUnaryOperator->getOpcode());
    loop_node.IncLhsVar = loop_node.VarName;
  } else if (IncBinaryOperator) {
    loop_node.isIncValid = true;
    loop_node.isBinaryIncOp = true;
    loop_node.IncOp =
        IncBinaryOperator->getOpcodeStr(IncBinaryOperator->getOpcode());
    loop_node.IncRhsVar = getValuefromIntegerLiterral(IncValR);
    loop_node.IncLhsVar = loop_node.VarName;
    // TODO, MIGHT NEED TO ADD VARIANT SUPPORT
  }
  // the remaining case is that there is NO MATCH, but since is already set to
  // false, nothing has to be done here
}

loop_type_org loop_classify(const MatchFinder::MatchResult &Result,
                            const clang::ForStmt *FS) {

  const VarDecl *IncVar = Result.Nodes.getNodeAs<VarDecl>("incVarName");
  const IntegerLiteral *IncValR =
      Result.Nodes.getNodeAs<IntegerLiteral>("IncValR");
  const VarDecl *CondVarL = Result.Nodes.getNodeAs<VarDecl>("condVarNameL");
  const VarDecl *CondVarR = Result.Nodes.getNodeAs<VarDecl>("condVarNameR");
  const IntegerLiteral *CondValR =
      Result.Nodes.getNodeAs<IntegerLiteral>("CondValR");
  const VarDecl *InitVar = Result.Nodes.getNodeAs<VarDecl>("initVarName");
  const VarDecl *InitrefVar = Result.Nodes.getNodeAs<VarDecl>("initrefVarName");
  const UnaryOperator *IncUnaryOperator =
      Result.Nodes.getNodeAs<UnaryOperator>("incUnaryOperator");
  const BinaryOperator *IncBinaryOperator =
      Result.Nodes.getNodeAs<BinaryOperator>("incBinaryOperator");
  const BinaryOperator *CondBinaryOperator =
      Result.Nodes.getNodeAs<BinaryOperator>("condBinaryOperator");
  const IntegerLiteral *InitVal =
      Result.Nodes.getNodeAs<IntegerLiteral>("InitVal");
  loop_type_org loop_node;
  //bool is_loop_valid = true;

  auto InitVarpt = checkLoopInit(InitVar, InitrefVar, InitVal, loop_node);

  if (!(areSameVariable(InitVarpt, CondVarL) ||
        areSameVariable(InitVarpt, CondVarR)) ||
      !areSameVariable(InitVarpt, IncVar)) {
    loop_node.isInitValid = false;
    loop_node.isCompValid = false;
    loop_node.isIncValid = false;
    return loop_node;
  }

  checkLoopCond(CondVarL, CondVarR, CondValR, InitVarpt, CondBinaryOperator,
                loop_node);

  checkLoopInc(IncVar, IncValR, IncUnaryOperator, IncBinaryOperator, loop_node);

  return loop_node;
}


ForLoopNode::ForLoopNode(const MatchFinder::MatchResult &Result, const ForStmt *FS):
CodeNode()
{
      auto tmp = FS->getSourceRange();
      start_wd = tmp.getBegin().getRawEncoding();
      end_wd = tmp.getEnd().getRawEncoding();
      tmp = FS->getBody()->getSourceRange();
      type=loop_classify(Result, FS);
      Insert(new OtherNode(tmp.getBegin().getRawEncoding(), tmp.getEnd().getRawEncoding()));
}

void ForLoopNode::printCode() { 
  for (int i=0;i<level;i++) llvm::outs() << "    ";
  
  if (type.isIncValid && type.isCompValid && type.isInitValid)
  {
    if ((type.InitialVal == "0") && (type.IncOp == "++"))
    {
      if (type.CompOP == "<")
      {
        llvm::outs() << "for " << type.VarName << " in range(" << type.CompRhsVar <<"):\n";
      }
      else llvm::outs() << "TODO Forloop " << start_wd << " " << end_wd << " " << offset << "\n"; 
    }
    else llvm::outs() << "TODO Forloop " << start_wd << " " << end_wd << " " << offset << "\n"; 
  }
  else
  {
    llvm::outs() << "TODO Forloop " << start_wd << " " << end_wd << " " << offset << "\n"; 
  }
  for (auto child:childs)
  {
    child.second->level=level+1;
    child.second->printCode();
  }  
  }
