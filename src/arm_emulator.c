// SPDX-License-Identifier: MIT
/** \file Implementation of the emulator. */
#include "arm_emulator.h"
#include <string.h>	// memset
#include "comm.h"	// Error message output.

static const char*	_rnames[16] = {
	"R0", "R1", "R2", "R3", "R4", "R5", "R6", "R7",
	"R8", "R9", "R10", "R11", "R12", "SP", "LR", "PC" };

#if defined(_MSC_VER) || defined(DESKTOP_BUILD)
#include <stdio.h>	// printf
// error printf
#define	xprintf(args)	do { printf args; } while(0)
// instruction printf
#define	iprintf(args)	do { printf args; } while(0)
static const char*	_cnames[16] = {
	"EQ", "NE", "CS", "CC",		"MI", "PL", "VS", "VC",
	"HI", "LS", "GE", "LT",		"GT", "LE",	"<UNDEF>", "<SVC>"
};
// print instructions :)
#define	_print(name)				do { printf("%04x:\t%s\n",  (prev_pc), (name)); } while (0)
#define	_print_x(name,x)			do { printf("%04x:\t%s 0x%04X\n",  (prev_pc), (name), (x)); } while (0)
#define	_print_RRx(name,r1,r2,x)	do { printf("%04x:\t%s %s, %s, 0x%04X\n",  (prev_pc), (name), _rnames[(r1)], _rnames[(r2)], (x)); } while (0)
#define	_print_Rx(name,r1,x)		do { printf("%04x:\t%s %s, 0x%04X\n",  (prev_pc), (name), _rnames[(r1)], (x)); } while (0)
#define	_print_R(name,r1)			do { printf("%04x:\t%s %s\n",  (prev_pc), (name), _rnames[(r1)]); } while (0)
#define	_print_RR(name,r1,r2)		do { printf("%04x:\t%s %s, %s\n",  (prev_pc), (name), _rnames[(r1)], _rnames[(r2)]); } while (0)
#define	_print_RRR(name,r1,r2, r3)	do { printf("%04x:\t%s %s, %s, %s\n",  (prev_pc), (name), _rnames[(r1)], _rnames[(r2)], _rnames[(r3)]); } while (0)
#define	_print_BC(cond, addr)		do { printf("%04x:\tB%s 0x%04X\n",  (prev_pc), _cnames[cond], addr); } while(0)

//================================================================================================================
static void _print_PUSH(
	const uint32_t	prev_pc,
	const uint8_t	m,
	uint8_t			list)
{
	uint8_t	i;

	printf("%04x:\tPUSH {", prev_pc);
	if (m)
	{
		printf(list==0 ? "LR" : "LR,");
	}
	for (i=0; i<8; ++i)
	{
		const uint8_t	this_round = list & 0x80;
		if (this_round)
		{
			printf("%s", _rnames[7-i]);
		}
		list = list << 1;
		if (this_round && list)
		{
			printf(",");
		}
	}
	printf("}\n");
}

//================================================================================================================
static void _print_POP(
	const uint32_t	prev_pc,
	const uint8_t	p,
	uint8_t			list)
{
	uint8_t	i;

	printf("%04x:\tPOP {", prev_pc);
	for (i=0; i<8; ++i)
	{
		const uint8_t	this_round = list & 1;
		if (this_round)
		{
			printf("%s", _rnames[i]);
		}
		list = list >> 1;
		if (this_round && (list || p))
		{
			printf(",");
		}
	}
	if (p)
	{
		printf("PC");
	}
	printf("}\n");
}

//================================================================================================================
static void _print_STM(
	const uint32_t	prev_pc,
	const uint8_t	Rn,
	uint8_t			list)
{
	uint8_t	i;

	printf("%04x:\tSTM %s, {", prev_pc, _rnames[Rn]);
	for (i=0; i<8; ++i)
	{
		const uint8_t	this_round = list & 1;
		if (this_round)
		{
			printf("%s", _rnames[i]);
		}
		list = list >> 1;
		if (this_round && list)
		{
			printf(",");
		}
	}
	printf("}\n");
}

//================================================================================================================
static void _print_LDM(
	const uint32_t	prev_pc,
	const uint8_t	Rn,
	uint8_t			list)
{
	uint8_t	i;

	printf("%04x:\tLDM %s!, {", prev_pc, _rnames[Rn]);
	for (i=0; i<8; ++i)
	{
		const uint8_t	this_round = list & 1;
		if (this_round)
		{
			printf("%s", _rnames[i]);
		}
		list = list >> 1;
		if (this_round && list)
		{
			printf(",");
		}
	}
	printf("}\n");
}

#else
#define	xprintf(args)	(void)0
#define	iprintf(args)	(void)0
#define	_print(name)				do { } while (0)
#define	_print_x(name,x)			do { } while (0)
#define	_print_RRx(name,r1,r2,x)	do { } while (0)
#define	_print_Rx(name,r1,x)		do { } while (0)
#define	_print_R(name,r1)			do { } while (0)
#define	_print_RR(name,r1,r2)		do { } while (0)
#define	_print_RRR(name,r1,r2,r3)	do { } while (0)
#define	_print_BC(cond,addr)		do { } while (0)
#define	_print_PUSH(ppc, p,list)	do { } while (0)
#define	_print_POP(ppc, p,list)		do { } while (0)
#define	_print_STM(ppc, Rn,list)	do { } while (0)
#define	_print_LDM(ppc, Rn,list)	do { } while (0)
#define	_print_(cond,addr)			do { } while (0)

#if (0)
#define	uart_write_hex32(s,x)		do { } while (0)
#define	uart_write_hex16(s,x)		do { } while (0)
#define	uart_write_hex(s,x)			do { } while (0)
#define	uart_write(s)				do { } while (0)
#define	uart_writeln(s)				do { } while (0)
#define	uart_write_crlf()			do { } while (0)
#endif

#endif

enum {
	EMULATOR_RETURN_ADDRESS = 0x11111111
};

ARM_EMULATOR_STATE	global_emulator;

#define	self	(&global_emulator)
enum {
	INDEX_SP = 13,	
	INDEX_LR = 14,
	INDEX_PC = 15
};
enum {
	FLAG_N_BIT = 31,
	FLAG_Z_BIT = 30,
	FLAG_C_BIT = 29,
	FLAG_V_BIT = 28,
};

#define	APSR_N	((self->APSR >> FLAG_N_BIT) & 1)
#define	APSR_Z	((self->APSR >> FLAG_Z_BIT) & 1)
#define	APSR_C	((self->APSR >> FLAG_C_BIT) & 1)
#define	APSR_V	((self->APSR >> FLAG_V_BIT) & 1)

#define	SP	(self->R[INDEX_SP])
#define	LR	(self->R[INDEX_LR])
#define	PC	(self->R[INDEX_PC])

