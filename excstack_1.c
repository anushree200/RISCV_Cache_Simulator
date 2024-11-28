#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#define DATA_START 0x00010000
#define TEXT_START 0x00000000
#define STACK_START 0x00050000
#define NUM_REGISTERS 32
#define INSTRUCTION_SIZE 1000
#define MEMORY_SIZE 0x00060000
#define MAX_BREAKPOINTS 5
#define STACK_SIZE 1000
#define MAX_LABEL_LENGTH 200
#define MAX_COMMAND_LENGTH 200
#define MAX_ASSOCIATIVITY 16
#define MAX_CACHE_SETS 1024
#define MAXLOG 1024
#define MAX_LOG_LENGTH 100

int cachecount = 0; // To keep track of the number of log entries
char cache_log[MAXLOG][MAX_LOG_LENGTH]; // Array to store log entries
uint64_t registers[NUM_REGISTERS];
char* memory[INSTRUCTION_SIZE]; // Storing instructions as strings
uint8_t Memory[MEMORY_SIZE];
uint64_t memorybin[INSTRUCTION_SIZE];
uint64_t mem_address;
uint64_t pc = 0x0; // Program Counter
int step = 0;
int linecount = 0;
int breakpoints[MAX_BREAKPOINTS] = {0};
bool is_paused = false;
bool breakpoint = false;
int numberbreak = 0;
char current_label[MAX_LABEL_LENGTH] = "main";
int current_line_num = 0;
int beforeinstr=0;
bool cachestat=false;


void load(const char *filename);
void regs();
void run(const char *filename);
void exit_simulator();
void reset_simulator();
void runone();
void execute(uint64_t bininstr, const char* instr);

typedef struct {
    int valid;
    int dirty;
    uint64_t data; // Assuming you have data stored here
    int last_access_time; // Add a field for LRU tracking
    unsigned long tag;
} CacheLine;

typedef struct {
    CacheLine lines[MAX_ASSOCIATIVITY];
} CacheSet;

typedef struct {
    CacheSet sets[MAX_CACHE_SETS];//lines[MAX_ASSOCIATIVITY][MAX_CACHE_SETS]
    int enabled;
    int cache_size;
    int block_size;
    int associativity;
    char replacement_policy[10];
    char write_policy[5];
    int access_count;
    int hit_count;
    int miss_count;
} Cache;
Cache dcache;



void initialize_memory() {
    for (uint64_t i = 0; i < MEMORY_SIZE; i++) {
        Memory[i] = 0x0; // Initialize all memory to zero
    }
}

void print_memory(uint64_t addr, int count) {
    // Ensure the starting address and count are within the bounds of the memory
    //printf("addr: 0x%llx\n",addr);
    if (addr < MEMORY_SIZE && (addr + count) <= MEMORY_SIZE) {
        for (int i = 0; i < count; i++) {
            uint64_t current_addr = addr + i; // Calculate current memory address
            printf("Memory[0x%llx] 0x%02x\n", (unsigned long long)current_addr, Memory[current_addr]);
        }
    } else {
        printf("Address 0x%llx is out of bounds.\n", (unsigned long long)addr);
    }
}


typedef struct {
    char label[MAX_LABEL_LENGTH];
    int line_num;
} StackEntry;

StackEntry stack[STACK_SIZE];
int sp = 0;

void push(const char *label, int line_num) {
    if (sp >= STACK_SIZE) {
        printf("Stack overflow!\n");
        return;
    }

    if (sp == 0 && strcmp(label, "main") != 0) {
        strncpy(stack[sp].label, "main", sizeof(stack[sp].label) - 1);
        stack[sp].label[sizeof(stack[sp].label) - 1] = '\0';
        stack[sp].line_num = line_num;
        sp++;
    }

    strncpy(stack[sp].label, label, sizeof(stack[sp].label) - 1);
    stack[sp].label[sizeof(stack[sp].label) - 1] = '\0';
    stack[sp].line_num = line_num;
    sp++;
}

void pop() {
    if (sp == 0) {
        printf("Stack underflow!\n");
        StackEntry empty = {"", -1};
    }
        sp--;
}

uint64_t log2_(uint64_t value) {
    uint64_t result = 0;
    while (value >>= 1) {
        result++;
    }
    return result;
}

// Function to print the current state of the stack
void print_stack() {
	if (pc / 4 >= linecount && sp == 1 && strcmp(stack[0].label, "main") == 0) {
	        pop();
    }
    if (sp == 0) {
        printf("Empty Call Stack: Execution complete\n");
        printf("Exited the simulator\n");
        exit(0);
    } else {
        printf("Call Stack:\n");
        for (int i = 0; i < sp; i++) {
            printf("%s:%d\n", stack[i].label, stack[i].line_num);
        }
    }
}

// Function to check if an instruction is a label
int is_label_instruction(const char* instruction) {
    if (instruction == NULL) return 0;
    char *colon_pos = strchr(instruction, ':');
    if (colon_pos && colon_pos != instruction && isalpha(instruction[0])) {
        return 1;
    }

    return 0;
}

// Function to get the label name from an instruction
const char* get_label_from_instruction(const char* instruction) {
    static char label[MAX_LABEL_LENGTH];
    char *colon_pos = strchr(instruction, ':');
    if (colon_pos) {
        size_t label_length = colon_pos - instruction;
        strncpy(label, instruction, label_length);
        label[label_length] = '\0';
        return label;
    }

    return "";
}

void initialize_stack() {
    push("main", 0);
}

void invalidate_cache() {
    for (int i = 0; i < MAX_CACHE_SETS; i++) {
	        for (int j = 0; j < dcache.associativity; j++) {
	            dcache.sets[i].lines[j].valid = 0;
	            dcache.sets[i].lines[j].dirty = 0;
	            dcache.sets[i].lines[j].tag = 0;
                dcache.sets[i].lines[j].data = 0;
                dcache.sets[i].lines[j].last_access_time = 0;
	        }
    }
    printf("Cache invalidated.\n");
}

void initialize_cache(){
    cachestat=0;
	dcache.enabled = 0;
    dcache.cache_size = 0;
    dcache.block_size = 0;
    dcache.associativity = 0;
    dcache.replacement_policy[0] = '\0'; 
    dcache.write_policy[0] = '\0'; 
    dcache.access_count = 0;
    dcache.hit_count = 0;
    dcache.miss_count = 0;
	invalidate_cache();
}



void load_cache_config(const char *config_file) {

    FILE *filecache = fopen(config_file, "r");
    if (filecache == NULL) {
        printf("Error: Invalid cache config file.\n");
        return;
    }
	cachestat = true;
    // Load values directly into the dcache instance
    fscanf(filecache, "%d", &dcache.cache_size);
    fscanf(filecache, "%d", &dcache.block_size);
    fscanf(filecache, "%d", &dcache.associativity);
    fscanf(filecache, "%s", &dcache.replacement_policy);
    fscanf(filecache, "%s", &dcache.write_policy);

    dcache.enabled = 1; // Enable cache after loading configuration
    dcache.access_count = 0; // Initialize access count
    dcache.hit_count = 0; // Initialize hit count
    dcache.miss_count = 0; // Initialize miss count
	for (int i = 0; i < MAX_CACHE_SETS; i++) {
	        for (int j = 0; j < dcache.associativity; j++) {
	            dcache.sets[i].lines[j].valid = 0;
	            dcache.sets[i].lines[j].dirty = 0;
	            dcache.sets[i].lines[j].tag = 0;
	        }
    }
    fclose(filecache);
    printf("Cache simulation enabled.\n");
    invalidate_cache();
}

