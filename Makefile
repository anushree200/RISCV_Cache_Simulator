final:
	@echo "Build Version 1.0.1"
	@sleep 3
	@echo "Entering Lab7 project- assembler+simulator"
	@sleep 4
	@echo "Compiling assembler - final.c and other stuffs......"
	gcc final.c instr_R.c instr_IS.c instr_B.c instr_U.c instr_J.c register_no.c -o out
	@echo "Completing the compilation..."
	@sleep 3
	@./out
	@sleep 3
	@echo "Compiling the simulator..."
	@sleep 3
	gcc excstack_1.c -o riscv_sim
	@echo "Completing the compilation..."
	@echo "Created out1 file.."
	@echo "Makefile Success!!"