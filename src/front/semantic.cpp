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
#define ANALYSIS(node, type, index)                          \
                                                             \
    auto node = dynamic_cast<type *>(root->children[index]); \
    assert(node);                                            \
    analysis##type(node, buffer);

#define COPY_EXP_NODE(from, to)              \
    to->is_computable = from->is_computable; \
    to->v = from->v;                         \
    to->t = from->t;

#define STR_ADD(str1, str2) (std::to_string(std::stoi(str1) + std::stoi(str2)))
#define STR_SUB(str1, str2) (std::to_string(std::stoi(str1) - std::stoi(str2)))
#define STR_MUL(x, y) (std::to_string(std::stoi(x) * std::stoi(y)))
#define STR_DIV(x, y) (std::to_string(std::stoi(x) / std::stoi(y)))
#define STR_MOD(x, y) (std::to_string(std::stoi(x) % std::stoi(y)))

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
    ScopeInfo s;
    s.cnt = this->scope_stack.size() + 1;
    this->scope_stack.push_back(s);
}
void frontend::SymbolTable::exit_scope()
{
    this->scope_stack.pop_back();
}

string frontend::SymbolTable::get_scoped_name(string id) const
{
    string ret = id + std::to_string(this->scope_stack.back().cnt);
    return ret;
}
void frontend::SymbolTable::add_symbol(string id, vector<int> *dimension)
{
    if (!this->scope_stack.back().table.count(get_scoped_name(id)))
    {
        STE ste;
        ste.operand = Operand(get_scoped_name(id));
        if (dimension != nullptr)
        {
            ste.dimension = *dimension;
        }
        ScopeInfo &sc = const_cast<ScopeInfo &>(scope_stack[this->scope_stack.size() - 1]);
        sc.table[get_scoped_name(id)] = ste;
    }
}

Operand frontend::SymbolTable::get_operand(string id) const
{
    return this->get_ste(id).operand;
}

frontend::STE frontend::SymbolTable::get_ste(string id) const
{
    auto it = this->scope_stack.back().table.find(get_scoped_name(id));
    if (it != this->scope_stack.back().table.end())
    {
        return it->second;
    }
    else
    {
        assert(0 && "cannot find symbol");
    }
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
    ANALYSIS(node, BType, 0);
    root->t=node->t;
    ANALYSIS(node1,VarDef,1);
    for(auto i=2;i<root->children.size();i+=2){
        if(dynamic_cast<Term*>(root->children[i])->token.type==TokenType::COMMA){
            ANALYSIS(node,VarDef,i+1);
        }
    }
}
void Analyzer::analysisBType(BType *root, vector<ir::Instruction *> &buffer)
{
    GET_CHILD_PTR(node, Term, 0);
    root->t = node->token.type == TokenType::INTTK ? Type::Int : Type::Float;
}
void Analyzer::analysisConstDef(ConstDef *root, vector<ir::Instruction *> &buffer)
{
}
void Analyzer::analysisVarDef(VarDef *root, vector<ir::Instruction *> &buffer)
{
    root->arr_name = dynamic_cast<Term *>(root->children[0])->token.value;
    vector<int> dimensions;

    for (auto i = 1; i < root->children.size(); i += 3)
    {
        if (dynamic_cast<Term *>(root->children[i])->token.type == TokenType::LBRACK)
        {
            ANALYSIS(node, ConstExp, i + 1);
            dimensions.push_back(stoi(node->v));
        }
        else if (dynamic_cast<Term *>(root->children[i])->token.type == TokenType::ASSIGN)
        {
            ANALYSIS(node, InitVal, i + 1);
            ir::Instruction *init = new ir::Instruction();
            init->des = this->symbol_table.get_operand(root->arr_name);
            init->op = symbol_table.get_operand(root->arr_name).type == Type::Int
                           ? Operator::mov
                           : Operator::fmov;
            init->op1 = node->v;
            buffer.push_back(init);
        }
    }
    this->symbol_table.add_symbol(root->arr_name, nullptr);
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
    this->symbol_table.add_scope(root);
    for (auto i = 1; i < root->children.size() - 1; ++i)
    {
        auto &buffer = insts;
        ANALYSIS(node, BlockItem, i);
    }
    this->symbol_table.exit_scope();
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
    ir::Instruction *inst = new ir::Instruction();
    insts.push_back(inst);
    {
        GET_CHILD_PTR(node, Term, 1);
        if (node)
        {
            if (node->token.type == TokenType::ASSIGN)
            {
                TODO
            }
        }
    }
    GET_CHILD_PTR(node, Term, 0);
    if (node)
    {
        if (node->token.type == TokenType::RETURNTK)
        {
            inst->op = Operator::_return;
            if (root->children.size() > 1)
            {
                auto &buffer = insts;
                ANALYSIS(node, Exp, 1);
                GET_CHILD_PTR(node1, Exp, 1);
                if (node1)
                {
                    if (node1->t == Type::IntLiteral || node1->t == Type::FloatLiteral)
                    {
                        inst->op1 = Operand(node1->v, node1->t);
                    }
                    else
                    {
                        inst->op1 = symbol_table.get_operand(node1->v);
                    }
                }
            }
        }
    }
}

