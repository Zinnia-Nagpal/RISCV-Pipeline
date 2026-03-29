#include <stdbool.h>
#include "cache.h"
#include "riscv.h"
#include "types.h"
#include "utils.h"
#include "pipeline.h"
#include "stage_helpers.h"

uint64_t total_cycle_counter = 0;
uint64_t miss_count = 0;
uint64_t hit_count = 0;
uint64_t stall_counter = 0;
uint64_t branch_counter = 0;
uint64_t fwd_exex_counter = 0;
uint64_t fwd_exmem_counter = 0;


simulator_config_t sim_config = {0};

///////////////////////////////////////////////////////////////////////////////

void bootstrap(pipeline_wires_t* pwires_p, pipeline_regs_t* pregs_p, regfile_t* regfile_p)
{
  // PC src must get the same value as the default PC value
  pwires_p->pc_src0 = regfile_p->PC;
}

///////////////////////////
/// STAGE FUNCTIONALITY ///
///////////////////////////

/**
 * STAGE  : stage_fetch
 * output : ifid_reg_t
 **/ 
ifid_reg_t stage_fetch(pipeline_wires_t* pwires_p, regfile_t* regfile_p, Byte* memory_p)
{
    ifid_reg_t ifid_reg = {0};

    // Step 1: Determine the next PC based on control signal
    if (pwires_p->pcsrc) {
        regfile_p->PC = pwires_p->pc_src1;  // Branch/jump target
        pwires_p->pcsrc = false;           // Reset after use
    } else {
        regfile_p->PC = pwires_p->pc_src0;  // Default: sequential PC
    }

    // Step 2: Store the current PC into the pipeline register
    ifid_reg.instr_addr = regfile_p->PC;

    // Step 3: Fetch instruction from memory
    uint32_t addr = ifid_reg.instr_addr;
    uint32_t instruction_bits =
        (uint32_t)memory_p[addr] |
        ((uint32_t)memory_p[addr + 1] << 8) |
        ((uint32_t)memory_p[addr + 2] << 16) |
        ((uint32_t)memory_p[addr + 3] << 24);

    ifid_reg.instr = parse_instruction(instruction_bits);
    ifid_reg.valid = true;

    // Step 4: Set predicted next PC
    ifid_reg.next_pc = addr + 4;
    pwires_p->pc_src0 = ifid_reg.next_pc;

    // Optional debug print
    #ifdef DEBUG_CYCLE
    printf("[IF ]: Instruction [%08x]@[%08x]: ", instruction_bits, ifid_reg.instr_addr);
    decode_instruction(instruction_bits);
    #endif

    return ifid_reg;
}




/**
 * STAGE  : stage_decode
 * output : idex_reg_t
 **/ 
idex_reg_t stage_decode(ifid_reg_t ifid_reg, pipeline_wires_t* pwires_p, regfile_t* regfile_p)
{
    idex_reg_t idex_reg = {0};

    // First, handle the pipeline control signal. If the instruction is
    // invalid (a bubble), we propagate that state and do no other work.
    idex_reg.valid = ifid_reg.valid;
    idex_reg.valid = true;

    // --- 1. Pass-through Values ---
    // Carry the instruction and its address directly to the next stage.
    idex_reg.instr = ifid_reg.instr;
    idex_reg.instr_addr = ifid_reg.instr_addr;

    // --- 2. Decode Register Operands & Read from File ---
    // Extract the register numbers (rs1, rs2, rd) from the instruction
    // and immediately use them to fetch the operand values from the register file.
    idex_reg.rs1 = ifid_reg.instr.rtype.rs1;
    idex_reg.read_rs1 = regfile_p->R[idex_reg.rs1];

    idex_reg.rs2 = ifid_reg.instr.rtype.rs2;
    idex_reg.read_rs2 = regfile_p->R[idex_reg.rs2];
    
    idex_reg.rd = ifid_reg.instr.rtype.rd;

    // --- 3. Generate Control and Immediate Values ---
    // Based on the instruction, generate all necessary control signals and the
    // sign-extended immediate value for the ALU and memory stages.
    idex_reg_t control_signals = gen_control(ifid_reg.instr);
    idex_reg.branch          = control_signals.branch;
    idex_reg.mem_read        = control_signals.mem_read;
    idex_reg.mem_to_reg      = control_signals.mem_to_reg;
    idex_reg.mem_write       = control_signals.mem_write;
    idex_reg.alu_src         = control_signals.alu_src;
    idex_reg.reg_write_en    = control_signals.reg_write_en;
    idex_reg.immediate       = gen_imm(ifid_reg.instr);
    
    #ifdef DEBUG_CYCLE
    printf("[ID ]: Instruction [%08x]@[%08x]: ", idex_reg.instr.bits, idex_reg.instr_addr);
    decode_instruction(idex_reg.instr.bits);
    #endif

    return idex_reg;
}

