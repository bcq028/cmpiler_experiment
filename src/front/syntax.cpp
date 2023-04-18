#include "front/syntax.h"

#include <iostream>
#include <cassert>

using frontend::Parser;

// #define DEBUG_PARSER
#define TODO assert(0 && "todo")
#define CUR_TOKEN_IS(tk_type) (token_stream[index].type == TokenType::tk_type)
#define PARSE_TOKEN(tk_type) root->children.push_back(parseTerm(root, TokenType::tk_type))
#define PARSE(name, type)       \
    auto name = new type(root); \
    assert(parse##type(name));  \
    root->children.push_back(name);

Parser::Parser(const std::vector<frontend::Token> &tokens) : index(0), token_stream(tokens)
{
}

Parser::~Parser() {}

bool Parser::parseCompUint(frontend::CompUnit *root)
{
    
}
bool Parser::parseDecl(frontend::Decl *root)
{
}
bool Parser::parseConstDecl(frontend::ConstDecl *root)
{
}
bool Parser::parseBType(frontend::BType *root)
{
}
bool Parser::parseConstDef(frontend::ConstDef *root)
{
}

frontend::CompUnit *Parser::get_abstract_syntax_tree()
{
    frontend::CompUnit* ret=new frontend::CompUnit();
    parseCompUint(ret);
    return ret;
}

void Parser::log(AstNode *node)
{
#ifdef DEBUG_PARSER
    std::cout << "in parse" << toString(node->type) << ", cur_token_type::" << toString(token_stream[index].type) << ", token_val::" << token_stream[index].value << '\n';
#endif
}
