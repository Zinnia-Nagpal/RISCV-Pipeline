#ifndef __PIPELINE_H__
#define __PIPELINE_H__

#include "config.h"
#include "types.h"
#include "cache.h"
#include <stdbool.h>

///////////////////////////////////////////////////////////////////////////////
/// Functionality
///////////////////////////////////////////////////////////////////////////////

extern simulator_config_t sim_config;
extern uint64_t miss_count;
extern uint64_t hit_count;
extern uint64_t total_cycle_counter;
extern uint64_t stall_counter;
extern uint64_t branch_counter;
extern uint64_t fwd_exex_counter;
extern uint64_t fwd_exmem_counter;

///////////////////////////////////////////////////////////////////////////////
/// RISC-V Pipeline Register Types
///////////////////////////////////////////////////////////////////////////////

typedef struct {
    uint32_t pc;            // Program counter for this instruction
    Instruction instr;       // The instruction itself
    uint32_t pc_plus4;      // PC + 4 (for next instruction)
    bool predicted_taken;    // For branch prediction
} ifid_reg_t;

typedef struct {
    uint32_t pc;            // Program counter
    Instruction instr;       // The instruction
    
    // Register file outputs
    uint32_t read_data1;    // RS1 data
    uint32_t read_data2;    // RS2 data
    
    // Immediate generation
    int32_t imm;            // Sign-extended immediate
    
    // Control signals
    bool RegWrite;         // Register write enable
    bool MemToReg;        // Memory to register
    bool MemRead;          // Memory read
    bool MemWrite;         // Memory write
    bool ALUSrc;          // ALU source selector
    bool Branch;           // Branch instruction
    uint8_t ALUOp;        // ALU operation
    bool jump;             // Jump instruction
    
    // Register identifiers
    uint32_t rs1;           // Source register 1
    uint32_t rs2;           // Source register 2
    uint32_t rd;            // Destination register
} idex_reg_t;

typedef struct {
    uint32_t pc;            // Program counter
    Instruction instr;       // The instruction
    
    // ALU results
    uint32_t alu_result;    // ALU computation result
    bool zero;              // ALU zero flag (for branches)
    
    // Memory data
    uint32_t write_data;    // Data to store (for S-type)
    
    // Control signals (propagated)
    bool RegWrite;         // Register write enable
    bool MemToReg;        // Memory to register
    bool MemRead;          // Memory read
    bool MemWrite;         // Memory write
    bool ALUSrc;          // ALU source selector
    bool Branch;           // Branch instruction
    uint8_t ALUOp;        // ALU operation
    bool jump;             // Jump instruction
    
    // Register identifiers
    uint32_t rd;            // Destination register
} exmem_reg_t;

typedef struct {
    uint32_t pc;            // Program counter
    Instruction instr;       // The instruction
    
    // Data sources
    uint32_t alu_result;    // From EX/MEM
    uint32_t read_data;     // From memory (loads)
    
    // Control signals (propagated)
    bool RegWrite;
    bool MemToReg;
    
    // Register identifiers
    uint32_t rd;            // Destination register
} memwb_reg_t;



///////////////////////////////////////////////////////////////////////////////
/// Register types with input and output variants for simulator
///////////////////////////////////////////////////////////////////////////////

typedef struct
{
  ifid_reg_t inp;
  ifid_reg_t out;
}ifid_reg_pair_t;

typedef struct
{
  idex_reg_t inp;
  idex_reg_t out;
}idex_reg_pair_t;

typedef struct
{
  exmem_reg_t inp;
  exmem_reg_t out;
}exmem_reg_pair_t;

typedef struct
{
  memwb_reg_t inp;
  memwb_reg_t out;
}memwb_reg_pair_t;

///////////////////////////////////////////////////////////////////////////////
/// Functional pipeline requirements
///////////////////////////////////////////////////////////////////////////////

typedef struct
{
  ifid_reg_pair_t  ifid_preg;
  idex_reg_pair_t  idex_preg;
  exmem_reg_pair_t exmem_preg;
  memwb_reg_pair_t memwb_preg;
}pipeline_regs_t;

typedef struct
{
  bool      pcsrc;
  uint32_t  pc_src0;
  uint32_t  pc_src1;
  /**
   * Add other fields here
   */
  // Branch status (for performance counting)
  bool branch_taken; // Track if branch was taken
  bool branch_mispredict; // For stats if needed
  
  // Memory interface
  bool mem_busy; // Memory access in progress
  
  // Pipeline control
  bool flush; // Flush pipeline (for jumps/branches)
  
  // Debug/statistics
  uint32_t  inst_retired; // Instructions completed
}pipeline_wires_t;


///////////////////////////////////////////////////////////////////////////////
/// Function definitions for different stages
///////////////////////////////////////////////////////////////////////////////

/**
 * output : ifid_reg_t
 **/ 
ifid_reg_t stage_fetch(pipeline_wires_t* pwires_p, regfile_t* regfile_p, Byte* memory_p);

/**
 * output : idex_reg_t
 **/ 
idex_reg_t stage_decode(ifid_reg_t ifid_reg, pipeline_wires_t* pwires_p, regfile_t* regfile_p);

/**
 * output : exmem_reg_t
 **/ 
exmem_reg_t stage_execute(idex_reg_t idex_reg, pipeline_wires_t* pwires_p);

/**
 * output : memwb_reg_t
 **/ 
memwb_reg_t stage_mem(exmem_reg_t exmem_reg, pipeline_wires_t* pwires_p, Byte* memory, Cache* cache_p);

/**
 * output : write_data
 **/ 
void stage_writeback(memwb_reg_t memwb_reg, pipeline_wires_t* pwires_p, regfile_t* regfile_p);

void cycle_pipeline(regfile_t* regfile_p, Byte* memory_p, Cache* cache_p, pipeline_regs_t* pregs_p, pipeline_wires_t* pwires_p, bool* ecall_exit);

void bootstrap(pipeline_wires_t* pwires_p, pipeline_regs_t* pregs_p, regfile_t* regfile_p);

#endif  // __PIPELINE_H__