#define	_IsNegative(x)	(((uint32_t)(x)) >> 31)
#define	_Align4Up(x)		((uint32_t)(((x) + 3) & ~0x3))
#define	_Align4Down(x)		((uint32_t)((x)  & ~0x3))

//================================================================================================================
#define	_set_APSR_of_NZCV(_arg_x, _arg_c, _arg_v)	\
	do { \
		self->APSR = ((_IsNegative(_arg_x)) << FLAG_N_BIT) \
				| (((_arg_x)==0 ? 1 : 0) << FLAG_Z_BIT) \
				| (((uint32_t)(_arg_c)) << FLAG_C_BIT) \
				| (((uint32_t)(_arg_v)) << FLAG_V_BIT) \
				| (self->APSR & 0x0FFFFFFF); \
	} while (0)

//================================================================================================================
#define	_set_APSR_of_NZC(_arg_x, _arg_c)	\
	do { \
		self->APSR = ((_IsNegative(_arg_x)) << FLAG_N_BIT) \
				| ((_arg_x==0 ? 1 : 0) << FLAG_Z_BIT) \
				| (((uint32_t)(_arg_c)) << FLAG_C_BIT) \
				| (self->APSR & 0x1FFFFFFF); \
	} while (0)

//================================================================================================================
#define	_set_APSR_of_NZ(_arg_x)	\
	do { \
		self->APSR = ((_IsNegative(_arg_x)) << FLAG_N_BIT) \
				| (((_arg_x)==0 ? 1 : 0) << FLAG_Z_BIT) \
				| (self->APSR & 0x3FFFFFFF); \
	} while (0)

//================================================================================================================
static ARM_EMULATOR_RETURN_VALUE
_check_new_PC(
	uint32_t*	new_pc)
{
	const uint32_t	x = *new_pc;
	if (x == EMULATOR_RETURN_ADDRESS)
	{
		return ARM_EMULATOR_FUNCTION_RETURNED;
	}
	else if (arm_emulator_callback_functioncall(x, &global_emulator) == 0)
	{
		// Function called, thus we must return.
		*new_pc = LR;
		return ARM_EMULATOR_OK;
	}
	// nothing to see here.
	return ARM_EMULATOR_OK;
}

//================================================================================================================
#define	set_PC(new_PC)	\
	do { \
		uint32_t	real_new_PC = new_PC; \
		const ARM_EMULATOR_RETURN_VALUE	r = _check_new_PC(&real_new_PC); \
		PC = (real_new_PC) & 0xFFFFFFFE; \
		if (r != ARM_EMULATOR_OK) { \
			return r; \
		} \
	} while (0)


//================================================================================================================
static void
_reset_registers(void)
{
	memset(self->R, 0, sizeof(self->R));
	self->R[INDEX_SP] = self->data_address + self->data_size;
	self->R[INDEX_PC] = self->program_address;
	self->APSR = 0;
}

//================================================================================================================
static int32_t
_SignExtendTo32(
	const uint32_t	x,
	const uint8_t	sign_bit)
{
	// see http://graphics.stanford.edu/~seander/bithacks.html
	// b = sign_bit+1
	// assembly: http://www.coranac.com/documents/bittrick/
	const uint32_t	m = 1U << sign_bit;
	return ((x & ((1U << (sign_bit+1)) - 1)) ^ m) - m;
}

//================================================================================================================
/** Add with carry and set the bits.
* Rd - Destination register index.
* x1 - Operand1.
* x2 - Operand2.
* carry - Carry bit.
*/
static void
_AddWithCarry(
	const uint8_t	Rd,
	uint32_t	x1,
	const uint32_t	x2,
	const uint8_t	carry)
{
#if defined(_MSC_VER) || defined(DESKTOP_BUILD)
	const uint64_t	u64 = ((uint64_t)x1) + ((uint64_t)x2) + carry;
	const int64_t	i64 = ((int64_t)((int32_t)x1)) + ((int64_t)((int32_t)x2)) + carry;
	self->R[Rd] = (uint32_t)u64;
	_set_APSR_of_NZCV(
		(uint32_t)u64,
		((uint32_t)(u64 >> 32)) ? 1 : 0,
		((int32_t)((uint32_t)u64))==i64 ? 0 : 1);
#else
	/* 64-bit multiplication is large and slow on Cortex-M0. */
	uint32_t	apsr = (carry * (1 << FLAG_C_BIT)) | (self->APSR & ~(1 << FLAG_C_BIT));
	asm(
		"msr apsr, %[apsr]\n\t"
		"adc %[x1], %[x2]\n\t"
		"mrs %[apsr], apsr\n\t"
		: [apsr]"+r"(apsr), [x1]"+r"(x1)
		: [x2]"r"(x2)
		: "cc"
		  );
	self->R[Rd] = x1;
	self->APSR = apsr;
#endif
}

//================================================================================================================
/** Add with carry, set the bits and discard result.
* Rd - Destination register index.
* x1 - Operand1.
* x2 - Operand2.
* carry - Carry bit.
*/
static void
_AddWithCarryDiscard(
	uint32_t		x1,
	const uint32_t	x2,
	const uint8_t	carry)
{
#if defined(_MSC_VER) || defined(DESKTOP_BUILD)
	const uint64_t	u64 = ((uint64_t)x1) + ((uint64_t)x2) + carry;
	const int64_t	i64 = ((int64_t)((int32_t)x1)) + ((int64_t)((int32_t)x2)) + carry;
	_set_APSR_of_NZCV(
		(uint32_t)u64,
		((uint32_t)u64)==u64 ? 0 : 1,
		((int32_t)((uint32_t)u64))==i64 ? 0 : 1);
#else
	/* 64-bit multiplication is large and slow on Cortex-M0. */
	uint32_t	apsr = (carry * (1 << FLAG_C_BIT)) | (self->APSR & ~(1 << FLAG_C_BIT));
	asm(
		"msr apsr, %[apsr]\n\t"
		"adc %[x1], %[x2]\n\t"
		"mrs %[apsr], apsr\n\t"
		: [apsr]"+r"(apsr), [x1]"+r"(x1)
		: [x2]"r"(x2)
		: "cc"
		  );
	self->APSR = apsr;
#endif
}

//================================================================================================================
void
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
)
{
	self->program = program_memory;
	self->program_address = program_memory_address;
	self->program_size = program_memory_size;

	self->data = (uint8_t*)data_memory;
	self->data_address = data_memory_address;
	self->data_size = data_memory_size;

	self->service = service;
	self->service_address = service_address;
	self->service_size = service_size;

	memset(self->data, 0, self->data_size);
	_reset_registers();
}

