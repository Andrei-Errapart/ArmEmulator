// SPDX-License-Identifier: MIT
/**
 * ARM Cortex-M0 emulator. Intended for use with LPC1114.
 */
#ifndef ARM_EMULATOR_H
#define ARM_EMULATOR_H

#include <stddef.h>
#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

/** Number of ARM registers. */
#define ARM_NREGISTERS 16

/**
 * Emulator state. Allocate one of these and pass to all API functions.
 */
struct arm_emulator_state {
	/* Program memory (read-only) */
	const uint8_t *program;
	uint32_t program_address;
	size_t program_size;

	/* Data memory (read-write) */
	uint8_t *data;
	uint32_t data_address;
	size_t data_size;

	/* Service memory (read-only) */
	const uint8_t *service;
	uint32_t service_address;
	size_t service_size;

	/* Registers */
	uint32_t R[ARM_NREGISTERS];
	uint32_t APSR;
};

/**
 * Return values from arm_emulator_execute().
 */
enum arm_emulator_result {
	ARM_EMULATOR_OK = 0,
	ARM_EMULATOR_ERROR = -1,
	ARM_EMULATOR_FUNCTION_RETURNED = 1,
};

/**
 * Initialize emulator state and configure memory regions.
 *
 * @param emu Emulator state to initialize.
 * @param program_memory Program memory buffer.
 * @param program_memory_address Base address for program memory.
 * @param program_memory_size Size of program memory in bytes.
 * @param data_memory Data memory buffer.
 * @param data_memory_address Base address for data memory.
 * @param data_memory_size Size of data memory in bytes.
 * @param service Service memory buffer (may be NULL).
 * @param service_address Base address for service memory.
 * @param service_size Size of service memory in bytes.
 */
void arm_emulator_init(
	struct arm_emulator_state *emu,
	const uint8_t *program_memory,
	uint32_t program_memory_address,
	size_t program_memory_size,
	uint8_t *data_memory,
	uint32_t data_memory_address,
	size_t data_memory_size,
	const uint8_t *service,
	uint32_t service_address,
	size_t service_size);

/**
 * Reset emulator registers to initial state.
 * Memory regions are preserved. Can be called to re-run code.
 *
 * @param emu Emulator state to reset.
 */
void arm_emulator_reset(struct arm_emulator_state *emu);

/**
 * Read memory from any region (program, data, or service).
 * Calls arm_emulator_callback_read_program_memory for addresses outside
 * defined regions.
 *
 * @param emu Emulator state.
 * @param buffer Buffer to read data into.
 * @param address Address to read from.
 * @param count Number of bytes to read.
 * @return 0 on success, negative on error.
 */
int arm_emulator_read_memory(
	struct arm_emulator_state *emu,
	uint8_t *buffer,
	uint32_t address,
	size_t count);

/**
 * Start a function call. Resets registers and sets up initial state.
 *
 * @param emu Emulator state.
 * @param function_address Function entry point (with Thumb bit set).
 * @param arguments Array of arguments to pass (up to 4).
 * @param arguments_count Number of arguments.
 * @return 0 on success, negative on error.
 */
int arm_emulator_start_function_call(
	struct arm_emulator_state *emu,
	const void *function_address,
	const uint32_t *arguments,
	unsigned int arguments_count);

/**
 * Execute instructions.
 *
 * @param emu Emulator state.
 * @param max_instructions Maximum number of instructions to execute.
 * @return ARM_EMULATOR_OK if still running, ARM_EMULATOR_FUNCTION_RETURNED
 *         if function returned, ARM_EMULATOR_ERROR on error.
 */
enum arm_emulator_result arm_emulator_execute(
	struct arm_emulator_state *emu,
	unsigned int max_instructions);

/**
 * Get function return value (R0).
 *
 * @param emu Emulator state.
 * @return Value of R0.
 */
uint32_t arm_emulator_get_function_return_value(
	struct arm_emulator_state *emu);

/**
 * Dump register state to UART.
 *
 * @param emu Emulator state.
 */
void arm_emulator_dump(struct arm_emulator_state *emu);

/**
 * Callback for external function calls. Implemented by user.
 *
 * Called when the emulator encounters a BL/BLX to an address outside
 * defined memory regions.
 *
 * @param emu Emulator state (may be modified by callback).
 * @param function_address Address of the function being called.
 * @return 0 if handled, negative if not handled.
 */
extern int arm_emulator_callback_functioncall(
	struct arm_emulator_state *emu,
	uint32_t function_address);

/**
 * Callback to read memory outside defined regions. Implemented by user.
 *
 * Called when arm_emulator_read_memory accesses an address not covered
 * by program, data, or service memory.
 *
 * @param emu Emulator state.
 * @param buffer Buffer to read data into.
 * @param address Address to read from.
 * @param count Number of bytes to read.
 * @return 0 on success, negative on error.
 */
extern int arm_emulator_callback_read_program_memory(
	struct arm_emulator_state *emu,
	uint8_t *buffer,
	uint32_t address,
	size_t count);

#if defined(__cplusplus)
}
#endif

#endif /* ARM_EMULATOR_H */
