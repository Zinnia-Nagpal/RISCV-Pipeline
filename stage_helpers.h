#ifndef _STAGE_HELPERS_H_
#define _STAGE_HELPERS_H_

#include <stdio.h>
#include "utils.h"
#include "pipeline.h"

/// EXECUTE STAGE HELPERS ///

/**
 * input  : idex_reg_t
 * output : uint32_t alu_control signal
 **/
uint32_t gen_alu_control(idex_reg_t idex_reg)
{
    //need to support more instructions
    uint32_t alu_control = 0;
    uint8_t alu_op = idex_reg.alu_op;
    Instruction instr = idex_reg.instr;
    switch (alu_op) {
        case 0:
            alu_control = 2; // add (e.g., for lw/sw/auipc/lui)
            break;

        case 1:
            alu_control = 6; // subtract (e.g., for beq)
            break;

        case 2:
            if (instr.opcode == 0x33) { // R-type
                switch (instr.rtype.funct3) {
                    case 0x0:
                        switch (instr.rtype.funct7) {
                            case 0x00: alu_control = 2; break;   // add
                            case 0x20: alu_control = 6; break;   // sub
                            case 0x01: alu_control = 9; break;   // mul
                        }
                        break;
                    case 0x1:
                        switch (instr.rtype.funct7) {
                            case 0x00: alu_control = 10; break;  // sll
                            case 0x01: alu_control = 11; break;  // mulh
                        }
                        break;
                    case 0x2: alu_control = 12; break; // slt
                    case 0x3: alu_control = 13; break; // sltu
                    case 0x4:
                        switch (instr.rtype.funct7) {
                            case 0x00: alu_control = 3; break;   // xor
                            case 0x01: alu_control = 14; break;  // div
                        }
                        break;
                    case 0x5:
                        switch (instr.rtype.funct7) {
                            case 0x00: alu_control = 4; break;   // srl
                            case 0x20: alu_control = 5; break;   // sra
                        }
                        break;
                    case 0x6:
                        switch (instr.rtype.funct7) {
                            case 0x00: alu_control = 1; break;   // or
                            case 0x01: alu_control = 15; break;  // rem
                        }
                        break;
                    case 0x7: alu_control = 0; break; // and
                }
            }
            else if (instr.opcode == 0x13) { // I-type ALU
                switch (instr.itype.funct3) {
                    case 0x0: alu_control = 2; break;       // addi
                    case 0x1: alu_control = 10; break;      // slli
                    case 0x2: alu_control = 12; break;      // slti
                    case 0x3: alu_control = 13; break;      // sltiu
                    case 0x4: alu_control = 3; break;       // xori
                   case 0x5: {
                        uint32_t imm_hi = (instr.itype.imm >> 5) & 0x7F;
                        if (imm_hi == 0x00)          alu_control = 4; // srli
                        else if (imm_hi == 0x20) alu_control = 5; // srai
                        break;
                    }   
                    case 0x6: alu_control = 1; break;       // ori
                    case 0x7: alu_control = 0; break;       // andi
                }
            }
            else if (instr.opcode == 0x63) { // B-type
                switch (instr.sbtype.funct3) {
                    case 0x1: alu_control = 6; break; // BNE → subtract
                    // Add more branch types like BLT/BGE here
                }
            }
            break;
        default:
            alu_control = 2; // default to ADD
            break;
    }

    return alu_control;
}


/**
 * input  : alu_inp1, alu_inp2, alu_control
 * output : uint32_t alu_result
 **/
