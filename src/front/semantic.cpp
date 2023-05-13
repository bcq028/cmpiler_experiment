#include "front/semantic.h"
#include <cassert>
#include <iostream>
#include <sstream>
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

#define GET_RANDOM_NAM()                              \
    []() -> std::string {                             \
        static int counter = 0;                       \
        return "random_" + std::to_string(counter++); \
    }()

#define STR_ADD(str1, str2) (std::to_string(std::stoi(str1) + std::stoi(str2)))
#define STR_SUB(str1, str2) (std::to_string(std::stoi(str1) - std::stoi(str2)))
#define STR_MUL(x, y) (std::to_string(std::stoi(x) * std::stoi(y)))
#define STR_DIV(x, y) (std::to_string(std::stoi(x) / std::stoi(y)))
#define STR_MOD(x, y) (std::to_string(std::stoi(x) % std::stoi(y)))

vector<string> split(const string &s, char delim)
{
    vector<string> elems;
    std::stringstream ss(s);
    string item;
    while (getline(ss, item, delim))
    {
        elems.push_back(item);
    }
    return elems;
}

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
    if (node != nullptr)
    {
        s.name = "block";
    }
    s.cnt = this->scope_stack.size() + 1;
    this->scope_stack.push_back(s);
}
void frontend::SymbolTable::exit_scope()
{
    this->scope_stack.pop_back();
}

string frontend::SymbolTable::get_scoped_name(string id) const
{
    for (auto i = this->scope_stack.size() - 1; i >= 0; i--)
    {
        auto &table = this->scope_stack[i].table;
        for (auto &&iter : table)
        {
            if (iter.second.operand.name.substr(0, id.size()) == id)
            {
                return iter.second.operand.name;
            }
        }
    }
    assert(0 && "symbol not found error");
}
void frontend::Analyzer::add_symbol(string id, vector<int> *dimension, Type t)
{
    this->tmp_cnt++;
    if (!this->symbol_table.scope_stack.back().table.count(id))
    {
        STE ste;
        ste.operand = Operand(id, t);
        if (dimension != nullptr)
        {
            ste.dimension = *dimension;
        }
        ScopeInfo &sc = const_cast<ScopeInfo &>(this->symbol_table.scope_stack[this->symbol_table.scope_stack.size() - 1]);
        sc.table[id] = ste;

        int len = 1;
        if (dimension)
        {
            for (auto i = 0; i < dimension->size(); ++i)
            {
                len *= (*dimension)[i];
            }
        }

        // 在 IR 测评机中，认为一个出现在 des 位置的且没有被分配空间的变量即为一个新的变量，会自动为其分配空间

        if (this->symbol_table.scope_stack.back().name != "block")
        {
            ir::GlobalVal *gv = len == 1 ? new ir::GlobalVal(this->symbol_table.get_operand(id))
                                         : new ir::GlobalVal(this->symbol_table.get_operand(id), len);
            this->cur_program->globalVal.push_back(*gv);
        }
    }
}

Operand &frontend::SymbolTable::get_operand(string id)
{
    return this->get_ste(id).operand;
}

frontend::STE &frontend::SymbolTable::get_ste(string id)
{
    for (auto i = this->scope_stack.size() - 1; i >= 0; i--)
    {
        auto &table = this->scope_stack[i].table;
        for (auto &&iter : table)
        {
            if (iter.second.operand.name == id)
            {
                return iter.second;
            }
        }
    }
    assert(0 && "symbol not found error");
}

frontend::Analyzer::Analyzer() : tmp_cnt(0), symbol_table()
{
}

ir::Program frontend::Analyzer::get_ir_program(CompUnit *root)
{
    ir::Program *buffer = new ir::Program();
    ir::Function *globalFunc = new ir::Function("global", ir::Type::null);
    this->cur_program = buffer;
    this->symbol_table.add_scope(nullptr);
    analysisCompUnit(root, *buffer);
    this->symbol_table.exit_scope();
    for (auto inst : this->g_init_inst)
    {
        globalFunc->addInst(inst);
    }
    this->cur_program->addFunction(*globalFunc);
    ir::CallInst *callGlobal = new ir::CallInst(ir::Operand("global", ir::Type::null),
                                                ir::Operand("t0", ir::Type::null));
    for (auto &func : buffer->functions)
    {
        if (func.name == "main")
        {
            func.InstVec.insert(func.InstVec.begin(), callGlobal);
            break;
        }
    }
    return *buffer;
}