void get_index_and_tag(uint64_t address, int *index, unsigned long *tag) {
    *index = (address / dcache.block_size) % MAX_CACHE_SETS;
    *tag = address / (dcache.block_size * MAX_CACHE_SETS);
}


int cache_lookup(uint64_t address) {
    if (!dcache.enabled) return 0; // Cache disabled, treat as miss

    int index;
    unsigned long tag;
    get_index_and_tag(address, &index, &tag);

    CacheSet *set = &dcache.sets[index];
    dcache.access_count++;

    // Check for a matching line in the set
    for (int i = 0; i < dcache.associativity; i++) {
        if (set->lines[i].valid && set->lines[i].tag == tag) {
            dcache.hit_count++;
            return 1; // Hit
        }
    }

    dcache.miss_count++;
    //line->valid = 1;
    // line->tag = tag;
    return 0; // Miss
}

void display_cache_status() {
    if (cachestat) {
        printf("Cache Simulation Status: Enabled\n");
        printf("Cache Size: %d Bytes\n", dcache.cache_size);
        printf("Block Size: %d Bytes\n", dcache.block_size);
        printf("Associativity: %d-way\n", dcache.associativity);
        printf("Replacement Policy: %s\n", dcache.replacement_policy);
        printf("Write-back Policy: %s\n", dcache.write_policy);
    } else {
        printf("Cache Simulation Status: Disabled\n");
    }
}

void disable_cache() {
    cachestat = false;
    invalidate_cache();
    printf("Cache simulation disabled.\n");
}



void dump_cache1(const char *input_filename) {
    // Open the output file for writing
    FILE *file = fopen(input_filename, "w");
    if (file == NULL) {
        printf("Error: Unable to open file %s for writing.\n", input_filename);
        return;
    }

    // Calculate the number of sets in the D-cache
    int num_sets = dcache.cache_size / (dcache.block_size * dcache.associativity);

    // Iterate through each set in the D-cache in increasing order
    for (int i = 0; i < num_sets; i++) {
        CacheSet *set = &dcache.sets[i];

        // Iterate through each line in the set
        for (int j = 0; j < dcache.associativity; j++) {
            CacheLine *line = &set->lines[j];

            // Check if the line is valid
            if (line->valid) {
                // Write the valid entry to the file
               const char *status = line->dirty ? "Dirty" : "Clean";

                // Write the valid entry to the file
                fprintf(file, "Set: 0x%lx, Tag: 0x%lx, Status: %s\n",
                        (unsigned long)i, line->tag, status);
            }
        }
    }

    fclose(file);
    //printf("D-cache entries dumped in increasing order of set value\n");
}



void dump_cache(const char *input_filename) {
    char output_filename[256];

    // Copy the input filename and remove any file extension
    strncpy(output_filename, input_filename, sizeof(output_filename) - 1);
    output_filename[sizeof(output_filename) - 1] = '\0'; // Ensure null termination

    // Find the last '.' in the filename
    char *dot = strrchr(output_filename, '.');
    if (dot != NULL) {
        *dot = '\0'; // Remove the extension
    }

    // Append ".output" to create the new filename
    strncat(output_filename, ".output", sizeof(output_filename) - strlen(output_filename) - 1);

    // Print cache logs to console
    /*for (int i = 0; i < cachecount; i++) {
        printf("%s\n", cache_log[i]);
    }*/

    // Open the output file for writing
    FILE *file = fopen(output_filename, "w");
    if (file == NULL) {
        printf("Error: Unable to open file %s for writing.\n", output_filename);
        return;
    }

    // Write log entries to the file
    for (int i = 0; i < cachecount; i++) {
        fprintf(file, "%s", cache_log[i]);
    }

    fclose(file);
    printf("Cache logs dumped to %s\n", output_filename);
}

void show_cache_stats() {
    printf("D-cache statistics: Accesses=%d, Hit=%d, Miss=%d, Hit Rate=%.2f\n",
	dcache.access_count, dcache.hit_count, dcache.miss_count,
    (dcache.access_count > 0) ? (double)dcache.hit_count / dcache.access_count : 0.0);

}


uint64_t load_from_memory(uint64_t address, int size,int sign) {
    // Ensure address is within bounds
    if (address >= MEMORY_SIZE) {
        fprintf(stderr, "Memory access error: address 0x%lx is out of bounds\n", address);
        exit(EXIT_FAILURE); // Exit or handle error appropriately
    }

    uint64_t value = 0;
    switch (size) {
        case 1: // Byte
            if(sign){
            value = (int8_t)Memory[address];}
            else{
                value = (uint8_t)Memory[address];
                }
            break;
        case 2: // Halfword (16-bit)
            if(sign){
            value = (int16_t)(Memory[address] | (Memory[address + 1] << 8));}
            else{
                value=(uint16_t)(Memory[address] | (Memory[address + 1] << 8));
                }
            break;
        case 4: // Word (32-bit)
            if(sign){
            value = (int32_t)(Memory[address] |
			                                  (Memory[address + 1] << 8) |
			                                  (Memory[address + 2] << 16) |
			                                  (Memory[address + 3] << 24));}
            else{value = (uint32_t)(Memory[address] |
			                                  (Memory[address + 1] << 8) |
			                                  (Memory[address + 2] << 16) |
			                                  (Memory[address + 3] << 24));}
            break;
        case 8: // Double-word (64-bit)
            value = (uint64_t)(Memory[address] |
			                                   (Memory[address + 1] << 8) |
			                                   (Memory[address + 2] << 16) |
			                                   (Memory[address + 3] << 24) |
			                                   ((uint64_t)Memory[address + 4] << 32) |
			                                   ((uint64_t)Memory[address + 5] << 40) |
			                                   ((uint64_t)Memory[address + 6] << 48) |
			                                   ((uint64_t)Memory[address + 7] << 56));
            break;
        default:
            fprintf(stderr, "Unsupported load size: %d\n", size);
            exit(EXIT_FAILURE);
    }
    return value;
}

void write_to_memory(uint64_t address, int size, uint64_t value) {
    // Ensure address is within bounds
    if (address >= MEMORY_SIZE) {
        fprintf(stderr, "Memory access error: address 0x%lx is out of bounds\n", address);
        exit(EXIT_FAILURE); // Exit or handle error appropriately
    }

    switch (size) {
        case 1: // Byte
            Memory[address] = value & 0xFF;
            break;
        case 2: // Halfword (16-bit)
            Memory[address] = value & 0xFF;
            Memory[address + 1] = (value >> 8) & 0xFF;
            break;
        case 4: // Word (32-bit)
            Memory[address] = value & 0xFF;
            Memory[address + 1] = (value >> 8) & 0xFF;
            Memory[address + 2] = (value >> 16) & 0xFF;
            Memory[address + 3] = (value >> 24) & 0xFF;
            break;
        case 8: // Double-word (64-bit)
            Memory[address] = value & 0xFF;
            Memory[address + 1] = (value >> 8) & 0xFF;
            Memory[address + 2] = (value >> 16) & 0xFF;
            Memory[address + 3] = (value >> 24) & 0xFF;
            Memory[address + 4] = (value >> 32) & 0xFF;
            Memory[address + 5] = (value >> 40) & 0xFF;
            Memory[address + 6] = (value >> 48) & 0xFF;
            Memory[address + 7] = (value >> 56) & 0xFF;
            break;
        default:
            fprintf(stderr, "Unsupported write size: %d\n", size);
            exit(EXIT_FAILURE);
    }
}