//================================================================================================================
extern int
arm_emulator_read_memory(
	uint8_t*		Buffer,
	const uint32_t		Address,
	const uint8_t		Count
)
{
	const uint32_t	program_offset = Address - self->program_address;
	const uint32_t	data_offset = Address - self->data_address;
	const uint32_t	service_offset = Address - self->service_address;
	if (program_offset < self->program_size)
	{
		return arm_emulator_callback_read_program_memory(Buffer, Address, Count);
	}
	else if (data_offset < self->data_size)
	{
		if (data_offset+Count <= self->data_size)
		{
			memcpy(Buffer, (const uint8_t*)(self->data + data_offset), Count);
			return 0;
		}
		else
		{
			uart_write_hex32("arm_emulator_read_memory: Data memory size exceeded when reading from address 0x", Address);
			uart_write_crlf();
		}
	}
	else if (service_offset < self->service_size)
	{
		if (service_offset+Count <= self->service_size)
		{
			memcpy(Buffer, (const uint8_t*)(self->service + service_offset), Count);
			return 0;
		}
		else
		{
			uart_write_hex32("arm_emulator_read_memory: Service memory size exceeded when reading from address 0x", Address);
			uart_write_crlf();
		}
	}
	else
	{
		uart_write_hex32("arm_emulator_read_memory: fetch from invalid address 0x", Address);
		uart_write_crlf();
	}
	return -1;
}

//================================================================================================================
int
arm_emulator_start_function_call(
	const void*			function_address,
	const uint32_t*		arguments,
	const unsigned int	arguments_size
)
{
	unsigned int	i;
	_reset_registers();

	LR = EMULATOR_RETURN_ADDRESS;
	PC = ((uint32_t)(uintptr_t)function_address) & ~(uint32_t)(1); // thumb all the way!
	if (arguments!=0 && arguments_size>0)
	{
		for (i=0; i<arguments_size && i<4; ++i)
		{
			self->R[i] = arguments[i];
		}
	}
	// yes!
	return 0;
}

#define	error_unknown_instruction()	\
	do { \
		uart_write_hex("arm_emulator_execute: unknown opcode 0x", i_h); \
		uart_write_hex("", i_l); \
		uart_write_hex32(" at 0x", prev_pc); \
		uart_write_crlf(); \
		return ARM_EMULATOR_ERROR; \
	} while (0)

//================================================================================================================
static uint8_t
_ConditionPassed(const uint8_t cond)
{
	uint8_t r = 0; // don't branch.
	switch (cond & 0x0E)
	{
	case 0x00:
		r = APSR_Z == 1; // EQ or NE
		break;
	case 0x02:
		r = APSR_C == 1; // CS or CC
		break;
	case 0x04:
		r = APSR_N == 1; // MI or PL
		break;
	case 0x06:
		r = APSR_V == 1; // VS or VC
		break;
	case 0x08:
		r = APSR_C==1 && APSR_Z==0;	// HI or LS
		break;
	case 0x0A:
		r = APSR_N == APSR_V;	// GE or LT
		break;
	case 0x0C:
		r = APSR_N==APSR_V && APSR_Z==0;	// GT or LE
		break;
	case 0x0E:
		r = 1;
		break;
	}
	return ((cond & 1)!=0 && (cond!=0x0F))
		? !r
		: r;
}

//================================================================================================================
// Logical Shift Left.
static uint32_t
_LSL_C(
	const uint32_t	x,
	const uint8_t	shift_n)
{
	if (shift_n>=32)
	{
		const uint8_t	carry = (shift_n==32 ? (x & 1) : 0);
		_set_APSR_of_NZC(0, carry);
		return 0;
	}
	else
	{
		const uint32_t	r =  x << shift_n;
		const uint8_t	carry = (x >> (32-shift_n)) & 1;
		_set_APSR_of_NZC(r, carry);
		return r;
	}
}

//================================================================================================================
// Logical Shift Right
static uint32_t
_LSR_C(
	const uint32_t	x,
	const uint8_t	shift_n)
{
	if (shift_n>=32)
	{
		const uint8_t	carry = (shift_n==32 ? (x >> 31) : 0);
		_set_APSR_of_NZC(0, carry);
		return 0;
	}
	else
	{
		const uint32_t	r =  x >> shift_n;
		const uint8_t	carry = (x >> (shift_n-1)) & 1;
		_set_APSR_of_NZC(r, carry);
		return r;
	}
}

//================================================================================================================
static uint32_t
_ASR_C(
	const uint32_t	x,
	const uint8_t	shift_n)
{
	if (shift_n>=32)
	{
		const uint8_t	carry = x >> 31;
		const uint32_t	r = (int32_t)(0 - carry);
		_set_APSR_of_NZC(r, carry);
		return r;
	}
	else
	{
		const uint32_t	r = _SignExtendTo32((x >> shift_n), (31-shift_n) & 31);
		const uint8_t	carry = (x >> (shift_n -1)) & 1;
		_set_APSR_of_NZC(r, carry);
		return r;
	}
}

//================================================================================================================
static uint32_t
_ROR_C(
	const uint32_t	x,
	const uint8_t	x_shift_n)
{
	const uint8_t	shift_n = x_shift_n & 31;
	const uint32_t	r = (x >> shift_n) | (x << (32-shift_n));
	const uint8_t	carry = r >> 31;
	_set_APSR_of_NZC(r, carry);
	return r;
}

//================================================================================================================
typedef uint32_t (*shift_operation_t)(const uint32_t, const uint8_t);

//================================================================================================================
static void
_shift_processing(
	const shift_operation_t	shift_operation,
	const uint8_t			Rdn,
	const uint8_t			Rm
)
{
	const uint32_t	x = self->R[Rdn];
	const uint32_t	shift_n = self->R[Rm];
	if (shift_n>0)
	{
		self->R[Rdn] = shift_operation(x, shift_n);
	} else {
		self->R[Rdn] = x;
		_set_APSR_of_NZ(x);
	}
}

//================================================================================================================
static void
_shift_processing_immediate(
	const shift_operation_t	shift_operation,
	const uint8_t			Rd,
	const uint8_t			Rm,
	const uint8_t			shift_n
)
{
	const uint32_t	x = self->R[Rm];
	if (shift_n>0)
	{
		self->R[Rd] = shift_operation(x, shift_n);
	} else {
		self->R[Rd] = x;
		_set_APSR_of_NZ(x);
	}
}

//================================================================================================================
static ARM_EMULATOR_RETURN_VALUE
_fetch_data32(
	const uint8_t	Rt,
	const uint32_t	addr)
{
	if (addr & 0x03)
	{
		// oops.
		uart_write_hex32("_fetch_data32: 32-bit fetch from non-aligned address 0x", addr);
		uart_write_crlf();
		return ARM_EMULATOR_ERROR;
	}
	else
	{
		const int r = arm_emulator_read_memory((uint8_t*)(&self->R[Rt]), addr, 4);
		return r==0
				? ARM_EMULATOR_OK
				: ARM_EMULATOR_ERROR;
	}
}