uint32_t execute_alu(uint32_t alu_inp1, uint32_t alu_inp2, uint32_t alu_control)
{
    uint32_t result;

    switch (alu_control) {
        // Bitwise operations (unsigned logic)
        case 0:  result = alu_inp1 & alu_inp2; break;                     // AND
        case 1:  result = alu_inp1 | alu_inp2; break;                     // OR
        case 3:  result = alu_inp1 ^ alu_inp2; break;                     // XOR
        case 4:  result = alu_inp1 >> (alu_inp2 & 0x1F); break;           // SRL (logical)
        case 10: result = alu_inp1 << (alu_inp2 & 0x1F); break;           // SLL

        // Signed arithmetic
        case 2:  result = (uint32_t)((int32_t)alu_inp1 + (int32_t)alu_inp2); break; // ADD
        case 6:  result = (uint32_t)((int32_t)alu_inp1 - (int32_t)alu_inp2); break; // SUB
        case 5:  result = (uint32_t)(((int32_t)alu_inp1) >> (alu_inp2)); break; // SRA
        case 7:  result = ((int32_t)alu_inp1 < (int32_t)alu_inp2) ? 1 : 0; break;      // SLT
        case 12: result = ((int32_t)alu_inp1 < (int32_t)alu_inp2) ? 1 : 0; break;      // SLT (alias)

        // Unsigned comparison
        case 13: result = (alu_inp1 < alu_inp2) ? 1 : 0; break;           // SLTU

        // Multiplication
        case 9:  result = alu_inp1 * alu_inp2; break;                    // MUL
        case 11: result = (uint32_t)(((int64_t)(int32_t)alu_inp1 * (int64_t)(int32_t)alu_inp2) >> 32); break; // MULH

        // Signed division and remainder
        case 14: result = (alu_inp2 != 0) ? (uint32_t)((int32_t)alu_inp1 / (int32_t)alu_inp2) : 0xFFFFFFFF; break; // DIV
        case 15: result = (alu_inp2 != 0) ? (uint32_t)((int32_t)alu_inp1 % (int32_t)alu_inp2) : 0xFFFFFFFF; break; // REM

        // Default case for invalid ALU control
        default: result = 0xBADCAFFE; break;
    }

    return result;
}
/// DECODE STAGE HELPERS ///

/**
 * input  : Instruction
 * output : idex_reg_t
 **/
int32_t gen_imm(Instruction instruction)
{
    int imm_val = 0;

    switch (instruction.opcode) {
        case 0x63: // SB-type (branches)
            imm_val = get_branch_offset(instruction);  // already sign-extended in helper
            break;

        case 0x6F: // UJ-type (JAL)
            imm_val = get_jump_offset(instruction);    // already sign-extended in helper
            break;

        case 0x23: // S-type (stores)
            imm_val = get_store_offset(instruction);   // already sign-extended in helper
            break;

        case 0x13: // I-type (immediate ALU)
        case 0x03: // I-type (loads)
        case 0x67: // I-type (JALR)
        case 0x73: // I-type (ECALL)
            imm_val = sign_extend_number(instruction.itype.imm, 12);
            break;

        case 0x37: // U-type (LUI)
        case 0x17: // U-type (AUIPC)
            imm_val = instruction.utype.imm << 12; // shift then sign-extend
            break;

        default: // R-type and unknowns — no immediate
            break;
    };

    return imm_val;
}

/**
 * generates all the control logic that flows around in the pipeline
 * input  : Instruction
 * output : idex_reg_t
 **/