void handle_load_instruction(uint64_t address, int size, uint64_t *register_dest,int sign) {
    if (!dcache.enabled) {
        // If cache is not enabled, load directly from memory

        return; // Exit since we've handled the load
    }
    if (dcache.enabled) {
        dcache.access_count++;

        int num_sets = dcache.cache_size / (dcache.block_size * dcache.associativity);
        int block_offset_bits = log2_(dcache.block_size); // e.g., 4 for 16 bytes
        int index_bits = log2_(num_sets); // e.g., 4 for 16 sets

        // Calculate set index
        uint64_t index = (address >> block_offset_bits) & (num_sets - 1); // Use bitwise AND

        // Calculate tag
        uint64_t tag = address >> (block_offset_bits + index_bits); // Shift out the block and index bits

        CacheSet *set = &dcache.sets[index];
        int hit = 0;

        // Check for cache hit
        for (int i = 0; i < dcache.associativity; i++) {
            if (set->lines[i].valid && set->lines[i].tag == tag) {
                hit = 1;
                dcache.hit_count++;
                set->lines[i].last_access_time = dcache.access_count; // Update LRU time

                *register_dest = set->lines[i].data; // Load data from cache line
                snprintf(cache_log[cachecount], MAX_LOG_LENGTH,
                         "R: Address: 0x%lx, Set: 0x%lx, Hit, Tag: 0x%lx, %s\n",
                         address, index, tag, set->lines[i].dirty ? "Dirty" : "Clean");
                cachecount++;
                return; // Exit on hit
            }
        }

        // Cache miss handling
        dcache.miss_count++;
        snprintf(cache_log[cachecount], MAX_LOG_LENGTH,
                 "R: Address: 0x%lx, Set: 0x%lx, Miss, Tag: 0x%lx, Clean\n",
                 address, index, tag);
        cachecount++;

        // Replace based on the selected policy
        CacheLine *line_to_replace = NULL;

        if (strcmp(dcache.replacement_policy, "FIFO") == 0) {
            static int fifo_index = 0; // Static to keep state between calls
            line_to_replace = &set->lines[fifo_index];
            fifo_index = (fifo_index + 1) % dcache.associativity; // Increment index for next FIFO replacement
        } else if (strcmp(dcache.replacement_policy, "LRU") == 0) {
            int lru_index = 0;
            int min_access_time = INT_MAX;

            // Find the LRU line
            for (int i = 0; i < dcache.associativity; i++) {
                if (set->lines[i].valid && set->lines[i].last_access_time < min_access_time) {
                    min_access_time = set->lines[i].last_access_time;
                    lru_index = i;
                }
            }
            line_to_replace = &set->lines[lru_index];
        } else if (strcmp(dcache.replacement_policy, "RANDOM") == 0) {
            int random_index = rand() % dcache.associativity; // Get a random index
            line_to_replace = &set->lines[random_index];
        }
		if (strcmp(dcache.write_policy, "WT") == 0) {
		            write_to_memory(load_from_memory(address, size, sign), size, tag);
		            //printf("Write-through: Data 0x%lx written to memory\n", load_from_memory(address, size, sign));
        }
        // Write-back if dirty and write-back policy is WB
        if (line_to_replace->valid && line_to_replace->dirty && strcmp(dcache.write_policy, "WB") == 0) {
            write_to_memory(line_to_replace->data,size, line_to_replace->tag);
        }

        // Load data into the chosen cache line
        if (line_to_replace) {
            line_to_replace->tag = tag; // Replace tag
            line_to_replace->valid = 1;  // Mark as valid
            line_to_replace->dirty = 0;  // Set to clean
            line_to_replace->data = load_from_memory(address, size,sign); // Load data from memory
            line_to_replace->last_access_time = dcache.access_count; // Set access time
            *register_dest = line_to_replace->data; // Load data into the register
            //printf("Data loaded into cache and register: 0x%lx\n", *register_dest);
        }
    }
}


void handle_store_instruction(uint64_t address, int size, uint64_t value) {
    if (!dcache.enabled) {
        // If cache is not enabled, store directly to memory
        write_to_memory(address, size, value);
        //printf("Stored directly to memory: 0x%lx at address 0x%lx\n", value, address);
        return; // Exit since we've handled the store
    }

    dcache.access_count++;

    // Calculate number of sets and indexing
    int num_sets = dcache.cache_size / (dcache.block_size * dcache.associativity);
    int block_offset_bits = log2_(dcache.block_size);
    int index_bits = log2_(num_sets);

    // Calculate set index and tag based on address
    uint64_t set_index = (address >> block_offset_bits) & ((1 << index_bits) - 1);
    uint64_t tag = address >> (block_offset_bits + index_bits);

    CacheSet *set = &dcache.sets[set_index];
    int hit = 0;

    // Check for hit in the cache
    for (int i = 0; i < dcache.associativity; i++) {
        if (set->lines[i].valid && set->lines[i].tag == tag) {
            hit = 1;
            dcache.hit_count++;
            set->lines[i].last_access_time = dcache.access_count;

            if (strcmp(dcache.write_policy, "WT") == 0) {
                // Write-through: write to both cache and memory
                set->lines[i].data = value;
                write_to_memory(address, size, value);
                //printf("Write-through: Stored data 0x%lx to cache and memory.\n", value);
            } else if (strcmp(dcache.write_policy, "WB") == 0) {
                // Write-back: only write to cache and mark as dirty
                set->lines[i].data = value;
                set->lines[i].dirty = 1;
                //printf("Write-back: Stored data 0x%lx to cache and marked as dirty.\n", value);
            }

            snprintf(cache_log[cachecount], MAX_LOG_LENGTH,
                     "W: Address: 0x%lx, Set: 0x%lx, Hit, Tag: 0x%lx, Dirty\n",
                     address, set_index, tag);
            cachecount++;
            return;
        }
    }

    // Miss handling
    dcache.miss_count++;
    snprintf(cache_log[cachecount], MAX_LOG_LENGTH,
             "W: Address: 0x%lx, Set: 0x%lx, Miss, Tag: 0x%lx, Clean\n",
             address, set_index, tag);
    cachecount++;

    if (strcmp(dcache.write_policy, "WT") == 0) {
        // WT miss with no write-allocate: write directly to memory only, no cache update
        write_to_memory(address, size, value);
        //printf("Write-through (miss): Stored data 0x%lx directly to memory.\n", value);
        return;
    }

    // For WB or WT with write-allocate, find a line to replace
    CacheLine *line_to_replace = NULL;

    if (strcmp(dcache.replacement_policy, "FIFO") == 0) {
        static int fifo_index = 0;
        fifo_index = fifo_index % dcache.associativity;
        line_to_replace = &set->lines[fifo_index];
        fifo_index = (fifo_index + 1) % dcache.associativity;

    } else if (strcmp(dcache.replacement_policy, "LRU") == 0) {
        int lru_index = 0;
        int min_access_time = INT_MAX;

        for (int i = 0; i < dcache.associativity; i++) {
            if (!set->lines[i].valid || set->lines[i].last_access_time < min_access_time) {
                min_access_time = set->lines[i].last_access_time;
                lru_index = i;
            }
        }
        line_to_replace = &set->lines[lru_index];

    } else if (strcmp(dcache.replacement_policy, "RANDOM") == 0) {
        int random_index = rand() % dcache.associativity;
        line_to_replace = &set->lines[random_index];
    }

    // Write-back if dirty and write-back policy is WB
    if (line_to_replace->valid && line_to_replace->dirty && strcmp(dcache.write_policy, "WB") == 0) {
        write_to_memory(line_to_replace->data, size, line_to_replace->tag);
    }

    // For write-back with write-allocate, load data into cache line and mark dirty
    line_to_replace->tag = tag;
    line_to_replace->valid = 1;
    line_to_replace->last_access_time = dcache.access_count;

    if (strcmp(dcache.write_policy, "WB") == 0) {
        line_to_replace->data = value;
        line_to_replace->dirty = 1;
        //printf("Write-back (miss): Stored data 0x%lx to cache and marked as dirty.\n", value);
    }
}







