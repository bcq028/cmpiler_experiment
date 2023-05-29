#include "backend/generator.h"

#include <assert.h>

#define TODO assert(0 && "todo");
using namespace rv;

// load oper to register
rv::rv_inst get_ld_inst(const ir::Operand &oper)
{
    TODO
}

rv::rvREG getRd(ir::Operand)
{
    return rvREG::t0;
}
rv::rvFREG fgetRd(ir::Operand)
{
    return rvFREG::ft0;
}
rv::rvREG getRs1(ir::Operand)
{
    return rvREG::t1;
}
rv::rvREG getRs2(ir::Operand)
{
    return rvREG::t2;
}
rv::rvFREG fgetRs1(ir::Operand)
{
    return rvFREG::ft1;
}
rv::rvFREG fgetRs2(ir::Operand)
{
    return rvFREG::ft2;
}

backend::Generator::Generator(ir::Program &p, std::ofstream &f) : program(p), fout(f) {}

void backend::Generator::gen()
{
    TODO
}

int backend::stackVarMap::add_operand(ir::Operand oper, uint32_t size = 4)
{
    this->offset_table[oper.name] = this->cur_offset;
    this->cur_offset += size;
}

int backend::stackVarMap::find_operand(ir::Operand oper)
{
    return this->offset_table[oper.name];
}

void backend::Generator::callee(ir::Function &f)
{
    this->fout << "addi sp,sp," << -1 << '\n';
    TODO
}

std::string rv::rv_inst::draw() const
{
    TODO
}

void backend::Generator::gen_instr(const ir::Instruction &inst)
{
    auto ld_op1 = get_ld_inst(inst.op1);
    auto ld_op2 = get_ld_inst(inst.op2);
    rv::rv_inst ir_inst;
    switch (inst.op)
    {
    case ir::Operator::add:
        ir_inst.op = rvOPCODE::ADD;
        ir_inst.rd = rvREG::t2;
        ir_inst.rs1 = rvREG::t0;
        ir_inst.rs2 = rvREG::t1;
        break;

    default:
        break;
    }
    this->fout << ir_inst.draw();
    TODO
}