idex_reg_t gen_control(Instruction instr) {
    idex_reg_t idex_reg = {0}; 

    switch (instr.opcode) {
        case 0x33:  // R-format
            idex_reg.alu_src     = false;
            idex_reg.mem_to_reg  = false;
            idex_reg.reg_write   = true;
            idex_reg.mem_read    = false;
            idex_reg.mem_write   = false;
            idex_reg.branch      = false;
            idex_reg.alu_op      = 2;  // ALUOp = 10b
            break;

        case 0x03:  // lw
            idex_reg.alu_src     = true;
            idex_reg.mem_to_reg  = true;
            idex_reg.reg_write   = true;
            idex_reg.mem_read    = true;
            idex_reg.mem_write   = false;
            idex_reg.branch      = false;
            idex_reg.alu_op      = 0;  // ALUOp = 00b
            break;

        case 0x23:  // sw
            idex_reg.alu_src     = true;
            idex_reg.mem_to_reg  = false;  // don't care, safe as false
            idex_reg.reg_write   = false;
            idex_reg.mem_read    = false;
            idex_reg.mem_write   = true;
            idex_reg.branch      = false;
            idex_reg.alu_op      = 0;  // ALUOp = 00b
            break;

        case 0x63:  
          if(instr.sbtype.funct3 == 0x0){ // beq 
            idex_reg.alu_src     = false;
            idex_reg.mem_to_reg  = false;  // don't care, safe as false
            idex_reg.reg_write   = false;
            idex_reg.mem_read    = false;
            idex_reg.mem_write   = false;
            idex_reg.branch      = true;
            idex_reg.alu_op      = 1;  // ALUOp = 01b = SUB in ALU
            break;
          } else {
            idex_reg.alu_src     = false;
            idex_reg.mem_to_reg  = false;  // don't care, safe as false
            idex_reg.reg_write   = false;
            idex_reg.mem_read    = false;
            idex_reg.mem_write   = false;
            idex_reg.branch      = true;
            idex_reg.alu_op      = 2;  // ALUOp = 01b -> need to check funct3 because sub doesn't work for all branch types
            break;
          }
        
        case 0x6F: //jal case
          idex_reg.alu_src = false;
          idex_reg.mem_to_reg = false;
          idex_reg.reg_write   = true;
          idex_reg.mem_read    = false;
          idex_reg.mem_write   = false;
          idex_reg.branch = false;
          idex_reg.alu_op  = 0; 
          break;
        
        case 0x37: //lui case
          idex_reg.alu_src = true;
          idex_reg.mem_to_reg = false;
          idex_reg.reg_write   = true;
          idex_reg.mem_read    = false;
          idex_reg.mem_write   = false;
          idex_reg.alu_op  = 0; //-> imm will already be shifted by 12 so we just need to add with r0 (which is always 0)
          break;    

        case 0x13:  // I-type ALU (e.g., addi)
            idex_reg.alu_src     = true;
            idex_reg.mem_to_reg  = false;
            idex_reg.reg_write   = true;
            idex_reg.mem_read    = false;
            idex_reg.mem_write   = false;
            idex_reg.branch      = false;
            idex_reg.alu_op      = 2;  // ALUOp = 10b -> need to check funct3 and find specfic operation for the alu operation
            break;
        case 0x73: // ECALL
            idex_reg.alu_src     = false;
            idex_reg.mem_to_reg  = false;
            idex_reg.reg_write   = false;
            idex_reg.mem_read    = false;
            idex_reg.mem_write   = false;
            idex_reg.branch      = false;
            idex_reg.alu_op      = 0; // Doesn't matter — not used
            break;

        default:
            // All fields stay zero (NOP behavior)
            break;
    }

    return idex_reg;
}
/// MEMORY STAGE HELPERS ///

/**
 * evaluates whether a branch must be taken
 * input  : <open to implementation>
 * output : bool
 **/
bool gen_branch(exmem_reg_t reg)
{
  if (reg.instr.opcode == 0x63) {
    switch (reg.instr.sbtype.funct3) {
      case 0x0: // BEQ
        return reg.branch && reg.zero;
      case 0x1: // BNE
        return reg.branch && !reg.zero;
      default:
        return false;
    }
  }

  // JAL (0x6F)
  if (reg.instr.opcode == 0x6F) return true;

  return false;
}


/// PIPELINE FEATURES ///

