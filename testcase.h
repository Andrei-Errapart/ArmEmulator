// SPDX-License-Identifier: MIT
#ifndef testcase_h_
#define testcase_h_

#include "arm_emulator.h"

#if defined(__cplusplus)
extern "C" {
#endif

enum {
	MAX_REGISTERS = 6
};

/* Memory sizes must fit microcontroller as well. */
enum {
	PLUGIN_API_ADDRESS = 0x6000,
	PLUGIN_API_SIZE = 0x2000,
	SERVICE_API_ADDRESS = 0x200,
	PLUGIN_DATA_ADDRESS = (0x10000000 + 0x200),
	DATA_MEMORY_SIZE = 1024,
	PLUGIN_STACK_ADDRESS = PLUGIN_DATA_ADDRESS + DATA_MEMORY_SIZE,
	DEFAULT_PC = PLUGIN_API_ADDRESS + 0x1C,
	DEFAULT_SP = PLUGIN_DATA_ADDRESS + DATA_MEMORY_SIZE,
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

typedef struct {
	// 0...14: normal
	// 15 = PC
	// 16 = APSR.
	int			index;
	uint32_t	value;
} testcase_register_t;

typedef struct {
	unsigned int	count;
	uint32_t		address;
	const char*		data;
} testcase_memory_access_t;

typedef struct {
	/** Name of the test. Nullpointer signals end of test cases. */
	const char*		name;
	/** 16-bit instructions. */
	uint16_t		instruction;
	/** Second part of 32-bit instruction, if >=0. */
	int32_t			instruction2;
	/** Register values to be set before execution.
	  All registers (incl. APSR) can be specified. */
	testcase_register_t			registers_before[MAX_REGISTERS];
	/** Memory expected to be read. */
	testcase_memory_access_t	expected_reads;

	/** Registers expected to be modified. Other registers are expected to remain intact.
	* Special cases: Do not specify PC unless the instruction explicitly modifies PC.
	*/
	testcase_register_t			registers_after[MAX_REGISTERS];

	/** Memory expected to be written. Other areas are expected to remain intact. */
	testcase_memory_access_t	expected_writes;
} testcase_t;

extern const testcase_t	testcases[];

extern int testcase_run(
	const testcase_t*	testcase);

#if defined(__cplusplus)
}
#endif

#endif /* testcase_h_ */
