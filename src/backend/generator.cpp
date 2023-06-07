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
        case rvREG::t3:
            return "t3";
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

void backend::Generator::load(ir::Operand oper, rv::rvREG t, ir::Operand offset = ir::Operand("0", ir::Type::IntLiteral))
{

    // process def/mov
    if (offset.name == "0")
    {
        if (oper.type == ir::Type::IntLiteral)
        {
            this->fout << "   li    " << toString(t) << ", " << oper.name << "\n";
            return;
        }
        else
        {
            if (this->find_operand(oper) == -1)
            {
                this->fout << "   la    " << toString(t) << "," << oper.name << "\n";
                this->fout << "   lw    " << toString(t) << ", " << 0 << "("<<toString(t)<<")\n";
                return;
            }
            else
            {
                this->fout << "   lw " << toString(t) << ", " << this->find_operand(oper) << "(s0)\n";
                return;
            }
        }
    }

    if (offset.type == ir::Type::IntLiteral)
    {
        this->fout << "   li    " << toString(rv::rvREG::t3) << ", " << offset.name << "\n";
    }
    else
    {
        this->fout << "   lw    " << toString(rv::rvREG::t3) << ", " << this->find_operand(offset) << "(s0)\n";
    }

    if (isLiteral(oper.type))
    {
        this->mv(rv::rvREG::t3, t);
        return;
    }
    if (this->find_operand(oper) == -1)
    {
        this->fout << "   la    " << toString(rvREG::t1) << "," << oper.name << "\n";
        this->fout << "   slli " << toString(rv::rvREG::t3) << ", " << toString(rvREG::t3) << ", " << 2 << '\n';
        this->fout << "   add " << toString(rv::rvREG::t3) << ", " << toString(rvREG::t3) << ", " << toString(rvREG::t1) << '\n';
        this->fout << "   lw    " << toString(t) << ", " << 0 << "(" << toString(rvREG::t3) << ")\n";
        return;
    }
    else
    {
        this->fout << "   li    " << toString(rv::rvREG::t1) << ", " << this->find_operand(oper) << "\n";
        this->fout << "   slli " << toString(rv::rvREG::t3) << ", " << toString(rvREG::t3) << ", " << 2 << '\n';
        this->fout << "   add " << toString(rv::rvREG::t3) << ", " << toString(rvREG::t3) << ", " << toString(rvREG::t1) << '\n';
        this->fout << "   add " << toString(rv::rvREG::t3) << ", " << toString(rvREG::t3) << ", " << toString(rvREG::s0) << '\n';
        this->fout << "   lw " << toString(t) << ", " << 0 << "(t3)\n";
        return;
    }
    assert(0 && " load fail ");
}

void backend::Generator::mv(rv::rvREG r1, rv::rvREG r2, int other)
{
    this->fout << "   addi " << toString(r2) << "," << toString(r1) << "," << other << '\n';
}

void backend::Generator::sw(rv::rvREG reg, ir::Operand oper, int offset = 0)
{
    this->fout << "   sw " << toString(reg) << ", " << this->find_operand(oper) + offset * 4 << "(s0)\n";
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

backend::Generator::Generator(ir::Program &p, std::ofstream &f) : program(p), fout(f) {}

void backend::Generator::setG(std::string label, int val)
{
    this->fout << "   la    t1," << label << "\n";
    this->mv(rvREG::zero, rvREG::t0, val);
    this->fout << "   sw    t0, 0(t1)"
               << "\n";
}

void backend::Generator::gen()
{

    // gloval V
    for (auto glovalV : this->program.globalVal)
    {
        this->fout << ".data"
                   << "\n";
        this->fout << glovalV.val.name << ":\n";
        this->globalVM[glovalV.val.name].resize(1);
        this->globalVM[glovalV.val.name][0] = "0";

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
                    this->fout << "    .word   " << i << '\n';
                }
                break;
            }
        }
        this->stacks.push_back(backend::stackVarMap());
    }

    std::string header = ".text\n.globl    main  \n";
    this->fout << header;

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
    if (!this->stacks[this->stacks.size() - 1].offset_table.count(oper.name))
    {
        std::string gv;
        for (auto i : this->program.globalVal)
        {
            if (i.val.name == oper.name)
            {
                return -1;
            }
        }
        this->stacks[this->stacks.size() - 1].add_operand(oper);
    }
    return this->stacks[this->stacks.size() - 1].offset_table[oper.name];
}

