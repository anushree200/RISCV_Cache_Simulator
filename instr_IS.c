#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "instr_IS.h"
#include "register_no.h"

void process_instructionIS(char *line, unsigned int funct3, unsigned int opcode, int n,const char *output_filename)
{
    FILE *fileo = fopen(output_filename, "a");
    if (fileo == NULL)
    {
        printf("Error opening file for appending!\n");
        return;
    }
    char reg1[10], reg2[10];
    char extra[10];
    unsigned int instruction;
    int immI;
    //immediate instructions
    if (opcode == 0b0010011)
    {
        int operand_count = sscanf(line, "%*s %[^,], %[^,], %d %s", reg1, reg2, &immI, extra);

    // Check if the third operand (immediate value) is missing and assume 0 if it is
    if (operand_count == 2) {
        immI = 0;
        operand_count = 3; // Correct the operand count to indicate that three operands were handled
    }
        if (operand_count > 3)
        {
            fprintf(fileo, "Error: Instruction has too many operands. Expected at most 3, but got %d in line %d\n", operand_count, n);
            fclose(fileo);
            return;
        }

        // Check for too few operands
        if (operand_count < 3)
        {
            fprintf(fileo, "Error: Instruction parsing failed, expected 3 operands but got %d in line %d\n", operand_count, n);
            fclose(fileo);
            return;
        }
        if (immI < -2048 || immI > 2047)
        { // 12-bit signed immediate range
            fprintf(fileo, "Error: Immediate value %d out of range for instruction in line %d\n", immI, n);
            fclose(fileo);
            return;
        }
        reg1[strcspn(reg1, ",")] = '\0';
        reg2[strcspn(reg2, ",")] = '\0';
        immI &= 0xFFF;
        int rd = get_register_number(reg1);
        int rs1 = get_register_number(reg2);
        if(rs1 == -1 || rd == -1){
            fprintf(fileo, "Invalid Register No.\n");
            fclose(fileo);
            return;
        }
        //construction instructions
        instruction = (immI << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
    }
    //load instructions
    else if (opcode == 0b0000011)
    {
        int operand_count = sscanf(line, "%*s %[^,], %d(%[^)]) %s", reg1, &immI, reg2, extra);
        if (operand_count > 3)
        {
            fprintf(fileo, "Error: Instruction has too many operands. Expected at most 3, but got %d in line %d\n", operand_count, n);
            fclose(fileo);
            return;
        }
        if (operand_count < 3)
        {
            fprintf(fileo, "Error: Instruction parsing failed, expected 3 operands but got %d in line %d\n", operand_count, n);
            fclose(fileo);
            return;
        }
        if (immI < -2048 || immI > 2047)
        { // 12-bit signed immediate range
            fprintf(fileo, "Error: Immediate value %d out of range for instruction in line %d\n", immI, n);
            fclose(fileo);
            return;
        }
        immI &= 0xFFF;
        int rd = get_register_number(reg1);
        int rs1 = get_register_number(reg2);
        if(rs1 == -1 || rd == -1){
            fprintf(fileo, "Invalid Register No.\n");
            fclose(fileo);
            return;
        }
        instruction = (immI << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
    }
    // JALR instruction
	// JALR instruction
	else if (opcode == 0b1100111)
	{
	    int operand_count = sscanf(line, "%*s %[^,], %d(%[^)]) %s", reg1, &immI, reg2, extra);

	    // Check for too many operands
	    if (operand_count > 3)
	    {
	        fprintf(fileo, "Error: Instruction has too many operands. Expected exactly 3, but got %d in line %d\n", operand_count, n);
	        fclose(fileo);
	        return;
	    }

	    // Check for too few operands
	    if (operand_count < 3)
	    {
	        fprintf(fileo, "Error: Instruction parsing failed, expected 3 operands but got %d in line %d\n", operand_count, n);
	        fclose(fileo);
	        return;
	    }

	    // Validate immediate range for 12-bit signed value
	    if (immI < -2048 || immI > 2047)
	    {
	        fprintf(fileo, "Error: Immediate value %d out of range for instruction in line %d\n", immI, n);
	        fclose(fileo);
	        return;
	    }

	    // Strip any trailing commas from register names
	    reg1[strcspn(reg1, ",")] = '\0';
	    reg2[strcspn(reg2, ",")] = '\0';

	    // Mask immediate to fit into 12 bits
	    immI &= 0xFFF;

	    // Convert register names to numbers
	    int rd = get_register_number(reg1);
	    int rs1 = get_register_number(reg2);

	    // Validate register numbers
	    if (rs1 == -1 || rd == -1)
	    {
	        fprintf(fileo, "Invalid Register No.\n");
	        fclose(fileo);
	        return;
	    }

	    // Construct the JALR instruction
	    instruction = (immI << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
	    //fprintf(fileo, "%08X\n", instruction);
	}

    //store type instructions
    else if (opcode == 0b0100011)
    {
        int operand_count = sscanf(line, "%*s %[^,], %d(%[^)]) %s", reg2, &immI, reg1, extra);
        if (operand_count > 3)
        {
            fprintf(fileo, "Error: Instruction has too many operands. Expected at most 3, but got %d in line %d\n", operand_count, n);
            fclose(fileo);
            return;
        }
        if (operand_count < 3)
        {
            fprintf(fileo, "Error: Instruction parsing failed, expected 3 operands but got %d in line %d\n", operand_count, n);
            fclose(fileo);
            return;
        }
        if (immI < -2048 || immI > 2047)
        { // 12-bit signed immediate range
            fprintf(fileo, "Error: Immediate value %d out of range for instruction in line %d\n", immI, n);
            fclose(fileo);
            return;
        }
        int imm_high = (immI >> 5) & 0x7F;
        int imm_low = immI & 0x1F;
        int rs1 = get_register_number(reg1);
        int rs2 = get_register_number(reg2);
        if(rs1 == -1 || rs2 == -1){
            fprintf(fileo, "Invalid Register No.\n");
            fclose(fileo);
            return;
        }
        instruction = (imm_high << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) | (imm_low << 7) | opcode;
    }
    fprintf(fileo, "%08X\n", instruction);
    fclose(fileo);
}

//for better understanding
void SRAI_IS(char *line, unsigned int funct3, unsigned int opcode, int n,const char *output_filename)
{
    FILE *fileo = fopen(output_filename, "a");
    if (fileo == NULL)
    {
        printf("Error opening file for appending!\n");
        return;
    }
    char reg1[10], reg2[10];
    char extra[10];
    unsigned int instruction;
    int immI;
    int operand_count = sscanf(line, "%*s %s %s %d %s", reg1, reg2, &immI, extra);
    if (operand_count > 3)
    {
        fprintf(fileo, "Error: Instruction has too many operands. Expected at most 3, but got %d in line %d\n", operand_count, n);
        fclose(fileo);
        return;
    }
    if (operand_count < 3)
    {
        fprintf(fileo, "Error: Instruction parsing failed, expected 3 operands but got %d in line %d\n", operand_count, n);
        fclose(fileo);
        return;
    }
    reg1[strcspn(reg1, ",")] = '\0';
    reg2[strcspn(reg2, ",")] = '\0';
    if (immI < 0 || immI > 31)
    {
        fprintf(fileo, "Error: Immediate value %d for SRAI is out of range (0-31) in line %d\n", immI, n);
        fclose(fileo);
        return;
    }
    int rd = get_register_number(reg1);
    int rs1 = get_register_number(reg2);
    if(rs1 == -1 || rd == -1){
            fprintf(fileo, "Invalid Register No.\n");
            fclose(fileo);
            return;
        }
    unsigned int lowshif = immI & 0x1F;    // Lower 5 bits for shift amount
    unsigned int highshif = 0b0100000; // Set the upper bits to indicate SRAI
    instruction = (highshif << 25) | (lowshif << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
    fprintf(fileo, "%08X\n", instruction);
    fclose(fileo);
}