//================================================================================================================
static ARM_EMULATOR_RETURN_VALUE
_fetch_data16(
	const uint8_t	Rt,
	const uint32_t	addr)
{
	if (addr & 0x01)
	{
		// oops.
		uart_write_hex32("_fetch_data16: 16-bit fetch from non-aligned address 0x", addr);
		uart_write_crlf();
		return ARM_EMULATOR_ERROR;
	}
	else
	{
		uint16_t	x;
		const int r = arm_emulator_read_memory((uint8_t*)(&x), addr, 2);
		if (r == 0)
		{
			self->R[Rt] = x;
			return ARM_EMULATOR_OK;
		}
		else
		{
			return ARM_EMULATOR_ERROR;
		}
	}
}

//================================================================================================================
static ARM_EMULATOR_RETURN_VALUE
_fetch_data8(
	const uint8_t	Rt,
	const uint32_t	addr)
{
	uint8_t		x;
	const int	r = arm_emulator_read_memory(&x, addr, 1);
	if (r == 0)
	{
		self->R[Rt] = x;
		return ARM_EMULATOR_OK;
	}
	else
	{
		return ARM_EMULATOR_ERROR;
	}
}

//================================================================================================================
static ARM_EMULATOR_RETURN_VALUE
_store_data32(
	const uint32_t	addr,
	const uint32_t	data)
{
	if (addr & 0x03)
	{
		// oops.
		uart_write_hex32("_store_data32: 32-bit store to non-aligned address 0x", addr);
		uart_write_crlf();
		return ARM_EMULATOR_ERROR;
	}
	else
	{
		const uint32_t	data_offset = addr-self->data_address;
		if (data_offset < self->data_size)
		{
			*((uint32_t*)(self->data + data_offset)) = data;
			return ARM_EMULATOR_OK;
		}
	}
	uart_write_hex32("_store_data32: 32-bit store to invalid address 0x", addr);
	uart_write_hex32(", data=0x", data);
	uart_write_crlf();
	return ARM_EMULATOR_ERROR;
}

//================================================================================================================
static ARM_EMULATOR_RETURN_VALUE
_store_data16(
	const uint32_t	addr,
	const uint32_t	data)
{
	if (addr & 0x01)
	{
		// oops.
		uart_write_hex32("_store_data16: 16-bit store to non-aligned address 0x", addr);
		uart_write_crlf();
		return ARM_EMULATOR_ERROR;
	}
	else
	{
		const uint32_t	data_offset = addr-self->data_address;
		if (data_offset < self->data_size)
		{
			*((uint16_t*)(self->data + data_offset)) = data;
			return ARM_EMULATOR_OK;
		}
	}
	uart_write_hex32("_store_data16: 16-bit store to invalid address 0x", addr);
	uart_write_hex16(", data=0x", data & 0xFFFF);
	uart_write_crlf();
	return ARM_EMULATOR_ERROR;
}

//================================================================================================================
static ARM_EMULATOR_RETURN_VALUE
_store_data8(
	const uint32_t	addr,
	const uint32_t	data)
{
	const uint32_t	data_offset = addr-self->data_address;
	if (data_offset < self->data_size)
	{
		*((uint8_t*)(self->data + data_offset)) = data;
		return ARM_EMULATOR_OK;
	}
	else
	{
		uart_write_hex32("_store_data8: 8-bit store to invalid address 0x", addr);
		uart_write_hex(", data=0x", data & 0xFF);
		return ARM_EMULATOR_ERROR;
	}
}

