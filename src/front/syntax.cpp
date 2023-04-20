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
            assert(0 && "token type wrong when parsing terminal");
        }
        index++;
        return new Term(tk, root);
    }

    bool Parser::parseCompUnit(CompUnit *root)
    {
        if (token_stream[index + 2].type == TokenType::LPARENT)
        {
            PARSE(funcDefNode, FuncDef);
        }
        else
        {
            PARSE(declNode, Decl);
        }
        while (index < token_stream.size())
        {
            PARSE(compUnitNode, CompUnit);
        }
        return true;
    }
    bool Parser::parseDecl(Decl *root)
    {
        if (CUR_TOKEN_IS(CONSTTK))
        {
            PARSE(constNode, ConstDecl);
        }
        else
        {
            PARSE(varNode, VarDecl);
        }
        return true;
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
        return true;
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
        return true;
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
        return true;
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
        return true;
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
        PARSE(funcTypeNode, FuncType);
        PARSE_TOKEN(IDENFR);
        PARSE_TOKEN(LPARENT);
        if (!CUR_TOKEN_IS(RPARENT))
        {
            PARSE(paramesNode, FuncFParams);
        }
        PARSE_TOKEN(RPARENT);
        PARSE(blockNode, Block);
        return true;
    }
    bool Parser::parseFuncType(FuncType *root)
    {
        if (CUR_TOKEN_IS(VOIDTK))
        {
            PARSE_TOKEN(VOIDTK);
        }
        else if (CUR_TOKEN_IS(INTTK))
        {
            PARSE_TOKEN(INTTK);
        }
        else
        {
            PARSE_TOKEN(FLOATTK);
        }
        return true;
    }
    bool Parser::parseFuncFParam(FuncFParam *root)
    {
        PARSE(BTypeNode, BType);
        PARSE_TOKEN(IDENFR);
        if (CUR_TOKEN_IS(LBRACK))
        {
            PARSE_TOKEN(LBRACK);
            PARSE_TOKEN(RBRACK);
            while (CUR_TOKEN_IS(LBRACK))
            {
                PARSE(expNode, Exp);
                PARSE_TOKEN(RBRACK);
            }
        }
        return true;
    }
    bool Parser::parseFuncFParams(FuncFParams *root)
    {
        PARSE(funcfNode, FuncFParam);
        while (CUR_TOKEN_IS(COMMA))
        {
            PARSE_TOKEN(COMMA);
            PARSE(funcFParamNode, FuncFParam);
        }
        return true;
    }
    bool Parser::parseBlock(Block *root)
    {
        PARSE_TOKEN(LBRACE);
        while (!CUR_TOKEN_IS(RBRACE))
        {
            PARSE(t, Block);
        }
        PARSE_TOKEN(RBRACE);
    }
    bool Parser::parseBlockItem(BlockItem *root)
    {
        if (CUR_TOKEN_IS(CONSTTK) || CUR_TOKEN_IS(INTTK) || CUR_TOKEN_IS(FLOATTK))
        {
            PARSE(t, Decl);
        }
        else
        {
            PARSE(t, Stmt);
        }
    }
    bool Parser::parseStmt(Stmt *root)
    {
        if (CUR_TOKEN_IS(IFTK))
        {
            PARSE_TOKEN(IFTK);
            PARSE_TOKEN(LPARENT);
            PARSE(condNode, Cond);
            PARSE_TOKEN(RPARENT);
            PARSE(stmtNode1, Stmt);
            if (CUR_TOKEN_IS(ELSETK))
            {
                PARSE_TOKEN(ELSETK);
                PARSE(stmtNode2, Stmt);
            }
        }
        else if (CUR_TOKEN_IS(WHILETK))
        {
            PARSE_TOKEN(WHILETK);
            PARSE_TOKEN(LPARENT);
            PARSE(condNode, Cond);
            PARSE_TOKEN(RPARENT);
            PARSE(stmtNode, Stmt);
        }
        else if (CUR_TOKEN_IS(BREAKTK))
        {
            PARSE_TOKEN(BREAKTK);
            PARSE_TOKEN(SEMICN);
        }
        else if (CUR_TOKEN_IS(CONTINUETK))
        {
            PARSE_TOKEN(CONTINUETK);
            PARSE_TOKEN(SEMICN);
        }
        else if (CUR_TOKEN_IS(RETURNTK))
        {
            PARSE_TOKEN(RETURNTK);
            if (!CUR_TOKEN_IS(SEMICN))
            {
                PARSE(expNode, Exp);
            }
            PARSE_TOKEN(SEMICN);
        }
        else if (CUR_TOKEN_IS(LBRACE))
        {
            PARSE(blockNode, Block);
        }
        else
        {
            PARSE(lValNode, LVal);
            PARSE_TOKEN(ASSIGN);
            PARSE(expNode, Exp);
            PARSE_TOKEN(SEMICN);
        }
        return true;
    }
    bool Parser::parseLVal(LVal *root)
    {
        PARSE_TOKEN(IDENFR);
        while (CUR_TOKEN_IS(LBRACK))
        {
            PARSE_TOKEN(LBRACK);
            PARSE(expNode, Exp);
            PARSE_TOKEN(RBRACK);
        }
        return true;
    }
    bool Parser::parseExp(Exp *root)
    {
        PARSE(addExpNode, AddExp);
        return true;
    }
    bool Parser::parseCond(Cond *root)
    {
        PARSE(lorNode, LOrExp);
        return true;
    }
    bool Parser::parseNumber(Number *root)
    {
        if (CUR_TOKEN_IS(INTLTR))
        {
            PARSE_TOKEN(INTLTR);
        }
        else
        {
            PARSE_TOKEN(FLOATLTR);
        }
        return true;
    }
    bool Parser::parseUnaryExp(UnaryExp *root)
    {
        if(CUR_TOKEN_IS(PLUS)|| CUR_TOKEN_IS(MINU)||CUR_TOKEN_IS(NOT)){
            PARSE(t,UnaryOp);
            PARSE(t2,UnaryExp);
        }else if (CUR_TOKEN_IS(IDENFR) && token_stream[index + 1].type == TokenType::LPARENT)
        {
            PARSE_TOKEN(IDENFR);
            PARSE_TOKEN(LPARENT);
            if(!CUR_TOKEN_IS(RPARENT)){
                PARSE(t,FuncRParams);
            }
            PARSE_TOKEN(RPARENT);
        }else{
            PARSE(t,PrimaryExp);
        }
    }
    bool Parser::parseAddExp(AddExp *root)
    {
        PARSE(MulExpNode, MulExp);
        while (true)
        {
            if (CUR_TOKEN_IS(PLUS))
            {
                PARSE_TOKEN(PLUS);
                PARSE(mulNode, MulExp);
            }
            else if (CUR_TOKEN_IS(MINU))
            {
                PARSE_TOKEN(MINU);
                PARSE(mulNode, MulExp);
            }
            else
            {
                break;
            }
        }
        return true;
    }
    bool Parser::parseRelExp(RelExp *root)
    {
        PARSE(addNode, AddExp);
        while (1)
        {
            if (CUR_TOKEN_IS(LSS))
            {
                PARSE_TOKEN(LSS);
                PARSE(addNode, AddExp);
            }
            else if (CUR_TOKEN_IS(GTR))
            {
                PARSE_TOKEN(GTR);
                PARSE(addNode, AddExp);
            }
            else if (CUR_TOKEN_IS(LEQ))
            {
                PARSE_TOKEN(LEQ);
                PARSE(addNode, AddExp);
            }
            else if (CUR_TOKEN_IS(GEQ))
            {
                PARSE_TOKEN(GEQ);
                PARSE(addNode, AddExp);
            }
            else
            {
                break;
            }
        }
        return true;
    }
    bool Parser::parseEqExp(EqExp *root)
    {
        PARSE(relNode, RelExp);
        while (1)
        {
            if (CUR_TOKEN_IS(EQL))
            {
                PARSE_TOKEN(EQL);
                PARSE(relNode, RelExp);
            }
            else if (CUR_TOKEN_IS(NEQ))
            {
                PARSE_TOKEN(NEQ);
                PARSE(relNode, RelExp);
            }
            else
            {
                break;
            }
        }
        return true;
    }
    bool Parser::parseLAndExp(LAndExp *root)
    {
        PARSE(eqNode, EqExp);
        if (CUR_TOKEN_IS(AND))
        {
            PARSE_TOKEN(AND);
            PARSE(andNode, LAndExp);
        }
        return true;
    }
    bool Parser::parseLOrExp(LOrExp *root)
    {
        PARSE(t, LAndExp);
        if (CUR_TOKEN_IS(OR))
        {
            PARSE_TOKEN(OR);
            PARSE(t, LOrExp);
        }
        return true;
    }
    bool Parser::parseInitVal(InitVal *root)
    {
        if (CUR_TOKEN_IS(LBRACE))
        {
            PARSE_TOKEN(LBRACE);

            while (!CUR_TOKEN_IS(RBRACE))
            {
                PARSE(initValNode, InitVal);
                if (CUR_TOKEN_IS(COMMA))
                {
                    PARSE_TOKEN(COMMA);
                }
                else
                {
                    break;
                }
            }
            PARSE_TOKEN(RBRACE);
        }
        else
        {
            PARSE(t, Exp);
        }
        return true;
    }
    bool Parser::parsePrimaryExp(PrimaryExp *root)
    {
        if (CUR_TOKEN_IS(LPARENT))
        {
            PARSE_TOKEN(LPARENT);
            PARSE(expNode, Exp);
            PARSE_TOKEN(RPARENT);
        }
        else if (CUR_TOKEN_IS(IDENFR))
        {
            PARSE(lvalNode, LVal);
        }
        else
        {
            PARSE(numberNode, Number);
        }
        return true;
    }
    bool Parser::parseFuncRParams(FuncRParams *root)
    {
        PARSE(expNode, Exp);
        while (CUR_TOKEN_IS(COMMA))
        {
            PARSE_TOKEN(COMMA);
            PARSE(expNode, Exp);
        }
        return true;
    }
    bool Parser::parseMulExp(MulExp *root)
    {
        PARSE(unaryNode, UnaryExp);
        while (true)
        {
            if (CUR_TOKEN_IS(MULT))
            {
                PARSE_TOKEN(MULT);
            }
            else if (CUR_TOKEN_IS(DIV))
            {
                PARSE_TOKEN(DIV);
            }
            else if (CUR_TOKEN_IS(MOD))
            {
                PARSE_TOKEN(MOD);
            }
            else
            {
                break;
            }
            PARSE(unaryNode, UnaryExp);
        }
        return true;
    }
    bool Parser::parseUnaryOp(UnaryOp *root)
    {
        if (CUR_TOKEN_IS(PLUS))
        {
            PARSE_TOKEN(PLUS);
        }
        else if (CUR_TOKEN_IS(MINU))
        {
            PARSE_TOKEN(MINU);
        }
        else
        {
            PARSE_TOKEN(NOT);
        }
        return true;
    }
}

frontend::CompUnit *Parser::get_abstract_syntax_tree()
{
    frontend::CompUnit *ret = new frontend::CompUnit();
    parseCompUnit(ret);
    return ret;
}

void Parser::log(AstNode *node)
{
#ifdef DEBUG_PARSER
    std::cout << "in parse" << toString(node->type) << ", cur_token_type::" << toString(token_stream[index].type) << ", token_val::" << token_stream[index].value << '\n';
#endif
}
