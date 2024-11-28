#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "instr_U.h"
#include "register_no.h"

void process_instructionU(char *line, int n,const char *output_filename)
{
    FILE *fileo = fopen(output_filename, "a");
    if (fileo == NULL)
    {
        printf("Error opening file for appending!\n");
        return;
    }
    char reg[10];
    char extra[10];
    unsigned int imm;
    int operand_count = sscanf(line, "%*s %s %x %s", reg, &imm, extra);
    if (operand_count > 2)
    {
        fprintf(fileo, "Error: Instruction has too many operands. Expected at most 2, but got %d in line %d\n", operand_count, n);
        fclose(fileo);
        return;
    }
    if (operand_count < 2)
    {
        fprintf(fileo, "Error: Instruction parsing failed, expected 2 operands but got %d in line %d\n", operand_count, n);
        fclose(fileo);
        return;
    }
    reg[strcspn(reg, ",")] = '\0';

    int rd = get_register_number(reg);
    if(rd == -1){
            fprintf(fileo, "Invalid Register No.");
            fclose(fileo);
            return;
        }
    if (imm > 0xFFFFF)
    {
        fprintf(fileo, "Immediate value %u for %s is out of range\n", imm, line);
        fclose(fileo);
        return;
    }

    unsigned int instruction = (imm << 12) | (rd << 7) | 0b0110111;

    fprintf(fileo, "%08X\n", instruction);
    fclose(fileo);
}