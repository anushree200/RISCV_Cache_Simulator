#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include "instr_R.h"
#include "instr_IS.h"
#include "instr_U.h"
#include "instr_B.h"
#include "instr_J.h"
#include "register_no.h"
void process_instructionR(char *line, unsigned int funct3, unsigned int funct7, int n,const char *output_filename);
void process_instructionIS(char *line, unsigned int funct3, unsigned int opcode, int n,const char *output_filename);
void SRAI_IS(char *line, unsigned int funct3, unsigned int opcode, int n,const char *output_filename);
void process_instructionB(char *line, unsigned int funct3, int labeladd, int curaddr, int n,const char *output_filename);
void process_instructionJ(char *line, unsigned int opcode, int labeladd, int curaddr, int n,const char *output_filename);
void process_instructionU(char *line, int n,const char *output_filename);
void process_instr(FILE *file,const char *output_filename);
int get_register_number(const char *reg);

#define MAX_LABELS 100
#define MAX_LABEL_LENGTH 50
#define MAX_FILES 100 // Maximum number of input files
#define FILENAME_LENGTH 256 // Length of each filename

typedef struct
{
    char label[MAX_LABEL_LENGTH];
    int address;
} Label;

Label labels[MAX_LABELS];
int label_count = 0;
int dup[MAX_LABELS];
int j = -1;
int is_label_instruction(const char* instruction) {
    if (instruction == NULL) return 0;
    char *colon_pos = strchr(instruction, ':');
    if (colon_pos && colon_pos != instruction && isalpha(instruction[0])) {
        return 1;
    }

    return 0;
}

char* get_label_from_instruction(const char* instruction) {
    static char label[MAX_LABEL_LENGTH];
    char *colon_pos = strchr(instruction, ':');
    if (colon_pos) {
        size_t label_length = colon_pos - instruction;
        strncpy(label, instruction, label_length);
        label[label_length] = '\0';
        printf("Extracted label: %s\n", label);
        return label;
    }
    return "";
}

void add_label(const char *label_name, int curaddr) {
    const char* lab = get_label_from_instruction(label_name);

    // Only add the label if it�s not empty
    if (label_count < MAX_LABELS && strlen(lab) > 0) {
        // Check for duplicates
        for (int i = 0; i < label_count; i++) {
            if (strcmp(labels[i].label, lab) == 0) {
                j += 1;
                dup[j] = curaddr;
                return;
            }
        }
        // Add label
        strcpy(labels[label_count].label, lab);
        labels[label_count].address = curaddr;
        label_count++;
    }
}
int find_label_address(const char *label)
{
    for (int i = 0; i < label_count; i++)
    {
        if (strcmp(labels[i].label, label) == 0)
        {
			//printf("Extracted labelADDRESS: %d\n", labels[i].address);
            return labels[i].address;
        }
    }
    return -1;
}
/*void trim_newline(char *str) {
    int len = strlen(str);
    if (len > 0 && str[len - 1] == '\n') {
        str[len - 1] = '\0';
    }
}*/

/*void print_label_arrays() {
    printf("\nLabels Array:\n");
    for (int i = 0; i < label_count; i++) {
        printf("Label: %s, Address: %d\n", labels[i].label, labels[i].address);
    }

    printf("\nDuplicate Addresses Array:\n");
    for (int i = 0; i <= j; i++) {
        printf("Duplicate Address: %d\n", dup[i]);
    }
}*/




int extract_number(const char *filename) {
    // Function to extract the first integer found in the filename
    while (*filename) {
        if (isdigit(*filename)) {
            return atoi(filename);
        }
        filename++;
    }
    return -1; // Return -1 if no number is found
}

int main() {
    char input_line[1024]; // Buffer to store the entire line of input
    char filenames[MAX_FILES][FILENAME_LENGTH]; // Array to store filenames
    char output_filenames[MAX_FILES][FILENAME_LENGTH]; // Array to store output filenames
    int file_count = 0;

    printf("Enter input filenames separated by commas:\n");
    fgets(input_line, sizeof(input_line), stdin); // Read the entire line of input

    // Use strtok to split the input line by commas
    char *token = strtok(input_line, ",");
    while (token != NULL && file_count < MAX_FILES) {
        // Trim leading whitespace
        while (*token == ' ' || *token == '\n') token++;

        // Ensure token is not empty after trimming
        if (*token != '\0') {
            // Store the filename
            snprintf(filenames[file_count], sizeof(filenames[file_count]), "%s", token);

            // Remove trailing whitespace or newline from stored filename
            char *end = filenames[file_count] + strlen(filenames[file_count]) - 1;
            while (end > filenames[file_count] && (*end == ' ' || *end == '\n')) {
                *end = '\0';
                end--;
            }

            // Extract the number from the input filename
            int number = extract_number(filenames[file_count]);
            if (number != -1) {
                // Generate output filename based on extracted number
                snprintf(output_filenames[file_count], sizeof(output_filenames[file_count]), "output%d.hex", number);
            } else {
                // Default to output0.hex if no number is found
                snprintf(output_filenames[file_count], sizeof(output_filenames[file_count]), "output0.hex");
            }
            
            file_count++;
        }

        token = strtok(NULL, ","); // Get the next token
    }

    // Debugging print to confirm file count
    printf("Number of files to process: %d\n", file_count);

    // Process each file
    for (int i = 0; i < file_count; i++) {
        FILE *file = fopen(filenames[i], "r");
        if (file == NULL) {
            printf("Error opening file %s\n", filenames[i]);
            continue; // Skip to the next file
        }

        // Process the input file
        process_instr(file, output_filenames[i]); // Pass the output filename to your processing function

        // Close the input file
        fclose(file);

        printf("Processed %s into %s\n", filenames[i], output_filenames[i]);
    }

    return 0;
}