/**
 * Task   : Sets the pipeline wires for the forwarding unit's control signals
 *           based on the pipeline register values.
 * input  : pipeline_regs_t*, pipeline_wires_t*
 * output : None
*/
void gen_forward(pipeline_regs_t* pregs_p, pipeline_wires_t* pwires_p)
{
    /*
    case 00 (0): i just need to use the normal register values from idex
    case 01 (1): i need to use value generated from previous instruction in the writeback stage
    case 10 (2): i need to use value generated from previous instruction in execute stage
    */
    (void)pregs_p;
    (void)pwires_p;
// // default values: /////////
    pwires_p->forwardA  = 0;
    pwires_p->forwardB = 0;
// ///////////////////////////

// things that the forwarding unit needs to generate control signals:
    uint8_t rs1_idex = pregs_p->idex_preg.out.rs1;
    uint8_t rs2_idex = pregs_p->idex_preg.out.rs2;
    uint32_t rd_exmem = pregs_p->exmem_preg.out.rd;
    uint32_t rd_memwb = pregs_p->memwb_preg.out.rd;
/////////////////////////////////////////////////////////////////////

//Resolving EX Hazard://///////////////////////////////////////////////////////////
// If the EX/MEM stage is writing to a register (reg_write == true) and its destination (rd) matches either rs1 or rs2 of the current ID/EX instruction:
// Sets forwardA or forwardB to 2 (forward from EX/MEM).
// Sets exmem_forward_val to the ALU result of EX/MEM stage.
// Increments fwd_exex_counter.
if(pregs_p->exmem_preg.out.r== rs1_ieg_write && (rd_exmem != 0) && (rd_exmem dex)){
    pwires_p->forwardA  = 2;
    pwires_p->exmem_forward_val = pregs_p->exmem_preg.out.result;
    fwd_exex_counter++;
        #ifdef DEBUG_CYCLE
        printf("[FWD]: Resolving EX hazard on rs1: x%d\n", rs1_idex);
        #endif
} 
// If MEM/WB is writing to a register and its rd matches the current instruction’s rs1 or rs2, and there is no EX hazard for that same source:
// Sets forwardA or forwardB to 1 (forward from MEM/WB).
// Increments fwd_exmem_counter
if ((pregs_p->exmem_preg.out.reg_write && (rd_exmem != 0) && (rd_exmem == rs2_idex))){
    pwires_p->forwardB = 2;
    pwires_p->exmem_forward_val = pregs_p->exmem_preg.out.result;
    fwd_exex_counter++;
           #ifdef DEBUG_CYCLE
        printf("[FWD]: Resolving EX hazard on rs2: x%d\n", rs2_idex);
        #endif
}
///////////////////////////////////////////////////////////////////////////////////////

//Resolving MEM Hazard://///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
if(pregs_p->memwb_preg.out.reg_write && (rd_memwb !=0) && !(pregs_p->exmem_preg.out.reg_write && (rd_exmem!=0)&& (rd_exmem == rs1_idex)) && (rd_memwb == rs1_idex)){
    pwires_p->forwardA  = 1;
      fwd_exmem_counter++;
    #ifdef DEBUG_CYCLE
      printf("[FWD]: Resolving MEM hazard on rs1: x%d\n", rs1_idex);
    #endif
}

if(pregs_p->memwb_preg.out.reg_write && (rd_memwb !=0) && !(pregs_p->exmem_preg.out.reg_write && (rd_exmem!=0)&& (rd_exmem == rs2_idex)) && (rd_memwb == rs2_idex)){
    pwires_p->forwardB = 1;
      fwd_exmem_counter++;
    #ifdef DEBUG_CYCLE
      printf("[FWD]: Resolving MEM hazard on rs2: x%d\n", rs2_idex);
    #endif
}

// Calculates the final value from MEM/WB stage to forward:
// If instruction is JAL (0x6F) or JALR (0x67): forward PC + 4.
// Otherwise:
// If mem_to_reg is true (load instruction): use data (loaded memory value).
// Else: use alu_result.
if (pregs_p->memwb_preg.out.reg_write && pregs_p->memwb_preg.out.rd != 0) {
        uint32_t value;
        if (pregs_p->memwb_preg.out.instr.opcode == 0x6F || pregs_p->memwb_preg.out.instr.opcode == 0x67) {
            value = pregs_p->memwb_preg.out.instr_addr + 4;
        } else {
            value = pregs_p->memwb_preg.out.mem_to_reg ? pregs_p->memwb_preg.out.data : pregs_p->memwb_preg.out.alu_result;
        }
        pwires_p->memwb_forward_val = value;
    }
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}
/**
 * Task   : Sets the pipeline wires for the hazard unit's control signals
 *           based on the pipeline register values.
 * input  : pipeline_regs_t*, pipeline_wires_t*
 * output : None
*/
void detect_hazard(pipeline_regs_t* pregs_p, pipeline_wires_t* pwires_p, regfile_t* regfile_p)
{
    // Detects and handles load-use data hazards, where an instruction following
    //  a lw (or any load) tries to use the value being loaded before it's available.

    idex_reg_t idex_out = pregs_p->idex_preg.out;
   Instruction ifid_instr = pregs_p->ifid_preg.out.instr;


    if (idex_out.mem_read) {
        uint8_t rd = idex_out.rd;
        uint8_t rs1 = 0, rs2 = 0;

        switch (ifid_instr.opcode) {
            case 0x33: // R-type
                rs1 = ifid_instr.rtype.rs1;
                rs2 = ifid_instr.rtype.rs2;
                break;
            case 0x13: // I-type ALU
            case 0x03: // I-type load
            case 0x67: // JALR
                rs1 = ifid_instr.itype.rs1;
                break;
            case 0x23: // Store
                rs1 = ifid_instr.stype.rs1;
                rs2 = ifid_instr.stype.rs2;
                break;
            case 0x63: // Branch
                rs1 = ifid_instr.sbtype.rs1;
                rs2 = ifid_instr.sbtype.rs2;
                break;
            default:
                break;
        }
//Checks if the instruction in ID/EX (idex_out) is a load instruction (mem_read is true).
        // Detect Load-Use hazard
        if (rd != 0 && (rd == rs1 || rd == rs2)) {
            // Clear control signals in IDEX (bubble)
            pwires_p->reg_write = false;
            pwires_p->mem_read  = false;
            pwires_p->mem_write = false;
            pwires_p->branch    = false;
            pwires_p->mem_to_reg = false;
            pwires_p->alu_op    = 0;
            pwires_p->alu_src   = false;
            pregs_p->ifid_preg.inp = pregs_p->ifid_preg.out;
//             // Prevent PC and IFID from updating (stall)
//             If a match is found (i.e., the next instruction uses the result too early), it triggers a stall:
// Suppresses control signals (e.g., disables reg_write, mem_read, etc.).
// Prevents PC from updating by setting both pc_src0 and pc_src1 to the current PC.
// Copies the current IF/ID instruction back into itself to re-fetch the same instruction.
// Sets stall flag and increments stall_counter.
            pwires_p->pcsrc = false;
            pwires_p->pc_src1 = regfile_p->PC; // stay on same PC
            pwires_p->pc_src0 = regfile_p->PC;
            pwires_p->stall = true;
            stall_counter++;

// #ifdef DEBUG_CYCLE
//             printf("[HZD]: Stalling and rewriting PC: 0x%08x\n", regfile_p->PC);
// #endif
        }
    }
}

