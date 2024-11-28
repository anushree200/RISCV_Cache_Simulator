# RISCV-Cache Simulator


## CONTENTS 
This file has been divided into the following sections for easy reference:
- Project Description
- What the Application Does
- Technologies Used
- Hardware requirements to run this program.
- Operating System and its requirements.
- Installation of RIPES Simulator
- Installation of the C compiler
- Contents of the folder
- About the code
- Executing the program
- Prerequisite theory
- Getting Help



## Project Description
This project is an extension of a RISC-V simulator built in C, enhanced with cache simulator 
for D-cache. It supports the execution and debugging of RISC-V assembly code, providing 
detailed simulation of cache behavior. The simulator reads assembly instructions from a file, 
simulates execution with 64-bit registers and memory (including the stack), and tracks the 
program counter (PC) in real-time. The following cache-specific commands extend the 
simulator’s functionality: 
- cache_sim enable <configuration file> : Enables cache simulation using settings 
from the specified configuration file
- cache_sim disable : Disables cache simulation (default state).
- cache_sim status : Displays the cache simulation status and configuration details if enabled.
- cache_sim invalidate : Clears all entries in the D-cache.
- cache_sim dump myFile.out : Exports valid D-cache entries to a file in a specified format.
- cache_sim stats : Shows current cache statistics, including access count, hits, misses, and hit rate.




## What the Application Does
The RISC-V Cache Simulator extends a RISC-V simulator to provide detailed cache-level 
operations, enhancing insight into data caching during program execution. It simulates a D
cache (data cache) to observe cache interactions in real-time, allowing users to enable, 
disable, configure, and monitor cache behavior. Key functionalities include:
- Cache Configuration and Management: Reads cache configuration settings from a file to customize cache size, block size, associativity, and policies (e.g., replacement and write policies).
- Cache Status and Statistics: Displays cache status, including configuration details, current enable/disable state, and real-time statistics, such as cache hits, misses, and hit rates.
- Cache Operations: Enables commands to invalidate cache entries, dump current cache contents to a file, and update cache states dynamically during simulation


## Technologies Used
- C Language: Used for writing the assembler code to convert assembly instructions into machine code.
- RISC-V Instruction Set Architecture: The target architecture for which the assembly code is written and translated.



## Hardware requirements to compile and run this program
- A good processor with a minimum quantity ram and a minimum harddisk space is enough to run this program. 
Actually this program will not consume much of memory and the harddisk space.
- Monitor to see the output
- Keyboard to compile and execute the code is required



## Operating System and its requirements
This code can run in any operating system as long as that operating system has a C compiler installed.
- The gcc compiler was used during the development phase.
- The RIPES Simulator can be installed to check the accuracy of the translated machine code.



## Installation of RIPES Simulator
This is required only for you to check the hexadecimal machine code, if its correct.
Ripes is a visual computer architecture simulator built around the RISC-V ISA.
As we have been adviced to use RIPES as the assembly code editor, there are also other code editors like VSCode you can use.