/**
 * STAGE  : stage_execute
 * output : exmem_reg_t
 **/ 
exmem_reg_t stage_execute(idex_reg_t idex_reg, pipeline_wires_t* pwires_p)
{
        exmem_reg_t exmem_reg = {0};

    // Extract instruction and PC
    Instruction instr = idex_reg.instr;
    Word pc = idex_reg.instr_addr;

    // ALU inputs (no forwarding logic)
    Word alu_input1 = idex_reg.read_rs1;
    Word alu_input2 = idex_reg.alu_src ? idex_reg.immediate : idex_reg.read_rs2;

    // ALU operation
    uint32_t alu_ctrl = gen_alu_control(idex_reg);
    Word alu_result = execute_alu(alu_input1, alu_input2, alu_ctrl);

    // Fill pipeline register
    exmem_reg.instr             = instr;
    exmem_reg.instr_addr        = pc;
    exmem_reg.alu_result        = alu_result;
    exmem_reg.alu_zero          = (alu_result == 0);
    exmem_reg.store_data_memory = idex_reg.read_rs2;  // used for SW
    exmem_reg.rd                = idex_reg.rd;

    // Control signal propagation
    exmem_reg.branch            = idex_reg.branch;
    exmem_reg.mem_read          = idex_reg.mem_read;
    exmem_reg.mem_write         = idex_reg.mem_write;
    exmem_reg.reg_write_en      = idex_reg.reg_write_en;
    exmem_reg.mem_to_reg        = idex_reg.mem_to_reg;

    // Handle branch and jump address (for beq, bne, jal)
    if (instr.opcode == 0x63 || instr.opcode == 0x6f) {
        exmem_reg.branch_address = pc + idex_reg.immediate;
    }

    exmem_reg.valid = true;

#ifdef DEBUG_CYCLE
    printf("[EX ]: Instruction [%08x]@[%08x]: ", instr.bits, pc);
    decode_instruction(instr.bits);
#endif

    return exmem_reg;
}

/**
 * STAGE  : stage_mem
 * output : memwb_reg_t
 **/ 
memwb_reg_t stage_mem(exmem_reg_t exmem_reg, pipeline_wires_t* pwires_p, Byte* memory_p, Cache* cache_p)
{
    memwb_reg_t memwb_reg = {0};

    const Instruction instr = exmem_reg.instr;
    const Word addr = exmem_reg.alu_result;

    // Basic field forwarding
    memwb_reg.instr = instr;
    memwb_reg.instr_addr = exmem_reg.instr_addr;
    memwb_reg.rd = exmem_reg.rd;
    memwb_reg.reg_write_en = exmem_reg.reg_write_en;
    memwb_reg.mem_to_reg = exmem_reg.mem_to_reg;
    memwb_reg.alu_result = addr;

    // Memory read (Load)
    if (exmem_reg.mem_read) {
        Byte loaded_byte;
        switch (instr.itype.funct3) {
            case 0x0:
                loaded_byte = load(memory_p, addr, LENGTH_BYTE);
                memwb_reg.read_data = (sByte)loaded_byte;
                break;
            case 0x1:
                memwb_reg.read_data = (sHalf)load(memory_p, addr, LENGTH_HALF_WORD);
                break;
            case 0x2:
                memwb_reg.read_data = (sWord)load(memory_p, addr, LENGTH_WORD);
                break;
        }
    }

    // Memory write (Store)
    if (exmem_reg.mem_write) {
        const Word data = exmem_reg.store_data_memory;
        const Byte funct3 = instr.stype.funct3;

        if (funct3 == 0x0)
            store(memory_p, addr, LENGTH_BYTE, (Byte)data);
        else if (funct3 == 0x1)
            store(memory_p, addr, LENGTH_HALF_WORD, (Half)data);
        else if (funct3 == 0x2)
            store(memory_p, addr, LENGTH_WORD, data);
    }

    // Branch or Jump
    pwires_p->pcsrc = (instr.opcode == 0x63 || instr.opcode == 0x6f) ? gen_branch(exmem_reg) : pwires_p->pcsrc;
    pwires_p->pc_src1 = (instr.opcode == 0x63 || instr.opcode == 0x6f) ? exmem_reg.branch_address : pwires_p->pc_src1;

    memwb_reg.valid = true;

#ifdef DEBUG_CYCLE
    printf("[MEM]: Instruction [%08x]@[%08x]: ", memwb_reg.instr.bits, memwb_reg.instr_addr);
    decode_instruction(memwb_reg.instr.bits);
#endif

    return memwb_reg;
  }