void detect_controlHazard(pipeline_regs_t* pregs_p, pipeline_wires_t* pwires_p, regfile_t* regfile_p){

    if (pwires_p->flush) {
    #ifdef PRINT_STATS
    printf("[CPL]: Pipeline Flushed\n");
     #endif
  
// Replaces instructions in the IF/ID, ID/EX, and EX/MEM
//  pipeline registers with NOPs (addi x0, x0, 0), effectively clearing invalid instructions.
    Instruction nop_instr = { .bits = 0x00000013 };
    uint32_t ifid_addr = pregs_p->ifid_preg.inp.instr_addr;
    // Flush IFID
    pregs_p->ifid_preg.inp.instr = nop_instr;
    pregs_p->ifid_preg.inp.instr_bits = 0x00000013;
    pregs_p->ifid_preg.inp.instr_addr = ifid_addr;

    // Flush IDEX
    uint32_t idex_addr = pregs_p->idex_preg.inp.instr_addr;
    pregs_p->idex_preg.inp = (idex_reg_t){0};
    pregs_p->idex_preg.inp.instr = nop_instr;
     pregs_p->idex_preg.inp.instr_addr = idex_addr;
    // Flush EXMEM
    uint32_t exmem_addr = pregs_p->exmem_preg.inp.instr_addr;
    pregs_p->exmem_preg.inp = (exmem_reg_t){0};
    pregs_p->exmem_preg.inp.instr = nop_instr;
    pregs_p->exmem_preg.inp.instr_addr = exmem_addr;

    // Reset flush
    pwires_p->flush = false;

    
}
 

}

///////////////////////////////////////////////////////////////////////////////


/// RESERVED FOR PRINTING REGISTER TRACE AFTER EACH CLOCK CYCLE ///
void print_register_trace(regfile_t* regfile_p)
{
  // print
  for (uint8_t i = 0; i < 8; i++)       // 8 columns
  {
    for (uint8_t j = 0; j < 4; j++)     // of 4 registers each
    {
      printf("r%2d=%08x ", i * 4 + j, regfile_p->R[i * 4 + j]);
    }
    printf("\n");
  }
  printf("\n");
}

#endif // _STAGE_HELPERS_H_