#ifndef GENERARATOR_H
#define GENERARATOR_H

#include "ir/ir.h"
#include "backend/rv_def.h"
#include "backend/rv_inst_impl.h"

#include<map>
#include<string>
#include<vector>
#include<fstream>

namespace backend {

// it is a map bewteen variable and its mem addr, the mem addr of a local variable can be identified by ($sp + off)
struct stackVarMap {
    int cur_offset=0;
    std::map<std::string, int> offset_table;

    /**
     * @brief find the addr of a ir::Operand
     * @return the offset
    */
    int find_operand(ir::Operand);

    /**
     * @brief add a ir::Operand into current map, alloc space for this variable in memory 
     * @param[in] size: the space needed(in byte)
     * @return the offset
    */
    int add_operand(ir::Operand, uint32_t size);
};


struct Generator {
    const ir::Program& program;         // the program to gen
    std::ofstream& fout;                 // output file
    stackVarMap stackMap;
    Generator(ir::Program&, std::ofstream&);

    void callee(ir::Function& f);

    void caller(std::string funcName);

    void declareGlobalV(ir::Operand);

    // reg allocate api
    rv::rvREG getRd(ir::Operand);
    rv::rvFREG fgetRd(ir::Operand);
    rv::rvREG getRs1(ir::Operand);
    rv::rvREG getRs2(ir::Operand);
    rv::rvFREG fgetRs1(ir::Operand);
    rv::rvFREG fgetRs2(ir::Operand);

    // generate wrapper function
    void gen();
    void gen_instr(const ir::Instruction&);
};



} // namespace backend


#endif