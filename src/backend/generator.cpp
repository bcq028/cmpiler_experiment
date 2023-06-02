#include "backend/generator.h"

#include <assert.h>
#include <iostream>
#define TODO assert(0 && "todo");
using namespace rv;

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
rv::rv_inst get_ld_inst(const ir::Operand &oper,rv::rvREG reg)
{
    rv::rv_inst inst;
    if (oper.type == ir::Type::IntLiteral)
    {
        inst.op = rv::rvOPCODE::LI;
        inst.rd = reg;
        inst.imm = stoi(oper.name);
    }
    return inst;
}

backend::Generator::Generator(ir::Program &p, std::ofstream &f) : program(p), fout(f) {}

void backend::Generator::gen()
{
    std::string header = ".file    \"00_main.c\"   \n\
    .option nopic   \n\
    .text     \n\
    .align    1  \n\
    .globl    main  \n\
    .type    main, @function\n";
    this->fout << header;
    for (auto func : this->program.functions)
    {
        if (func.name == "global")
            continue;
        this->fout << func.name << ":\n";
        this->callee(func);
    }
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
                        }
                    }
                }
                break;
            }
        }
    }
}

int backend::stackVarMap::add_operand(ir::Operand oper, uint32_t size = 4)
{
    this->offset_table[oper.name] = this->cur_offset;
    this->cur_offset -= size;
    return this->offset_table[oper.name];
}

int backend::stackVarMap::find_operand(ir::Operand oper)
{
    if (!this->offset_table.count(oper.name))
    {
        this->add_operand(oper);
    }
    return this->offset_table[oper.name];
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
        ir::Instruction ins=*inst;
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
    default:
        return "";
    }
}

void backend::Generator::caller(std::string s){
    std::cout<<"debug::"<<s<<std::endl;
    assert(0 && "todo");
}

void backend::Generator::gen_instr(const ir::Instruction &inst)
{
    auto ld_op1 = get_ld_inst(inst.op1,rvREG::t1);
    auto ld_op2 = get_ld_inst(inst.op2,rvREG::t2);
    rv::rv_inst ir_inst;
    switch (inst.op)
    {
    case ir::Operator::_return:
        if (inst.op1.name != "null")
        {
            this->fout << ld_op1.draw();
            fout << "addi a0," << toString(ld_op1.rd) << ",0" << '\n';
        }
        ir_inst.op = rvOPCODE::RET;
        this->fout << ir_inst.draw();
        break;
    case ir::Operator::mov:
    case ir::Operator::def:
        this->fout << ld_op1.draw();
        this->fout << "sw " << toString(ld_op1.rd) << ", " << this->stackMap.find_operand(inst.op1) << "(s0)\n";
        break;
    case ir::Operator::add:
            this->fout << ld_op1.draw();
            this->fout << ld_op2.draw();
            this->fout<< "add "<<toString(this->getRd(inst.des))<<", "<<toString(ld_op1.rd)<<toString(ld_op2.rd)<<'\n';
            break;
    case ir::Operator::call:
            if(inst.des.name=="global") break;
            //todo: finish call func
            this->caller(inst.des.name);
            break;
    default:
        break;
    }
}