//================================================================================================================
ARM_EMULATOR_RETURN_VALUE
arm_emulator_execute(
	const unsigned int	max_number_of_instructions)
{
	unsigned int instruction_count;
	for (instruction_count=0; instruction_count<max_number_of_instructions; ++instruction_count)
	{
		uint16_t		instruction;
		if (arm_emulator_callback_read_program_memory((uint8_t*)(&instruction), PC, 2)==0)
		{
			const uint32_t	prev_pc = PC;
			// Little-Endian?
#define	i_l	((uint8_t)instruction)
#define	i_h	((uint8_t)(instruction >> 8))
			const uint8_t	opcode = i_h & 0xFC;
			const uint8_t	opcode2bits = i_h & 0xC0;
			const uint8_t	opcode3bits = i_h & 0xE0;
			const uint8_t	opcode4bits = i_h & 0xF0;
			const uint8_t	opcode5bits = i_h & 0xF8;
			PC += 2;
			if (opcode2bits==0)
			{
				// Shift (immediate), add, subtract, move, and compare.
				const uint8_t	scode = i_h & 0x3E;
				const uint8_t	scode3bits = i_h & 0x38;
				const uint8_t	Rdn = instruction & 0x07;
				const uint8_t	Rm = (instruction >> 3) & 0x07; // Rn for some instructions.
				const uint8_t	Rm2 = (instruction >> 6) & 0x07; // Rm for some instructions.
				const uint8_t	imm5 = (instruction >> 6) & 0x1F;
				const uint8_t	imm3 = (instruction >> 6) & 0x07;
				if (scode3bits==0x00)
				{
					// pp00 0xxp
					// LSL (Logical Shift Left, immediate)
					_print_RRx("LSLS", Rdn, Rm, imm5);
					_shift_processing_immediate(_LSL_C, Rdn, Rm, imm5);
				}
				else if (scode3bits==0x08)
				{
					// pp00 1xxp
					// LSR (Logical Shift Right, immediate)
					_print_RRx("LSR", Rdn, Rm, imm5==0 ? 32 : imm5);
					_shift_processing_immediate(_LSR_C, Rdn, Rm, imm5==0 ? 32 : imm5);
				}
				else if (scode3bits==0x10)
				{
					// pp01 0xxp
					// ASR (Arithmetic Shift Right)
					_print_RRx("ASR", Rdn, Rm, imm5==0 ? 32 : imm5);
					_shift_processing_immediate(_ASR_C, Rdn, Rm, imm5==0 ? 32 : imm5);
				}
				else if (scode==0x18)
				{
					// pp01 100p
					// Add register (ADD)
					// Encoding T1
					_print_RRR("ADD", Rdn, Rm, Rm2);
					_AddWithCarry(Rdn, self->R[Rm], self->R[Rm2], 0);
				}
				else if (scode==0x1A)
				{
					// pp01 101p
					// SUB (Subtract register)
					_print_RRR("SUB", Rdn, Rm, Rm2);
					_AddWithCarry(Rdn, self->R[Rm], ~self->R[Rm2], 1);
				}
				else if (scode==0x1C)
				{
					// pp01 110p
					// ADD (Add 3-bit, immediate)
					_print_RRx("ADD", Rdn, Rm, imm3);
					_AddWithCarry(Rdn, self->R[Rm], imm3, 0);
				}
				else if (scode==0x1E)
				{
					// pp01 111p
					// SUB (Subtract 3-bit, immediate)
					_print_RRx("SUB", Rdn, Rm, imm3);
					_AddWithCarry(Rdn, self->R[Rm], ~((uint32_t)imm3), 1);
				}
				else
				{
					const uint8_t	Rd2 = (instruction >> 8) & 0x07;
					if (scode3bits==0x20)
					{
						// pp10 0xxp
						// MOV (Move, immediate)
						_print_Rx("MOV", Rd2, i_l);
						self->R[Rd2] = i_l;
						_set_APSR_of_NZ(i_l);
					}
					else if (scode3bits==0x28)
					{
						// pp10 1xxp
						// CMP (Compare, immediate)
						_print_Rx("CMP", Rd2, i_l);
						_AddWithCarryDiscard(self->R[Rd2], ~((uint32_t)(i_l)), 1);
					}
					else if (scode3bits==0x30)
					{
						// pp11 0xx
						// ADD (Add 8-bit, immediate)
						// Encoding: T2.
						_print_Rx("ADD", Rd2, i_l);
						_AddWithCarry(Rd2, self->R[Rd2], i_l, 0);
					}
					else if (scode3bits==0x38)
					{
						// pp11 1xxp
						// SUB (Subtract 8-bit, immediate)
						_print_Rx("SUB", Rd2, i_l);
						_AddWithCarry(Rd2, self->R[Rd2], ~((uint32_t)(i_l)), 1);
					}
					else
					{
						// oops!
						error_unknown_instruction();
					}
				}
			}
			else if (opcode==0x40) 
			{
				// Data processing.
				const uint8_t	Rdn = i_l & 0x07;
				const uint8_t	Rm = (i_l >> 3) & 0x07;
				const uint8_t	scode = (instruction >> 2) & 0xF0;
				switch (scode)
				{
				case 0x00:
					// AND (Bitwise AND)
					_print_RR("AND", Rdn, Rm);
					self->R[Rdn] = self->R[Rdn] & self->R[Rm];
					_set_APSR_of_NZC(self->R[Rdn], 0);
					break;
				case 0x10:
					// EOR (Exclusive OR)
					_print_RR("EOR", Rdn, Rm);
					self->R[Rdn] = self->R[Rdn] ^ self->R[Rm];
					_set_APSR_of_NZC(self->R[Rdn], 0);
					break;
				case 0x20:
					// LSL (Logical Shift Left)
					_print_RR("LSL", Rdn, Rm);
					_shift_processing(_LSL_C, Rdn, Rm);
					break;
				case 0x30:
					// LSR (Logical Shift Right)
					_print_RR("LSR", Rdn, Rm);
					_shift_processing(_LSR_C, Rdn, Rm);
					break;
				case 0x40:
					// ASR (Arithmetic Shift Right)
					_print_RR("ASR", Rdn, Rm);
					_shift_processing(_ASR_C, Rdn, Rm);
					break;
				case 0x50:
					// ADC (Add with Carry)
					_print_RR("ADC", Rdn, Rm);
					_AddWithCarry(Rdn, self->R[Rdn], self->R[Rm], APSR_C);
					break;
				case 0x60:
					// SBC (Subtract with Carry)
					_print_RR("SBC", Rdn, Rm);
					_AddWithCarry(Rdn, self->R[Rdn], ~self->R[Rm], APSR_C);
					break;
				case 0x70:
					// ROR (Rotate Right). Note: ARMv6-M doesn't support RRX.
					_print_RR("ROR", Rdn, Rm);
					_shift_processing(_ROR_C, Rdn, Rm);
					break;
				case 0x80:
					// TST (Set flags on bitwise AND)
					_print_RR("TST", Rdn, Rm);
					{
						const uint32_t	x = self->R[Rdn] & self->R[Rm];
						_set_APSR_of_NZC(x, 0);
					}
					break;
				case 0x90:
					// RSB (Reverse Subtract from 0)
					_print_RR("RSB", Rdn, Rm);
					_AddWithCarry(Rdn, ~self->R[Rm], 0, 1);
					break;
				case 0xA0:
					// CMP (Compare Registers)
					// Encoding T1:
					_print_RR("CMP", Rdn, Rm);
					_AddWithCarryDiscard(self->R[Rdn], ~self->R[Rm], 1);
					break;
				case 0xB0:
					// CMN (Compare Negative)
					_print_RR("CMN", Rdn, Rm);
					_AddWithCarryDiscard(self->R[Rdn], self->R[Rm], 0);
					break;
				case 0xC0:
					// ORR (Logical OR)
					_print_RR("ORR", Rdn, Rm);
					self->R[Rdn] = self->R[Rdn] | self->R[Rm];
					_set_APSR_of_NZC(self->R[Rdn], 0);
					break;
				case 0xD0:
					// MUL (Multiply Two Registers)
					_print_RR("MUL", Rdn, Rm);
					self->R[Rdn] = self->R[Rdn] * self->R[Rm];
					_set_APSR_of_NZ(self->R[Rdn]);
					break;
				case 0xE0:
					// BIC (Bit Clear)
					_print_RR("BIC", Rdn, Rm);
					self->R[Rdn] = self->R[Rdn] & (~self->R[Rm]);
					_set_APSR_of_NZC(self->R[Rdn], 0);
					break;
				case 0xF0:
					// MVN (Bitwise NOT)
					_print_RR("MVN", Rdn, Rm);
					self->R[Rdn] = ~self->R[Rm];
					_set_APSR_of_NZC(self->R[Rdn], 0);
					break;
				default:
					error_unknown_instruction();
					break;
				}
			}
			else if (opcode==0x44)
			{
				// Special data instructions and branch and exchange
				const uint8_t	scode =  (instruction >> 2) & 0xF0;
				const uint8_t	scode2bits = scode & 0xC0;
				const uint8_t	scode3bits = scode & 0xE0;
				const uint8_t	Rdn = ((i_l >> 4) & 0x08) | (i_l & 0x07);
				const uint8_t	Rm = (i_l >> 3) & 0x0F;
				if (scode2bits==0x00)
				{
					// ADD (Add registers).
					// Encoding T2
					const uint32_t	prev_APSR = self->APSR;
					_print_RR("ADD", Rdn, Rm);
					_AddWithCarry(Rdn, self->R[Rdn], self->R[Rm], 0);
					if (Rdn == INDEX_PC)
					{
						self->APSR = prev_APSR;
						set_PC(PC);
					}
					else if (Rdn == INDEX_SP)
					{
						self->APSR = prev_APSR;
					}
				}
				else if (scode==0x40)
				{
					// UNPREDICTABLE
					error_unknown_instruction();
				}
				else if ((scode==0x50) || (scode3bits==0x60))
				{
					// CMP (Compare registers).
					// Encoding: T2
					_print_RR("CMP", Rdn, Rm);
					_AddWithCarryDiscard(self->R[Rdn], ~self->R[Rm], 1);
				}
				else if (scode2bits==0x80)
				{
					// MOV (Move Registers)
					// Encoding: T1
					_print_RR("MOV", Rdn, Rm);
					self->R[Rdn] = self->R[Rm];
					// not in encoding T1: _set_APSR_of_NZ(self->R[Rdn]);
					if (Rdn==INDEX_PC)
					{
						set_PC(self->R[Rdn]);
					}
				}
				else
				{
					const uint8_t	Rm2 = (instruction >> 3) & 0x0F;
					if (scode3bits==0xC0)
					{
						// BX (Branch and Exchange)
						_print_R("BX", Rm2);
						if (Rm2 == INDEX_PC)
						{
							error_unknown_instruction();
						}
						else
						{
							set_PC(self->R[Rm2]);
						}
					}
					else if (scode3bits==0xE0)
					{
						// BLX (Branch with Link and Exchange)
						_print_R("BLX", Rm2);
						if (Rm2 == INDEX_PC)
						{
							error_unknown_instruction();
						}
						else
						{
							const uint32_t	target = self->R[Rm2];
							LR = PC | 1;
							set_PC(target);
						}
					}
					else
					{
						// oops!
						error_unknown_instruction();
					}
				}
			}
			else if (opcode5bits==0x48)
			{
				// Load from Literal Pool (LDR).
				const uint8_t				Rt = (instruction >> 8) & 0x07;
				ARM_EMULATOR_RETURN_VALUE	r;
				_print_Rx("LDR", Rt, i_l * 4);
				r = _fetch_data32(Rt, _Align4Down(PC+2) + i_l*4);
				if (r != ARM_EMULATOR_OK)
				{
					return r;
				}
			}
			else if (opcode4bits==0x50)
			{
				// Load/store single data item.
				const uint8_t	scode = i_h & 0xFE;
				const uint8_t	Rt = instruction & 0x07;
				const uint8_t	Rn = (instruction >> 3) & 0x07;
				const uint8_t	Rm = (instruction >> 6) & 0x07;
				ARM_EMULATOR_RETURN_VALUE	return_value = ARM_EMULATOR_ERROR;
				if (scode == 0x50)
				{
					// STR (Store Register)
					_print_RRR("STR", Rt, Rn, Rm);
					return_value = _store_data32(self->R[Rn] + self->R[Rm], self->R[Rt]);
				}
				else if (scode==0x52)
				{
					// STRH (Store Register Halfword)
					_print_RRR("STRH", Rt, Rn, Rm);
					return_value = _store_data16(self->R[Rn] + self->R[Rm], self->R[Rt]);
				}
				else if (scode==0x54)
				{
					// STRB (Store Register Byte)
					_print_RRR("STRB", Rt, Rn, Rm);
					return_value = _store_data8(self->R[Rn] + self->R[Rm], self->R[Rt]);
				}
				else if (scode==0x56)
				{
					// LDRSB (Load Register Signed Byte)
					_print_RRR("LDRSB", Rt, Rn, Rm);
					return_value = _fetch_data32(Rt, self->R[Rn] + self->R[Rm]);
					if (return_value == ARM_EMULATOR_OK)
					{
						self->R[Rt] = _SignExtendTo32(self->R[Rt], 7);
					}
				}
				else if (scode==0x58)
				{
					// LDR (Load Register)
					_print_RRR("LDR", Rt, Rn, Rm);
					return_value = _fetch_data32(Rt, self->R[Rn] + self->R[Rm]);
				}
				else if (scode==0x5A)
				{
					// LDRH (Load Register Halfword)
					_print_RRR("LDRH", Rt, Rn, Rm);
					return_value = _fetch_data16(Rt, self->R[Rn] + self->R[Rm]);
				}
				else if (scode==0x5C)
				{
					// LDRB (Load Register Byte)
					_print_RRR("LDRB", Rt, Rn, Rm);
					return_value = _fetch_data8(Rt, self->R[Rn] + self->R[Rm]);
				}
				else if (scode==0x5E)
				{
					//  LDRSH (Load Register Signed Halfword)
					_print_RRR("LDRSH", Rt, Rn, Rm);
					return_value = _fetch_data16(Rt, self->R[Rn] + self->R[Rm]);
					if (return_value == ARM_EMULATOR_OK)
					{
						self->R[Rt] = _SignExtendTo32(self->R[Rt], 15);
					}
				}
				else
				{
					error_unknown_instruction();
				}
				if (return_value!=ARM_EMULATOR_OK)
				{
					return return_value;
				}
			}
			else if (opcode3bits==0x60)
			{
				// Load/store single data item.
				const uint8_t	scode5bits = i_h & 0xF8;
				const uint8_t	Rt = instruction & 0x07;
				const uint8_t	Rn = (instruction >> 3) & 0x07;
				const uint8_t	imm5 = (instruction >> 6) & 0x1F;
				ARM_EMULATOR_RETURN_VALUE	return_value = ARM_EMULATOR_ERROR;
				if (scode5bits==0x60)
				{
					// STR (Store Register, immediate)
					_print_RRx("STR", Rt, Rn, imm5*4);
					return_value = _store_data32(self->R[Rn] + imm5*4, self->R[Rt]);
				}
				else if (scode5bits==0x68)
				{
					// LDR (Load Register, immediate)
					_print_RRx("LDR", Rt, Rn, imm5*4);
					return_value = _fetch_data32(Rt, self->R[Rn] + imm5*4);
				}
				else if (scode5bits==0x70)
				{
					// STRB (Store Register Byte, immediate)
					_print_RRx("STRB", Rt, Rn, imm5);
					return_value = _store_data8(self->R[Rn] + imm5, self->R[Rt]);
				}
				else if (scode5bits==0x78)
				{
					// LDRB (Load Register Byte, immediate)
					_print_RRx("LDRB", Rt, Rn, imm5);
					return_value = _fetch_data8(Rt, self->R[Rn] + imm5);
				}
				else 
				{
					error_unknown_instruction();
				}
				if (return_value!=ARM_EMULATOR_OK)
				{
					return return_value;
				}
			}
			else if (opcode3bits==0x80)
			{
				// Load/store single data item.
				const uint8_t	scode5bits = i_h & 0xF8;
				const uint8_t	Rt = instruction & 0x07;
				const uint8_t	Rn = (instruction >> 3) & 0x07;
				const uint8_t	Rt2 = (instruction >> 8) & 0x07;
				const uint8_t	imm5 = (instruction >> 6) & 0x1F;
				ARM_EMULATOR_RETURN_VALUE	return_value = ARM_EMULATOR_ERROR;
				if (scode5bits==0x80)
				{
					// STRH (Store Register Halfword, immediate)
					_print_RRx("STRH", Rt, Rn, 2*imm5);
					return_value = _store_data16(self->R[Rn] + 2*imm5, self->R[Rt]);
				}
				else if (scode5bits==0x88)
				{
					// LDRH (Load Register Halfword, immediate)
					_print_RRx("LDRH", Rt, Rn, 2*imm5);
					return_value = _fetch_data16(Rt, self->R[Rn] + 2*imm5);
				}
				else if (scode5bits==0x90)
				{
					// STR (Store Register SP relative, immediate)
					_print_RRx("STR", Rt2, INDEX_SP, 4*i_l);
					return_value = _store_data32(SP + 4*i_l, self->R[Rt2]);
				}
				else if (scode5bits==0x98)
				{
					// LDR (Load Register SP relative, immediate)
					_print_RRx("LDR", Rt2, INDEX_SP, 4*i_l);
					return_value = _fetch_data32(Rt2, SP + 4*i_l);
				}
				else 
				{
					// oops!
					error_unknown_instruction();
				}
				if (return_value!=ARM_EMULATOR_OK)
				{
					return return_value;
				}
			}
			else if (opcode5bits==0xA0)
			{
				// Generate PC-relative address (ADR)
				const uint8_t	Rd = i_h & 0x07;
				const uint32_t	addr = _Align4Down(PC + 2) + 4*i_l;
				_print_Rx("ADR", Rd, addr);
				self->R[Rd] = addr;
			}
			else if (opcode5bits==0xA8)
			{
				// Generate SP-relative address (ADD, SP plus immediate).
				// Encoding: T1.
				const uint8_t	Rd = i_h & 0x07;
				const uint16_t	imm8 = 4*i_l;
				_print_RRx("ADD", Rd, INDEX_SP, imm8);
				self->R[Rd] = SP + imm8;
			}
			else if (opcode4bits==0xB0)
			{
				// Miscellaneous 16-bit instructions.
				const uint8_t	scode = instruction >> 4 & 0xFE;
				const uint8_t	scode3bits = scode & 0xE0;
				const uint8_t	scode4bits = scode & 0xF0;
				const uint8_t	scode5bits = scode & 0xF8;
				const uint8_t	scode6bits = scode & 0xFC;
				const uint8_t	imm7 = instruction & 0x7F;
				const uint8_t	Rd = instruction & 0x07;
				const uint8_t	Rm = (instruction >> 3) & 0x07;
				if (scode5bits==0x00)
				{
					// ADD (Add Immediate to SP)
					// Encoding T2
					_print_RRx("ADD", INDEX_SP, INDEX_SP, 4*imm7);
					SP = SP + 4*imm7;
				}
				else if (scode5bits==0x08)
				{
					// SUB (Subtract Immediate from SP)
					_print_RRx("SUB", INDEX_SP, INDEX_SP, 4*imm7);
					SP = SP - 4*imm7;
				}
				else if (scode6bits==0x20)
				{
					// SXTH (Signed Extend Halfword)
					_print_RR("SXTH", Rd, Rm);
					self->R[Rd] = _SignExtendTo32(self->R[Rm], 15);
				}
				else if (scode6bits==0x24)
				{
					// SXTB (Signed Extend Byte)
					_print_RR("SXTB", Rd, Rm);
					self->R[Rd] = _SignExtendTo32(self->R[Rm], 7);
				}
				else if (scode6bits==0x28)
				{
					// UXTH (Unsigned Extend Halfword)
					_print_RR("UXTH", Rd, Rm);
					self->R[Rd] = (const uint16_t)self->R[Rm];
				}
				else if (scode6bits==0x2C)
				{
					// UXTB (Unsigned Extend Byte)
					_print_RR("UXTB", Rd, Rm);
					self->R[Rd] = (const uint8_t)self->R[Rm];
				}
				else if (scode3bits==0x40)
				{
					// PUSH (Push Multiple Registers)
					const uint16_t	list = (i_h & 1)*0x4000 | i_l;
					int8_t			i;

					_print_PUSH(prev_pc, i_h & 1, i_l);
					for (i=14; i>=0; --i)
					{
						if (list & (1 << i))
						{
							const ARM_EMULATOR_RETURN_VALUE	r = _store_data32(SP-4, self->R[i]);
							if (r==ARM_EMULATOR_OK)
							{
								SP -= 4;
							}
							else
							{
								return r;
							}
						}
					}
				}
				else if (scode==0x66)
				{
					// CPS (Change Processor State)
					_print_x(((opcode >> 4) & 1) ? "CPSIM" : "CPSI0", 0);
					// Not needed.
				}
				else if (scode6bits==0xA0)
				{
					// REV (Byte-Reverse Word)
					const uint32_t	x = self->R[Rm];
					_print_RR("REV", Rd, Rm);
					self->R[Rd] = ((x << 24) & 0xFF000000)
								| ((x <<  8) & 0x00FF0000)
								| ((x >>  8) & 0x0000FF00)
								| ((x >> 24) & 0x000000FF);
				}
				else if (scode6bits==0xA4)
				{
					// REV16 (Byte-Reverse Packed Halfword)
					const uint32_t	x = self->R[Rm];
					_print_RR("REV16", Rd, Rm);
					self->R[Rd] = ((x << 8) & 0xFF00FF00)
								| ((x >> 8) & 0x00FF00FF);
				}
				else if (scode6bits==0xAC)
				{
					// REVSH (Byte-Reverse Signed Halfword)
					const uint32_t	x = self->R[Rm];
					_print_RR("REVSH", Rd, Rm);
					self->R[Rd] = (_SignExtendTo32(x & 0xFF, 7) << 8) | ((x >> 8) & 0xFF);
				}
				else if (scode3bits==0xC0)
				{
					// POP (Pop Multiple Registers.
					const uint8_t	p = (instruction >> 8) & 1;
					uint8_t			i;

					_print_POP(prev_pc, p, i_l);
					for (i=0; i<8; ++i)
					{
						if (i_l & (1<<i))
						{
							const ARM_EMULATOR_RETURN_VALUE	r = _fetch_data32(i, SP);
							if (r==ARM_EMULATOR_OK)
							{
								SP += 4;
							}
							else
							{
								return r;
							}
						}
					}
					if (p)
					{
						const ARM_EMULATOR_RETURN_VALUE	r = _fetch_data32(INDEX_PC, SP);
						if (r==ARM_EMULATOR_OK)
						{
							SP += 4;
							set_PC(PC);
						}
						else
						{
							return r;
						}
					}
				}
				else if (scode4bits==0xE0)
				{
					// BKPT (Breakpoint)
					// NOT IMPLEMENTED.
					_print("BKPT");
				}
				else if (scode4bits==0xF0)
				{
					// Hints??
					const uint8_t	scode = i_l;
					if (scode == 0x00)
					{
						// NOP
					}
					else
					{
						// NOT IMPLEMENTED :)
						error_unknown_instruction();
					}
				}
				else
				{
					// oops!
					error_unknown_instruction();
				}
			}
			else if (opcode5bits==0xC0)
			{
				// Store multiple registers (STM, STMIA, STMEA)
				const uint8_t	Rn = i_h & 0x07;
				int8_t			i;

				_print_STM(prev_pc, Rn, i_l);
				for (i=0; i<8; ++i)
				{
					if (i_l & (1 << i))
					{
						const ARM_EMULATOR_RETURN_VALUE	r = _store_data32(self->R[Rn], self->R[i]);
						if (r==ARM_EMULATOR_OK)
						{
							self->R[Rn] += 4;
						}
						else
						{
							return r;
						}
					}
				}
			}
			else if (opcode5bits==0xC8)
			{
				// Load multiple registers (LDM, LDMIA, LDMFD)
				// ! modifier always present.
				const uint8_t	Rn = i_h & 0x07;
				const uint8_t	writeback = i_l & (1<<Rn);
				uint32_t		addr = self->R[Rn];
				int8_t			i;

				_print_LDM(prev_pc, Rn, i_l);
				for (i=0; i<8; ++i)
				{
					if (i_l & (1 << i))
					{
						const ARM_EMULATOR_RETURN_VALUE	r = _fetch_data32(i, addr);
						if (r==ARM_EMULATOR_OK)
						{
							addr += 4;
							if (writeback)
							{
								self->R[Rn] = addr;
							}
						}
						else
						{
							return r;
						}
					}
				}
				self->R[Rn] = addr;
			}
			else if (opcode4bits==0xD0)
			{
				// Conditional branch, and Supervisor Call.
				const uint8_t	cond = i_h & 0x0F;
				// 1101 opcode
				if (cond==0x0E || cond==0x0F)
				{
					// SVC (Supervisor Call) - NOT IMPLEMENTED.
					error_unknown_instruction();
				}
				else
				{
					// B (Conditional branch).
					const int32_t	imm32 = _SignExtendTo32(i_l*2, 8);
					const uint32_t	addr = PC + 2 + imm32;
					_print_BC(cond, addr);
					if (_ConditionPassed(cond))
					{
						// do the branch...
						set_PC(addr);
					}
				}
			}
			else if (opcode5bits==0xE0)
			{
				// Unconditional branch (B)
				const uint16_t	imm11 = 2*(instruction & 0x7FF);
				const uint32_t	new_pc = PC + 2 + _SignExtendTo32(imm11, 11);
				_print_x("B", new_pc);
				set_PC(new_pc);
			}
			else if ((opcode3bits == 0xE0) && (opcode5bits!=0xE0))
			{
				// 32-bit Thumb instruction
				// Little-Endian?
				uint16_t	instruction2;
				if (arm_emulator_callback_read_program_memory((uint8_t*)(&instruction2), PC, 2) == 0)
				{
					PC += 2;
				}
				else
				{
					xprintf(("ARM Emulator: unable to read instruction.\n"));
					return ARM_EMULATOR_ERROR;
				}
#define	i2_l	((uint8_t)instruction2)
#define	i2_h	((uint8_t)(instruction2 >> 8))
				if ((opcode5bits== 0xF0) && ((i2_h & 0x80) == 0x80))
				{
					// Branch and miscellaneous control
					const uint8_t	op1 = ((i_h << 3) & 0xE0) | ((i_l>>3) & 0x1E);
					const uint8_t	op2 = (i2_h << 1) & 0xE0;
					const uint8_t	op16bits = op1 & 0xFC;
					if ((op2 & 0xA0) == 0)
					{
						if (op16bits == 0x70)
						{
							// MSR (Move to Special Register)
							// Only APSR supported.
							if (i2_l ==0)
							{
								const uint8_t	Rn = instruction & 0x07;
								_print_R("MSR APSR, ", Rn);
								self->APSR = self->R[Rn] & 0xF8000000;
							}
							else
							{
								// Not implemented.
								error_unknown_instruction();
							}
						}
						else if (op1 == 0x76)
						{
							// Miscellaneous control instructions
							// DSB (Data Synchronization Barrier) - Not needed.
							// DMB (Data Memory Barrier) - Not needed.
							// ISB (Instruction Synchronization Barrier) - Not needed.
						}
						else if (op16bits==0x7C)
						{
							// MRS (Move from Special Register)
							// Only APSR supported.
							if (i2_l ==0)
							{
								const uint8_t	Rn = instruction & 0x07;
								iprintf(("MRS %s, APSR\n", _rnames[Rn]));
								self->R[Rn] = self->APSR;
							}
							else
							{
								// Not implemented.
								// TODO: should do it?
								error_unknown_instruction();
							}
						}
						else
						{
							error_unknown_instruction();
						}
					}
					else if (op2==0x40 && op1==0xFE)
					{
						// UDF (Permanently Undefined)
						error_unknown_instruction();
					}
					else if ((op2 & 0xA0) == 0xA0)
					{
						// BL (Branch with Link)
						// Bits distribution:
						// 0: 0
						// 1..11: imm11
						// 12..21: imm10
						// 22: !(j1 xor s)
						// 23: !(j2 xor s)
						// 24: s
						const unsigned int	s = (instruction >> 10) & 1;
						const unsigned int	j1 = (instruction2 >> 13) & 1;
						const unsigned int	j2 = (instruction2 >> 11) & 1;
						const uint32_t	x =
							  ((instruction2 & 0x7FF) << 1)
							| ((instruction & 0x3FF) << 12)
							| ((1 - (j2 ^ s)) * (1<<22))
							| ((1 - (j1 ^ s)) * (1<<23))
							| (s * (1<<24));
						const uint32_t	new_PC = PC + _SignExtendTo32(x, 24);
						LR = PC | 1;
						_print_x("BL", new_PC);
						set_PC(new_PC);
					}
					else
					{
						error_unknown_instruction();
					}
				}
				else
				{
					error_unknown_instruction();
				}
			}
			else
			{
				// oops!
				error_unknown_instruction();
			}
		}
		else
		{
			xprintf(("PC=0x%04X is out of the permitted range!\n", PC));
			return ARM_EMULATOR_ERROR;
		}
	}

	return ARM_EMULATOR_OK;
}

//================================================================================================================
uint32_t
arm_emulator_get_function_return_value(void)
{
	return self->R[0];
}

//================================================================================================================
void
arm_emulator_dump(void)
{
	const uint32_t	apsr = self->APSR;
	uint8_t	i;
	for (i=0; i<16; ++i)
	{
		uart_write(_rnames[i]);
		uart_write_hex32(":", self->R[i]);
		if (((i+1) % 4) == 0)
		{
			uart_write_crlf();
		}
		else
		{
			uart_write(" ");
		}
	}
	uart_write("APSR:");
	uart_write((apsr & (1<<FLAG_N_BIT)) ? "N" : "n");
	uart_write((apsr & (1<<FLAG_Z_BIT)) ? "Z" : "z");
	uart_write((apsr & (1<<FLAG_C_BIT)) ? "C" : "c");
	uart_write((apsr & (1<<FLAG_V_BIT)) ? "V" : "v");
	uart_write_hex32(" = 0x", apsr);
	uart_write_crlf();
}
