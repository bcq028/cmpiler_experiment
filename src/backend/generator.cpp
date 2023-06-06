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

void backend::Generator::load(ir::Operand oper, int offset, rv::rvREG t)
{
    if (this->find_operand(oper) == -1)
    {
        this->fout << "lui    t0,\%hi(" << oper.name << ")\n";
        this->fout << "addi    t0,t0,\%lo(" << oper.name << ")\n";
        this->fout << "lw    t0," << offset * 4 << "(t0)\n";
        this->mv(rv::rvREG::t0, t);
    }
    else
    {
        this->fout << "lw " << toString(t) << ", " << this->find_operand(oper) + offset * 4 << "(s0)\n";
    }
}

void backend::Generator::mv(rv::rvREG r1, rv::rvREG r2)
{
    this->fout << "addi " << toString(r2) << "," << toString(r1) << ",0" << '\n';
}

void backend::Generator::sw(rv::rvREG reg, ir::Operand oper, int offset = 0)
{
    this->fout << "sw " << toString(reg) << ", " << this->find_operand(oper) + offset * 4 << "(s0)\n";
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
rv::rv_inst backend::Generator::get_ld_inst(const ir::Operand &oper, rv::rvREG reg, int ind = 0)
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
            inst.imm = stoi(this->globalVM[oper.name][ind]);
        }
    }
    this->fout << inst.draw();
    return inst;
}

backend::Generator::Generator(ir::Program &p, std::ofstream &f) : program(p), fout(f) {}

void backend::Generator::setG(std::string label, int val)
{
    this->fout << "lui    t0,\%hi(" << label << ")\n";
    this->fout << "li    t1," << val << '\n';
    this->fout << "sw    t1,\%lo(" << label << ")(t0)\n";
}

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
        this->globalVM[glovalV.val.name].resize(1);
        this->globalVM[glovalV.val.name][0]="0";

        for (auto func : this->program.functions)
        {
            if (func.name == "global")
            {
                for (auto inst : func.InstVec)
                {
                    if (inst->op == ir::Operator::alloc)
                    {
                        assert(inst->op1.type == ir::Type::IntLiteral && "todo:alloc var");
                        this->globalVM[inst->des.name].resize(stoi(inst->op1.name));
                    }
                    if (inst->op == ir::Operator::mov)
                    {
                        if (inst->des.name == glovalV.val.name)
                        {
                            this->globalVM[glovalV.val.name].resize(1);
                            if (inst->op1.type == ir::Type::IntLiteral)
                            {
                                this->globalVM[glovalV.val.name][0] = inst->op1.name;
                            }
                        }
                    }
                    else if (inst->op == ir::Operator::store)
                    {
                        if (inst->op1.name == glovalV.val.name)
                        {
                            if (inst->op2.type == ir::Type::IntLiteral)
                            {
                                this->globalVM[glovalV.val.name][stoi(inst->op2.name)] = inst->des.name;
                            }
                        }
                    }
                }
                for (auto i : this->globalVM[glovalV.val.name])
                {
                    this->fout << "   .word   " << i << '\n';
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

int backend::stackVarMap::add_operand(ir::Operand oper, uint32_t size = 1)
{
    this->cur_offset -= size * 4;
    this->offset_table[oper.name] = this->cur_offset;
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
    int sz = f.InstVec.size();
    for (auto t : f.InstVec)
    {
        if (t->op == ir::Operator::alloc)
        {
            assert(t->op1.type == ir::Type::IntLiteral && "todo:alloc var");
            sz += stoi(t->op1.name);
        }
    }
    int size = f.InstVec.size() * 4 + 12;
    this->fout << "addi sp,sp," << -size << '\n';
    this->fout << "sw ra,12(sp)" << '\n';
    this->fout << "sw s0,8(sp)" << '\n';
    this->fout << "addi s0,sp," << size << '\n';

    // sw func params

    for (int i = 0; i < f.ParameterList.size(); ++i)
    {
        this->fout << "sw a" << i << ",   " << this->find_operand(f.ParameterList[i]) << "(s0)\n";
    }

    for (auto inst : f.InstVec)
    {
#ifdef DEBUG_BE
        std::string t1 = inst->draw();
        std::cout << t1 << '\n';
#endif
        this->gen_instr(inst);
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

void backend::Generator::caller(ir::CallInst *callinst)
{
    for (int i = 0; i < callinst->argumentList.size(); ++i)
    {
        ir::Operand p = callinst->argumentList[i];
        this->load(p.name, 0, rv::rvREG::t0);
        // load param to ai
        this->fout << "addi a" << i << "," << toString(rv::rvREG::t0) << ",0" << '\n';
    }
    this->fout << "call   " << callinst->op1.name << '\n';
    this->sw(rv::rvREG::a0, callinst->des);
}

void backend::Generator::gen_instr(ir::Instruction *inst)
{
    rv::rv_inst ir_inst;
    rv::rv_inst ld_op1;
    rv::rv_inst ld_op2;
    switch (inst->op)
    {
    case ir::Operator::_return:
        if (inst->op1.name != "null")
        {
            ld_op1 = get_ld_inst(inst->op1, rvREG::t1);
            fout << "addi a0," << toString(ld_op1.rd) << ",0" << '\n';
        }
        ir_inst.op = rvOPCODE::RET;
        this->fout << ir_inst.draw();
        break;
    case ir::Operator::mov:
    case ir::Operator::def:
        if (this->find_operand(inst->des))
        {
            ld_op1 = get_ld_inst(inst->op1, rvREG::t1);
            // 从寄存器保存到栈空间
            this->sw(ld_op1.rd, inst->des);
        }
        else
        {
            assert(inst->op1.type == ir::Type::IntLiteral && "todo:process gval def");
            this->setG(inst->des.name, stoi(inst->op1.name));
        }

        break;
    case ir::Operator::add:
        ld_op1 = get_ld_inst(inst->op1, rvREG::t1);
        ld_op2 = get_ld_inst(inst->op2, rvREG::t2);
        this->fout << "add " << toString(this->getRd(inst->des)) << ", " << toString(ld_op1.rd) << ", " << toString(ld_op2.rd) << '\n';
        this->sw(this->getRd(inst->des), inst->des);
        break;
    case ir::Operator::call:
        if (inst->op1.name == "global")
            break;
        // todo: finish call func
        this->caller(dynamic_cast<ir::CallInst *>(inst));
        break;
    case ir::Operator::alloc:
        // process in callee func
        this->stackMap.add_operand(inst->des, stoi(inst->op1.name));
        break;
    case ir::Operator::store:
        ld_op1 = get_ld_inst(inst->des, rvREG::t1);
        assert(inst->op2.type == ir::Type::IntLiteral && "todo:store non literal");
        this->sw(rv::rvREG::t1, inst->op1, stoi(inst->op2.name));
        break;
    case ir::Operator::load:
        assert(inst->op2.type == ir::Type::IntLiteral && "todo:store non literal");
        this->load(inst->op1.name, stoi(inst->op2.name), rv::rvREG::t1);
        this->sw(rv::rvREG::t0, inst->des);
        break;
    case ir::Operator::subi:
        ld_op1 = get_ld_inst(inst->des, rvREG::t1);
        fout << "addi a0," << toString(ld_op1.rd) << "," << inst->op2.name << '\n';
        break;
    default:
        assert(0 && "not supported ir type");
        break;
    }
}