/**
 * @file semantic.h
 * @author Yuntao Dai (d1581209858@live.com)
 * @brief
 * @version 0.1
 * @date 2023-01-06
 *
 * a Analyzer should
 * @copyright Copyright (c) 2023
 *
 */

#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ir/ir.h"
#include "front/abstract_syntax_tree.h"

#include <map>
#include <string>
#include <vector>
using std::map;
using std::string;
using std::vector;

namespace frontend
{

    // definition of symbol table entry
    struct STE
    {
        ir::Operand operand;
        vector<int> dimension;
    };

    using map_str_ste = map<string, STE>;
    // definition of scope infomation
    struct ScopeInfo
    {
        int cnt;
        string name;
        map_str_ste table;
    };

    // surpport lib functions
    map<std::string, ir::Function *> *get_lib_funcs();

    // definition of symbol table
    struct SymbolTable
    {
        vector<ScopeInfo> scope_stack;
        map<std::string, ir::Function *> functions;
        void add_scope(Block *);
        void exit_scope();
        string get_scoped_name(string id) const;
        ir::Operand& get_operand(string id);
        STE& get_ste(string id);
    };

    // singleton class
    struct Analyzer
    {
        int tmp_cnt;
        vector<ir::Instruction *> g_init_inst;
        SymbolTable symbol_table;

        /**
         * @brief constructor
         */
        Analyzer();

        // analysis functions
        ir::Program get_ir_program(CompUnit *);

        ir::Program* cur_program;

        // reject copy & assignment
        Analyzer(const Analyzer &) = delete;
        Analyzer &operator=(const Analyzer &) = delete;
        void add_symbol(string id,vector<int>* dimension,Type);
        void analysisTerm(Term*,string&);
        void analysisCompUnit(CompUnit *,ir::Program&);
        void analysisDecl(Decl *,vector<ir::Instruction*>&);
        void analysisConstDecl(ConstDecl *,vector<ir::Instruction*>&);
        void analysisVarDecl(VarDecl *,vector<ir::Instruction*>&);
        void analysisBType(BType *,vector<ir::Instruction*>&);
        void analysisConstDef(ConstDef *,vector<ir::Instruction*>&);
        void analysisVarDef(VarDef *,vector<ir::Instruction*>&);
        void analysisConstExp(ConstExp *,vector<ir::Instruction*>&);
        void analysisConstInitVal(ConstInitVal *,vector<ir::Instruction*>&);
        void analysisFuncDef(FuncDef *,ir::Function&);
        void analysisFuncType(FuncType *,ir::Type&);
        void analysisFuncFParam(FuncFParam *,vector<ir::Instruction*>&);
        void analysisFuncFParams(FuncFParams *,vector<ir::Operand>&);
        void analysisBlock(Block *,vector<ir::Instruction*>&);
        void analysisBlockItem(BlockItem *,vector<ir::Instruction*>&);
        void analysisStmt(Stmt *,vector<ir::Instruction*>&);
        void analysisLVal(LVal *,vector<ir::Instruction*>&);
        void analysisExp(Exp *,vector<ir::Instruction*>&);
        void analysisCond(Cond *,vector<ir::Instruction*>&);
        void analysisNumber(Number *,string&);
        void analysisPrimaryExp(PrimaryExp *,vector<ir::Instruction*>&);
        void analysisUnaryExp(UnaryExp *,vector<ir::Instruction*>&);
        void analysisFuncRParams(FuncRParams *,vector<ir::Instruction*>&);
        void analysisMulExp(MulExp *,vector<ir::Instruction*>&);
        void analysisAddExp(AddExp *,vector<ir::Instruction*>&);
        void analysisRelExp(RelExp *,vector<ir::Instruction*>&);
        void analysisEqExp(EqExp *,vector<ir::Instruction*>&);
        void analysisLAndExp(LAndExp *,vector<ir::Instruction*>&);
        void analysisLOrExp(LOrExp *,vector<ir::Instruction*>&);
        void analysisInitVal(InitVal *,vector<ir::Instruction*>&);
        void analysisUnaryOp(UnaryOp *,vector<ir::Instruction*>&);
    };

} // namespace frontend

#endif