int main() {
    char command[MAX_COMMAND_LENGTH];
    char filename[256];
    while (1) {
        printf("> "); // Prompt for command
		scanf("%s", command);

        if (strcmp(command, "load") == 0) {

            scanf("%s", filename);
            load(filename);
            printf("\n");
        } else if (strcmp(command, "run") == 0) {
            run(filename);
            printf("\n");
        } else if (strcmp(command, "regs") == 0) {
            regs();
            printf("\n");
        } else if (strcmp(command, "exit") == 0) {
            exit_simulator();
            printf("\n");
        } else if (strcmp(command, "step") == 0) {
            runone();
            printf("\n");
        } else if (strcmp(command, "mem") == 0) {
            uint64_t addr, count;
            scanf("%llx %llu", (long long unsigned int *)&addr, (long long unsigned int *)&count);
            print_memory(addr, count);
            printf("\n");
        } else if (strcmp(command, "break") == 0) {
            if (numberbreak < MAX_BREAKPOINTS) {
                int break_line = 0;
                scanf("%d", &break_line);
                bool exists = false;
                for (int i = 0; i < numberbreak; i++) {
                    if (breakpoints[i] == break_line) {
                        exists = true;
                        break;
                    }
                }

                if (!exists) {
                    breakpoints[numberbreak] = break_line;
                    numberbreak++;
                    printf("Breakpoint set at line %d\n", break_line);
                    breakpoint = true;
                } else {
                    printf("Breakpoint already exists at line %d\n", break_line);
                }
            } else {
                printf("Already 5 breakpoints set.\n");
            }
			printf("\n");
        } else if (strcmp(command, "del") == 0) {
            char subcommand[256];
            scanf("%s", subcommand);
            if (strcmp(subcommand, "break") == 0) {
                int delbreak = 0;
                scanf("%d", &delbreak);
                bool found = false;
                for (int i = 0; i < numberbreak; i++) {
                    if (breakpoints[i] == delbreak) {
                        found = true;
                        for (int j = i; j < numberbreak - 1; j++) {
                            breakpoints[j] = breakpoints[j + 1];
                        }
                        breakpoints[numberbreak - 1] = 0;
                        numberbreak--;
                        printf("Breakpoint at line %d deleted.\n", delbreak);
                        if (numberbreak == 0) {
                            breakpoint = false;
                        }
                        break;
                    }
                }

                if (!found) {
                    printf("No breakpoint at line %d\n", delbreak);
                }
            }
            printf("\n");
        } else if (strcmp(command, "show-stack") == 0) {
                print_stack();
                printf("\n");
            }else if(strcmp(command, "cache_sim") == 0){
			char subcommand[256];
            scanf("%s", subcommand);
			if (strcmp(subcommand, "enable") == 0) {
			                char config_file[MAX_COMMAND_LENGTH];
			                scanf("%s", config_file);
			                load_cache_config(config_file);
			            } else if (strcmp(subcommand, "disable") == 0) {
			                disable_cache();
			            } else if (strcmp(subcommand, "status") == 0) {
			                display_cache_status();
			            } else if (strcmp(subcommand, "invalidate") == 0) {
			                invalidate_cache();
			            } else if (strcmp(subcommand, "dump") == 0) {
			                char filename1[MAX_COMMAND_LENGTH];
			                scanf("%s", filename1);
			                dump_cache1(filename1);
			            } else if (strcmp(subcommand, "stats") == 0) {
			                show_cache_stats();
            }
    else {
			        printf("Unknown cache_sim command!\n");
    	}
	}//cache
	else {
            printf("Unknown command!\n");
            printf("\n");
        }
    }

    return 0;
}



void load(const char *filename) {
	initialize_cache();
    reset_simulator();
    initialize_memory();
    initialize_stack();
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }
	char output_filename[256];
	    int num;
	    if (sscanf(filename, "input%d.s", &num) == 1) {
	        sprintf(output_filename, "output%d.hex", num);
	    } else {
	        fprintf(stderr, "Error: Unexpected input filename format\n");
	        fclose(file);
	        return;
    }
    char instruction[256];
    mem_address = 0;
	int memn=0;
    while (fgets(instruction, sizeof(instruction), file) != NULL) {
        instruction[strcspn(instruction, "\n")] = 0;
	if (strcmp(instruction,".data")==0){
	beforeinstr+=1;
	}
	else if (strcmp(instruction,".text")==0){
		beforeinstr+=1;
	}
        else if (strncmp(instruction, ".dword", 6) == 0) {
    uint64_t value;
    beforeinstr+=1;
    sscanf(instruction + 7, "%llu", (long long unsigned *)&value);
    uint64_t address = 0x10000 + memn;
    if (address + 7 < MEMORY_SIZE) {
        for (uint8_t i = 0; i < 8; i++) {
            uint8_t byte_to_store = (value >> (i * 8)) & 0xFF;
            Memory[address + i] = byte_to_store;
        }
        memn += 8; // Increase by 8 bytes for .dword
    } else {
        printf("Memory overflow when trying to store .dword\n");
    }
} else if (strncmp(instruction, ".word", 5) == 0) {
    uint32_t value;
    beforeinstr+=1;
    sscanf(instruction + 6, "%u", &value);
    size_t address = 0x10000 + memn;
    if (address + 3 < MEMORY_SIZE) {
        for (size_t i = 0; i < 4; i++) {
            uint8_t byte_to_store = (value >> (i * 8)) & 0xFF;
            Memory[address + i] = byte_to_store;
        }
        memn += 4; // Increase by 4 bytes for .word
    } else {
        printf("Memory overflow when trying to store .word\n");
    }
} else if (strncmp(instruction, ".half", 5) == 0) {
    uint16_t value;
    beforeinstr+=1;
    sscanf(instruction + 6, "%hu", &value);
    size_t address = 0x10000 + memn;
    if (address + 1 < MEMORY_SIZE) {
        for (size_t i = 0; i < 2; i++) {
            uint8_t byte_to_store = (value >> (i * 8)) & 0xFF;
            Memory[address + i] = byte_to_store;
        }
        memn += 2; // Increase by 2 bytes for .half
    } else {
        printf("Memory overflow when trying to store .half\n");
    }
} else if (strncmp(instruction, ".byte", 5) == 0) {
    uint8_t value;
    beforeinstr+=1;
    sscanf(instruction + 6, "%hhu", &value);
    size_t address = 0x10000 + memn;
    if (address < MEMORY_SIZE) {
        Memory[address] = value;
        memn += 1; // Increase by 1 byte for .byte
    } else {
        printf("Memory overflow when trying to store .byte\n");
    }
}
 else {
            if (mem_address >= INSTRUCTION_SIZE) {
                printf("Memory overflow!\n");
                break;
            }
            memory[mem_address] = (char*)malloc(strlen(instruction) + 1);
            if (memory[mem_address] == NULL) {
                perror("Memory allocation failed");
                return;
            }
            strcpy(memory[mem_address++], instruction);
        }
    }
    fclose(file);


    FILE *fileo = fopen(output_filename, "r");
		    if (fileo == NULL) {
		        perror("Error opening file");
		        return;
		    }
		    char line[256];

		    while (fgets(line, sizeof(line), fileo) != NULL && linecount < INSTRUCTION_SIZE) {
		        uint64_t code;
		        if (sscanf(line, "%lx", &code) != 1) {
		            printf("Invalid instruction format: %s", line);
		            continue;
		        }
		        memorybin[linecount++] = code;
		        uint64_t address = linecount * sizeof(uint32_t) - sizeof(uint32_t);

				        // Store each byte of the instruction directly in memory
				        if (address < MEMORY_SIZE - 3) {
				            Memory[address] = code & 0xFF;
				            Memory[address + 1] = (code >> 8) & 0xFF;
				            Memory[address + 2] = (code >> 16) & 0xFF;
				            Memory[address + 3] = (code >> 24) & 0xFF;
        				}
		    }
    fclose(fileo);

    pc = 0x0;
    //printf("%d\n",beforeinstr);
    // Set PC to the start of the program
    /*printf("Loaded instructions from file: %s\n", filename);

    for (int i = 0; i < mem_address; i++) {
        printf("Loaded instruction: %s\n", memory[i]); // Print instructions
    }*/
}