void process_instr(FILE *file,const char *output_filename)
{
	FILE *fileo = fopen(output_filename, "w");
	        if (fileo == NULL) {
	            printf("Error opening file for writing: %s\n", output_filename);
	            fclose(file);
	            //continue; // Skip to the next file
        }
        fclose(fileo);
    char line[256];
    char instruction[10];
    char label[50];
    int n = 1;
    int curaddr = 0;
    int add;
    bool labelerror = false;
    while (fgets(line, sizeof(line), file))
    {
        char found_label[50];
        //trim_newline(line);
        if (is_label_instruction(line))
        {
            add_label(line, curaddr);
            curaddr += 4;
        }
        else {curaddr += 4;}
    }
	//print_label_arrays();
    rewind(file);
    curaddr = 0;

    while (fgets(line, sizeof(line), file))
    {

        char instruction[10];
        char label[50];
        sscanf(line, "%s", instruction);
        if (n != 1)
        {
            curaddr += 4;
        }
		if (strchr(instruction, '.')){
			n+=1;
			continue;
			}
        if (strchr(instruction, ':'))
        {
            sscanf(line, "%[^:]: %[^\n]", label, line);
            for (int i = 0; i < j + 1; i++)
            {
                if (dup[i] == curaddr)
                {
                    FILE *fileo = fopen(output_filename, "a");
                    if (fileo == NULL)
                    {
                        printf("Error opening file for appending!\n");
                        return;
                    }
                    fprintf(fileo, "This is a multiple label error for label %s in line number %d \n", label, n);
                    labelerror = true;
                    fclose(fileo);

                    continue;
                }
            }
            n += 1;
        }
        if (labelerror)
        {
            continue;
        }
        sscanf(line, "%s", instruction);
        if (strcmp(instruction, "sra") == 0)
        {
            process_instructionR(line, 0b101, 0b0100000, n,output_filename);
            n += 1;
        }
        else if (strcmp(instruction, "sub") == 0)
        {
            process_instructionR(line, 0b000, 0b0100000, n,output_filename);
            n += 1;
        }
        else if (strcmp(instruction, "or") == 0)
        {
            process_instructionR(line, 0b110, 0b0000000, n,output_filename);
            n += 1;
        }
        else if (strcmp(instruction, "sll") == 0)
        {
            process_instructionR(line, 0b001, 0b0000000, n,output_filename);
            n += 1;
        }
        else if (strcmp(instruction, "and") == 0)
        {
            process_instructionR(line, 0b111, 0b0000000, n,output_filename);
            n += 1;
        }
        else if (strcmp(instruction, "srl") == 0)
        {
            process_instructionR(line, 0b101, 0b0000000, n,output_filename);
            n += 1;
        }
        else if (strcmp(instruction, "add") == 0)
        {
            process_instructionR(line, 0b000, 0b0000000, n,output_filename);
            n += 1;
        }
        else if (strcmp(instruction, "xor") == 0)
        {
            process_instructionR(line, 0b100, 0b0000000, n,output_filename);
            n += 1;
        }
        else if (strcmp(instruction, "addi") == 0)
        {
            process_instructionIS(line, 0b000, 0b0010011, n,output_filename);
            n += 1;
        }
        else if (strcmp(instruction, "andi") == 0)
        {
            process_instructionIS(line, 0b111, 0b0010011, n,output_filename);
            n += 1;
        }
        else if (strcmp(instruction, "ori") == 0)
        {
            process_instructionIS(line, 0b110, 0b0010011, n,output_filename);
            n += 1;
        }
        else if (strcmp(instruction, "xori") == 0)
        {
            process_instructionIS(line, 0b100, 0b0010011, n,output_filename);
            n += 1;
        }
        else if (strcmp(instruction, "slli") == 0)
        {
            process_instructionIS(line, 0b001, 0b0010011, n,output_filename);
            n += 1;
        }
        else if (strcmp(instruction, "srli") == 0)
        {
            process_instructionIS(line, 0b101, 0b0010011, n,output_filename);
            n += 1;
        }
        else if (strcmp(instruction, "srai") == 0)
        {
            SRAI_IS(line, 0b101, 0b0010011, n,output_filename);
            n += 1;
        }

        else if (strcmp(instruction, "ld") == 0)
        {
            process_instructionIS(line, 0b011, 0b0000011, n,output_filename);
            n += 1;
        }
        else if (strcmp(instruction, "lw") == 0)
        {
            process_instructionIS(line, 0b010, 0b0000011, n,output_filename);
            n += 1;
        }
        else if (strcmp(instruction, "lh") == 0)
        {
            process_instructionIS(line, 0b001, 0b0000011, n,output_filename);
            n += 1;
        }
        else if (strcmp(instruction, "lb") == 0)
        {
            process_instructionIS(line, 0b000, 0b0000011, n,output_filename);
            n += 1;
        }
        else if (strcmp(instruction, "lwu") == 0)
        {
            process_instructionIS(line, 0b110, 0b0000011, n,output_filename);
            n += 1;
        }
        else if (strcmp(instruction, "lhu") == 0)
        {
            process_instructionIS(line, 0b101, 0b0000011, n,output_filename);
            n += 1;
        }
        else if (strcmp(instruction, "lbu") == 0)
        {
            process_instructionIS(line, 0b100, 0b0000011, n,output_filename);
            n += 1;
        }
		else if (strcmp(instruction, "jalr") == 0)
		        {
		            process_instructionIS(line, 0b000, 0b1100111, n,output_filename);
		            n += 1;
        }
        // s format
        else if (strcmp(instruction, "sd") == 0)
        {
            process_instructionIS(line, 0b011, 0b0100011, n,output_filename);
            n += 1;
        }
        else if (strcmp(instruction, "sw") == 0)
        {
            process_instructionIS(line, 0b010, 0b0100011, n,output_filename);
            n += 1;
        }
        else if (strcmp(instruction, "sh") == 0)
        {
            process_instructionIS(line, 0b001, 0b0100011, n,output_filename);
            n += 1;
        }
        else if (strcmp(instruction, "sb") == 0)
        {
            process_instructionIS(line, 0b000, 0b0100011, n,output_filename);
            n += 1;
        }
        else if (strcmp(instruction, "jalr") == 0)
        {
            process_instructionIS(line, 0b000, 0b1100111, n,output_filename);
            n += 1;
        }
        // b
        else if (strcmp(instruction, "beq") == 0)
        {
            sscanf(line, "%*s %*s %*s %s", label);
            add = find_label_address(label);
            process_instructionB(line, 0b000, add, curaddr, n,output_filename);
            n += 1;
        }
        else if (strcmp(instruction, "bne") == 0)
        {
            sscanf(line, "%*s %*s %*s %s", label);
            add = find_label_address(label);

            process_instructionB(line, 0b001, add, curaddr, n,output_filename);
            n += 1;
        }
        else if (strcmp(instruction, "blt") == 0)
        {
            sscanf(line, "%*s %*s %*s %s", label);
            add = find_label_address(label);
            process_instructionB(line, 0b100, add, curaddr, n,output_filename);
            n += 1;
        }
        else if (strcmp(instruction, "bge") == 0)
        {
            sscanf(line, "%*s %*s %*s %s", label);
            add = find_label_address(label);

            process_instructionB(line, 0b101, add, curaddr, n,output_filename);
            n += 1;
        }
        else if (strcmp(instruction, "bltu") == 0)
        {
            sscanf(line, "%*s %*s %*s %s", label);
            add = find_label_address(label);
            process_instructionB(line, 0b110, add, curaddr, n,output_filename);
            n += 1;
        }
        else if (strcmp(instruction, "bgeu") == 0)
        {
            sscanf(line, "%*s %*s %*s %s", label);
            add = find_label_address(label);
            process_instructionB(line, 0b111, add, curaddr, n,output_filename);
            n += 1;
        }
        // j
        else if (strcmp(instruction, "jal") == 0)
        {
            sscanf(line, "%*s %*s %s", label);
            add = find_label_address(label);
            process_instructionJ(line, 0b1101111, add, curaddr, n,output_filename);
            n += 1;
        }
        // u
        else if (strcmp(instruction, "lui") == 0)
        {
            process_instructionU(line, n,output_filename);
            n += 1;
        }
        else
        {
            fprintf(stderr,"Invalid Instruction, in line no. %d", n);
            n += 1;
        }

    }
}
