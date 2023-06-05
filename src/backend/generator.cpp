#include "backend/generator.h"

#include <assert.h>
#include <iostream>
#define TODO assert(0 && "todo");
using namespace rv;

bool isLiteral(ir::Type t)
{
    return t == ir::Type::IntLiteral || t == ir::Type::FloatLiteral;
}

namespace rv
{
    std::string toString(rvREG r)
    {
        switch (r)
        {
        case rvREG::zero:
            return "zero";
            break;
        case rvREG::ra:
            return "ra";
            break;
        case rvREG::sp:
            return "sp";
            break;
        case rvREG::gp:
            return "gp";
            break;
        case rvREG::tp:
            return "tp";
            break;
        case rvREG::t0:
            return "t0";
            break;
        case rvREG::t1:
            return "t1";
            break;
        case rvREG::t2:
            return "t2";
            break;
        case rvREG::s0:
            return "s0";
            break;
        case rvREG::a0:
            return "a0";
            break;
        default:
            break;
        }
    }

}

rv::rvREG backend::Generator::getRd(ir::Operand = ir::Operand())
{
    return rvREG::t0;
}
rv::rvFREG backend::Generator::fgetRd(ir::Operand = ir::Operand())
{
    return rvFREG::ft0;
}
rv::rvREG backend::Generator::getRs1(ir::Operand = ir::Operand())
{
    return rvREG::t1;
}
rv::rvREG backend::Generator::getRs2(ir::Operand = ir::Operand())
{
    return rvREG::t2;
}
rv::rvFREG backend::Generator::fgetRs1(ir::Operand = ir::Operand())
{
    return rvFREG::ft1;
}
rv::rvFREG backend::Generator::fgetRs2(ir::Operand = ir::Operand())
{
    return rvFREG::ft2;
}

// load oper to register
rv::rv_inst backend::Generator::get_ld_inst(const ir::Operand &oper, rv::rvREG reg)
{
    rv::rv_inst inst;
    if (oper.type == ir::Type::IntLiteral)
    {
        inst.op = rv::rvOPCODE::LI;
        inst.rd = reg;
        inst.imm = stoi(oper.name);
    }
    else if (oper.type == ir::Type::Int)
    {
        inst.op = rv::rvOPCODE::LW;
        inst.rd = reg;
        inst.rs1 = rv::rvREG::s0;
        inst.imm = this->find_operand(oper.name);
        // process globalV
        if ((int)inst.imm == -1)
        {
            inst.op = rv::rvOPCODE::LI;
            inst.rd = reg;
            assert(globalVM.count(oper.name) && "check globalVM");
            inst.imm = stoi(this->globalVM[oper.name]);
        }
    }
    return inst;
}

backend::Generator::Generator(ir::Program &p, std::ofstream &f) : program(p), fout(f) {}

void backend::Generator::gen()
{
    std::string file_name = "\"main.c\"";
    std::string header = ".file    " + file_name + "\n\
    .option nopic   \n\
    .text     \n\
    .align    1  \n\
    .globl    main  \n\
    .type    main, @function\n";
    this->fout << header;

    // gloval V
    for (auto glovalV : this->program.globalVal)
    {
        this->fout << glovalV.val.name << ":\n";

        for (auto func : this->program.functions)
        {
            if (func.name == "global")
            {
                for (auto inst : func.InstVec)
                {
                    if (inst->des.name == glovalV.val.name)
                    {
                        if (inst->op1.type == ir::Type::IntLiteral)
                        {
                            this->fout << "   .word   " << inst->op1.name << '\n';
                            this->globalVM[glovalV.val.name] = inst->op1.name;
                        }
                    }
                }
                break;
            }
        }
    }

    for (auto func : this->program.functions)
    {
        if (func.name == "global")
            continue;
        this->fout << func.name << ":\n";
        this->callee(func);
    }
}

int backend::stackVarMap::add_operand(ir::Operand oper, uint32_t size = 4)
{
    this->offset_table[oper.name] = this->cur_offset;
    this->cur_offset -= size;
    return this->offset_table[oper.name];
}

int backend::Generator::find_operand(ir::Operand oper)
{
    if (!this->stackMap.offset_table.count(oper.name))
    {
        std::string gv;
        for (auto i : this->program.globalVal)
        {
            if (i.val.name == oper.name)
            {
                return -1;
            }
        }
        this->stackMap.add_operand(oper);
    }
    return this->stackMap.offset_table[oper.name];
}

void backend::Generator::callee(ir::Function &f)
{
    int size = f.InstVec.size() * 4 + 12;
    this->fout << "addi sp,sp," << -size << '\n';
    this->fout << "sw ra,12(sp)" << '\n';
    this->fout << "sw s0,8(sp)" << '\n';
    this->fout << "addi s0,sp," << size << '\n';
    for (auto inst : f.InstVec)
    {
        ir::Instruction ins = *inst;
#ifdef DEBUG_BE
        std::string t1 = ins.draw();
        std::cout << t1 << '\n';
#endif
        this->gen_instr(ins);
    }
    this->fout << "lw ra,12(sp)" << '\n';
    this->fout << "lw s0,8(sp)" << '\n';
    this->fout << "addi sp,sp," << size << '\n';
}

std::string rv::rv_inst::draw() const
{
    switch (this->op)
    {
    case rv::rvOPCODE::LI:
        return "li " + toString(this->rd) + ',' + std::to_string(this->imm) + '\n';
    case rv::rvOPCODE::RET:
        return "ret\n";
    case rv::rvOPCODE::LW:
        return "lw  " + toString(this->rd) + "," + std::to_string((int)this->imm) + "(" + toString(this->rs1) + ")" + '\n';
    default:
        return "";
    }
}

void backend::Generator::caller(std::string s)
{
    std::cout << "debug::" << s << std::endl;
    assert(0 && "todo");
}

void backend::Generator::gen_instr(const ir::Instruction &inst)
{
    rv::rv_inst ir_inst;
    rv::rv_inst ld_op1;
    rv::rv_inst ld_op2;
    switch (inst.op)
    {
    case ir::Operator::_return:
        if (inst.op1.name != "null")
        {
            ld_op1 = get_ld_inst(inst.op1, rvREG::t1);
            this->fout << ld_op1.draw();
            fout << "addi a0," << toString(ld_op1.rd) << ",0" << '\n';
        }
        ir_inst.op = rvOPCODE::RET;
        this->fout << ir_inst.draw();
        break;
    case ir::Operator::mov:
    case ir::Operator::def:
        ld_op1 = get_ld_inst(inst.op1, rvREG::t1);
        this->fout << ld_op1.draw();
        // 从寄存器保存到栈空间
        this->fout << "sw " << toString(ld_op1.rd) << ", " << this->find_operand(inst.des) << "(s0)\n";
        break;
    case ir::Operator::add:
        ld_op1 = get_ld_inst(inst.op1, rvREG::t1);
        ld_op2 = get_ld_inst(inst.op2, rvREG::t2);
        this->fout << ld_op1.draw();
        this->fout << ld_op2.draw();
        this->fout << "add " << toString(this->getRd(inst.des)) << ", " << toString(ld_op1.rd) << ", " << toString(ld_op2.rd) << '\n';
        this->fout << "sw " << toString(this->getRd(inst.des)) << ", " << this->find_operand(inst.des) << "(s0)\n";
        break;
    case ir::Operator::call:
        if (inst.op1.name == "global")
            break;
        // todo: finish call func
        this->caller(inst.op1.name);
        break;
    default:
        break;
    }
}