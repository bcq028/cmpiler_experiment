#include "front/semantic.h"
#include <cassert>
#include <iostream>
#include <regex>
#include <sstream>
using ir::Function;
using ir::Operand;
using ir::Operator;

#define TODO assert(0 && "TODO");

#define GET_CHILD_PTR(node, type, index) \
    auto node = dynamic_cast<type *>(root->children[index]);
#define ANALYSIS(node, type, index)                          \
                                                             \
    auto node = dynamic_cast<type *>(root->children[index]); \
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

inline bool IS_INT_T(ir::Type t)
{
    return t == Type::Int || t == Type::IntLiteral || t == Type::IntPtr;
}

inline bool IS_FLOAT_T(ir::Type t)
{
    return t == Type::Float || t == Type::FloatLiteral || t == Type::FloatPtr;
}

bool isNumber(std::string s, bool &isFloat)
{
    isFloat = false;
    std::regex num_regex("^[-+]?((\\d+)|(0x[\\da-fA-F]*)|(0o[0-7]*)|(0b[01]*))$");
    for (char c : s)
    {
        if (c == '.')
        {
            isFloat = true;
            break;
        }
    }
    return std::regex_match(s, num_regex) || isFloat;
}
string GET_RANDOM_NAM()
{
    static int counter = 0;
    return "random_" + std::to_string(counter++);
}
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
    for (int i = (int)this->scope_stack.size() - 1; i >= 0; i--)
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
    if (const_val_map.count(id))
        return *(new Operand(std::to_string(const_val_map[id]), Type::IntLiteral));
    bool isFloat;
    if (isNumber(id, isFloat))
    {
        ir::Type returnType;
        returnType = isFloat ? ir::Type::FloatLiteral : ir::Type::IntLiteral;
        return *new Operand(id, returnType);
    }
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
    globalFunc->addInst(new ir::Instruction(Operand(), Operand(), Operand(), Operator::__unuse__));
    this->cur_program = buffer;
    this->symbol_table.add_scope(nullptr);
    analysisCompUnit(root, *buffer);
    this->symbol_table.exit_scope();
    for (auto inst : this->g_init_inst)
    {
        globalFunc->addInst(inst);
    }
    globalFunc->addInst(new ir::Instruction(ir::Operand("null", Type::null), ir::Operand(), ir::Operand(), Operator::_return));
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

bool IS_SAME_TYPE(Type t1, Type t2)
{
    return (IS_INT_T(t1) && IS_INT_T(t2)) || (IS_FLOAT_T(t1) && IS_FLOAT_T(t2));
}

void Analyzer::assign(vector<ir::Instruction *> &buffer, const ir::Operand &t1, ir::Operand *des)
{
    assert(des->type != Type::null);
    if (des->name == "null" || des->name == "")
    {
        auto name = GET_RANDOM_NAM();
        add_symbol(name, nullptr, des->type);
        des->name = symbol_table.get_operand(name).name;
    }

    ir::Operand val = t1;
    if (!IS_SAME_TYPE(t1.type, des->type))
    {
        val = convert(buffer, des->type == Type::Int ? false : true, t1);
    }
    ir::Instruction *inst = new ir::Instruction();
    inst->op = IS_INT_T(des->type) ? Operator::mov : Operator::fmov;
    inst->des.name = des->name;
    inst->des.type = des->type;
    inst->op1 = val;
    buffer.push_back(inst);
}

ir::Operand Analyzer::convert(vector<ir::Instruction *> &buffer, bool int2float, const ir::Operand &op1)
{
    ir::Instruction *convertInst = new ir::Instruction();
    string name = GET_RANDOM_NAM();
    add_symbol(name, nullptr, int2float ? Type::Float : Type::Int);
    convertInst->op = int2float ? Operator::cvt_i2f : Operator::cvt_f2i;
    convertInst->op1 = op1;
    convertInst->des = symbol_table.get_operand(name);
    buffer.push_back(convertInst);
    ir::Operand ret;
    ret.name = symbol_table.get_operand(name).name;
    ret.type = symbol_table.get_operand(name).type;
    return ret;
}

