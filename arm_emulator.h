/** An Cortex-M0 emulator. Intended for use in LPC1114.
*/
#ifndef arm_emulator_h_
#define arm_emulator_h_

#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

enum
{
	/** Number of registers. */
	ARM_NREGISTERS = 16,
};

//================================================================================================================
typedef struct {
	// PROGRAM MEMORY
	const uint8_t*	program;
	uint32_t		program_address;
	uint16_t		program_size;
	// DATA MEMORY
	uint8_t*		data;
	uint32_t		data_address;
	uint16_t		data_size;
	// SERVICE MEMORY, READ-ONLY.
	const uint8_t*	service;
	uint32_t		service_address;
	uint16_t		service_size;

	// REGISTERS
	uint32_t		R[ARM_NREGISTERS];
	uint32_t		APSR;
} ARM_EMULATOR_STATE;

/** Testcases need to access the registers. */
extern ARM_EMULATOR_STATE global_emulator;

/** Reset emulator state. */
extern void
arm_emulator_reset(
	const uint8_t*	program_memory,
	const unsigned int	program_memory_address,
	const unsigned int	program_memory_size,
	uint8_t*			data_memory,
	const unsigned int	data_memory_address,
	const unsigned int	data_memory_size,
	const uint8_t*		service,
	uint32_t			service_address,
	uint16_t			service_size
);

/** Read memory, either data or program. Calls arm_emulator_callback_read_program_memory if needed.
 * Buffer:	Buffer to read data into.
 * Address:	Address to read from.
 * Count:	Number of bytes to read.
 * Return value:	0=OK, <0: Error.
 * */
extern int
arm_emulator_read_memory(
	uint8_t*			Buffer,
	const uint32_t		Address,
	const uint8_t		Count
);

/** Start function call. Registers are reset. */
extern int
arm_emulator_start_function_call(
	const void*			function_address,
	const uint32_t*		arguments,
	const unsigned int	arguments_size
);

typedef enum {
	ARM_EMULATOR_OK = 0,
	ARM_EMULATOR_ERROR = -1,
	ARM_EMULATOR_FUNCTION_RETURNED = 1,
} ARM_EMULATOR_RETURN_VALUE;

/** Continue function call execution. */
extern ARM_EMULATOR_RETURN_VALUE
arm_emulator_execute(
	const unsigned int	max_number_of_instructions);

/** Get function return value, i.e. R0. */
extern const uint32_t
arm_emulator_get_function_return_value(void);

/** Write register dump to uart. */
extern void
arm_emulator_dump(void);

/** Callback for a function call. Implemented by user.
 * FunctionAddress:	Address of the function to call.
 * State:			Emulator state.
 * Return values: 	0=function executed, <0: Not executed.
 */
extern int
arm_emulator_callback_functioncall(
	const uint32_t		FunctionAddress,
	ARM_EMULATOR_STATE*	State
);

/** Callback to read program memory area. Implemented by user.
 * Buffer:	Buffer to read data into.
 * Address:	Address to read from.
 * Count:	Number of bytes to read.
 * Return value:	0=OK, <0: Error.
 * */
extern int
arm_emulator_callback_read_program_memory(
	uint8_t*			Buffer,
	const uint32_t		Address,
	const uint8_t		Count
);

#if defined(__cplusplus)
}
#endif

#endif /* arm_emulator_h_ */