To install RIPES, click on the [RIPES](https://github.com/mortbopet/Ripes/releases)

Under the ASSETS Section of the recent version, select the package to be installed based on the OS you are working on.



## Installation of the C Compiler
This code was written in C and the Windows operating system was used during the development. 
MinGW (Minimalist GNU for Windows) freeware which is the collection of GNU Compiler collection (GCC)
was used to install the gcc compiler for windows. 

The gcc compiler was used to compile and execute this C code.

If you want to replicate the environment, you can download the MinGW from 
[MinGW](https://sourceforge.net/projects/mingw/)

After installing the MinGW, you can select the gcc compiler and the same will get installed.

Add the gcc to the windows environment variable so that the gcc command can be invoked from anywhere. 



## Contents of the folder
- final.c : The main assembler code responsible for translating assembly instructions into machine code.
- instr_R/instr_IS/instr_B/instr_J/instr_U.h : Header files for the each format process instruction
- instr_R/instr_IS/instr_B/instr_J/instr_U.c : Contains the function for processing each format
- register_no.h and register_no.c : Its the header and function containing file respectively for getting the source and destination register numbers from the string type and also from the alias name.
- input1.s/input2.s: sample files containing RISC-V assembly instructions to be processed by the assembler.
- output1.hex/output2.hex: The output files that contains the translated machine code in hexadecimal format.its created.
- Config.txt: Contains the configurations of the cache to be used
- excstack.c : The whole code of the simulator implementation
- README.pdf: The present file, containing the installation and brief on how to start the project.
- Report.pdf : Detailed explanation of the main code.

To know the details of each function, variables and the flow of code execution, refer the report.pdf



## About the Code

This project extends the RISCV64 simulator to include a D-cache simulation, enabling tracking of cache hits and misses for RISC-V assembly programs.  

The code is modular, with separate functions handling cache configuration, enabling/disabling cache, cache status reporting, and cache operations such as invalidate, dump, and stats.  

Cache parameters, including cache size, block size, associativity, replacement policy, and write policy, are customizable via an external configuration file. The simulator executes read and write operations, using cache logic to determine hit or miss outcomes, and records cache statistics in an output file.  

The run command outputs final cache statistics, providing a comprehensive view of memory access performance.



## Executing the program

To compile and run your program, you typically enter the following command in the command prompt:

`gcc final.c instr_R.c instr_IS.c instr_SRAI_IS.c instr_B.c instr_U.c instr_J.c register_no.c -o out`

and then, run the program with:

`out`

Upon successful execution of the program, you will get an output file named output.hex with the translated machine codes in hexadecimal format.
But it obviously seems a long command as there are many source files to be executed at the same time. So here is a better option.


### Creating a Makefile 
- Instead of typing a long command every time, you can use a Makefile to simplify the compilation process. A Makefile automates the build process, especially useful when dealing with multiple source files.

Create a `Makefile` in your project directory with the following content:

`final:`

`<tab>gcc final.c instr_R.c instr_IS.c instr_SRAI_IS.c instr_B.c instr_U.c instr_J.c register_no.c -o out`

Then:
- Save the Makefile in the same directory as your source files.
- Open a terminal in UBUNTU.
- Navigate to your project directory.
- Type `make` and press Enter. This will compile your project.
  Then you can type `./out`
Upon successful execution of the program, you will get an output file named output.hex with the translated machine codes in hexadecimal format.



## Prerequisite Theory
- Instruction Set Architecture (ISA): Familiarity with RISC-V ISA you're working with in this project.
- Assembly Language: Knowledge of writing and understanding assembly language instructions.
- Binary and Hexadecimal Notation: Ability to work with and convert between binary, hexadecimal, and decimal systems.
- Basic C Programming: Proficiency in C programming for implementing the assembler logic.
- Instruction Formats: Understanding the various instruction formats and their encodings.
  The RISCV Instruction Set for each format is given below for reference.



### Assembly Language
An assembly language is a low-level programming language for a computer, or other programmable device, in which there is a very strong (generally one-to-one) correspondence between the language and the architecture's machine code instructions. Each assembly language is specific to a particular computer architecture, in contrast to most high-level programming languages, which are generally portable across multiple architectures, but require interpreting or compiling. Assembly language uses a mnemonic to represent each low-level machine operation or opcode. Some op-codes require one or more operands as part of the instruction.

### Assembler
Assembly language is converted into executable machine code by a utility program referred to as an assembler; the conversion process is referred to as assembly, or assembling the code.


![Assembler](images/assemblerpic.jpeg)


An assembler is a translator that translates an assembler program into a conventional machine language program. Basically, the assembler goes through the program one line at a time, and generates machine code for that instruction. Then the assembler proceeds to the next instruction. In this way, the entire machine code program is created.



### Instruction Set Architecture
The Instruction Set Architecture (ISA) defines the set of instructions a processor can execute, including data types, registers, and the format of instructions.
It acts as an interface between software and hardware, enabling the development of programs that can run on any processor implementing the same ISA. 
Common examples include x86, ARM, and RISC-V.

### Instruction Set- Formats
### R-Format

<table>
  <tr>
    <th>31-25</th>
    <th>24-20</th>
    <th>19-15</th>
    <th>14-12</th>
    <th>11-7</th>
    <th>6-0</th>
  </tr>
  <tr>
     <td>`funct7`</td>
     <td>`rs2`</td>
     <td>`rs1`</td>
     <td>`funct3`</td>
     <td>`rd`</td>
     <td>`opcode`</td>
  </tr>
</table>

- funct7: A 7-bit function code that helps specify the exact operation (e.g., add, sub).
- rs2: A 5-bit field representing the second source register.
- rs1: A 5-bit field representing the first source register.
- funct3: A 3-bit function code that further specifies the operation, often used in combination with funct7.
- rd: A 5-bit field representing the destination register where the result of the operation is stored.
- opcode: A 7-bit operation code that specifies the type of instruction. Here, R format: 0110011

### I-Format

<table>
  <tr>
    <th>31-20</th>
    <th>19-15</th>
    <th>14-12</th>
    <th>11-7</th>
    <th>6-0</th>
  </tr>
  <tr>
     <td>`imm[11:0]`</td>
     <td>`rs1`</td>
     <td>`funct3`</td>
     <td>`rd`</td>
     <td>`opcode`</td>
  </tr>
</table>

- imm[11:0]: A 12-bit immediate value that is sign-extended to the register width.
- rs1: A 5-bit field representing the first source register.
- funct3: A 3-bit function code that specifies the operation.
- rd: A 5-bit field representing the destination register.
- opcode: A 7-bit operation code that specifies the type of instruction. Here, I format: 0010011 , 1100111[Only for jalr]

### S-Format

<table>
  <tr>
    <th>31-25</th>
    <th>24-20</th>
    <th>19-15</th>
    <th>14-12</th>
    <th>11-7</th>
    <th>6-0</th>
  </tr>
  <tr>
     <td>`imm[11:5]`</td>
     <td>`rs2`</td>
     <td>`rs1`</td>
     <td>`funct3`</td>
     <td>`imm[4:0]'</td>
     <td>`opcode`</td>
  </tr>
</table>

- imm[11:5]: The upper 7 bits of the immediate value.
- rs2: A 5-bit field representing the second source register.
- rs1: A 5-bit field representing the first source register.
- funct3: A 3-bit function code that specifies the operation.
- imm[4:0]: The lower 5 bits of the immediate value.
- opcode: A 7-bit operation code that specifies the type of instruction. Here, S format: 0100011

### B-Format

<table>
  <tr>
    <th>31</th>
    <th>30-25</th>
    <th>24-20</th>
    <th>19-15</th>
    <th>14-12</th>
    <th>11-8</th>
    <th>7</th>
    <th>6-0</th>
  </tr>
  <tr>
     <td>`imm[12]`</td>
     <td>`imm[10:5]`</td>
     <td>`rs2`</td>
     <td>`rs1`</td>
     <td>`funct3`</td>
     <td>`imm[4:1]`</td>
     <td>`imm[11]`</td>
     <td>`opcode`</td>
  </tr>
</table>

- imm[12|10:5]: The concatenated 7 bits of the immediate value.
- rs2: A 5-bit field representing the second source register.
- rs1: A 5-bit field representing the first source register.
- funct3: A 3-bit function code that specifies the operation.
- imm[4:1|11]: The concatenated 5 bits of the immediate value and the 11th bit.
- opcode: A 7-bit operation code that specifies the type of instruction. Here, B format: 1100011

### U-Format

<table>
  <tr>
    <th>31-12</th>
    <th>11-7</th>
    <th>6-0</th>
  </tr>
  <tr>
     <td>`imm[31:12]`</td>
     <td>`rd`</td>
     <td>`opcode`</td>
  </tr>
</table>

- imm[31:12]: A 20-bit immediate value that is shifted left by 12 bits and added to the PC.
- rd: A 5-bit field representing the destination register.
- opcode: A 7-bit operation code that specifies the type of instruction. Here, U format: 0110111[For lui] , 0010111 [For auipc]

### J-Format

<table>
  <tr>
    <th>31</th>
    <th>30-21</th>
    <th>20</th>
    <th>19-12</th>
    <th>11-7</th>
    <th>6-0</th>
  </tr>
  <tr>
     <td>`imm[20]`</td>
     <td>`imm[10:1]`</td>
     <td>`imm[11]`</td>
     <td>`imm[19:12]`</td>
     <td>`rd`</td>
     <td>`opcode`</td>
  </tr>
</table>

- imm[20|10:1|11|19:12]: The concatenated 20-bit immediate value used for jumps.
- rd: A 5-bit field representing the destination register.
- opcode: A 7-bit operation code that specifies the type of instruction. Here, J format: 1101111

## Getting Help
If you are stuck and need help in understanding the code as well as for executing this C program, you may please write to the email address: co23btech11003@iith.ac.in . Kindly provide with details of the exact issues that you are facing to get the correct guidence.