bool is_logic_op(TokenType t)
{
    return t == TokenType::LSS || t == TokenType::LEQ || t == TokenType::GTR || t == TokenType::GEQ || t == TokenType::EQL || t == TokenType::NEQ;
}

void Analyzer::processExp(vector<ir::Instruction *> &buffer, const ir::Operand &t1, const ir::Operand &t2, ir::Operand *des, TokenType c)
{

    if (is_logic_op(c))
    {
        if (IS_FLOAT_T(t1.type) || IS_FLOAT_T(t2.type))
        {
            processFloatExp(buffer, t1, t2, des, c);
        }
        else
        {
            processIntExp(buffer, t1, t2, des, c);
        }
        return;
    }

    if (c == TokenType::AND || c == TokenType::OR || c == TokenType::NOT)
    {
        processIntExp(buffer, t1, t2, des, c);
        assert(des->type == Type::Int);
        return;
    }

    if (IS_INT_T(des->type))
    {
        processIntExp(buffer, t1, t2, des, c);
    }
    else
    {
        processFloatExp(buffer, t1, t2, des, c);
    }
}

void Analyzer::processIntExp(vector<ir::Instruction *> &buffer, const ir::Operand &t1o, const ir::Operand &t2o, ir::Operand *des, TokenType c)
{
    ir::Operand t1 = t1o, t2 = t2o;

    if (IS_FLOAT_T(t1o.type))
    {
        t1 = convert(buffer, false, t1o);
    }
    if (IS_FLOAT_T(t2o.type))
    {
        t2 = convert(buffer, false, t2o);
    }
    ir::Instruction *inst = new ir::Instruction();

    string t1_name = t1.name;
    string t2_name = t2.name;

    auto process_no_i = [&]()
    {
        if (t1.type == Type::IntLiteral)
        {
            ir::Operand *ret = new ir::Operand();
            ret->type = Type::Int;
            assign(buffer, t1, ret);
            t1_name = ret->name;
        }

        if (t2.type == Type::IntLiteral)
        {
            ir::Operand *ret = new ir::Operand();
            ret->type = Type::Int;
            assign(buffer, t2, ret);
            t2_name = ret->name;
        }
    };

    if (c == TokenType::PLUS)
    {
        inst->op = Operator::add;

        if (t1.type == Type::IntLiteral && t2.type == Type::IntLiteral)
        {
            des->name = STR_ADD(t1.name, t2.name);
            des->type = Type::IntLiteral;
            return;
        }
        else if (t1.type == Type::IntLiteral)
        {
            inst->op = Operator::addi;
            t1_name = t2.name;
            t2_name = t1.name;
        }
        else if (t2.type == Type::IntLiteral)
        {
            inst->op = Operator::addi;
            t1_name = t1.name;
            t2_name = t2.name;
        }
    }
    else if (c == TokenType::MINU)
    {
        inst->op = Operator::sub;
        if (t1.type == Type::IntLiteral && t2.type == Type::IntLiteral)
        {
            des->name = STR_SUB(t1.name, t2.name);
            des->type = Type::IntLiteral;
            return;
        }
        else if (t1.type == Type::IntLiteral)
        {
            inst->op = Operator::subi;
            t1_name = t2.name;
            t2_name = t1.name;
        }
        else if (t2.type == Type::IntLiteral)
        {
            inst->op = Operator::subi;
            t1_name = t1.name;
            t2_name = t2.name;
        }
    }
    else if (c == TokenType::MULT)
    {
        if (t1.type == Type::IntLiteral && t2.type == Type::IntLiteral)
        {
            des->name = STR_MUL(t1.name, t2.name);
            des->type = Type::IntLiteral;
            return;
        }
        process_no_i();
        inst->op = Operator::mul;
    }
    else if (c == TokenType::DIV)
    {
        if (t1.type == Type::IntLiteral && t2.type == Type::IntLiteral)
        {
            des->name = STR_DIV(t1.name, t2.name);
            des->type = Type::IntLiteral;
            return;
        }
        process_no_i();
        inst->op = Operator::div;
    }
    else if (c == TokenType::MOD)
    {
        if (t1.type == Type::IntLiteral && t2.type == Type::IntLiteral)
        {
            des->name = STR_MOD(t1.name, t2.name);
            des->type = Type::IntLiteral;
            return;
        }
        process_no_i();
        inst->op = Operator::mod;
    }
    else if (c == TokenType::LSS)
    {
        process_no_i();
        inst->op = Operator::lss;
    }
    else if (c == TokenType::LEQ)
    {
        process_no_i();
        inst->op = Operator::leq;
    }
    else if (c == TokenType::GTR)
    {
        process_no_i();
        inst->op = Operator::gtr;
    }
    else if (c == TokenType::GEQ)
    {
        process_no_i();
        inst->op = Operator::geq;
    }
    else if (c == TokenType::EQL)
    {
        process_no_i();
        inst->op = Operator::eq;
    }
    else if (c == TokenType::NEQ)
    {
        process_no_i();
        inst->op = Operator::neq;
    }
    else if (c == TokenType::AND)
    {
        process_no_i();
        inst->op = Operator::_and;
    }
    else if (c == TokenType::OR)
    {
        process_no_i();
        inst->op = Operator::_or;
    }
    else if (c == TokenType::NOT)
    {
        process_no_i();
        inst->op = Operator::_not;
    }

    string ret_name = GET_RANDOM_NAM();
    add_symbol(ret_name, nullptr, Type::Int);
    inst->op1 = symbol_table.get_operand(t1_name);
    if (t2.type != Type::null)
    {
        inst->op2 = symbol_table.get_operand(t2_name);
    }
    inst->des = symbol_table.get_operand(ret_name);
    des->name = symbol_table.get_operand(ret_name).name;
    des->type = Type::Int;
    buffer.push_back(inst);
}