using namespace frontend;

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
        vector<ir::Instruction *> &buffer = this->g_init_inst;
        ANALYSIS(node, Decl, 0);
    }
    if (root->children.size() > 1)
    {
        ir::Program buffer;
        ANALYSIS(node, CompUnit, 1);
        for (auto func : buffer.functions)
        {
            this->cur_program->addFunction(func);
        }
        for (auto val : buffer.globalVal)
        {
            this->cur_program->globalVal.push_back(val);
        }
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
    ANALYSIS(node, BType, 1);
    root->t = node->t;
    ANALYSIS(node1, ConstDef, 2);
    this->symbol_table.get_operand(node1->arr_name).type = root->t;
    for (auto i = 3; i < root->children.size(); i += 2)
    {
        if (dynamic_cast<Term *>(root->children[i])->token.type == TokenType::COMMA)
        {
            ANALYSIS(node, ConstDef, i + 1);
            this->symbol_table.get_operand(node->arr_name).type = root->t;
        }
    }
}
void Analyzer::analysisVarDecl(VarDecl *root, vector<ir::Instruction *> &buffer)
{
    ANALYSIS(node, BType, 0);

    root->t = node->t;
    ANALYSIS(node1, VarDef, 1);
    this->symbol_table.get_operand(node1->arr_name).type = root->t;
    for (auto i = 2; i < root->children.size(); i += 2)
    {
        if (dynamic_cast<Term *>(root->children[i])->token.type == TokenType::COMMA)
        {
            ANALYSIS(node, VarDef, i + 1);
            this->symbol_table.get_operand(node->arr_name).type = root->t;
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
    root->arr_name = dynamic_cast<Term *>(root->children[0])->token.value + "_" + std::to_string(this->symbol_table.scope_stack.size());
    vector<int> *dimensions = new vector<int>();
    for (auto i = 1; i < root->children.size(); i += 3)
    {
        if (dynamic_cast<Term *>(root->children[i])->token.type == TokenType::LBRACK)
        {
            ANALYSIS(node, ConstExp, i + 1);
            dimensions->push_back(stoi(node->v));
        }
        else
        {
            break;
        }
    }
    auto parent = dynamic_cast<ConstDecl *>(root->parent);
    if (dimensions->size())
    {
        parent->t = parent->t == Type::Int ? Type::IntPtr : Type::FloatPtr;
    }
    this->add_symbol(root->arr_name, dimensions, dynamic_cast<ConstDecl *>(root->parent)->t);

    if (dimensions->size())
    {
        ir::Instruction *allocInst = new ir::Instruction();
        allocInst->des = this->symbol_table.get_operand(root->arr_name);
        allocInst->op = Operator::alloc;
        int op1 = 1;
        for (auto i : *dimensions)
        {
            op1 *= i;
        }
        allocInst->op1 = Operand(std::to_string(op1), Type::IntLiteral);
        buffer.push_back(allocInst);
    }

    for (auto i = 1; i < root->children.size(); i += 3)
    {
        if (dynamic_cast<Term *>(root->children[i])->token.type == TokenType::ASSIGN)
        {
            ANALYSIS(node, ConstInitVal, i + 1);
            if (symbol_table.get_operand(root->arr_name).type == Type::Int)
            {
                ir::Instruction *init = new ir::Instruction();
                init->des = this->symbol_table.get_operand(root->arr_name);
                init->op = Operator::mov;
                init->op1 = node->v;
                buffer.push_back(init);
            }
            else if (symbol_table.get_operand(root->arr_name).type == Type::Float)
            {
                ir::Instruction *init = new ir::Instruction();
                init->des = this->symbol_table.get_operand(root->arr_name);
                init->op = Operator::fmov;
                init->op1 = node->v;
                buffer.push_back(init);
            }
            else if (symbol_table.get_operand(root->arr_name).type == Type::IntPtr || symbol_table.get_operand(root->arr_name).type == Type::FloatPtr)
            {
                vector<string> values = split(node->v, ' ');
                if (values.size())
                {
                    values.erase(values.begin());
                }
                int size = values.size();
                for (int i = 0; i < size; i++)
                {
                    ir::Instruction *storeInst = new ir::Instruction();
                    storeInst->op = Operator::store;
                    storeInst->des = values[i];
                    storeInst->op1 = this->symbol_table.get_operand(root->arr_name);
                    storeInst->op2 = ir::Operand(std::to_string(i), Type::IntLiteral);
                    buffer.push_back(storeInst);
                }
            }
        }
    }
    this->symbol_table.get_ste(root->arr_name).dimension = *dimensions;
}
void Analyzer::analysisVarDef(VarDef *root, vector<ir::Instruction *> &buffer)
{
    root->arr_name = dynamic_cast<Term *>(root->children[0])->token.value + "_" + std::to_string(this->symbol_table.scope_stack.size());
    vector<int> *dimensions = new vector<int>();
    for (auto i = 1; i < root->children.size(); i += 3)
    {
        if (dynamic_cast<Term *>(root->children[i])->token.type == TokenType::LBRACK)
        {
            ANALYSIS(node, ConstExp, i + 1);
            dimensions->push_back(stoi(node->v));
        }
        else
        {
            break;
        }
    }
    auto parent = dynamic_cast<VarDecl *>(root->parent);
    if (dimensions->size())
    {
        parent->t = parent->t == Type::Int ? Type::IntPtr : Type::FloatPtr;
    }
    this->add_symbol(root->arr_name, dimensions, dynamic_cast<VarDecl *>(root->parent)->t);

    if (dimensions->size())
    {
        ir::Instruction *allocInst = new ir::Instruction();
        allocInst->des = this->symbol_table.get_operand(root->arr_name);
        allocInst->op = Operator::alloc;
        int op1 = 1;
        for (auto i : *dimensions)
        {
            op1 *= i;
        }
        allocInst->op1 = Operand(std::to_string(op1), Type::IntLiteral);
        buffer.push_back(allocInst);
    }

    for (auto i = 1; i < root->children.size(); i += 3)
    {
        if (dynamic_cast<Term *>(root->children[i])->token.type == TokenType::ASSIGN)
        {
            ANALYSIS(node, InitVal, i + 1);
            if (symbol_table.get_operand(root->arr_name).type == Type::Int)
            {
                ir::Instruction *init = new ir::Instruction();
                init->des = this->symbol_table.get_operand(root->arr_name);
                init->op = Operator::mov;
                init->op1 = node->v;
                buffer.push_back(init);
            }
            else if (symbol_table.get_operand(root->arr_name).type == Type::Float)
            {
                ir::Instruction *init = new ir::Instruction();
                init->des = this->symbol_table.get_operand(root->arr_name);
                init->op = Operator::fmov;
                init->op1 = node->v;
                buffer.push_back(init);
            }
            else if (symbol_table.get_operand(root->arr_name).type == Type::IntPtr || symbol_table.get_operand(root->arr_name).type == Type::FloatPtr)
            {
                vector<string> values = split(node->v, ' ');
                if (values.size())
                {
                    values.erase(values.begin());
                }
                int size = values.size();
                for (int i = 0; i < size; i++)
                {
                    ir::Instruction *storeInst = new ir::Instruction();
                    storeInst->op = Operator::store;
                    storeInst->des = values[i];
                    storeInst->op1 = this->symbol_table.get_operand(root->arr_name);
                    storeInst->op2 = ir::Operand(std::to_string(i), Type::IntLiteral);
                    buffer.push_back(storeInst);
                }
            }
        }
    }
    this->symbol_table.get_ste(root->arr_name).dimension = *dimensions;
}
void Analyzer::analysisConstExp(ConstExp *root, vector<ir::Instruction *> &buffer)
{
    ANALYSIS(node, AddExp, 0);
    COPY_EXP_NODE(node, root);
}
void Analyzer::analysisConstInitVal(ConstInitVal *root, vector<ir::Instruction *> &buffer)
{
    GET_CHILD_PTR(exp, ConstExp, 0);
    if (exp)
    {
        ANALYSIS(exp1, ConstExp, 0);
        root->t = exp1->t;
        root->v = exp1->v;
    }
    else
    {
        for (auto i = 1; i < root->children.size(); i += 2)
        {
            GET_CHILD_PTR(node, ConstInitVal, i);
            if (node)
            {
                ANALYSIS(initNode, ConstInitVal, i);
                root->v += ' ';
                root->v += initNode->v;
            }
            else
            {
                break;
            }
        }
    }
}
void Analyzer::analysisFuncDef(FuncDef *root, ir::Function &func)
{
    {
        auto &buffer = func.returnType;
        ANALYSIS(node, FuncType, 0);
    }
    {
        func.name = dynamic_cast<Term *>(root->children[1])->token.value;
    }
    this->symbol_table.add_scope(new Block());
    GET_CHILD_PTR(node, FuncFParams, 3);
    if (node)
    {
        auto &buffer = func.ParameterList;
        ANALYSIS(node, FuncFParams, 3);
    }
    auto &buffer = func.InstVec;
    ANALYSIS(node3, Block, root->children.size() - 1);
    this->symbol_table.exit_scope();
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
void Analyzer::analysisFuncFParam(FuncFParam *root, vector<ir::Operand> &oprands)
{
    auto buffer = this->g_init_inst;
    ANALYSIS(node, BType, 0);
    auto name = dynamic_cast<Term *>(root->children[1])->token.value;
    // TODO support more param type
    this->add_symbol(name, nullptr, node->t);
    oprands.push_back(this->symbol_table.get_operand(name));
}
void Analyzer::analysisFuncFParams(FuncFParams *root, vector<ir::Operand> &buffer)
{
    ANALYSIS(node, FuncFParam, 0);
    for (auto i = 1; i < root->children.size(); ++i)
    {
        if (dynamic_cast<Term *>(root->children[i])->token.type == TokenType::COMMA)
        {
            ANALYSIS(node, FuncFParam, i + 1);
        }
    }
}
void Analyzer::analysisBlock(Block *root, vector<ir::Instruction *> &insts)
{
    for (auto i = 1; i < root->children.size() - 1; ++i)
    {
        auto &buffer = insts;
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
void Analyzer::analysisStmt(Stmt *root, vector<ir::Instruction *> &buffer)
{
    ir::Instruction *inst = new ir::Instruction();
    {
        GET_CHILD_PTR(node, Term, 1);
        if (node)
        {
            if (node->token.type == TokenType::ASSIGN)
            {
                ANALYSIS(expNode, Exp, 2);
                ANALYSIS(lvalNode, LVal, 0);
                inst->op = Operator::mov;
                inst->des = this->symbol_table.get_operand(lvalNode->v);
                if (expNode->t == Type::Int || expNode->t == Type::Float)
                {
                    inst->op1 = this->symbol_table.get_operand(expNode->v);
                }
                else if (expNode->t == Type::IntLiteral || expNode->t == Type::FloatLiteral)
                {
                    inst->op1 = expNode->v;
                }
                else
                {
                    vector<string> values = split(expNode->v, ' ');
                    Operand des = this->symbol_table.get_operand(lvalNode->v);
                    int size = values.size();
                    for (int i = 0; i < size; i++)
                    {
                        ir::Instruction *storeInst = new ir::Instruction();
                        storeInst->op = Operator::store;
                        storeInst->des = values[i];
                        storeInst->op1 = this->symbol_table.get_operand(lvalNode->v);
                        storeInst->op2 = ir::Operand(std::to_string(i), Type::IntLiteral);
                        buffer.push_back(storeInst);
                    }
                }
            }
        }
    }
    GET_CHILD_PTR(node, Term, 0);
    if (node)
    {
        if (node->token.type == TokenType::RETURNTK)
        {
            inst->op = Operator::_return;
            if (root->children.size() > 2)
            {
                ANALYSIS(expNode, Exp, 1);
                if (expNode)
                {
                    if (expNode->t == Type::IntLiteral || expNode->t == Type::FloatLiteral)
                    {
                        inst->op1 = Operand(expNode->v, expNode->t);
                    }
                    else if (expNode->t == Type::Int || expNode->t == Type::Float)
                    {
                        inst->op1 = symbol_table.get_operand(expNode->v);
                    }
                    else
                    {
                        // ptr
                        ir::Instruction *inst = new ir::Instruction();
                        inst->op = Operator::load;
                        string desName = GET_RANDOM_NAM();
                        this->add_symbol(desName, nullptr, Type::Int);
                        inst->des = this->symbol_table.get_operand(desName);
                        inst->op1 = symbol_table.get_operand(expNode->v);
                        buffer.push_back(inst);
                    }
                }
            }
        }
    }
    buffer.push_back(inst);
}

void Analyzer::analysisLVal(LVal *root, vector<ir::Instruction *> &insts)
{
    string buffer = this->symbol_table.get_scoped_name(dynamic_cast<Term *>(root->children[0])->token.value);
    root->v = this->symbol_table.get_operand(buffer).name;
    root->t = this->symbol_table.get_operand(buffer).type;
    vector<int> dimen;
    for (auto i = 1; i < root->children.size(); i += 3)
    {
        auto &buffer = insts;
        ANALYSIS(node, Exp, i + 1);
        dimen.push_back(stoi(node->v));
    }
    int ind = 0;
    if (dimen.size())
    {
        for (auto i = 0; i < dimen.size() - 1; ++i)
        {
            ind += dimen[i] * this->symbol_table.get_ste(root->v).dimension[i];
        }
        ind += dimen[dimen.size() - 1];
    }
    root->i = ind;
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
        ANALYSIS(node, LVal, 0);
        COPY_EXP_NODE(node, root);
        if (node->i)
        {
            string t = GET_RANDOM_NAM();
            ir::Instruction *loadInst = new ir::Instruction();
            loadInst->op = Operator::load;
            loadInst->op1 = node->v;
            loadInst->op2 = Operand(std::to_string(node->i), Type::IntLiteral);
            string desName = GET_RANDOM_NAM();
            this->add_symbol(desName, nullptr, Type::Int);
            loadInst->des = this->symbol_table.get_operand(desName);
            buffer.push_back(loadInst);
            root->v = desName;
            root->t = node->t == Type::IntPtr ? Type::Int : Type::Float;
        }
        return;
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
        return;
    }
    GET_CHILD_PTR(op, UnaryOp, 0);
    if (op)
    {
        string c = dynamic_cast<Term *>(op->children[0])->token.value;
        ANALYSIS(uexp, UnaryExp, 1);
        root->v = c + uexp->v;
        root->t = uexp->t;
        return;
    }
    string func = dynamic_cast<Term *>(root->children[0])->token.value;
    ir::Type returnType;
    for (auto funct : this->symbol_table.functions)
    {
        if (funct.first == func)
        {
            returnType = funct.second->returnType;
            break;
        }
    }
    vector<string> params;
    if (root->children.size() > 3)
    {
        ANALYSIS(rp, FuncRParams, 2);
        for (auto c : funcRParam_ret)
        {
            func += c.name;
        }
    }
    string desName = GET_RANDOM_NAM();
    this->add_symbol(desName, nullptr, returnType);
    std::vector<ir::Operand> paramList;
    //TODO:support paramList
    ir::Instruction *inst = new ir::CallInst(ir::Operand(func, ir::Type::null),paramList,
                                             this->symbol_table.get_operand(desName));
    root->v = this->symbol_table.get_operand(desName).name;
    buffer.push_back(inst);
}
void Analyzer::analysisFuncRParams(FuncRParams *root, vector<ir::Instruction *> &buffer)
{
    // TODO
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
    ANALYSIS(node1, MulExp, 0);
    COPY_EXP_NODE(node1, root);
    for (auto i = 2; i < root->children.size(); i += 2)
    {
        ANALYSIS(node, MulExp, i);
        GET_CHILD_PTR(node2, Term, i - 1);

        if (node2->token.type == TokenType::PLUS)
        {
            if (node->t == Type::IntLiteral && root->t == Type::IntLiteral)
            {
                root->v = STR_ADD(root->v, node->v);
            }
            else
            {
                // TODO: consider float
                ir::Instruction *inst = new ir::Instruction();
                inst->op = root->t == Type::IntLiteral || node->t == Type::IntLiteral ? Operator::addi : Operator::add;
                string desName = GET_RANDOM_NAM();
                this->add_symbol(desName, nullptr, Type::Int);
                inst->des = this->symbol_table.get_operand(desName);
                if (inst->op == Operator::add)
                {
                    inst->op1 = this->symbol_table.get_operand(root->v);
                    inst->op2 = this->symbol_table.get_operand(node->v);
                }
                else
                {
                    inst->op2 = root->t == Type::Int ? root->v : node->v;
                    inst->op1 = root->t == Type::Int ? node->v : root->v;
                }
                root->v = this->symbol_table.get_operand(desName).name;
                buffer.push_back(inst);
            }
        }
        else if (node2->token.type == TokenType::MINU)
        {
            if (node->t == Type::IntLiteral && root->t == Type::IntLiteral)
            {
                root->v = STR_SUB(root->v, node->v);
            }
            else
            {
                // TODO: consider float
                ir::Instruction *inst = new ir::Instruction();
                inst->op = root->t == Type::IntLiteral || node->t == Type::IntLiteral ? Operator::subi : Operator::sub;
                string desName = GET_RANDOM_NAM();
                this->add_symbol(desName, nullptr, Type::Int);
                inst->des = this->symbol_table.get_operand(desName);
                if (inst->op == Operator::sub)
                {
                    inst->op1 = this->symbol_table.get_operand(root->v);
                    inst->op2 = this->symbol_table.get_operand(node->v);
                }
                else
                {
                    inst->op2 = root->t == Type::Int ? root->v : node->v;
                    inst->op1 = root->t == Type::Int ? node->v : root->v;
                }
                root->v = this->symbol_table.get_operand(desName).name;
                buffer.push_back(inst);
            }
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
        for (auto i = 1; i < root->children.size(); i += 2)
        {
            GET_CHILD_PTR(node, InitVal, i);
            if (node)
            {
                ANALYSIS(initNode, InitVal, i);
                root->v += ' ';
                root->v += initNode->v;
            }
            else
            {
                break;
            }
        }
    }
}
void Analyzer::analysisUnaryOp(UnaryOp *root, vector<ir::Instruction *> &buffer)
{
}