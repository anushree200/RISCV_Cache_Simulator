#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "instr_R.h"
#include "register_no.h"

void process_instructionR(char *line, unsigned int funct3, unsigned int funct7, int n,const char *output_filename)
{
    FILE *fileo = fopen(output_filename, "a");
    if (fileo == NULL)
    {
        printf("Error opening file for appending!\n");
        return;
    }
    char reg1[10], reg2[10], reg3[10];
    char extra[10];
    int operand_count = sscanf(line, "%*s %s %s %s %s", reg1, reg2, reg3, extra);

    // Check for too many operands
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
    reg1[strcspn(reg1, ",")] = '\0';
    reg2[strcspn(reg2, ",")] = '\0';
    reg3[strcspn(reg3, ",")] = '\0';

    int rd = get_register_number(reg1);
    int rs1 = get_register_number(reg2);
    int rs2 = get_register_number(reg3);
    if(rs1 == -1 || rd == -1 || rs2 == -1){
            fprintf(fileo, "Invalid Register No.");
            fclose(fileo);
            return;
        }
    unsigned int opcode = 0b0110011;
    //constructing instruction
    unsigned int instruction = (funct7 << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;

    fprintf(fileo, "%08X\n", instruction);
    fclose(fileo);
}
