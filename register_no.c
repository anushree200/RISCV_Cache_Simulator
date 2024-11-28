#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "register_no.h"

int get_register_number(const char *reg)
{
    // Register name and their corresponding numbers
    struct
    {
        const char *name;
        int number;
    } reg_map[] = {
        {"zero", 0},
        {"x0", 0},
        {"x1", 1},
        {"x2", 2},
        {"x3", 3},
        {"x4", 4},
        {"x5", 5},
        {"x6", 6},
        {"x7", 7},
        {"x8", 8},
        {"x9", 9},
        {"x10", 10},
        {"x11", 11},
        {"x12", 12},
        {"x13", 13},
        {"x14", 14},
        {"x15", 15},
        {"x16", 16},
        {"x17", 17},
        {"x18", 18},
        {"x19", 19},
        {"x20", 20},
        {"x21", 21},
        {"x22", 22},
        {"x23", 23},
        {"x24", 24},
        {"x25", 25},
        {"x26", 26},
        {"x27", 27},
        {"x28", 28},
        {"x29", 29},
        {"x30", 30},
        {"x31", 31},
        {"t0", 5},
        {"t1", 6},
        {"t2", 7},
        {"t3", 28},
        {"t4", 29},
        {"t5", 30},
        {"t6", 31},
        {"s0", 8},
        {"s1", 9},
        {"s2", 18},
        {"s3", 19},
        {"s4", 20},
        {"s5", 21},
        {"s6", 22},
        {"s7", 23},
        {"s8", 24},
        {"s9", 25},
        {"s10", 26},
        {"s11", 27},
        {"a0", 10},
        {"a1", 11},
        {"a2", 12},
        {"a3", 13},
        {"a4", 14},
        {"a5", 15},
        {"a6", 16},
        {"a7", 17},
        {"sp", 2},
        {"ra", 1},
        {"gp", 3},
        {"tp", 4},
    };
    size_t num_regs = sizeof(reg_map) / sizeof(reg_map[0]);

    // Loop through the register map to find a match
    for (size_t i = 0; i < num_regs; i++)
    {
        if (strcmp(reg, reg_map[i].name) == 0)
        {
            return reg_map[i].number; // Return the corresponding register number
        }
    }
    return -1;
}