void Analyzer::processFloatExp(vector<ir::Instruction *> &buffer, const ir::Operand &t1o, const ir::Operand &t2o, ir::Operand *des, TokenType c)
{
    ir::Operand t1 = t1o, t2 = t2o;

    if (IS_INT_T(t1o.type))
    {
        t1 = convert(buffer, true, t1o);
    }
    if (IS_INT_T(t2o.type))
    {
        t2 = convert(buffer, true, t2o);
    }
    ir::Instruction *inst = new ir::Instruction();

    string t1_name = t1.name;
    string t2_name = t2.name;

    if (t1.type == Type::FloatLiteral)
    {
        ir::Operand *ret = new ir::Operand();
        ret->type = Type::Float;
        assign(buffer, t1, ret);
        t1_name = ret->name;
    }

    if (t2.type == Type::FloatLiteral)
    {
        ir::Operand *ret = new ir::Operand();
        ret->type = Type::Float;
        assign(buffer, t2, ret);
        t2_name = ret->name;
    }

    if (c == TokenType::PLUS)
    {
        inst->op = Operator::fadd;
    }
    else if (c == TokenType::MINU)
    {
        inst->op = Operator::fsub;
    }
    else if (c == TokenType::MULT)
    {
        inst->op = Operator::fmul;
    }
    else if (c == TokenType::DIV)
    {
        inst->op = Operator::fdiv;
    }
    else if (c == TokenType::MOD)
    {
        assert(0 && "mod float not supported");
    }
    else if (c == TokenType::LSS)
    {
        inst->op = Operator::flss;
    }
    else if (c == TokenType::LEQ)
    {
        inst->op = Operator::fleq;
    }
    else if (c == TokenType::GTR)
    {
        inst->op = Operator::fgtr;
    }
    else if (c == TokenType::GEQ)
    {
        inst->op = Operator::fgeq;
    }
    else if (c == TokenType::EQL)
    {
        inst->op = Operator::feq;
    }
    else if (c == TokenType::NEQ)
    {
        inst->op = Operator::fneq;
    }
    else if (c == TokenType::NOT)
    {
        assert(0 &&"not cannot float");
    }else{
        assert(0 && "unknown type");
    }

    string ret_name = GET_RANDOM_NAM();
    add_symbol(ret_name, nullptr, Type::Float);
    inst->op1 = symbol_table.get_operand(t1_name);
    if (t2.type != Type::null)
    {
        inst->op2 = symbol_table.get_operand(t2_name);
    }
    inst->des = symbol_table.get_operand(ret_name);

    des->name = symbol_table.get_operand(ret_name).name;
    des->type = Type::Float;
    buffer.push_back(inst);
}