void run(const char *filename) {
    while (pc / 4 < linecount) {  // pc is byte address, so divide by 4 for instruction index
        uint64_t current_inst = memorybin[pc / 4];
        current_line_num = pc / 4;
        const char* current_instruction = memory[pc / 4];

        // Check if the instruction contains a label
        if (is_label_instruction(current_instruction)) {
            const char* label_name = get_label_from_instruction(current_instruction);
            //printf("yes label\n");

            if (sp == 0 || strcmp(stack[sp - 1].label, label_name) != 0) {
                if (pc / 4 > 0) {
                    uint64_t prev_inst = memorybin[(pc / 4) - 2];
                    //printf("Previous instruction: 0x%08lx\n", prev_inst);

                    // If the previous instruction is `jal` (opcode: 0x6F)
                    if ((prev_inst & 0x7F) == 0x6F) {
                        //printf("Previous instruction is jal\n");
                        push(label_name, current_line_num);  // Push label only if it's a function
                    }
                }
            }
            else if(strcmp(stack[sp-1].label,label_name)==0){
				if (pc / 4 > 0) {
				                    uint64_t prev_inst = memorybin[(pc / 4) - 2];
				                    //printf("Previous instruction: 0x%08lx\n", prev_inst);

				                    // If the previous instruction is `jal` (opcode: 0x6F)
				                    if ((prev_inst & 0x7F) == 0x6F) {
				                        //printf("Previous instruction is jal\n");
				                        push(label_name, current_line_num);  // Push label only if it's a function
				                    }
                }
			}
        }

        if (sp > 0) {
            stack[sp - 1].line_num=current_line_num+1;
        }

        execute(current_inst, current_instruction);

        if ((current_inst & 0x7F) == 0x67) {
            pop();  // Pop the function return off the stack
            	//print_stack();
        }

        step++;
        if (breakpoint) {
            for (int i = 0; i < numberbreak; i++) {
                if (current_line_num+2 == breakpoints[i]) {
                    printf("Execution stopped at breakpoint\n");
                    is_paused = true;
                    for (int j = i; j < numberbreak - 1; j++) {
                        breakpoints[j] = breakpoints[j + 1];
                    }
                    numberbreak--;
                    return;
                }
            }
        }

    }
    if(cachestat){
    show_cache_stats();
    dump_cache(filename);
        }
}

void runone() {
	step++;
    while (1) {
        if (breakpoint) {
		            for (int i = 0; i < numberbreak; i++) {
		                if (current_line_num+2== breakpoints[i]) {
		                    printf("Execution stopped at breakpoint\n");
		                    is_paused = true;
		                    for (int j = i; j < numberbreak - 1; j++) {
							                        breakpoints[j] = breakpoints[j + 1];
		                    }
		                    numberbreak--;
		                    return;
		                }
		            }
        }

        uint64_t current_inst = memorybin[pc / 4];
		        current_line_num = pc / 4;
		        const char* current_instruction = memory[pc / 4];


		        if (is_label_instruction(current_instruction)) {
		            const char* label_name = get_label_from_instruction(current_instruction);
		            //printf("yes label\n");

		            if (sp == 0 || strcmp(stack[sp - 1].label, label_name) != 0) {
		                if (pc / 4 > 0) {
		                    uint64_t prev_inst = memorybin[(pc / 4) - 2];
		                    //printf("Previous instruction: 0x%08lx\n", prev_inst);

		                    if ((prev_inst & 0x7F) == 0x6F) {
		                        //printf("Previous instruction is jal\n");
		                        push(label_name, current_line_num);
		                    }
		                }
		            }
		            else if(strcmp(stack[sp-1].label,label_name)==0){
									if (pc / 4 > 0) {
									                    uint64_t prev_inst = memorybin[(pc / 4) - 2];
									                    //printf("Previous instruction: 0x%08lx\n", prev_inst);

									                    // If the previous instruction is `jal` (opcode: 0x6F)
									                    if ((prev_inst & 0x7F) == 0x6F) {
									                        //printf("Previous instruction is jal\n");
									                        push(label_name, current_line_num);  // Push label only if it's a function
									                    }
					                }
					}
		        }

		        if (sp > 0) {
		            stack[sp - 1].line_num=current_line_num+1;
        }
        if (!current_inst) {
            printf("Nothing to step\n");
            break;
        }


        execute(current_inst, current_instruction);

        if ((current_inst & 0x7F) == 0x67) {
            pop();
            	//print_stack();
        }

        break;
    }


}







