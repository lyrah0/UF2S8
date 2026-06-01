#include <stdint.h>

typedef union {
	uint16_t raw;

	struct {
		uint16_t op : 7;
		uint16_t mod : 3;
		uint16_t src : 3;
		uint16_t dst : 3;
	};

	struct {
		uint16_t op : 10;
		uint16_t src : 3;
		uint16_t op1 : 3;
	} ss;

	struct {
		uint16_t op : 13;
		uint16_t dst : 3;
	} sd;

	struct {
		uint16_t op : 7;
		uint16_t mod : 3;
		uint16_t src : 3;
		uint16_t op1 : 3;
	} ds;

	struct {
		uint16_t op : 10;
		uint16_t reg_src : 3;
		uint16_t reg_dst : 3;
	} sdss;

	struct {
		uint16_t op : 7;
		uint16_t mod : 3;
		uint16_t src : 3;
		uint16_t dst : 3;
	} sdds;

	struct {
		uint16_t op : 5;
		uint16_t imm : 8;
		uint16_t dst : 3;
	} li;

	struct {
		uint16_t op : 4;
		uint16_t imm1 : 3;
		uint16_t imm0 : 3;
		uint16_t src : 3;
		uint16_t dst : 3;
	} addi;

	struct {
		uint16_t op : 4;
		uint16_t offset0 : 4;
		uint16_t base : 2;
		uint16_t src : 3;
		uint16_t offset1 : 3;
	} sb;

	struct {
		uint16_t op : 4;
		uint16_t offset0 : 4;
		uint16_t base : 2;
		uint16_t offset1 : 3;
		uint16_t dst : 3;
	} lb;

	struct {
		uint16_t op : 4;
		uint16_t offset : 9;
		uint16_t cond : 3;
	} branch;
} Instruction;