void Analyzer::analysisLVal(LVal *root, ir::Operand &buffer)
{
    TODO
}
void Analyzer::analysisExp(Exp *root, vector<ir::Instruction *> &buffer)
{
    ANALYSIS(node, AddExp, 0);
    GET_CHILD_PTR(node1, AddExp, 0);
    COPY_EXP_NODE(node1, root);
}
void Analyzer::analysisCond(Cond *root, vector<ir::Instruction *> &buffer)
{
}
void Analyzer::analysisNumber(Number *root, string &buffer)
{
    GET_CHILD_PTR(node, Term, 0);
    root->t = node->token.type == TokenType::INTLTR ? ir::Type::IntLiteral : ir::Type::FloatLiteral;
    root->v = node->token.value;
    root->is_computable = true;
}
void Analyzer::analysisPrimaryExp(PrimaryExp *root, vector<ir::Instruction *> &buffer)
{
    GET_CHILD_PTR(node, Number, 0);
    if (node)
    {
        string buffer;
        ANALYSIS(node, Number, 0);
        COPY_EXP_NODE(node, root);
        return;
    }
    GET_CHILD_PTR(node1, LVal, 0);
    if (node1)
    {
        TODO return;
    }
    ANALYSIS(node2, Exp, 1);
    COPY_EXP_NODE(node2, root);
}
void Analyzer::analysisUnaryExp(UnaryExp *root, vector<ir::Instruction *> &buffer)
{
    GET_CHILD_PTR(node, PrimaryExp, 0);
    if (node)
    {
        ANALYSIS(node1, PrimaryExp, 0);
        COPY_EXP_NODE(node1, root);
    }
}
void Analyzer::analysisFuncRParams(FuncRParams *root, vector<ir::Instruction *> &buffer)
{
}

void Analyzer::analysisMulExp(MulExp *root, vector<ir::Instruction *> &buffer)
{
    ANALYSIS(node, UnaryExp, 0);
    GET_CHILD_PTR(node1, UnaryExp, 0);
    COPY_EXP_NODE(node1, root);
    for (auto i = 2; i < root->children.size(); i += 2)
    {
        ANALYSIS(node, UnaryExp, i);
        GET_CHILD_PTR(node2, Term, i - 1);
        if (node2->token.type == TokenType::MULT)
        {
            root->v = STR_MUL(root->v, node->v);
        }
        else if (node2->token.type == TokenType::DIV)
        {
            root->v = STR_DIV(root->v, node->v);
        }
        else if (node2->token.type == TokenType::MOD)
        {
            root->v = STR_MOD(root->v, node->v);
        }
    }
}

void Analyzer::analysisAddExp(AddExp *root, vector<ir::Instruction *> &buffer)
{
    {ANALYSIS(node, MulExp, 0)};
    GET_CHILD_PTR(node1, MulExp, 0);
    COPY_EXP_NODE(node1, root);
    for (auto i = 2; i < root->children.size(); i += 2)
    {
        ANALYSIS(node, MulExp, i);
        GET_CHILD_PTR(node2, Term, i - 1);

        if (node2->token.type == TokenType::PLUS)
        {
            root->v = STR_ADD(root->v, node->v);
        }
        else if (node2->token.type == TokenType::MINU)
        {
            root->v = STR_SUB(root->v, node->v);
        }
    }
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
    GET_CHILD_PTR(exp, Exp, 0);
    if (exp)
    {
        ANALYSIS(exp1, Exp, 0);
        COPY_EXP_NODE(exp1, root);
    }
    else
    {
        TODO
    }
}
void Analyzer::analysisUnaryOp(UnaryOp *root, vector<ir::Instruction *> &buffer)
{
}