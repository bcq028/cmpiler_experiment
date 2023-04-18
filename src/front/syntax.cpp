#include "front/syntax.h"

#include <iostream>
#include <cassert>

using frontend::Parser;

// #define DEBUG_PARSER
#define TODO assert(0 && "todo")
#define CUR_TOKEN_IS(tk_type) (token_stream[index].type == TokenType::tk_type)
#define PARSE_TOKEN(tk_type) root->children.push_back(parseTerm(root, TokenType::tk_type));
#define PARSE(name, type)       \
    auto name = new type(root); \
    assert(parse##type(name));  \
    root->children.push_back(name);

Parser::Parser(const std::vector<frontend::Token> &tokens) : index(0), token_stream(tokens)
{
}

Parser::~Parser() {}

namespace frontend
{

    Term *Parser::parseTerm(AstNode *root, TokenType t)
    {
        Token tk = token_stream[index];
        if (tk.type != t)
        {
            assert(0 && 'token type wrong when parsing terminal');
        }
        index++;
        return new Term(tk, root);
    }

    bool Parser::parseCompUint(CompUnit *root)
    {
    }
    bool Parser::parseDecl(Decl *root)
    {
    }
    bool Parser::parseConstDecl(ConstDecl *root)
    {
        PARSE_TOKEN(CONSTTK);
        PARSE(bTypeNode, BType);
        PARSE(constDefNode, ConstDef);
        while (CUR_TOKEN_IS(COMMA))
        {
            PARSE_TOKEN(COMMA);
            PARSE(constDefNode, ConstDef);
        }
        PARSE_TOKEN(SEMICN);
    }
    bool Parser::parseVarDecl(VarDecl *root)
    {
        PARSE(btypeNode, BType);
        PARSE(varDefNode, VarDef);
        while (CUR_TOKEN_IS(COMMA))
        {
            PARSE_TOKEN(COMMA);
            PARSE(vardefNode, VarDef);
        }
        PARSE_TOKEN(SEMICN);
    }
    bool Parser::parseBType(BType *root)
    {
        if (CUR_TOKEN_IS(INTTK))
        {
            PARSE_TOKEN(INTTK);
        }
        else
        {
            PARSE_TOKEN(FLOATTK);
        }
    }
    bool Parser::parseConstDef(ConstDef *root)
    {
        PARSE_TOKEN(IDENFR);
        while (CUR_TOKEN_IS(LBRACK))
        {
            PARSE_TOKEN(LBRACK);
            PARSE(constExpNode, ConstExp);
            PARSE_TOKEN(RBRACK);
        }
        PARSE_TOKEN(ASSIGN);
        PARSE(constInitValNode, ConstInitVal);
    }
    bool Parser::parseVarDef(VarDef *root)
    {
        PARSE_TOKEN(IDENFR);
        while (CUR_TOKEN_IS(LBRACK))
        {
            PARSE_TOKEN(LBRACK);
            PARSE(constExpNode, ConstExp);
            PARSE_TOKEN(RBRACK);
        }
        if (CUR_TOKEN_IS(ASSIGN))
        {
            PARSE_TOKEN(ASSIGN);
            PARSE(initValNode, InitVal);
        }
        return true;
    }
    bool Parser::parseConstExp(ConstExp *root)
    {
        PARSE(addExpNode, AddExp);
        return true;
    }
    bool Parser::parseConstInitVal(ConstInitVal *root)
    {
        if (CUR_TOKEN_IS(LBRACE))
        {
            PARSE_TOKEN(LBRACE);
            PARSE(constExpNode, ConstExp);
            while (CUR_TOKEN_IS(COMMA))
            {
                PARSE_TOKEN(COMMA);
                PARSE(constExpNode, ConstExp);
            }
            PARSE_TOKEN(RBRACE);
        }
        else
        {
            PARSE(constExpNode, ConstExp);
        }
        return true;
    }
    bool Parser::parseFuncDef(FuncDef *root)
    {
    }
    bool Parser::parseFuncType(FuncType *root)
    {
    }
    bool Parser::parseFuncFParam(FuncFParam *root)
    {
    }
    bool Parser::parseFuncFParams(FuncFParams *root)
    {
    }
    bool Parser::parseBlock(Block *root)
    {
    }
    bool Parser::parseBlockItem(BlockItem *root)
    {
    }
    bool Parser::parseStmt(Stmt *root)
    {
    }
    bool Parser::parseLVal(LVal *root)
    {
    }
    bool Parser::parseExp(Exp *root)
    {
    }
    bool Parser::parseCond(Cond *root)
    {
    }
    bool Parser::parseNumber(Number *root)
    {
    }
    bool Parser::parsePrimaryExp(PrimaryExp *root)
    {
    }
    bool Parser::parseUnaryExp(UnaryExp *root)
    {
    }
    bool Parser::parseFuncRParams(FuncRParams *root)
    {
    }
    bool Parser::parseMulExp(MulExp *root)
    {
    }
    bool Parser::parseAddExp(AddExp *root)
    {
    }
    bool Parser::parseRelExp(RelExp *root)
    {
    }
    bool Parser::parseEqExp(EqExp *root)
    {
    }
    bool Parser::parseLAndExp(LAndExp *root)
    {
    }
    bool Parser::parseLOrExp(LOrExp *root)
    {
    }
    bool Parser::parseInitVal(InitVal *root)
    {
    }
    bool Parser::parseFuncFParam(FuncFParam *root)
    {
    }
    bool Parser::parseFuncFParams(FuncFParams *root)
    {
    }
    bool Parser::parseFuncType(FuncType *root)
    {
    }
    bool Parser::parseFuncDef(FuncDef *root)
    {
    }
    bool Parser::parseBlock(Block *root)
    {
    }
    bool Parser::parseBlockItem(BlockItem *root)
    {
    }
    bool Parser::parseStmt(Stmt *root)
    {
    }
    bool Parser::parseLVal(LVal *root)
    {
    }
    bool Parser::parseCond(Cond *root)
    {
    }
    bool Parser::parsePrimaryExp(PrimaryExp *root)
    {
    }
    bool Parser::parseUnaryExp(UnaryExp *root)
    {
    }
    bool Parser::parseFuncRParams(FuncRParams *root)
    {
    }
    bool Parser::parseMulExp(MulExp *root)
    {
    }
    bool Parser::parseAddExp(AddExp *root)
    {
    }
    bool Parser::parseRelExp(RelExp *root)
    {
    }
    bool Parser::parseEqExp(EqExp *root)
    {
    }
    bool Parser::parseLAndExp(LAndExp *root)
    {
    }
    bool Parser::parseLOrExp(LOrExp *root)
    {
    }
    bool Parser::parseConstExp(ConstExp *root)
    {
    }
}

frontend::CompUnit *Parser::get_abstract_syntax_tree()
{
    frontend::CompUnit *ret = new frontend::CompUnit();
    parseCompUint(ret);
    return ret;
}

void Parser::log(AstNode *node)
{
#ifdef DEBUG_PARSER
    std::cout << "in parse" << toString(node->type) << ", cur_token_type::" << toString(token_stream[index].type) << ", token_val::" << token_stream[index].value << '\n';
#endif
}