void backend::Generator::callee(ir::Function &f)
{
    this->stacks.push_back(backend::stackVarMap());
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
    this->fout << "   addi sp,sp," << -size << '\n';
    this->fout << "   sw ra,12(sp)" << '\n';
    this->fout << "   sw s0,8(sp)" << '\n';
    this->fout << "   addi s0,sp," << size << '\n';

    // sw func params

    for (int i = 0; i < f.ParameterList.size(); ++i)
    {
        this->fout << "   sw a" << i << ",   " << this->find_operand(f.ParameterList[i]) << "(s0)\n";
    }

    // label pass
    this->cur_label.clear();
    for (int i = 0; i < f.InstVec.size(); ++i)
    {
        auto inst = f.InstVec[i];
        if (inst->op == ir::Operator::_goto)
        {
            assert(inst->des.type == ir::Type::IntLiteral);
            if (!this->cur_label.count(i + stoi(inst->des.name)))
            {
                this->cur_label[i + stoi(inst->des.name)] = ".LBB0_" + std::to_string(label_number);
                label_number += 1;
            }
        }
    }

    // gen instr pass
    for (int i = 0; i < f.InstVec.size(); ++i)
    {
        if (this->cur_label.count(i))
        {
            this->fout << cur_label[i] << ":\n";
        }
        auto inst = f.InstVec[i];
        this->cur_label_number = i;
        this->gen_instr(inst);
        if (this->ret)
        {
            this->fout << "   lw ra,12(sp)" << '\n';
            this->fout << "   lw s0,8(sp)" << '\n';
            this->fout << "   addi sp,sp," << size << '\n';
            this->fout << "   ret\n";
            this->ret = false;
        }
    }
    this->stacks.pop_back();
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
        this->load(p.name, rv::rvREG::t0);
        // load param to ai
        this->fout << "   addi a" << i << "," << toString(rv::rvREG::t0) << ",0" << '\n';
    }
    this->fout << "   call   " << callinst->op1.name << '\n';
    this->sw(rv::rvREG::a0, callinst->des);
}

