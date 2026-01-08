// SPDX-License-Identifier: MIT
#ifndef TESTCASE_H
#define TESTCASE_H

#include "arm_emulator.h"

#if defined(__cplusplus)
extern "C" {
#endif

enum {
	MAX_REGISTERS = 6
};

/* Memory sizes must fit microcontroller as well. */
enum {
	TESTCASE_PLUGIN_API_ADDRESS = 0x6000,
	TESTCASE_PLUGIN_API_SIZE = 0x2000,
	TESTCASE_SERVICE_API_ADDRESS = 0x200,
	TESTCASE_PLUGIN_DATA_ADDRESS = (0x10000000 + 0x200),
	TESTCASE_DATA_MEMORY_SIZE = 1024,
	TESTCASE_PLUGIN_STACK_ADDRESS = TESTCASE_PLUGIN_DATA_ADDRESS + TESTCASE_DATA_MEMORY_SIZE,
	TESTCASE_DEFAULT_PC = TESTCASE_PLUGIN_API_ADDRESS + 0x1C,
	TESTCASE_DEFAULT_SP = TESTCASE_PLUGIN_DATA_ADDRESS + TESTCASE_DATA_MEMORY_SIZE,
};

enum {
	INDEX_SP = 13,
	INDEX_LR = 14,
	INDEX_PC = 15,
	INDEX_APSR = 16
};

enum {
	FLAG_N = 1 << 31,
	FLAG_Z = 1 << 30,
	FLAG_C = 1 << 29,
	FLAG_V = 1 << 28,
};

enum {
	COND_EQ = 0x00,
	COND_NE = 0x01,
	COND_CS = 0x02,
	COND_CC = 0x03,
	COND_MI = 0x04,
	COND_PL = 0x05,
	COND_VS = 0x06,
	COND_VC = 0x07,
	COND_HI = 0x08,
	COND_LS = 0x09,
	COND_GE = 0x0A,
	COND_LT = 0x0B,
	COND_GT = 0x0C,
	COND_LE = 0x0D,
	COND_TRUE = 0x0E,
	COND_TRUE2 = 0x0F
};

struct testcase_register {
	/* 0...14: normal, 15 = PC, 16 = APSR. */
	int index;
	uint32_t value;
};

struct testcase_memory_access {
	unsigned int count;
	uint32_t address;
	const char *data;
};

struct testcase {
	/** Name of the test. Nullpointer signals end of test cases. */
	const char *name;
	/** 16-bit instructions. */
	uint16_t instruction;
	/** Second part of 32-bit instruction, if >=0. */
	int32_t instruction2;
	/** Register values to be set before execution.
	    All registers (incl. APSR) can be specified. */
	struct testcase_register registers_before[MAX_REGISTERS];
	/** Memory expected to be read. */
	struct testcase_memory_access expected_reads;

	/** Registers expected to be modified. Other registers are expected
	    to remain intact.
	    Special cases: Do not specify PC unless the instruction
	    explicitly modifies PC. */
	struct testcase_register registers_after[MAX_REGISTERS];

	/** Memory expected to be written. Other areas are expected to
	    remain intact. */
	struct testcase_memory_access expected_writes;
};

extern const struct testcase testcases[];

extern int testcase_run(const struct testcase *testcase);

#if defined(__cplusplus)
}
#endif

#endif /* TESTCASE_H */
