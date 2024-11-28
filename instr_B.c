#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "instr_B.h"
#include "register_no.h"

void process_instructionB(char *line, unsigned int funct3, int labeladd, int curaddr, int n,const char *output_filename) {
    // File opening
    FILE *fileo = fopen(output_filename, "a");
    if (fileo == NULL) {
        printf("Error opening file for appending!\n");
        return;
    }

    char reg1[10], reg2[10], label[50];
    char extra[10];
    int operand_count = sscanf(line, "%*s %s %s %s %s", reg1, reg2, label, extra);

    // Check for too many operands
    if (operand_count > 3) {
        fprintf(fileo, "Error: Instruction has too many operands. Expected at most 3, but got %d in line %d\n", operand_count, n);
        fclose(fileo);
        return;
    }

    // Check for too few operands
    if (operand_count < 3) {
        fprintf(fileo, "Error: Instruction parsing failed, expected 3 operands but got %d in line %d\n", operand_count, n);
        fclose(fileo);
        return;
    }

    reg1[strcspn(reg1, ",")] = '\0';
    reg2[strcspn(reg2, ",")] = '\0';

    int rs1 = get_register_number(reg1);
    int rs2 = get_register_number(reg2);

    // Validate register numbers
    if (rs1 == -1 || rs2 == -1) {
        fprintf(fileo, "Invalid Register No. in line %d\n", n);
        fclose(fileo);
        return;
    }

    // Check for valid label address
    if (labeladd == -1) {
        fprintf(fileo, "Error: Undefined label %s in line %d\n", label, n);
        fclose(fileo);
        return;
    }
    printf("%s\n",line);
	printf("label:%d\n",labeladd);
    printf("cur:%d\n",curaddr);
    // Calculate offset
    int calculatedOffset = labeladd - curaddr;
    printf("off:%d\n",calculatedOffset);
    if (calculatedOffset < -4096 || calculatedOffset > 4095) {
        fprintf(fileo, "Error: B-Type immediate value out of range: %d in line %d\n", calculatedOffset, n);
        fclose(fileo);
        return;
    }

    // Split immediate into B-type fields and mask bits for correctness
    int imm_12 = (calculatedOffset >> 12) & 0x1;        // Bit 12
    int imm_10_5 = (calculatedOffset >> 5) & 0x3F;      // Bits 10 to 5
    int imm_4_1 = (calculatedOffset >> 1) & 0xF;        // Bits 4 to 1
    int imm_11 = (calculatedOffset >> 11) & 0x1;        // Bit 11

    // Construct the 32-bit instruction in B-type format
    unsigned int instruction = (imm_12 << 31) | (imm_11 << 7) |
                               (imm_10_5 << 25) | (imm_4_1 << 8) |
                               (rs2 << 20) | (rs1 << 15) |
                               (funct3 << 12) | 0b1100011;

    // Write to file in hexadecimal
    fprintf(fileo, "%08X\n", instruction);
    fclose(fileo);
}