void Analyzer::GOTO(vector<ir::Instruction *> &buffer, int label, const ir::Operand &cond, ir::Instruction *inst)
{
    if (inst == nullptr)
    {
        inst = new ir::Instruction();
    }

    inst->op = Operator::_goto;
    inst->op1 = cond;
    inst->des = Operand(std::to_string(label - (int)buffer.size()), Type::IntLiteral);
    buffer.push_back(inst);
}

void Analyzer::analysisCompUnit(CompUnit *root, ir::Program &program)
{
    ir::Function *buffer = new ir::Function();
    GET_CHILD_PTR(node, FuncDef, 0);
    if (node)
    {
        ANALYSIS(node, FuncDef, 0);
        program.addFunction(*buffer);
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
        parent->t = IS_INT_T(parent->t) ? Type::IntPtr : Type::FloatPtr;
    }
    else
    {
        parent->t = IS_INT_T(parent->t) ? Type::Int : Type::Float;
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
                assert(root->arr_name != "");
                assign(buffer, symbol_table.get_operand(node->v), &symbol_table.get_operand(root->arr_name));
                if (node->t == Type::IntLiteral)
                {
                    symbol_table.const_val_map[root->arr_name] = stoi(node->v);
                }
            }
            else if (symbol_table.get_operand(root->arr_name).type == Type::Float)
            {
                assert(root->arr_name != "");
                assign(buffer, symbol_table.get_operand(node->v), &symbol_table.get_operand(root->arr_name));
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
                    storeInst->des = this->symbol_table.get_operand(values[i]);
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
    if (dimensions->size())
    {
        dynamic_cast<VarDecl *>(root->parent)->t = IS_INT_T(dynamic_cast<VarDecl *>(root->parent)->t) ? Type::IntPtr : Type::FloatPtr;
    }
    else
    {
        dynamic_cast<VarDecl *>(root->parent)->t = IS_INT_T(dynamic_cast<VarDecl *>(root->parent)->t) ? Type::Int : Type::Float;
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
                assert(root->arr_name != "");
                assign(buffer, symbol_table.get_operand(node->v), &symbol_table.get_operand(root->arr_name));
            }
            else if (symbol_table.get_operand(root->arr_name).type == Type::Float)
            {
                assert(root->arr_name != "");
                assign(buffer, symbol_table.get_operand(node->v), &symbol_table.get_operand(root->arr_name));
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
                    storeInst->des = this->symbol_table.get_operand(values[i]);
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
void Analyzer::analysisFuncDef(FuncDef *root, ir::Function *func)
{
    cur_func = func;
    {
        auto &buffer = func->returnType;
        ANALYSIS(node, FuncType, 0);
        this->symbol_table.cur_return_type = func->returnType;
    }
    {
        func->name = dynamic_cast<Term *>(root->children[1])->token.value;
    }
    this->symbol_table.functions[func->name] = func;
    this->symbol_table.add_scope(new Block());
    GET_CHILD_PTR(node, FuncFParams, 3);
    if (node)
    {
        auto &buffer = func->ParameterList;
        ANALYSIS(node, FuncFParams, 3);
    }
    auto &buffer = func->InstVec;
    ANALYSIS(node3, Block, root->children.size() - 1);
    this->symbol_table.exit_scope();
    func->InstVec.push_back(new ir::Instruction(symbol_table.get_operand("0"), Operand(), Operand(), Operator::_return));
    this->symbol_table.cur_return_type = Type::null;
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
void Analyzer::analysisFuncFParam(FuncFParam *root, vector<ir::Operand> &operands)
{
    auto buffer = this->g_init_inst;
    auto bTypeNode = dynamic_cast<BType *>(root->children[0]);
    analysisBType(bTypeNode, buffer);
    auto nameNode = dynamic_cast<Term *>(root->children[1]);
    auto name = nameNode->token.value;
    bool isArray = false;
    for (int i = 2; i < root->children.size(); i++)
    {
        if (dynamic_cast<Term *>(root->children[i])->token.type == TokenType::LBRACK)
        {
            isArray = true;
            break;
        }
    }

    ir::Type finalType = bTypeNode->t;
    if (isArray)
    {
        if (IS_INT_T(finalType))
        {
            finalType = Type::IntPtr;
        }
        else if (IS_FLOAT_T(finalType))
        {
            finalType = Type::FloatPtr;
        }
    }

    this->add_symbol(name, nullptr, finalType);
    operands.push_back(this->symbol_table.get_operand(name));
}

void Analyzer::analysisFuncFParams(FuncFParams *root, vector<ir::Operand> &buffer)
{
    ANALYSIS(node, FuncFParam, 0);
    for (auto i = 1; i < root->children.size(); i += 2)
    {
        if (dynamic_cast<Term *>(root->children[i]) && dynamic_cast<Term *>(root->children[i])->token.type == TokenType::COMMA)
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
    GET_CHILD_PTR(blockNode, Block, 0);
    if (blockNode)
    {
        ANALYSIS(node, Block, 0);
        return;
    }
    if (dynamic_cast<Term *>(root->children[1]) && dynamic_cast<Term *>(root->children[1])->token.type == TokenType::ASSIGN)
    {
        ANALYSIS(expNode, Exp, 2);
        ANALYSIS(lvalNode, LVal, 0);
        if (lvalNode->t != Type::IntPtr && lvalNode->t != Type::FloatPtr)
        {
            assert(lvalNode->v != "");
            assign(buffer, symbol_table.get_operand(expNode->v), &symbol_table.get_operand(lvalNode->v));
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
                storeInst->des = this->symbol_table.get_operand(values[i]);
                storeInst->op1 = this->symbol_table.get_operand(lvalNode->v);
                storeInst->op2 = this->symbol_table.get_operand(lvalNode->i);
                buffer.push_back(storeInst);
            }
            return;
        }
    }
    if (dynamic_cast<Term *>(root->children[0]) && dynamic_cast<Term *>(root->children[0])->token.type == TokenType::IFTK)
    {
        ANALYSIS(cond, Cond, 2);
        ir::Operand *notCond = new ir::Operand();
        notCond->type = Type::Int;
        processExp(buffer, symbol_table.get_operand(cond->v), ir::Operand(), notCond, TokenType::NOT);
        ir::Instruction *jumpToFail = new ir::Instruction();
        int jumptoFail_pc = buffer.size();
        GOTO(buffer, 0, *notCond, jumpToFail);
        ANALYSIS(stmt, Stmt, 4);
        ir::Instruction *jumpOut = new ir::Instruction();
        int jumpOut_pc = buffer.size();
        GOTO(buffer, 0, ir::Operand(), jumpOut);
        if (root->children.size() > 5)
        {
            jumpToFail->des.name = std::to_string((int)buffer.size() - jumptoFail_pc);
            ANALYSIS(elseStmt, Stmt, 6);
            jumpOut->des.name = std::to_string((int)buffer.size() - jumpOut_pc);
            insertEmpt(buffer);
        }
        else
        {
            jumpToFail->des.name = std::to_string((int)buffer.size() - jumptoFail_pc);
            jumpOut->des.name = std::to_string((int)buffer.size() - jumpOut_pc);
            insertEmpt(buffer);
        }

        return;
    }
    if (dynamic_cast<Term *>(root->children[0]) && dynamic_cast<Term *>(root->children[0])->token.type == TokenType::WHILETK)
    {
        int prevc_pc = this->continue_pc;
        this->continue_pc = buffer.size();
        ANALYSIS(cond, Cond, 2);
        ir::Operand *notCond = new ir::Operand();
        notCond->type = Type::Int;
        processExp(buffer, symbol_table.get_operand(cond->v), ir::Operand(), notCond, TokenType::NOT);
        ir::Instruction *breakwhile_inst = new ir::Instruction();
        int cur_pc = buffer.size();
        GOTO(buffer, 0, *notCond, breakwhile_inst);
        ANALYSIS(stmt, Stmt, 4);
        // while end, continue
        GOTO(buffer, this->continue_pc, ir::Operand(), nullptr);
        this->continue_pc = prevc_pc;
        insertEmpt(buffer);
        for (int i = 0; i < this->break_insts.size(); ++i)
        {
            break_insts[i]->des.name = std::to_string((int)buffer.size() - 1 - this->break_pcs[i]);
        }
        breakwhile_inst->des.name = std::to_string((int)buffer.size() - 1 - cur_pc);
        this->break_insts.clear();
        this->break_pcs.clear();
        return;
    }
    if (dynamic_cast<Term *>(root->children[0]) && dynamic_cast<Term *>(root->children[0])->token.type == TokenType::CONTINUETK)
    {
        GOTO(buffer, this->continue_pc, ir::Operand(), nullptr);
        return;
    }
    if (dynamic_cast<Term *>(root->children[0]) && dynamic_cast<Term *>(root->children[0])->token.type == TokenType::BREAKTK)
    {
        auto break_inst = new ir::Instruction();
        this->break_insts.push_back(break_inst);
        GOTO(buffer, 0, ir::Operand(), break_inst);
        this->break_pcs.push_back(buffer.size() - 1);
        return;
    }
    if (dynamic_cast<Term *>(root->children[0]) && dynamic_cast<Term *>(root->children[0])->token.type == TokenType::RETURNTK)
    {
        ir::Instruction *inst = new ir::Instruction();
        inst->op = Operator::_return;

        if (root->children.size() > 2)
        {
            ANALYSIS(expNode, Exp, 1);
            ir::Operand *ret = new ir::Operand();
            ret->type = symbol_table.cur_return_type;
            if (expNode->t != Type::IntPtr && expNode->t != Type::FloatPtr)
            {
                assign(buffer, symbol_table.get_operand(expNode->v), ret);
                inst->op1 = *ret;
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
            }
            buffer.push_back(inst);
        }
    }
    if (dynamic_cast<Exp *>(root->children[0]))
    {
        ANALYSIS(node, Exp, 0);
        return;
    }

    if (dynamic_cast<Term *>(root->children[0]) && dynamic_cast<Term *>(root->children[0])->token.type == TokenType::SEMICN)
    {
        return;
    }
}

void Analyzer::analysisLVal(LVal *root, vector<ir::Instruction *> &insts)
{
    string buffer = this->symbol_table.get_scoped_name(dynamic_cast<Term *>(root->children[0])->token.value);
    root->v = this->symbol_table.get_operand(buffer).name;
    root->t = this->symbol_table.get_operand(buffer).type;

    vector<ir::Operand> dimen;
    for (auto i = 1; i < root->children.size(); i += 3)
    {
        auto &buffer = insts;
        ANALYSIS(node, Exp, i + 1);
        dimen.push_back(symbol_table.get_operand(node->v));
    }

    if (!dimen.size())
        return;

    ir::Operand *ret = new ir::Operand("0", Type::IntLiteral);
    for (int i = 0; i < (int)dimen.size() - 1; ++i)
    {
        ir::Operand *t = new ir::Operand();
        t->type = Type::Int;
        ir::Operand op1 = ir::Operand(std::to_string(symbol_table.get_ste(root->v).dimension[symbol_table.get_ste(root->v).dimension.size() - 1 - i]), Type::IntLiteral);
        processExp(insts, dimen[i], op1, t, TokenType::MULT);
        processExp(insts, *ret, *t, ret, TokenType::PLUS);
    }
    processExp(insts, *ret, dimen[dimen.size() - 1], ret, TokenType::PLUS);
    root->i = ret->name;
}

void Analyzer::analysisExp(Exp *root, vector<ir::Instruction *> &buffer)
{
    ANALYSIS(node, AddExp, 0);
    GET_CHILD_PTR(node1, AddExp, 0);
    COPY_EXP_NODE(node1, root);
}
void Analyzer::analysisCond(Cond *root, vector<ir::Instruction *> &buffer)
{
    ANALYSIS(node, LOrExp, 0);
    COPY_EXP_NODE(node, root);
}
void Analyzer::analysisNumber(Number *root, string &buffer)
{
    GET_CHILD_PTR(node, Term, 0);
    bool isFLoat;
    isNumber(node->token.value, isFLoat);
    root->t = isFLoat ? Type::FloatLiteral : Type::IntLiteral;
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
        if (node->i != "")
        {
            ir::Instruction *loadInst = new ir::Instruction();
            loadInst->op = Operator::load;
            loadInst->op1 = symbol_table.get_operand(node->v);
            loadInst->op2 = symbol_table.get_operand(node->i);
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
        if (c == "+")
        {
            root->v = uexp->v;
            root->t = uexp->t;
        }
        else if (c == "-")
        {
            ir::Operand *ret = new ir::Operand();
            ret->type = uexp->t;
            ir::Operand op1 = Operand("-1", Type::IntLiteral);
            processExp(buffer, op1, symbol_table.get_operand(uexp->v), ret, TokenType::MULT);
            root->v = ret->name;
            root->t = ret->type;
        }
        else if (c == "!")
        {
            ir::Operand *des = new ir::Operand();
            processExp(buffer, this->symbol_table.get_operand(uexp->v), ir::Operand(), des, TokenType::NOT);
            root->v = des->name;
            root->t = des->type;
        }
        return;
    }

    // call function

    string func = dynamic_cast<Term *>(root->children[0])->token.value;
    ir::Type returnType;
    auto lib_funcs = get_lib_funcs();
    ir::Function *lastfunc = this->cur_func;
    if (lib_funcs->count(func))
    {
        this->cur_func = (*lib_funcs)[func];
        returnType = (*lib_funcs)[func]->returnType;
    }
    else
    {
        for (auto funct : this->symbol_table.functions)
        {
            if (funct.first == func)
            {
                this->cur_func = funct.second;
                returnType = funct.second->returnType;
                break;
            }
        }
    }
    vector<string> params;
    std::vector<ir::Operand> paramList;
    if (root->children.size() > 3)
    {
        ANALYSIS(rp, FuncRParams, 2);
        paramList = analysisFuncRParams(dynamic_cast<FuncRParams *>(root->children[2]), buffer);
    }
    string desName = GET_RANDOM_NAM();
    this->add_symbol(desName, nullptr, returnType);
    assert(paramList.size() == cur_func->ParameterList.size());
    for (int i = 0; i < paramList.size(); ++i)
    {
        if (!IS_SAME_TYPE(paramList[i].type, cur_func->ParameterList[i].type))
        {
            ir::Operand *t = new ir::Operand();
            t->type = cur_func->ParameterList[i].type;
            assign(buffer, paramList[i], t);
            paramList[i] = *t;
        }
    }
    ir::Instruction *inst = new ir::CallInst(ir::Operand(func, ir::Type::null), paramList,
                                             this->symbol_table.get_operand(desName));
    root->v = this->symbol_table.get_operand(desName).name;
    buffer.push_back(inst);
    this->cur_func = lastfunc;
}
std::vector<ir::Operand> Analyzer::analysisFuncRParams(FuncRParams *root, vector<ir::Instruction *> &buffer)
{
    std::vector<ir::Operand> funcRParam_ret;

    ANALYSIS(node0, Exp, 0);
    funcRParam_ret.push_back(this->symbol_table.get_operand(node0->v));
    for (auto i = 2; i < root->children.size(); i += 2)
    {
        ANALYSIS(node, Exp, i);
        funcRParam_ret.push_back(this->symbol_table.get_operand(node->v));
    }
    return funcRParam_ret;
}

void Analyzer::analysisMulExp(MulExp *root, vector<ir::Instruction *> &buffer)
{
    ANALYSIS(node, UnaryExp, 0);
    COPY_EXP_NODE(node, root);
    for (auto i = 2; i < root->children.size(); i += 2)
    {
        ANALYSIS(node, UnaryExp, i);
        GET_CHILD_PTR(op, Term, i - 1);
        ir::Operand *ret = new ir::Operand();
        ret->type = IS_FLOAT_T(node->t) || IS_FLOAT_T(root->t) ? Type::Float : Type::Int;
        processExp(buffer, symbol_table.get_operand(root->v), symbol_table.get_operand(node->v), ret, op->token.type);
        root->v = ret->name;
        root->t = ret->type;
    }
}

void Analyzer::analysisAddExp(AddExp *root, vector<ir::Instruction *> &buffer)
{
    ANALYSIS(node1, MulExp, 0);
    COPY_EXP_NODE(node1, root);
    for (size_t i = 2; i < root->children.size(); i += 2)
    {
        ANALYSIS(node, MulExp, i);
        GET_CHILD_PTR(node2, Term, i - 1);
        ir::Operand *ret = new ir::Operand();
        ret->type = IS_FLOAT_T(node->t) || IS_FLOAT_T(root->t) ? Type::Float : Type::Int;
        processExp(buffer, symbol_table.get_operand(root->v), symbol_table.get_operand(node->v), ret, node2->token.type);
        root->v = ret->name;
        root->t = ret->type;
    }
}

void Analyzer::analysisRelExp(RelExp *root, vector<ir::Instruction *> &buffer)
{
    ANALYSIS(node, AddExp, 0);
    COPY_EXP_NODE(node, root);
    for (size_t i = 2; i < root->children.size(); i += 2)
    {
        ANALYSIS(node2, AddExp, 2);
        Operand *des;
        des->type = Type::Int;
        auto op = dynamic_cast<Term *>(root->children[1])->token.type;
        processExp(buffer, symbol_table.get_operand(root->v), symbol_table.get_operand(node2->v), des, op);
        root->v = des->name;
        root->t = des->type;
    }
}
void Analyzer::analysisEqExp(EqExp *root, vector<ir::Instruction *> &buffer)
{
    ANALYSIS(node, RelExp, 0);
    COPY_EXP_NODE(node, root);
    for (size_t i = 2; i < root->children.size(); i += 2)
    {
        ANALYSIS(node2, RelExp, 2);
        Operand *des;
        des->type = Type::Int;
        auto op = dynamic_cast<Term *>(root->children[1])->token.type;
        processExp(buffer, symbol_table.get_operand(root->v), symbol_table.get_operand(node2->v), des, op);
        root->v = des->name;
        root->t = des->type;
    }
}
void Analyzer::analysisLAndExp(LAndExp *root, vector<ir::Instruction *> &buffer)
{
    ANALYSIS(node, EqExp, 0);
    COPY_EXP_NODE(node, root);
    if (root->children.size() > 1)
    {
        ANALYSIS(node2, LAndExp, 2);
        Operand *des;
        des->type = Type::Int;
        auto op = dynamic_cast<Term *>(root->children[1])->token.type;
        processExp(buffer, symbol_table.get_operand(root->v), symbol_table.get_operand(node2->v), des, op);
        root->v = des->name;
        root->t = des->type;
    }
}
void Analyzer::analysisLOrExp(LOrExp *root, vector<ir::Instruction *> &buffer)
{
    ANALYSIS(node, LAndExp, 0);
    COPY_EXP_NODE(node, root);
    if (root->children.size() > 1)
    {
        ANALYSIS(node2, LOrExp, 2);
        Operand *des;
        des->type = Type::Int;
        auto op = dynamic_cast<Term *>(root->children[1])->token.type;
        processExp(buffer, symbol_table.get_operand(root->v), symbol_table.get_operand(node2->v), des, op);
        root->v = des->name;
        root->t = des->type;
    }
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