void execute(uint64_t bininstr, const char* instr) {
    uint8_t opcode = bininstr & 0x7F; // Last 7 bits
    //printf("Opcode: 0x%02x\n", opcode); // Debugging output

    switch (opcode) {
        case 0x33: { // R-Type
            			uint64_t rd = (bininstr >> 7) & 0x1F; // Extract destination register
			            uint64_t funct3 = (bininstr >> 12) & 0x07; // Extract funct3
			            uint64_t rs1 = (bininstr >> 15) & 0x1F; // Extract source register 1
			            uint64_t rs2 = (bininstr >> 20) & 0x1F; // Extract source register 2
			            uint64_t funct7 = (bininstr >> 25) & 0x7F; // Extract funct7

			            // Fetch values of rs1 and rs2 from the register file
			            uint64_t value_rs1 = registers[rs1];
			            uint64_t value_rs2 = registers[rs2];
						if (rd == 0) {
								printf("Executed %s; PC=0x%08lx\n", instr, pc);
								pc+=4;
						        return;
   						 }

			            switch (funct3) {
			                case 0x00: // ADD
			                    if (funct7 == 0x00) {
			                        registers[rd] = value_rs1 + value_rs2;
			                        printf("Executed %s; PC=0x%08lx\n", instr, pc);
			                    }

			                	else if (funct7 == 0x20) { //SUB
			                        registers[rd] = value_rs1 - value_rs2;
			                        printf("Executed %s; PC=0x%08lx\n", instr, pc);
			                    }
			                    pc+=4;
			                    break;
							case 0x01: // SLL (Shift Left Logical)
							    registers[rd] = registers[rs1] << registers[rs2];
							        printf("Executed %s; PC=0x%08lx\n", instr, pc);

							    pc+=4;
							    break;
							case 0x02: // SLT (Set Less Than)
							    registers[rd] = (registers[rs1] < registers[rs2]) ? 1 : 0;
							         printf("Executed %s; PC=0x%08lx\n", instr, pc);

							    pc+=4;
							    break;
							case 0x03: // SLTU (Set Less Than Unsigned)
							    registers[rd] = ((uint64_t)registers[rs1] < (uint64_t)registers[rs2]) ? 1 : 0;
							          printf("Executed %s; PC=0x%08lx\n", instr, pc);

							    pc+=4;
								break;
							case 0x04: // XOR
							    if (funct7 == 0x00) {
							          registers[rd] = registers[rs1] ^ registers[rs2];
							          printf("Executed %s; PC=0x%08lx\n", instr, pc);
							    }
							    pc+=4;
							     break;
							case 0x05: // SRL (Shift Right Logical)
							    registers[rd] = registers[rs1] >> registers[rs2];
							          printf("Executed %s; PC=0x%08lx\n", instr, pc);
								pc+=4;
							    break;
							case 0x20: // SRA (Shift Right Arithmetic)
							     registers[rd] = (int64_t)registers[rs1] >> registers[rs2];
							     printf("Executed %s; PC=0x%08lx\n", instr, pc);
							    pc+=4;
							    break;
							case 0x06: // OR
							    if (funct7 == 0x00) {
							          registers[rd] = registers[rs1] | registers[rs2];
							          printf("Executed %s; PC=0x%08lx\n", instr, pc);
							     }
							     pc+=4;
							    break;
							case 0x07: // AND
							     if (funct7 == 0x00) {
							           registers[rd] = registers[rs1] & registers[rs2];
							           printf("Executed %s; PC=0x%08lx\n", instr, pc);
							     }
							     pc+=4;
                    			 break;
			                default:
			                    printf("Unknown R-Type funct3: 0x%02lx\n", funct3);
			                    break;
			            }
			            break;
        }

        case 0x13: { // I-Type
		            uint64_t rd = (bininstr >> 7) & 0x1F; // Extract destination register
		            uint64_t funct3 = (bininstr >> 12) & 0x07; // Extract funct3
		            uint64_t rs1 = (bininstr >> 15) & 0x1F; // Extract source register 1
		            int64_t imm = (bininstr >> 20) & 0xFFF; // Extract immediate value
					if (rd == 0) {
						printf("Executed %s; PC=0x%08lx\n", instr, pc);
						pc+=4;
							return;
   						 }
		            // Check for sign extension of the immediate
		            if (imm & 0x800) { // If sign bit is set
		                imm |= 0xFFFFFFFFFFFFF000; // Sign extend to 64 bits
		            }

		            // Fetch value of rs1 from the register file
		            uint64_t value_rs1 = registers[rs1];

		            switch (funct3) {
		                case 0x00: // ADDI
		                    registers[rd] = value_rs1 + imm; // Update rd with rs1 + immediate
		                    printf("Executed %s; PC=0x%08lx\n", instr, pc);
		                    pc+=4;
		                    break;
		                case 0x04: // XORI
						     registers[rd] = registers[rs1] ^ imm;
						     printf("Executed %s; PC=0x%08lx\n", instr, pc);
						     pc+=4;
						     break;
						case 0x06: // ORI
						     registers[rd] = registers[rs1] | imm;
						     printf("Executed %s; PC=0x%08lx\n", instr, pc);
						     pc+=4;
						     break;
						case 0x07: // ANDI
						     registers[rd] = registers[rs1] & imm;
						     printf("Executed %s; PC=0x%08lx\n", instr, pc);
						     pc+=4;
						     break;
						case 0x01: // SLLI

						        registers[rd] = registers[rs1] << (imm & 0x3F); // Get lower 6 bits
						        printf("Executed %s; PC=0x%08lx\n", instr, pc);

						     pc+=4;
						     break;
						case 0x05: // SRLI
						          registers[rd] = registers[rs1] >> (imm & 0x3F);
						           printf("Executed %s; PC=0x%08lx\n", instr, pc);

						      pc+=4;
						      break;
						 case 0x02: // SLTI
						      registers[rd] = (registers[rs1] < imm) ? 1 : 0; // Set if less than
						      printf("Executed %s; PC=0x%08lx\n", instr, pc);
						      pc+=4;
                    		  break;
		                default:
		                    printf("Unknown I-Type funct3: 0x%02lx\n", funct3);
		                    pc+=4;
		                    break;
		            }
		            break;
		        }

		    case 0x03: { // Load
			    uint64_t rd = (bininstr >> 7) & 0x1F; // Destination register
			    uint64_t funct3 = (bininstr >> 12) & 0x07; // funct3
			    uint64_t rs1 = (bininstr >> 15) & 0x1F; // Source register 1
			    int64_t imm = (bininstr >> 20) & 0xFFF; // Immediate value
				if (rd == 0) {
					printf("Executed %s; PC=0x%08lx\n", instr, pc);
					pc+=4;
										        return;
   						 }
			    // Sign extend immediate if necessary
			    if (imm & 0x800) {
			        imm |= 0xFFFFFFFFFFFFF000; // Sign extend to 64 bits
			    }

			    // Calculate effective address
			    uint64_t address = (uint64_t)registers[rs1] + imm;

			    // Check if the address is within bounds
					if (address < TEXT_START || address >= MEMORY_SIZE) {
						printf("Load Address out of bounds: 0x%016lx\n", address);
						pc += 4;
						break;
					}

			    // This will hold the value we load
			    uint64_t loadValue = 0;
			    switch (funct3) {
			        case 0x00: // LB (Load Byte)
			            //loadValue = (int8_t)Memory[address]; // Load 1 byte
    					handle_load_instruction(address,1, &loadValue,1);
    					if (!dcache.enabled) {
							loadValue = (int8_t)Memory[address]; // Load 1 byte
							}
			            registers[rd] = loadValue;
			            printf("Executed %s; PC=0x%08lx\n", instr, pc);
			            pc += 4;
			            break;

			        case 0x01: // LH (Load Halfword)
			            //loadValue = (int16_t)(Memory[address] | (Memory[address + 1] << 8)); // Load 2 bytes
    handle_load_instruction(address,2, &loadValue,1);
    					if (!dcache.enabled) {
    					loadValue = (int16_t)(Memory[address] | (Memory[address + 1] << 8)); // Load 2 bytes
					}
			            registers[rd] = loadValue;
			            printf("Executed %s; PC=0x%08lx\n", instr, pc);
			            pc += 4;
			            break;

			        case 0x02: // LW (Load Word)
			            /*loadValue = (int32_t)(Memory[address] |
			                                  (Memory[address + 1] << 8) |
			                                  (Memory[address + 2] << 16) |
			                                  (Memory[address + 3] << 24)); // Load 4 bytes*/
    handle_load_instruction(address,4, &loadValue,1);
    					if (!dcache.enabled) {
    					loadValue = (int32_t)(Memory[address] |
				                                  (Memory[address + 1] << 8) |
				                                  (Memory[address + 2] << 16) |
			                                  (Memory[address + 3] << 24)); // Load 4 bytes
										  }
			            registers[rd] = loadValue;
			            printf("Executed %s; PC=0x%08lx\n", instr, pc);
			            pc += 4;
			            break;

			        case 0x03: // LD (Load Doubleword)
			            /*loadValue = (uint64_t)(Memory[address] |
			                                   (Memory[address + 1] << 8) |
			                                   (Memory[address + 2] << 16) |
			                                   (Memory[address + 3] << 24) |
			                                   ((uint64_t)Memory[address + 4] << 32) |
			                                   ((uint64_t)Memory[address + 5] << 40) |
			                                   ((uint64_t)Memory[address + 6] << 48) |
			                                   ((uint64_t)Memory[address + 7] << 56)); // Load 8 bytes*/
    					handle_load_instruction(address,8, &loadValue,1);
    					if (!dcache.enabled) {
    					loadValue = (uint64_t)(Memory[address] |
									                                   (Memory[address + 1] << 8) |
									                                   (Memory[address + 2] << 16) |
									                                   (Memory[address + 3] << 24) |
									                                   ((uint64_t)Memory[address + 4] << 32) |
									                                   ((uint64_t)Memory[address + 5] << 40) |
									                                   ((uint64_t)Memory[address + 6] << 48) |
			                                   ((uint64_t)Memory[address + 7] << 56));
										   }
			            registers[rd] = loadValue;
			            printf("Executed %s; PC=0x%08lx\n", instr, pc);
			            pc += 4;
			            break;

			        case 0x04: // LBU (Load Byte Unsigned)
			            //loadValue = (uint8_t)Memory[address]; // Load 1 byte unsigned
    handle_load_instruction(address,1, &loadValue,0);
    if (!dcache.enabled) {
    loadValue = (uint8_t)Memory[address];}
			            registers[rd] = loadValue;
			            printf("Executed %s; PC=0x%08lx\n", instr, pc);
			            pc += 4;
			            break;

			        case 0x05: // LHU (Load Halfword Unsigned)
			            //loadValue = (uint16_t)(Memory[address] | (Memory[address + 1] << 8)); // Load 2 bytes unsigned

    handle_load_instruction(address,2, &loadValue,0);
    if (!dcache.enabled) {
	loadValue = (uint16_t)(Memory[address] | (Memory[address + 1] << 8));
	}

			            registers[rd] = loadValue;
			            printf("Executed %s; PC=0x%08lx\n", instr, pc);
			            pc += 4;
			            break;

			        case 0x06: // LWU (Load Word Unsigned)
			            /*loadValue = (uint32_t)(Memory[address] |
			                                   (Memory[address + 1] << 8) |
			                                   (Memory[address + 2] << 16) |
			                                   (Memory[address + 3] << 24)); // Load 4 bytes unsigned*/

    handle_load_instruction(address,4, &loadValue,0);
    if (!dcache.enabled) {
    loadValue = (uint32_t)(Memory[address] |
				                                   (Memory[address + 1] << 8) |
				                                   (Memory[address + 2] << 16) |
			                                   (Memory[address + 3] << 24));
			            registers[rd] = loadValue;
					}
			            printf("Executed %s; PC=0x%08lx\n", instr, pc);
			            pc += 4;
			            break;

			        default:
			            printf("Unknown Load funct3: 0x%02lx; PC=0x%08lx\n", funct3, pc);
			            pc += 4;
			            break;
			    }
			    break;
			}






			case 0x23: { // S-Type
			    int64_t imm = ((bininstr >> 25) & 0x7F) << 5 | ((bininstr >> 7) & 0x1F); // Immediate value
			    if (imm & 0x800) { // Check sign bit for immediate
			        imm |= 0xFFFFFFFFFFFFF000; // Sign-extend to 64 bits
			    }

			    uint64_t funct3 = (bininstr >> 12) & 0x07; // funct3
			    uint64_t rs1 = (bininstr >> 15) & 0x1F; // Source register 1
			    uint64_t rs2 = (bininstr >> 20) & 0x1F; // Source register 2

			    uint64_t address = registers[rs1] + imm; // Calculate effective address
			    if (address < DATA_START || address >= MEMORY_SIZE) {
				    printf("Store Address out of bounds: 0x%08lx\n", address);
				    pc += 4;
				    break;
				}

			    uint64_t value_rs2 = registers[rs2]; // Load the value from rs2 for storage
			    //printf("value_rs2 = 0x%016llx\n", value_rs2);


			    switch (funct3) {
			        case 0x00: // SB (Store Byte)
			            //Memory[address] = value_rs2 & 0xFF; // Store 1 byte
                        handle_store_instruction(address,1, value_rs2);
			            printf("Executed %s; PC=0x%08lx\n", instr, pc);
			            pc += 4;
			            break;

			        case 0x01: // SH (Store Halfword)
			            /*Memory[address] = value_rs2 & 0xFF; // Store 1st byte
			            Memory[address + 1] = (value_rs2 >> 8) & 0xFF; // Store 2nd byte*/
                        handle_store_instruction(address,2, value_rs2);
			            printf("Executed %s; PC=0x%08lx\n", instr, pc);
			            pc += 4;
			            break;

			        case 0x02: // SW (Store Word)
			            /*Memory[address] = value_rs2 & 0xFF; // Store 1st byte
			            Memory[address + 1] = (value_rs2 >> 8) & 0xFF; // Store 2nd byte
			            Memory[address + 2] = (value_rs2 >> 16) & 0xFF; // Store 3rd byte
			            Memory[address + 3] = (value_rs2 >> 24) & 0xFF; // Store 4th byte*/
                        handle_store_instruction(address, 4,value_rs2);
			            printf("Executed %s; PC=0x%08lx\n", instr, pc);
			            pc += 4;
			            break;

			        case 0x03: // SD (Store Doubleword)
			            /*Memory[address] = value_rs2 & 0xFF;
			            Memory[address + 1] = (value_rs2 >> 8) & 0xFF;
			            Memory[address + 2] = (value_rs2 >> 16) & 0xFF;
			            Memory[address + 3] = (value_rs2 >> 24) & 0xFF;
			            Memory[address + 4] = (value_rs2 >> 32) & 0xFF;
			            Memory[address + 5] = (value_rs2 >> 40) & 0xFF;
			            Memory[address + 6] = (value_rs2 >> 48) & 0xFF;
			            Memory[address + 7] = (value_rs2 >> 56) & 0xFF;*/
                        handle_store_instruction(address,8, value_rs2);
			            printf("Executed %s; PC=0x%08lx\n", instr, pc);
						pc += 4;
			            break;

			        default:
			            printf("Unknown Store funct3: 0x%02lx\n", funct3);
			            pc += 4;
			            break;
			    }
			    break;
			}




        		case 0x63: { // B-Type
				    // Extracting bits correctly
				    unsigned int imm_12 = (bininstr >> 31) & 0x1;   // imm[12]
				    unsigned int imm_11 = (bininstr >> 7) & 0x1;    // imm[11]
				    unsigned int imm_10_5 = (bininstr >> 25) & 0x3F; // imm[10:5]
				    unsigned int imm_4_1 = (bininstr >> 8) & 0xF;    // imm[4:1]

				    // Reconstruct the immediate value
				    uint64_t imm = ((imm_12 & 0x1) << 12) |      // imm[12]
					                  ((imm_11 & 0x1) << 11) |      // imm[11]
					                  ((imm_10_5 & 0x3F) << 5) |    // imm[10:5]
                  						((imm_4_1 & 0xF) << 1);
				    //printf("Immediate calculation: imm=0x%lx\n", imm);

				    // Sign extend the immediate value if necessary
				    if (imm & 0x1000) { // Check if the sign bit (bit 12) is set
				        imm |= 0xFFFFFFFFFFFFE000; // Extend to 64 bits for negative values
				    }

				    //printf("Sign extended imm: 0x%lx\n", imm);

				    uint64_t funct3 = (bininstr >> 12) & 0x07; // Extract funct3
				    uint64_t rs1 = (bininstr >> 15) & 0x1F; // Extract source register 1
				    uint64_t rs2 = (bininstr >> 20) & 0x1F; // Extract source register 2

				    int64_t value_rs1 = registers[rs1];
   					 int64_t value_rs2 = registers[rs2];

				            switch (funct3) {
				                case 0x00: // BEQ (Branch Equal)
				                    if (value_rs1 == value_rs2) {

				                        printf("Executed %s; PC=0x%08lx\n", instr, pc);
				                        pc += imm; // Update PC
				                        break;
				                    }
				                    printf("Executed %s; PC=0x%08lx\n", instr, pc);
				                   pc+=4;
				                    break;
				                case 0x01: // BNE (Branch Not Equal)
				                    if (value_rs1 != value_rs2) {

				                        printf("Executed %s; PC=0x%08lx\n", instr, pc);
				                        pc += imm; // Update PC
				                        break;
				                    }
				                    printf("Executed %s; PC=0x%08lx\n", instr, pc);
				                    pc+=4;
				                    break;
				                case 0x04: // BLT (Branch Less Than)
				                    if (value_rs1 < value_rs2) {

				                        printf("Executed %s; PC=0x%08lx\n", instr, pc);
				                        pc += imm; // Update PC
				                        break;
				                    }
				                    printf("Executed %s; PC=0x%08lx\n", instr, pc);
				                    pc+=4;
				                    break;
				                case 0x05: // BGE (Branch Greater Than or Equal)
				                    if (value_rs1 >= value_rs2) {

				                        printf("Executed %s; PC=0x%08lx\n", instr, pc);
				                        pc += imm; // Update PC
				                        break;
				                    }
				                    printf("Executed %s; PC=0x%08lx\n", instr, pc);
				                    pc+=4;
				                    break;
				                case 0x06: // BLTU (Branch Less Than Unsigned)
				                    if ((uint64_t)value_rs1 < (uint64_t)value_rs2) {

				                        printf("Executed %s; PC=0x%08lx\n", instr, pc);
				                        pc += imm; // Update PC
				                        break;
				                    }
				                    printf("Executed %s; PC=0x%08lx\n", instr, pc);
				                    pc+=4;
				                    break;
				                case 0x07: // BGEU (Branch Greater Than or Equal Unsigned)
				                    if ((uint64_t)value_rs1 >= (uint64_t)value_rs2) {

				                        printf("Executed %s; PC=0x%08lx\n", instr, pc);
				                        pc += imm; // Update PC
				                        break;
				                    }
				                    printf("Executed %s; PC=0x%08lx\n", instr, pc);
				                   pc+=4;
				                    break;
				                default:
				                    printf("Unknown B-Type funct3: 0x%02lx\n", funct3);
				                    pc+=4;
				                    break;
				            }
				            break;
        		}
        		case 0x6F: { // J-Type (JAL)
				            uint64_t rd = (bininstr >> 7) & 0x1F; // Extract destination register
				            uint64_t imm = ((bininstr >> 31) & 0x1) << 20 | // 1 bit
				                            ((bininstr >> 12) & 0xFF) << 12 | // 8 bits
				                            ((bininstr >> 20) & 0x1) << 11 | // 1 bit
				                            ((bininstr >> 21) & 0x3FF) << 1; // 10 bits
							if (imm & (1 << 20)) {
        						imm |= 0xFFFFFFFFFFF00000;  // Sign extension to 64 bits
    						}
				            // Save the return address (PC + 4)
				            registers[rd] = pc + 4;


				            // Update the PC with the immediate value

				            printf("Executed %s; PC=0x%08lx\n", instr, pc);
				            pc += imm; // Update PC
				            break;
        		}
        		case 0x67: { // I-Type (JALR)
				    uint64_t rd = (bininstr >> 7) & 0x1F;
				    uint64_t rs1 = (bininstr >> 15) & 0x1F;
				    int64_t imm = (int64_t)((int32_t)(bininstr & 0xFFF00000) >> 20);

				    if(rd!=0){
				    registers[rd] = pc + 4;
				    printf("JALR: Saving return address to x%lu = 0x%016lx\n", rd, registers[rd]);
					}


				    printf("Executed %s; PC=0x%08lx\n", instr, pc);
				    pc = (registers[rs1] + imm) & ~0x1;  // The PC must be aligned (even)
				    break;
				}

        		case 0x37: { // U-Type (LUI)
				            uint64_t rd = (bininstr >> 7) & 0x1F;
				            uint64_t imm = (bininstr >> 12) & 0xFFFFF;
							if (rd == 0) {
								printf("Executed %s; PC=0x%08lx\n", instr, pc);
								pc+=4;
													        return;
   						 }
				            registers[rd] = imm << 12;
				            //printf("LUI: Loading upper immediate into x%lu = 0x%016lx\n", rd, registers[rd]);

				            printf("Executed %s; PC=0x%08lx\n", instr, pc);
				            pc+=4;
				            break;
        		}
        		default:
				     printf("Unknown opcode: 0x%02x\n", opcode);
				     pc+=4;
            		 break;
}
}




void regs() {
	printf("Registers:\n");
    for (int i = 0; i < NUM_REGISTERS; i++) {
	        if (registers[i] == 0) {
	            printf("x%d  = 0x0\n", i);
	        } else {
	            printf("x%d  = 0x%016llx\n", i, (long long unsigned)registers[i]);
	        }
    }
}

void reset_simulator() {
    beforeinstr=0;
    linecount=0;
    for (int i = 0; i < NUM_REGISTERS; i++) {
        registers[i] = 0;
    }
    for (int i = 0; i < INSTRUCTION_SIZE; i++) {
        free(memory[i]);
        memory[i] = NULL;
    }
    pc = 0x0;
    step=0;
}

void exit_simulator() {
    printf("Exited the simulator\n");
    reset_simulator();
    exit(0);
}