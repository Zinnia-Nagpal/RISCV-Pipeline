
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

/* Unpacks the 32-bit machine code instruction given into the correct
 * type within the instruction struct */
Instruction parse_instruction(uint32_t instruction_bits) {
  /* YOUR CODE HERE */
  Instruction instruction;
  // add x9, x20, x21   hex: 01 5A 04 B3, binary = 0000 0001 0101 1010 0000 0100 1011 0011
  // Opcode: 0110011 (0x33) Get the Opcode by &ing 0x1111111, bottom 7 bits
  instruction.opcode = instruction_bits & ((1U << 7) - 1);
  instruction.bits = instruction_bits;
  // Shift right to move to pointer to interpret next fields in instruction.
  instruction_bits >>= 7;

  switch (instruction.opcode) {
  // R-Type
  case 0x33:
    // instruction: 0000 0001 0101 1010 0000 0100 1, destination : 01001
    instruction.rtype.rd = instruction_bits & ((1U << 5) - 1);
    instruction_bits >>= 5;

    // instruction: 0000 0001 0101 1010 0000, func3 : 000
    instruction.rtype.funct3 = instruction_bits & ((1U << 3) - 1);
    instruction_bits >>= 3;

    // instruction: 0000 0001 0101 1010 0, src1: 10100
    instruction.rtype.rs1 = instruction_bits & ((1U << 5) - 1);
    instruction_bits >>= 5;

    // instruction: 0000 0001 0101, src2: 10101
    instruction.rtype.rs2 = instruction_bits & ((1U << 5) - 1);
    instruction_bits >>= 5;

    // funct7: 0000 000
    instruction.rtype.funct7 = instruction_bits & ((1U << 7) - 1);
    break;
  // cases for other types of instructions
  /* YOUR CODE HERE */
  //I-type
    case 0x03:
    instruction.itype.rd = instruction_bits & ((1U << 5) - 1);  //rd
    instruction_bits >>= 5;
    instruction.itype.funct3 = instruction_bits & ((1U << 3) - 1);// funct3
    instruction_bits >>= 3; 
    instruction.itype.rs1 = instruction_bits & ((1U << 5) - 1);//rs1
    instruction_bits >>= 5;
    instruction.itype.imm = instruction_bits & ((1U << 12) -1);//imm
    break;

    case 0x13:
    instruction.itype.rd = instruction_bits & ((1U << 5) - 1);//rd
    instruction_bits >>= 5;
    instruction.itype.funct3 = instruction_bits & ((1U << 3) - 1);//funct3
    instruction_bits >>= 3; 
    instruction.itype.rs1 = instruction_bits & ((1U << 5) - 1);//rs1
    instruction_bits >>= 5;
    instruction.itype.imm = instruction_bits & ((1U << 12) -1);//imm
    break;   

    case 0x73:
    instruction.itype.rd = instruction_bits & ((1U << 5) - 1);//rd
    instruction_bits >>= 5;
    instruction.itype.funct3 = instruction_bits & ((1U << 3) - 1);//funct3
    instruction_bits >>= 3; 
    instruction.itype.rs1 = instruction_bits & ((1U << 5) - 1);//rs1
    instruction_bits >>= 5;
    instruction.itype.imm = instruction_bits & ((1U << 12) -1);//imm
    break;  

  //S-type
    case 0x23:
    instruction.stype.imm5 = instruction_bits & ((1U << 5) - 1);//imm5
    instruction_bits >>= 5;
    instruction.stype.funct3 = instruction_bits & ((1U << 3) - 1);//funct3
    instruction_bits >>= 3; 
    instruction.stype.rs1 = instruction_bits & ((1U << 5) - 1);//rs1
    instruction_bits >>= 5;
    instruction.stype.rs2 = instruction_bits & ((1U << 5) - 1);//rs2
    instruction_bits >>= 5;
    instruction.stype.imm7 = instruction_bits & ((1U << 7) - 1);//imm7
    break;

  //SB-type
    case 0x63:
    instruction.sbtype.imm5 = instruction_bits & ((1U << 5) - 1);  //imm5
    instruction_bits >>= 5;
    instruction.sbtype.funct3 = instruction_bits & ((1U << 3) - 1);// funct3
    instruction_bits >>= 3; 
    instruction.sbtype.rs1 = instruction_bits & ((1U << 5) - 1);//rs1
    instruction_bits >>= 5;
    instruction.sbtype.rs2 = instruction_bits & ((1U << 5) - 1);//rs2
    instruction_bits >>= 5;
    instruction.sbtype.imm7 = instruction_bits & ((1U << 7) - 1);//imm7
    break;

  //U-type
    case 0x37:
    instruction.utype.rd = instruction_bits & ((1U << 5) - 1);//rd
    instruction_bits >>= 5;
    instruction.utype.imm = instruction_bits & ((1U << 20) - 1);// imm
    break;

  //UJ-type
    case 0x6f:
    instruction.ujtype.rd = instruction_bits & ((1U << 5) - 1);//rd
    instruction_bits >>= 5;
    instruction.ujtype.imm = instruction_bits & ((1U << 20) - 1);// imm
    break;
   

  #ifndef TESTING
  default:
    exit(EXIT_FAILURE);
  #endif
  }
  return instruction;
}