/**
 * STAGE  : stage_writeback
 * output : nothing - The state of the register file may be changed
 **/ 
void stage_writeback(memwb_reg_t memwb_reg, pipeline_wires_t* pwires_p, regfile_t* regfile_p)
{
  // Pick data to write
  uint32_t data = memwb_reg.mem_to_reg ? memwb_reg.read_data : memwb_reg.alu_result;
  uint32_t rd = memwb_reg.rd;

  // Only write if enabled and not writing to x0
  if (memwb_reg.reg_write_en && rd != 0) {
    regfile_p->R[rd] = data;
  }

  #ifdef DEBUG_CYCLE
  printf("[WB ]: Instruction [%08x]@[%08x]: ", memwb_reg.instr.bits, memwb_reg.instr_addr);
  decode_instruction(memwb_reg.instr.bits);
  #endif
}



///////////////////////////////////////////////////////////////////////////////

/** 
 * excite the pipeline with one clock cycle
 **/
void cycle_pipeline(regfile_t* regfile_p, Byte* memory_p, Cache* cache_p, pipeline_regs_t* pregs_p, pipeline_wires_t* pwires_p, bool* ecall_exit)
{
  #ifdef DEBUG_CYCLE
  printf("v==============");
  printf("Cycle Counter = %5ld", total_cycle_counter);
  printf("==============v\n\n");
  #endif

  // process each stage

  /* Output               |    Stage      |       Inputs  */

  
  pregs_p->ifid_preg.inp  = stage_fetch     (pwires_p, regfile_p, memory_p);

  pregs_p->idex_preg.inp  = stage_decode    (pregs_p->ifid_preg.out, pwires_p, regfile_p);


  // //Check if Forwarding is necessary
  // gen_forward(pregs_p, pwires_p);

  pregs_p->exmem_preg.inp = stage_execute   (pregs_p->idex_preg.out, pwires_p);

  pregs_p->memwb_preg.inp = stage_mem       (pregs_p->exmem_preg.out, pwires_p, memory_p, cache_p);

                            stage_writeback (pregs_p->memwb_preg.out, pwires_p, regfile_p);
    




  // update all the output registers for the next cycle from the input registers in the current cycle
  pregs_p->ifid_preg.out  = pregs_p->ifid_preg.inp;
  pregs_p->idex_preg.out  = pregs_p->idex_preg.inp;
  pregs_p->exmem_preg.out = pregs_p->exmem_preg.inp;
  pregs_p->memwb_preg.out = pregs_p->memwb_preg.inp;
  
  //   //Check for jumps
  // detect_jump(pregs_p, pwires_p);

  // //Check for data hazards
  // detect_hazard(pregs_p,pwires_p);

  /////////////////// NO CHANGES BELOW THIS ARE REQUIRED //////////////////////

  // increment the cycle
  total_cycle_counter++;

  #ifdef DEBUG_REG_TRACE
  print_register_trace(regfile_p);
  #endif

  /**
   * check ecall condition
   * To do this, the value stored in R[10] (a0 or x10) should be 10.
   * Hence, the ecall condition is checked by the existence of following
   * two instructions in sequence:
   * 1. <instr>  x10, <val1>, <val2> 
   * 2. ecall
   * 
   * The first instruction must write the value 10 to x10.
   * The second instruction is the ecall (opcode: 0x73)
   * 
   * The condition checks whether the R[10] value is 10 when the
   * memwb_reg.instr.opcode == 0x73 (to propagate the ecall)
   * 
   * If more functionality on ecall needs to be added, it can be done
   * by adding more conditions on the value of R[10]
   */
  if( (pregs_p->memwb_preg.out.instr.bits == 0x00000073) &&
      (regfile_p->R[10] == 10) )
  {
    *(ecall_exit) = true;
  }
}