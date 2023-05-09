#include "front/semantic.h"

#include <cassert>
#include <iostream>
using ir::Function;
using ir::Instruction;
using ir::Operand;
using ir::Operator;

#define TODO assert(0 && "TODO");

#define GET_CHILD_PTR(node, type, index)                                                          \
    auto node = dynamic_cast<type *>(root->children[index]);                                      \
    if (!node)                                                                                    \
    {                                                                                             \
        std::cerr << "Error: failed to cast node " << index << " to type " << #type << std::endl; \
    }
#define ANALYSIS(node, type, index)                              \
    {                                                            \
        auto node = dynamic_cast<type *>(root->children[index]); \
        assert(node);                                            \
        analysis##type(node, buffer);} 

#define COPY_EXP_NODE(from, to)              \
    to->is_computable = from->is_computable; \
    to->v = from->v;                         \
    to->t = from->t;

map<std::string, ir::Function *> *frontend::get_lib_funcs()
{
    static map<std::string, ir::Function *> lib_funcs = {
        {"getint", new Function("getint", Type::Int)},
        {"getch", new Function("getch", Type::Int)},
        {"getfloat", new Function("getfloat", Type::Float)},
        {"getarray", new Function("getarray", {Operand("arr", Type::IntPtr)}, Type::Int)},
        {"getfarray", new Function("getfarray", {Operand("arr", Type::FloatPtr)}, Type::Int)},
        {"putint", new Function("putint", {Operand("i", Type::Int)}, Type::null)},
        {"putch", new Function("putch", {Operand("i", Type::Int)}, Type::null)},
        {"putfloat", new Function("putfloat", {Operand("f", Type::Float)}, Type::null)},
        {"putarray", new Function("putarray", {Operand("n", Type::Int), Operand("arr", Type::IntPtr)}, Type::null)},
        {"putfarray", new Function("putfarray", {Operand("n", Type::Int), Operand("arr", Type::FloatPtr)}, Type::null)},
    };
    return &lib_funcs;
}

void frontend::SymbolTable::add_scope(Block *node)
{
    TODO;
}
void frontend::SymbolTable::exit_scope()
{
    TODO;
}

string frontend::SymbolTable::get_scoped_name(string id) const
{
    TODO;
}

Operand frontend::SymbolTable::get_operand(string id) const
{
    TODO;
}

frontend::STE frontend::SymbolTable::get_ste(string id) const
{
    TODO;
}

frontend::Analyzer::Analyzer() : tmp_cnt(0), symbol_table()
{
}

ir::Program frontend::Analyzer::get_ir_program(CompUnit *root)
{
    ir::Program buffer;
    analysisCompUnit(root, buffer);
    return buffer;
}

using namespace frontend;

void Analyzer::analysisTerm(Term *root, string &s)
{
    s = root->token.value;
}

void Analyzer::analysisCompUnit(CompUnit *root, ir::Program &program)
{
    ir::Function buffer;
    GET_CHILD_PTR(node, FuncDef, 0);
    if (node)
    {
        ANALYSIS(node, FuncDef, 0);
        program.addFunction(buffer);
    }
    else
    {
        GET_CHILD_PTR(node, Decl, 0);
        if (node)
        {
            vector<ir::Instruction *> buffer;
            ANALYSIS(node1, Decl, 0);
            TODO
        }
    }
    if (root->children.size() > 1)
    {
        ir::Program buffer;
        ANALYSIS(node, CompUnit, 1);
        TODO
    }
}
void Analyzer::analysisDecl(Decl *root, vector<ir::Instruction *> &buffer)
{
    if (dynamic_cast<ConstDecl *>(root->children[0]))
    {
        ANALYSIS(node, ConstDecl, 0);
    }
    else
    {
        ANALYSIS(node, VarDecl, 0);
    }
}
void Analyzer::analysisConstDecl(ConstDecl *root, vector<ir::Instruction *> &buffer)
{
}
void Analyzer::analysisVarDecl(VarDecl *root, vector<ir::Instruction *> &buffer)
{
}
void Analyzer::analysisBType(BType *root, vector<ir::Instruction *> &buffer)
{
}
void Analyzer::analysisConstDef(ConstDef *root, vector<ir::Instruction *> &buffer)
{
}
void Analyzer::analysisVarDef(VarDef *root, vector<ir::Instruction *> &buffer)
{
}
void Analyzer::analysisConstExp(ConstExp *root, vector<ir::Instruction *> &buffer)
{
}
void Analyzer::analysisConstInitVal(ConstInitVal *root, vector<ir::Instruction *> &buffer)
{
}
void Analyzer::analysisFuncDef(FuncDef *root, ir::Function &func)
{
    {
        auto &buffer = func.returnType;
        ANALYSIS(node, FuncType, 0);
    }
    {
        auto &buffer = func.name;
        ANALYSIS(node, Term, 1);
    }
    GET_CHILD_PTR(node, FuncFParams, 3);
    if (node)
    {
        auto &buffer = func.ParameterList;
        ANALYSIS(node, FuncFParams, 3);
    }
    auto &buffer = func.InstVec;
    ANALYSIS(node3, Block, root->children.size() - 1);
}
void Analyzer::analysisFuncType(FuncType *root, ir::Type &buffer)
{
    GET_CHILD_PTR(node, Term, 0);
    if (node->token.type == TokenType::INTTK)
    {
        buffer = ir::Type::Int;
    }
    else if (node->token.type == TokenType::VOIDTK)
    {
        buffer = ir::Type::null;
    }
    else if (node->token.type == TokenType::FLOATTK)
    {
        buffer = ir::Type::Float;
    }
}
void Analyzer::analysisFuncFParam(FuncFParam *root, vector<ir::Instruction *> &buffer)
{
}
void Analyzer::analysisFuncFParams(FuncFParams *root, vector<ir::Operand> &paramlist)
{
}
void Analyzer::analysisBlock(Block *root, vector<ir::Instruction *> &insts)
{
    for (auto i = 1; i < root->children.size() - 1; ++i)
    {
        auto buffer = insts;
        ANALYSIS(node, BlockItem, i);
    }
}
void Analyzer::analysisBlockItem(BlockItem *root, vector<ir::Instruction *> &buffer)
{
    if (dynamic_cast<Decl *>(root->children[0]))
    {
        ANALYSIS(node, Decl, 0);
    }
    else
    {
        ANALYSIS(node, Stmt, 0);
    }
}
void Analyzer::analysisStmt(Stmt *root, vector<ir::Instruction *> &insts)
{
    ir::Instruction inst;
    insts.push_back(&inst);
    {
        GET_CHILD_PTR(node, Term, 1);
        if (node)
        {
            if (node->token.type == TokenType::ASSIGN)
            {
                inst.op = Operator::def;
                auto buffer = inst.des;
                ANALYSIS(node1, LVal, 0);
                buffer = inst.op1;
                ANALYSIS(node2, Exp, 2);
            }
        }
    }
    GET_CHILD_PTR(node, Term, 0);
    if (node)
    {
        if (node->token.type == TokenType::RETURNTK)
        {
            inst.op = Operator::_return;
            if (root->children.size() > 1)
            {
                auto buffer = inst.op1;
                ANALYSIS(node, Exp, 1);
            }
        }
    }
}

void Analyzer::analysisLVal(LVal *root, ir::Operand &buffer)
{
    TODO
}
void Analyzer::analysisExp(Exp *root, ir::Operand &buffer)
{
    ANALYSIS(node,AddExp,0);
}
void Analyzer::analysisCond(Cond *root, vector<ir::Instruction *> &buffer)
{
}
void Analyzer::analysisNumber(Number *root, vector<ir::Instruction *> &buffer)
{
}
void Analyzer::analysisPrimaryExp(PrimaryExp *root, vector<ir::Instruction *> &buffer)
{
}
void Analyzer::analysisUnaryExp(UnaryExp *root, vector<ir::Instruction *> &buffer)
{
}
void Analyzer::analysisFuncRParams(FuncRParams *root, vector<ir::Instruction *> &buffer)
{
}
void Analyzer::analysisMulExp(MulExp *root, vector<ir::Instruction *> &buffer)
{
}
void Analyzer::analysisAddExp(AddExp *root, vector<ir::Instruction *> &buffer)
{
}
void Analyzer::analysisRelExp(RelExp *root, vector<ir::Instruction *> &buffer)
{
}
void Analyzer::analysisEqExp(EqExp *root, vector<ir::Instruction *> &buffer)
{
}
void Analyzer::analysisLAndExp(LAndExp *root, vector<ir::Instruction *> &buffer)
{
}
void Analyzer::analysisLOrExp(LOrExp *root, vector<ir::Instruction *> &buffer)
{
}
void Analyzer::analysisInitVal(InitVal *root, vector<ir::Instruction *> &buffer)
{
}
void Analyzer::analysisUnaryOp(UnaryOp *root, vector<ir::Instruction *> &buffer)
{
}