void backend::Generator::gen_instr(ir::Instruction *inst)
{
    rv::rv_inst ir_inst;
    switch (inst->op)
    {
    case ir::Operator::_return:
        if (inst->op1.name != "null")
        {
            load(inst->op1, rvREG::t1);
            fout << "    addi a0," << toString(rvREG::t1) << ",0" << '\n';
        }
        this->ret = true;
        break;
    case ir::Operator::mov:
    case ir::Operator::def:
        if (this->find_operand(inst->des) != -1)
        {
            this->load(inst->op1, rvREG::t1);
            // 从寄存器保存到栈空间
            this->sw(rvREG::t1, inst->des);
        }
        else
        {
            assert(inst->op1.type == ir::Type::IntLiteral && "todo:process gval def");
            this->setG(inst->des.name, stoi(inst->op1.name));
        }

        break;

    case ir::Operator::call:
        if (inst->op1.name == "global")
            break;
        // todo: finish call func
        this->caller(dynamic_cast<ir::CallInst *>(inst));
        break;
    case ir::Operator::alloc:
        // process in callee func
        this->stacks[this->stacks.size() - 1].add_operand(inst->des, stoi(inst->op1.name));
        break;
    case ir::Operator::store:
        load(inst->des, rvREG::t1);
        assert(inst->op2.type == ir::Type::IntLiteral && "todo:store non literal");
        this->sw(rv::rvREG::t1, inst->op1, stoi(inst->op2.name));
        break;
    case ir::Operator::load:
        this->load(inst->op1.name, rv::rvREG::t1, inst->op2);
        this->sw(rv::rvREG::t1, inst->des);
        break;
    case ir::Operator::subi:
        this->load(inst->op1, rv::rvREG::t1);
        this->mv(rvREG::t1, rvREG::t0, -stoi(inst->op2.name));
        this->sw(rvREG::t0, inst->des);
        break;
    case ir::Operator::addi:
        this->load(inst->op1, rv::rvREG::t1);
        this->mv(rvREG::t1, rvREG::t0, stoi(inst->op2.name));
        this->sw(rvREG::t0, inst->des);
        break;
    case ir::Operator::add:
        load(inst->op1, rvREG::t1);
        load(inst->op2, rvREG::t2);
        this->fout << "   add " << toString(this->getRd(inst->des)) << ", " << toString(rvREG::t1) << ", " << toString(rvREG::t2) << '\n';
        this->sw(this->getRd(inst->des), inst->des);
        break;
    case ir::Operator::sub:
        load(inst->op1, rvREG::t1);
        load(inst->op2, rvREG::t2);
        this->fout << "   sub " << toString(this->getRd(inst->des)) << ", " << toString(rvREG::t1) << ", " << toString(rvREG::t2) << '\n';
        this->sw(this->getRd(inst->des), inst->des);
        break;
    case ir::Operator::div:
        load(inst->op1, rvREG::t1);
        load(inst->op2, rvREG::t2);
        this->fout << "   div " << toString(this->getRd(inst->des)) << ", " << toString(rvREG::t1) << ", " << toString(rvREG::t2) << '\n';
        this->sw(this->getRd(inst->des), inst->des);
        break;
    case ir::Operator::mul:
        load(inst->op1, rvREG::t1);
        load(inst->op2, rvREG::t2);
        this->fout << "   mul " << toString(this->getRd(inst->des)) << ", " << toString(rvREG::t1) << ", " << toString(rvREG::t2) << '\n';
        this->sw(this->getRd(inst->des), inst->des);
        break;
    case ir::Operator::mod:
        load(inst->op1, rvREG::t1);
        load(inst->op2, rvREG::t2);
        this->fout << "   rem " << toString(this->getRd(inst->des)) << ", " << toString(rvREG::t1) << ", " << toString(rvREG::t2) << '\n';
        this->sw(this->getRd(inst->des), inst->des);
        break;

    case ir::Operator::lss:
        load(inst->op1, rvREG::t1);
        load(inst->op2, rvREG::t2);
        this->fout << "   slt " << toString(this->getRd(inst->des)) << ", " << toString(rvREG::t1) << ", " << toString(rvREG::t2) << '\n';
        this->sw(this->getRd(inst->des), inst->des);
        break;

    case ir::Operator::eq:
        load(inst->op1, rvREG::t1);
        load(inst->op2, rvREG::t2);
        this->fout << "   sub " << toString(this->getRd(inst->des)) << ", " << toString(rvREG::t1) << ", " << toString(rvREG::t2) << '\n';
        this->fout << "   seqz " << toString(this->getRd(inst->des)) << ", " << toString(this->getRd(inst->des)) << '\n';
        this->sw(this->getRd(inst->des), inst->des);
        break;
    case ir::Operator::gtr:
        load(inst->op1, rvREG::t1);
        load(inst->op2, rvREG::t2);
        this->fout << "   sgt " << toString(this->getRd(inst->des)) << ", " << toString(rvREG::t1) << ", " << toString(rvREG::t2) << '\n';
        this->sw(this->getRd(inst->des), inst->des);
        break;
    case ir::Operator::geq:
        load(inst->op1, rvREG::t1);
        load(inst->op2, rvREG::t2);
        this->fout << "   sub " << toString(rvREG::t3) << ", " << toString(rvREG::t1) << ", " << toString(rvREG::t2) << '\n';
        this->fout << "   seqz " << toString(rvREG::t3) << ", " << toString(rvREG::t3) << '\n';
        this->fout << "   sgt " << toString(rvREG::t0) << ", " << toString(rvREG::t1) << ", " << toString(rvREG::t2) << '\n';
        this->fout << "   or " << toString(rvREG::t0) << ", " << toString(rvREG::t0) << ", " << toString(rvREG::t3) << '\n';
        this->sw(rvREG::t0, inst->des);
        break;
    case ir::Operator::leq:
        load(inst->op1, rvREG::t1);
        load(inst->op2, rvREG::t2);
        this->fout << "   sub " << toString(rvREG::t3) << ", " << toString(rvREG::t1) << ", " << toString(rvREG::t2) << '\n';
        this->fout << "   seqz " << toString(rvREG::t3) << ", " << toString(rvREG::t3) << '\n';
        this->fout << "   slt " << toString(rvREG::t0) << ", " << toString(rvREG::t1) << ", " << toString(rvREG::t2) << '\n';
        this->fout << "   or " << toString(rvREG::t0) << ", " << toString(rvREG::t0) << ", " << toString(rvREG::t3) << '\n';
        this->sw(rvREG::t0, inst->des);
        break;

    case ir::Operator::neq:
        load(inst->op1, rvREG::t1);
        load(inst->op2, rvREG::t2);
        this->fout << "   sub " << toString(this->getRd(inst->des)) << ", " << toString(rvREG::t1) << ", " << toString(rvREG::t2) << '\n';
        this->fout << "   snez " << toString(this->getRd(inst->des)) << ", " << toString(this->getRd(inst->des)) << '\n';
        this->sw(this->getRd(inst->des), inst->des);
        break;
    case ir::Operator::_not:
        load(inst->op1, rvREG::t1);
        this->fout << "   seqz " << toString(this->getRd(inst->des)) << ", " << toString(rvREG::t1) << '\n';
        this->sw(this->getRd(inst->des), inst->des);
        break;
    case ir::Operator::_and:
        load(inst->op1, rvREG::t1);
        load(inst->op2, rvREG::t2);
        this->fout << "   and " << toString(this->getRd(inst->des)) << ", " << toString(rvREG::t1) << ", " << toString(rvREG::t2) << '\n';
        this->sw(this->getRd(inst->des), inst->des);
        break;
    case ir::Operator::_or:
        load(inst->op1, rvREG::t1);
        load(inst->op2, rvREG::t2);
        this->fout << "   or " << toString(this->getRd(inst->des)) << ", " << toString(rvREG::t1) << ", " << toString(rvREG::t2) << '\n';
        this->sw(this->getRd(inst->des), inst->des);
        break;
    case ir::Operator::cvt_f2i:
    case ir::Operator::cvt_i2f:
    case ir::Operator::_goto:
        if (inst->op1.type != ir::Type::Int)
        {
            this->fout << "   j " << cur_label[this->cur_label_number + std::stoi(inst->des.name)] << '\n';
        }
        else
        {
            load(inst->op1, rvREG::t1);
            this->fout << "   bnez " << toString(rvREG::t1) << ", " << cur_label[this->cur_label_number + std::stoi(inst->des.name)] << '\n';
        }
        break;
    case ir::Operator::__unuse__:
        break;
    case ir::Operator::fleq:
    case ir::Operator::flss:
    case ir::Operator::fgeq:
    case ir::Operator::fneq:
    case ir::Operator::fdef:
    case ir::Operator::fmov:
    case ir::Operator::fadd:
    case ir::Operator::fsub:
    case ir::Operator::fmul:
    case ir::Operator::fdiv:
    case ir::Operator::fgtr:
    default:
        assert(0 && "not supported ir type");
        break;
    }
}