/************************Helper functions************************/
/* Here, you will need to implement a few common helper functions, 
 * which you will call in other functions when parsing, printing, 
 * or executing the instructions. */

/* Sign extends the given field to a 32-bit integer where field is
 * interpreted an n-bit integer. */
int sign_extend_number(unsigned int field, unsigned int n) {
  /* YOUR CODE HERE */
    int mask = 1 << (n - 1);
  field = field & ((1 << n) - 1); // Mask to n bits
  return (field ^ mask) - mask;
}

/* Return the number of bytes (from the current PC) to the branch label using
 * the given branch instruction */
int get_branch_offset(Instruction instruction) {
  /* YOUR CODE HERE */
  // to extract immediate value for SB-Type instruction
  int imm11 = instruction.sbtype.imm5 & 1;
  int imm4_1 = (instruction.sbtype.imm5>> 1) & 0xF;
  int imm10_5 = instruction.sbtype.imm7 & 0x3F;
  int imm12 = (instruction.sbtype.imm7 >> 6) & 1;

  int offset = (imm12 << 12) | (imm11 << 11) | (imm10_5 << 5) | (imm4_1 << 1);
  return sign_extend_number(offset, 12);
}

/* Returns the number of bytes (from the current PC) to the jump label using the
 * given jump instruction */
int get_jump_offset(Instruction instruction) {
  /* YOUR CODE HERE */
  // to extract immediate value for UJ-Type instruction
  int imm20 = (instruction.ujtype.imm >> 19) & 1;
  int imm10_1 = (instruction.ujtype.imm >> 9) & 0x3FF;
  int imm11 = (instruction.ujtype.imm >> 8) & 1;
  int imm19_12 = instruction.ujtype.imm & 0xFF;

  int offset = (imm20 << 20) | (imm19_12 << 12) | (imm11 << 11) | (imm10_1<<1);
  return sign_extend_number(offset, 20);
}

/* Returns the number of bytes (from the current PC) to the base address using the
 * given store instruction */
int get_store_offset(Instruction instruction) {
  /* YOUR CODE HERE */
  int offset = (instruction.stype.imm7 << 5) | instruction.stype.imm5;
  return sign_extend_number(offset, 12);

}
/************************Helper functions************************/

void handle_invalid_instruction(Instruction instruction) {
  printf("Invalid Instruction: 0x%08x\n", instruction.bits);
}

void handle_invalid_read(Address address) {
  printf("Bad Read. Address: 0x%08x\n", address);
  exit(-1);
}

void handle_invalid_write(Address address) {
  printf("Bad Write. Address: 0x%08x\n", address);
  exit(-1);
}
