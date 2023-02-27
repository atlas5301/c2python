#include "code_reprocess.h"


bool areSameVariable(const clang::ValueDecl *First,
                            const clang::ValueDecl *Second) {
  return First && Second &&
         First->getCanonicalDecl() == Second->getCanonicalDecl();
}

std::string_view get_source_text(clang::SourceRange range,
                                 const clang::SourceManager &sm) {
  clang::LangOptions lo;

  // NOTE: sm.getSpellingLoc() used in case the range corresponds to a
  // macro/preprocessed source.
  auto start_loc = sm.getSpellingLoc(range.getBegin());
  auto last_token_loc = sm.getSpellingLoc(range.getEnd());
  auto end_loc = clang::Lexer::getLocForEndOfToken(last_token_loc, 0, sm, lo);
  auto printable_range = clang::SourceRange{start_loc, end_loc};
  return get_source_text_raw(printable_range, sm);
}

std::string_view get_source_text_raw(clang::SourceRange range,
                                     const clang::SourceManager &sm) {
  return clang::Lexer::getSourceText(
      clang::CharSourceRange::getCharRange(range), sm, clang::LangOptions());
}

int CodeNode::CompareNode(CodeNode *node) {
  if (node->end_wd <= start_wd) {
    return -1;
  }
  if (node->start_wd >= end_wd) {
    return 1;
  }
  return 0;
}

bool CodeNode::isInclude(CodeNode *node) {
  return (node->start_wd >= start_wd && node->end_wd <= end_wd);
}

CodeNode *CodeNode::Insert(CodeNode *node) {
  node->get_source(code, offset);
  if (!isInclude(node)) {
    node->Insert(this);
    return node;
  }
  auto child = childs.upper_bound(node->start_wd);
  if (child != childs.end()) {
    if (child->second->CompareNode(node) == 0) {
      auto tmp = child->second->Insert(node);
      if (tmp != child->second) {
        childs.erase(child->first);
        childs.insert_or_assign(node->start_wd, node);
        // childs[tmp->start_wd] = tmp;
      }
      return this;
    }
  }
  if (child != childs.begin()) {
    child--;
    if (child->second->CompareNode(node) == 0) {
      auto tmp = child->second->Insert(node);
      if (tmp != child->second) {
        childs.erase(child->first);
        childs.insert_or_assign(node->start_wd, node);
        //childs[tmp->start_wd] = tmp;
      }
      return this;
  }
  }
  childs.insert_or_assign(node->start_wd, node);
  //childs[node->start_wd] = node;
  return this;
  // #TODO
}

CodeNode::CodeNode(unsigned long long _start_wd, unsigned long long _end_wd):
    code(nullptr), start_wd(_start_wd), end_wd(_end_wd),level(0) {}

CodeNode::CodeNode(): code(nullptr), level(0){}

void CodeNode::get_source(std::string_view* _code, unsigned long long _offset)
{
  if (code) return;
  code = _code;
  offset = _offset;
  for (auto child:childs)
  {
    child.second->get_source(_code, _offset);
  }
}


void OtherNode::printCode() { 
  auto start_pos = start_wd- offset;  
  long long int end_pos;
  for (auto child:childs)
  {
    end_pos = child.second->start_wd-offset;
    auto tokens = parse_lines(code->substr(start_pos, end_pos-start_pos));
    for (auto line:tokens)
    {
      for (int i=0;i<level;i++) llvm::outs() << "    ";    
      llvm::outs() << line.second << "\n";  
    }
    child.second->level=level+1;
    child.second->printCode();
    start_pos = child.second->end_wd+1-offset;    
  }
    end_pos = end_wd - offset +1;
    auto tokens = parse_lines(code->substr(start_pos, end_pos-start_pos));
    for (auto line:tokens)
    {
      for (int i=0;i<level;i++) llvm::outs() << "    ";    
      llvm::outs() << line.second << "\n";  
    }      
    // llvm::outs() << code->substr(start_pos, end_pos-start_pos);
}

OtherNode::OtherNode(unsigned long long _start_wd, unsigned long long _end_wd):
    CodeNode(_start_wd, _end_wd) 
    {

        
    }

CodeNode::~CodeNode()
  {
    if (code) delete code;
    for (auto child:childs)
    {
      child.second->code = nullptr;
      delete child.second;
    }
  }

std::map<unsigned long long, std::string_view> parse_lines(std::string_view code)
{
  unsigned long long i=0;
  std::map<unsigned long long, std::string_view> tokens;
  std::string_view::size_type pos = 0;
  std::string_view::size_type prev = 0;
  while ((pos = code.find('\n', prev)) != std::string_view::npos) {
      tokens.insert_or_assign(i, code.substr(prev, pos - prev));
      prev = pos + 1;
      i++;
    }
    tokens.insert_or_assign(i, code.substr(prev));
    return tokens; 
}