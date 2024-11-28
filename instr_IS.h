#ifndef INSTRUCTION_IS_H
#define INSTRUCTION_IS_H

void process_instructionIS(char *line, unsigned int funct3, unsigned int opcode, int n, const char *output_filename);
void SRAI_IS(char *line, unsigned int funct3, unsigned int opcode, int n, const char *output_filename);

#endif
