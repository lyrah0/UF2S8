#include <stdint.h>

typedef union {
	uint16_t raw;

	struct {
		uint16_t opcode : 7;
		uint16_t reg_mod : 3;
		uint16_t reg_src : 3;
		uint16_t reg_dst : 3;
	};

	struct {
		uint16_t opcode : 13;
		uint16_t reg_dst : 3;
	} reg1;

	struct {
		uint16_t opcode : 10;
		uint16_t reg_src : 3;
		uint16_t reg_dst : 3;
	} reg2;

	struct {
		uint16_t opcode : 7;
		uint16_t reg_mod : 3;
		uint16_t reg_src : 3;
		uint16_t reg_dst : 3;
	} reg3;

	struct {
		uint16_t opcode : 5;
		uint16_t imm : 8;
		uint16_t reg_dst : 3;
	} load_imm;

	struct {
		uint16_t opcode : 5;
		uint16_t imm : 5;
		uint16_t reg_src : 3;
		uint16_t reg_dst : 3;
	} addi;

	struct {
		uint16_t opcode : 4;
		uint16_t offset : 7;
		uint16_t reg_base : 2;
		uint16_t reg_dst : 3;
	} loadstore;

	struct {
		uint16_t opcode : 4;
		uint16_t offset : 9;
		uint16_t cond : 3;
	} branch;
} Instruction;
