// SPDX-License-Identifier: MIT
#include <assert.h>	// programmer's best friend.
#include <stdint.h>	// int8_t
#include <stdio.h>	// printf
#include <string.h>	// memcpy
#include <stdlib.h>	// rand and srand.

#include "testcase.h"
#include "arm_emulator.h"

/// End of register definition.
#define	REG_END		{-1,-1}
/// No memory access.
#define	NO_MEMORY	{0}
/// No registers.
#define	NO_REGISTERS		{ REG_END }
/// 16-bit instruction only.
#define	INSTRUCTION_16BIT	(-1)

// Bits are right-aligned.
// Register ordering might differ from the one in assembler instruction!
// #define	INSTR_6_4_Rm_Rdn(bits6, bits4, Rm, Rdn) (uint16_t)((bits6 << 10) | (bits4 << 6) | (Rm << 3) | Rdn)
#define	INSTR_10_Rm_Rdn(bits10, Rm, Rdn)			((bits10 << 6) | (Rm << 3) | Rdn), INSTRUCTION_16BIT
#define	INSTR_10_Rm_Rn(bits10, Rm, Rn)				INSTR_10_Rm_Rdn(bits10, Rm, Rn)
#define	INSTR_10_Rm_Rd(bits10, Rm, Rd)				INSTR_10_Rm_Rdn(bits10, Rm, Rd)
#define	INSTR_10_Rn_Rd(bits10, Rn, Rd)				INSTR_10_Rm_Rdn(bits10, Rn, Rd)
#define	INSTR_9_RRm_000(bits9, RRm)					((bits9 << 7) | (RRm << 3)), INSTRUCTION_16BIT
#define	INSTR_9_imm7(bits9, imm7)					((bits9 << 7) | (imm7 & 0x7F)), INSTRUCTION_16BIT
// bits8 : DN : Rm : Rdn<0:2>
#define	INSTR_8_RRm_RRdn(bits8, RRm, RRdn)			((bits8 << 8) | ((RRdn & 0x8) << 4) | (RRm << 3) | (RRdn & 0x7)), INSTRUCTION_16BIT
#define	INSTR_8_RRm_RRn(bits8, RRm, RRn)			INSTR_8_RRm_RRdn(bits8, RRm, RRn)
#define	INSTR_8_RRm_RRd(bits8, RRm, RRd)			INSTR_8_RRm_RRdn(bits8, RRm, RRd)
// bits8 : DM : bits4 : Rdm<0:2>
#define	INSTR_8_4_RRdm(bits8, bits4, RRdm)			INSTR_8_RRm_RRdn(bits8, bits4, RRdm)
// bits8: bits1 : Rm : bits3
#define	INSTR_8_1_RRm_3(bits8, bits1, RRdm, bits3)	((bits8<<8) | (bits1 << 7) | (RRdm << 3) | bits3), INSTRUCTION_16BIT
#define INSTR_7_imm3_Rn_Rd(bits7, imm3, Rn, Rd)		((bits7 << 9) | ((imm3 & 7) << 6) | (Rn << 3) | Rd), INSTRUCTION_16BIT
#define	INSTR_7_Rm_Rn_Rd(bits7, Rm, Rn, Rd)			((bits7 << 9) | (Rm << 6) | (Rn << 3) | Rd), INSTRUCTION_16BIT
#define	INSTR_7_Rm_Rn_Rt(bits7, Rm, Rn, Rt)			INSTR_7_Rm_Rn_Rd(bits7, Rm, Rn, Rt)
#define	INSTR_5_imm5_Rm_Rd(bits5, imm5, Rm, Rd)		((bits5<<11) | ((imm5 & 0x1F) << 6) | (Rm << 3) | Rd), INSTRUCTION_16BIT
#define	INSTR_5_imm5_Rn_Rt(bits5, imm5, Rn, Rt)		INSTR_5_imm5_Rm_Rd(bits5, imm5, Rn, Rt)
#define	INSTR_5_Rdn_imm8(bits5, Rdn, imm8)			((bits5 << 11) | (Rdn << 8) | (imm8 & 0xFF)), INSTRUCTION_16BIT
#define	INSTR_5_Rn_imm8(bits5, Rn, imm8)			INSTR_5_Rdn_imm8(bits5, Rn, imm8)
#define	INSTR_5_Rt_imm8(bits5, Rt, imm8)			INSTR_5_Rdn_imm8(bits5, Rt, imm8)
#define	INSTR_5_Rd_imm8(bits5, Rd, imm8)			INSTR_5_Rdn_imm8(bits5, Rd, imm8)
#define	INSTR_5_imm11(bits5, imm11)					((bits5 << 11) | (imm11 & 0x7FF)), INSTRUCTION_16BIT
#define	INSTR_4_cond_imm8(bits4, cond, imm8)		((bits4 << 12) | (cond << 8) | (imm8 & 0xFF)), INSTRUCTION_16BIT
#define	S_BL(imm24)		((imm24 >> 23) & 1)
#define	J1_BL(imm24)	(1 - (((imm24 >> 22) & 1) ^ S_BL(imm24)))
#define	J2_BL(imm24)	(1 - (((imm24 >> 21) & 1) ^ S_BL(imm24)))
// BL: 32-bit instruction
#define	INSTR_BL_imm24(imm24)						(0xF000 | (S_BL(imm24) << 10) | (imm24 >> 11) & 0x3FF), (0xD000 | (J1_BL(imm24) << 13) | (J2_BL(imm24) << 11) | (imm24 & 0x7FF))

// #define	INSTR_BLX_RRm(RRm)							(0x4780 | (RRm << 3)), INSTRUCTION_16BIT

#define TESTCASE_BRANCH(scond, cond, offset, flags, takes_jump) \
	{ "b" scond " +#" #offset, INSTR_4_cond_imm8(0xD, (cond), offset/2), \
	{ {INDEX_APSR, flags }, REG_END }, NO_MEMORY, \
	{ {INDEX_PC, TESTCASE_DEFAULT_PC+(takes_jump ? 4 + offset : 2) }, REG_END }, NO_MEMORY }

#define	TESTCASE_BRANCH_AND_LINK(offset) \
	{ "b +#" #offset, INSTR_BL_imm24((offset/2)), \
	NO_REGISTERS, NO_MEMORY, \
	{ {INDEX_PC, TESTCASE_DEFAULT_PC+4 + offset }, { INDEX_LR, (TESTCASE_DEFAULT_PC + 4) | 1}, REG_END}, NO_MEMORY }

const struct testcase testcases[] =
{
	// name							I16.0	I16.1	R.BEFORE
/* ADC */
	{ "adc r2, r3 = 0x415A", INSTR_10_Rm_Rdn(0x105, 3, 2),
	{ {2, 0x88776655}, {3, 0x99887766}, {INDEX_APSR, FLAG_N}, REG_END}, NO_MEMORY,
	{ {2, 0x21ffddbb}, {INDEX_APSR, FLAG_C|FLAG_V }, REG_END}, NO_MEMORY },
/* ADC */
	{ "adc r2, r3 ; test carry_in", INSTR_10_Rm_Rdn(0x105, 3, 2),
	{ {2, 0x88776655}, {3, 0x99887766}, {INDEX_APSR, FLAG_N|FLAG_C}, REG_END}, NO_MEMORY,
	{ {2, 0x21ffddbc}, {INDEX_APSR, FLAG_C|FLAG_V }, REG_END}, NO_MEMORY },
/* ADD (immediate), Encoding T1: 3bit immediate. */
	{ "add r2, r3,#5; Encoding T1 = 0x1D5A", INSTR_7_imm3_Rn_Rd(0x0E, 5, 3, 2),
	{ {2, 0x88776655}, {3, 0x99887766}, {INDEX_APSR, FLAG_N}, REG_END}, NO_MEMORY,
	{ {2, 0x9988776B}, {INDEX_APSR, FLAG_N }, REG_END}, NO_MEMORY },
/* ADD (immediate), Encoding T2: 8bit immediate. */
	{ "add, r2, 0x81; Encoding T2 = 0x3281", INSTR_5_Rdn_imm8(0x06, 2, 0x81),
	{ {2, 0x88776655}, {INDEX_APSR, FLAG_N}, REG_END}, NO_MEMORY,
	{ {2, 0x887766D6}, {INDEX_APSR, FLAG_N}, REG_END}, NO_MEMORY},
/* ADD (register), Encoding T1 */
	{ "add r0, r2, r7", INSTR_7_Rm_Rn_Rd(0x0C, 7, 2, 0),
	{ {7, 0x1111}, {2, 0x2222}, {INDEX_APSR, FLAG_Z}, REG_END}, NO_MEMORY,
	{ {0, 0x3333}, {INDEX_APSR, 0}, REG_END}, NO_MEMORY },
/* ADD (register), Encoding T2 */
	{ "add r10, r3", INSTR_8_RRm_RRdn(0x44, 3,10),
	{ {3, 0x2222}, {10, 0x4444}, {INDEX_APSR, FLAG_V}, REG_END}, NO_MEMORY,
	{ {10, 0x6666}, {INDEX_APSR, 0}, REG_END}, NO_MEMORY },
/* ADD (SP plus immediate) */
	{ "add r5, sp, #1020", INSTR_5_Rdn_imm8(0x15, 5, 1020/4),
	{ {INDEX_SP, 0x5555}, REG_END}, NO_MEMORY,
	{ {5, 0x5555+1020}, REG_END}, NO_MEMORY },
/* ADD (SP plus register), Encoding T1 */
	{ "add r8, sp, r8; Encoding T1", INSTR_8_4_RRdm(0x44, INDEX_SP, 8),
	{ {INDEX_SP, 0xBBBB}, {8, 0x1111}, {INDEX_APSR, FLAG_C}, REG_END}, NO_MEMORY,
	{ {8, 0xCCCC}, {INDEX_APSR, 0}, REG_END}, NO_MEMORY },
/* ADD (SP plus register), Encoding T2 */
	{ "add sp, r11; Encoding T2", INSTR_8_1_RRm_3(0x44, 0x1, 11, 0x5),
	{ {INDEX_SP, 0xFFFF }, { 11, 0x0001}, REG_END}, NO_MEMORY,
	{ {INDEX_SP, 0x10000}, REG_END}, NO_MEMORY },
/* ADR */
	{ "adr r6, #540; default_pc + 0", INSTR_5_Rdn_imm8(0x14, 6, 540/4),
	{ { INDEX_PC, TESTCASE_DEFAULT_PC }, REG_END }, NO_MEMORY,
	{ { 6, TESTCASE_DEFAULT_PC + 4 + 540 }, REG_END }, NO_MEMORY },
/* ADR - aligned */
	{ "adr r6, #540 ; default_pc+2 ", INSTR_5_Rdn_imm8(0x14, 6, 540/4),
	{ { INDEX_PC, TESTCASE_DEFAULT_PC + 2 }, REG_END }, NO_MEMORY,
	{ { 6, TESTCASE_DEFAULT_PC + 4 + 540 }, REG_END }, NO_MEMORY },
/* AND (register) */
	{ "and r5, r4; PSR=NZCV", INSTR_10_Rm_Rdn(0x100, 4, 5),
	{ { 4, 0xFFFF3333 }, { 5, 0x4444FFFF }, { INDEX_APSR, FLAG_N|FLAG_Z|FLAG_C|FLAG_V}, REG_END}, NO_MEMORY,
	{ { 5, 0x44443333 }, { INDEX_APSR, FLAG_V}, REG_END }, NO_MEMORY },
/* AND (register) */
	{ "and r5, r4; PSR=NZCv", INSTR_10_Rm_Rdn(0x100, 4, 5),
	{ { 4, 0xFFFF3333 }, { 5, 0x4444FFFF }, { INDEX_APSR, FLAG_N|FLAG_Z|FLAG_C}, REG_END}, NO_MEMORY,
	{ { 5, 0x44443333 }, { INDEX_APSR, 0}, REG_END }, NO_MEMORY },
/* ASR (immediate) */
	{ "asr r3, r2, 30", INSTR_5_imm5_Rm_Rd(0x02, 30, 2, 3),
	{ { 2, 0x6ABCDEF5 }, { INDEX_APSR, FLAG_V }, REG_END }, NO_MEMORY,
	{ { 3, 0x00000001 }, { INDEX_APSR, FLAG_C | FLAG_V }, REG_END }, NO_MEMORY },
/* ASR (immediate) */
	{ "asr r3, r2, 30", INSTR_5_imm5_Rm_Rd(0x02, 30, 2, 3),
	{ { 2, 0x8ABCDEF5 }, { INDEX_APSR, FLAG_V }, REG_END }, NO_MEMORY,
	{ { 3, 0xFFFFFFFE }, { INDEX_APSR, FLAG_N|FLAG_V }, REG_END }, NO_MEMORY },
/* ASR (immediate) */
	{ "asr r3, r2, 32", INSTR_5_imm5_Rm_Rd(0x02, 0, 2, 3),
	{ { 2, 0xCABCDEF5 }, { INDEX_APSR, FLAG_V }, REG_END }, NO_MEMORY,
	{ { 3, 0xFFFFFFFF }, { INDEX_APSR, FLAG_C|FLAG_N|FLAG_V }, REG_END }, NO_MEMORY },
/* ASR (register) */
	{ "asr r1, r3", INSTR_10_Rm_Rdn(0x104, 3, 1),
	{ { 1, 0x5555FFFF }, { 3, 16 }, { INDEX_APSR, 0 }, REG_END }, NO_MEMORY,
	{ { 1, 0x00005555 }, { INDEX_APSR, FLAG_C }, REG_END }, NO_MEMORY },
/* B: Encoding T1 */
	TESTCASE_BRANCH("eq", COND_EQ, 254, 0, 0),
	TESTCASE_BRANCH("eq", COND_EQ, 254, FLAG_Z, 1),
	TESTCASE_BRANCH("ne", COND_NE, 254, 0, 1),
	TESTCASE_BRANCH("ne", COND_NE, 254, FLAG_Z, 0),
	TESTCASE_BRANCH("eq", COND_EQ, -256, FLAG_Z, 1),
	TESTCASE_BRANCH("eq", COND_EQ, -16, FLAG_Z, 1),
/* B: Encoding T2 */
	{ "b #2020", INSTR_5_imm11(0x1C, 2020/2),
	NO_REGISTERS, NO_MEMORY, \
	{ {INDEX_PC, TESTCASE_DEFAULT_PC + 4 + 2020 }, REG_END }, NO_MEMORY },
/* B: Encoding T2 */
	{ "b #-2048", INSTR_5_imm11(0x1C, -2048/2),
	NO_REGISTERS, NO_MEMORY, \
	{ {INDEX_PC, TESTCASE_DEFAULT_PC + 4 - 2048 }, REG_END }, NO_MEMORY },
/* BIC (register) */
	{ "bic r1, r0", INSTR_10_Rm_Rdn(0x10E, 0, 1),
	{ {1, 0xFFFF}, {0, 0x8421}, {INDEX_APSR, FLAG_C|FLAG_Z|FLAG_N}, REG_END }, NO_MEMORY,
	{ {1, 0x7BDE}, {INDEX_APSR, 0}, REG_END }, NO_MEMORY },
/* BKPT: not implemented. */
/* BL: */
	TESTCASE_BRANCH_AND_LINK(1020),
/* BL: from the plugin.s */
	{ "b #-268", 0xf7ff, 0xff7a,
	{ {INDEX_PC, 0x7124}, REG_END}, NO_MEMORY,
	{ {INDEX_PC, 0x701c}, { INDEX_LR, (0x7124 + 4) | 1}, REG_END}, NO_MEMORY },
/* BLX (register) */
	{ "blx r11", INSTR_9_RRm_000(0x08F, 11),
	{ {INDEX_PC, TESTCASE_DEFAULT_PC}, {11, 0x7348}, REG_END}, NO_MEMORY,
	{ {INDEX_PC, 0x7348}, {INDEX_LR, (TESTCASE_DEFAULT_PC+2) | 1}, REG_END}, NO_MEMORY},
/* BX (register) */
	{ "bx r11", INSTR_9_RRm_000(0x08E, 11),
	{ {INDEX_PC, TESTCASE_DEFAULT_PC}, {11, 0x7348}, REG_END}, NO_MEMORY,
	{ {INDEX_PC, 0x7348}, REG_END}, NO_MEMORY},
/* CMN (register) */
	{ "cmn r0, r1", INSTR_10_Rm_Rn(0x10B, 1, 0), 
	{ {0, 200}, {1, -400}, {INDEX_APSR, 0xF << 28}, REG_END}, NO_MEMORY,
	{ {INDEX_APSR, FLAG_N}, REG_END}, NO_MEMORY},
/* CMN (register) */
	{ "cmn r0, r1", INSTR_10_Rm_Rn(0x10B, 1, 0), 
	{ {0, 200}, {1, -100}, {INDEX_APSR, 0xF << 28}, REG_END}, NO_MEMORY,
	{ {INDEX_APSR, FLAG_C}, REG_END}, NO_MEMORY},
/* CMP (immediate) */
	{ "cmp r3, #22", INSTR_5_Rn_imm8(0x05, 3, 22),
	{ { 3, 11 }, {INDEX_APSR, 0}, REG_END}, NO_MEMORY,
	{ { INDEX_APSR, FLAG_N}, REG_END}, NO_MEMORY},
/* CMP (register): Encoding T1 */
	{ "cmp r3, r4", INSTR_10_Rm_Rn(0x10A, 4, 3),
	{ { 3, 11 }, { 4, 11}, {INDEX_APSR, 0}, REG_END}, NO_MEMORY,
	{ { INDEX_APSR, FLAG_Z|FLAG_C}, REG_END}, NO_MEMORY},
/* CMP (register): Encoding T2 */
	{ "cmp r11, r12", INSTR_8_RRm_RRn(0x45, 12, 11),
	{ { 12, -1 }, { 11, -1}, {INDEX_APSR, 0}, REG_END}, NO_MEMORY,
	{ { INDEX_APSR, FLAG_Z|FLAG_C}, REG_END}, NO_MEMORY},
/* CPS: not implemented. */
/* CPY: see MOV. */
/* DMB: not implemented. */
/* DSB: not implemented. */
/* EOR (register) */
	{ "eor r4, r5", INSTR_10_Rm_Rdn(0x101, 5, 4),
	{ {4, 0xFF330033}, {5, 0x440044FF}, {INDEX_APSR, FLAG_V}, REG_END}, NO_MEMORY,
	{ {4, 0xBB3344CC}, {INDEX_APSR, FLAG_N|FLAG_V}, REG_END}, NO_MEMORY},
/* ISB: not implemented. */
/* LDM */
	{ "ldm r6!, {r7, r0}", INSTR_5_Rn_imm8(0x19, 6, ((1<<7)|(1<<0))),
	{ {6, TESTCASE_PLUGIN_DATA_ADDRESS}, REG_END}, { 8, TESTCASE_PLUGIN_DATA_ADDRESS, "\x04\x03\x03\x02\x08\x07\x06\x05"},
	{ {6, TESTCASE_PLUGIN_DATA_ADDRESS+8}, {0, 0x02030304}, {7, 0x05060708}, REG_END}, NO_MEMORY},
/* LDR (immediate): Encoding T1. */
	{ "ldr r4, r0, #16", INSTR_5_imm5_Rn_Rt(0x0D, 4, 0, 4),
	{ { 0, TESTCASE_PLUGIN_API_ADDRESS+100}, REG_END}, { 4, TESTCASE_PLUGIN_API_ADDRESS+100+16, "\xFF\x55\xFF\x11"},
	{ { 4, 0x11FF55FF}, REG_END}, NO_MEMORY},
/* LDR (immediate): Encoding T2. */
	{ "ldr r7, sp, +#1000", INSTR_5_Rt_imm8(0x13, 7, 1000/4),
	{ { INDEX_SP, TESTCASE_PLUGIN_DATA_ADDRESS}, REG_END}, { 4, TESTCASE_PLUGIN_DATA_ADDRESS+1000, "\x67\x45\x23\x01"},
	{ { 7, 0x01234567 }, REG_END}, NO_MEMORY},
/* LDR (literal) */
	{ "ldr r3,  +#28", INSTR_5_Rt_imm8(0x09, 3, 28/4),
	{ {INDEX_PC, 0x7056}, REG_END}, { 4, 0x7056+2+28, "\x78\x56\x34\x12" },
	{ {3, 0x12345678}, REG_END}, NO_MEMORY},
/* LDR (literal) */
	{ "ldr r3,  +#28", INSTR_5_Rt_imm8(0x09, 3, 28/4),
	{ {INDEX_PC, 0x7054}, REG_END}, { 4, 0x7054+4+28, "\x78\x56\x34\x12" },
	{ {3, 0x12345678}, REG_END}, NO_MEMORY},
/* LDR (register) */
	{ "ldr r3, r4, r5", INSTR_7_Rm_Rn_Rt(0x2C, 5, 4, 3),
	{ {4, TESTCASE_PLUGIN_DATA_ADDRESS}, {5, 100}, REG_END}, {4, TESTCASE_PLUGIN_DATA_ADDRESS+100, "\xFF\x00\xFF\x00"},
	{ {3, 0x00FF00FF}, REG_END}, NO_MEMORY},
/* LDRB (immediate) */
	{ "ldrb r1, r0, +#30", INSTR_5_imm5_Rn_Rt(0x0F, 30, 0, 1),
	{ {0, TESTCASE_PLUGIN_DATA_ADDRESS}, REG_END}, {1, TESTCASE_PLUGIN_DATA_ADDRESS+30, "\x33"},
	{ {1, 0x33}, REG_END}, NO_MEMORY},
/* LDRB (register) */
	{ "ldrb r7, r6, r5", INSTR_7_Rm_Rn_Rt(0x2E, 5, 6, 7),
	{ {6, 100}, {5, TESTCASE_PLUGIN_DATA_ADDRESS}, REG_END}, {1, TESTCASE_PLUGIN_DATA_ADDRESS+100, "\x33"},
	{ {7, 0x33}, REG_END}, NO_MEMORY},
/* LDRH (immediate) */
	{ "ldrh r5, r4, +#60", INSTR_5_imm5_Rn_Rt(0x11, 60/2, 4, 5),
	{ {4, TESTCASE_PLUGIN_DATA_ADDRESS}, REG_END}, {2, TESTCASE_PLUGIN_DATA_ADDRESS+60, "\x44\x33"},
	{ {5, 0x3344}, REG_END}, NO_MEMORY},
/* LDRH (register) */
	{ "ldrh r4, r3, r2", INSTR_7_Rm_Rn_Rt(0x2D, 2, 3, 4),
	{ {3, TESTCASE_PLUGIN_DATA_ADDRESS}, {2, 50}, REG_END}, {2, TESTCASE_PLUGIN_DATA_ADDRESS+50, "\x33\x44"},
	{ {4, 0x4433}, REG_END}, NO_MEMORY},
/* LDRSB (register) */
	{ "ldrsb r2, r1, r0", INSTR_7_Rm_Rn_Rt(0x2B, 0, 1, 2),
	{ {1, 200}, {0, TESTCASE_PLUGIN_DATA_ADDRESS}, REG_END}, {1, TESTCASE_PLUGIN_DATA_ADDRESS+200, "\x11"},
	{ {2, 0x11}, REG_END}, NO_MEMORY},
/* LDRSB (register) */
	{ "ldrsb r0, r7, r6", INSTR_7_Rm_Rn_Rt(0x2B, 6, 7, 0),
	{ {7, 200}, {6, TESTCASE_PLUGIN_DATA_ADDRESS}, REG_END}, {1, TESTCASE_PLUGIN_DATA_ADDRESS+200, "\x81"},
	{ {0, 0xFFFFFF81}, REG_END}, NO_MEMORY},
/* LDRSH (register) */
	{ "ldrsh r7, r6, r5", INSTR_7_Rm_Rn_Rt(0x2F, 5, 6, 7),
	{ {6, TESTCASE_PLUGIN_API_ADDRESS}, {5, 400}, REG_END}, {2, TESTCASE_PLUGIN_API_ADDRESS+400, "\x11\x22"},
	{ {7, 0x2211}, REG_END}, NO_MEMORY},
/* LDRSH (register) */
	{ "ldrsh r7, r6, r5", INSTR_7_Rm_Rn_Rt(0x2F, 5, 6, 7),
	{ {6, TESTCASE_PLUGIN_API_ADDRESS}, {5, 400}, REG_END}, {2, TESTCASE_PLUGIN_API_ADDRESS+400, "\x11\xA2"},
	{ {7, 0xFFFFA211}, REG_END}, NO_MEMORY},
/* LSL (immediate) */
	{ "lsls r6, r5, #16", INSTR_5_imm5_Rm_Rd(0x00, 16, 5, 6),
	{ {5, 0x12355678}, {INDEX_APSR, FLAG_V}, REG_END}, NO_MEMORY,
	{ {6, 0x56780000}, {INDEX_APSR, FLAG_C|FLAG_V}, REG_END}, NO_MEMORY},
/* LSL (immediate) */
	{ "lsls r6, r5, #16", INSTR_5_imm5_Rm_Rd(0x00, 16, 5, 6),
	{ {5, 0x12345678}, {INDEX_APSR, FLAG_C|FLAG_V}, REG_END}, NO_MEMORY,
	{ {6, 0x56780000}, {INDEX_APSR, FLAG_V}, REG_END}, NO_MEMORY},
/* LSL (register) */
	{ "lsls r4, r3", INSTR_10_Rm_Rdn(0x102, 3, 4),
	{ {4, 0x12345678}, {3, 16}, {INDEX_APSR, FLAG_C|FLAG_V}, REG_END}, NO_MEMORY,
	{ {4, 0x56780000}, {INDEX_APSR, FLAG_V}, REG_END}, NO_MEMORY},
/* LSL (register) */
	{ "lsls r0, r2", INSTR_10_Rm_Rdn(0x102, 2, 0),
	{ {0, 0xFEEEACF0}, {2, 16}, {INDEX_APSR, FLAG_N|FLAG_C}, REG_END}, NO_MEMORY,
	{ {0, 0xACF00000}, {INDEX_APSR, FLAG_N}, REG_END}, NO_MEMORY},
/* LSR (immediate) */
	{ "lsrs r3, r2, #8", INSTR_5_imm5_Rm_Rd(0x01, 8, 2, 3),
	{ {2, 0x12345678}, {INDEX_APSR, FLAG_C|FLAG_V}, REG_END}, NO_MEMORY,
	{ {3, 0x00123456}, {INDEX_APSR, FLAG_V}, REG_END}, NO_MEMORY},
/* LSR (register) */
	{ "lsrs r1, r2", INSTR_10_Rm_Rdn(0x103, 2, 1),
	{ {1, 0xABCDEF0F}, {2, 4}, {INDEX_APSR, 0x00}, REG_END}, NO_MEMORY,
	{ {1, 0x0ABCDEF0}, {INDEX_APSR, FLAG_C}, REG_END}, NO_MEMORY},
/* MOV (immediate) */
	{ "mov r1, #100", INSTR_5_Rd_imm8(0x04, 1, 100),
	{ {INDEX_APSR, FLAG_C|FLAG_V}, REG_END}, NO_MEMORY,
	{ {1, 100}, {INDEX_APSR, FLAG_C|FLAG_V}, REG_END}, NO_MEMORY},
/* MOV (immediate) */
	{ "mov r0, #0", INSTR_5_Rd_imm8(0x04, 0, 0),
	{ {INDEX_APSR, FLAG_C|FLAG_V}, REG_END}, NO_MEMORY,
	{ {0, 0}, {INDEX_APSR, FLAG_Z|FLAG_C|FLAG_V}, REG_END}, NO_MEMORY},
/* MOV (register): Encoding T1. */
	{ "mov r3, r11", INSTR_8_RRm_RRd(0x46, 11, 3),
	{ {11, 0x98765432}, REG_END}, NO_MEMORY,
	{ {3, 0x98765432}, REG_END}, NO_MEMORY},
/* MOV (register): Encoding T2 = LSLS r2, r3, #0 */
	{ "movs r2, r3", INSTR_10_Rm_Rd(0x000, 3, 2),
	{ {3, 0x87654321}, {INDEX_APSR, 0}, REG_END}, NO_MEMORY,
	{ {2, 0x87654321}, {INDEX_APSR, FLAG_N}, REG_END}, NO_MEMORY},
/* MOV (shifted register): Encoded as ASR, LSL, LSR and ROR. */
/* MRS: not implemented. */
/* MSR: not implemented. */
/* MUL */
	{ "mul r2, r1", INSTR_10_Rm_Rdn(0x10D, 1, 2), 
	{ {1, 10 }, {2, 100}, {INDEX_APSR, FLAG_C|FLAG_V}, REG_END}, NO_MEMORY,
	{ {2, 10*100}, {INDEX_APSR, FLAG_C|FLAG_V}, REG_END}, NO_MEMORY},
/* MVN */
	{ "mvn r0, r1", INSTR_10_Rm_Rd(0x10F, 1, 0),
	{ {1, 0x11111111}, {INDEX_APSR, FLAG_C|FLAG_V}, REG_END}, NO_MEMORY,
	{ {0, 0xEEEEEEEE}, {INDEX_APSR, FLAG_N|FLAG_V}, REG_END}, NO_MEMORY},
/* NEG: Encoded as RSB (immediate) #0. */
/* NOP: Encoded as LDM R7!,{} */
	{ "nop", 0xCF00, INSTRUCTION_16BIT,
	NO_REGISTERS, NO_MEMORY,
	NO_REGISTERS, NO_MEMORY},
/* ORR (register) */
	{ "orr r1, r7", INSTR_10_Rm_Rdn(0x10C, 7, 1),
	{ {1, 0x55555555}, {7, 0xAAAAAAAA}, {INDEX_APSR, FLAG_C|FLAG_V}, REG_END}, NO_MEMORY,
	{ {1, 0xFFFFFFFF}, {INDEX_APSR, FLAG_N|FLAG_V}, REG_END}, NO_MEMORY},
/* POP */
	{ "pop {r1, r7, pc}", 0xbd82, INSTRUCTION_16BIT,
	{ {INDEX_SP, TESTCASE_PLUGIN_STACK_ADDRESS-3*4}, REG_END},
	{ 3*4, TESTCASE_PLUGIN_STACK_ADDRESS-3*4, "\x11\x11\x11\x11\x77\x77\x77\x77\x10\x70\x00\x00"},
	{ {1, 0x11111111}, {7, 0x77777777}, {INDEX_PC, 0x7010}, {INDEX_SP, TESTCASE_PLUGIN_STACK_ADDRESS}, REG_END}, NO_MEMORY},
/* PUSH */
	{ "push {r6, r7, lr}",	0xb5c0, INSTRUCTION_16BIT,
	{ {6, 0x16}, {7, 0x17}, {INDEX_LR, 0x44332211}, {INDEX_SP, TESTCASE_DEFAULT_SP}, REG_END},
	NO_MEMORY,
	{{INDEX_SP, TESTCASE_DEFAULT_SP - 3*4}, REG_END},
	{ 3*4, TESTCASE_DEFAULT_SP-3*4, "\x16\x00\x00\x00" "\x17\x00\x00\x00"  "\x11\x22\x33\x44"} },
/* REV */
	{ "rev r0, r6", INSTR_10_Rm_Rd(0x2E8, 6, 0),
	{ {6, 0x87654321}, REG_END}, NO_MEMORY,
	{ {0, 0x21436587}, REG_END}, NO_MEMORY},
/* REV16 */
	{ "rev16 r7, r5", INSTR_10_Rm_Rd(0x2E9, 5, 7),
	{ {5, 0x87654321}, REG_END}, NO_MEMORY,
	{ {7, 0x65872143}, REG_END}, NO_MEMORY},
/* REVSH */
	{ "revsh r4, r6", INSTR_10_Rm_Rd(0x2EB, 6, 4),
	{ {6, 0x12345688}, REG_END}, NO_MEMORY,
	{ {4, 0xFFFF8856}, REG_END}, NO_MEMORY},
/* REVSH */
	{ "revsh r3, r5", INSTR_10_Rm_Rd(0x2EB, 5, 3),
	{ {5, 0x87654321}, REG_END}, NO_MEMORY,
	{ {3, 0x00002143}, REG_END}, NO_MEMORY},
/* ROR (register) */
	{ "ror r2, r4", INSTR_10_Rm_Rdn(0x107, 4, 2),
	{ {2, 0x12345678}, {4, 4}, {INDEX_APSR, FLAG_V}, REG_END}, NO_MEMORY,
	{ {2, 0x81234567}, {INDEX_APSR, FLAG_N|FLAG_C|FLAG_V}, REG_END}, NO_MEMORY},
/* RSB (immediate) */
	{ "rsbs r1,r3,#0", INSTR_10_Rn_Rd(0x109, 3, 1),
	{ {3, 0x70000000}, REG_END}, NO_MEMORY,
	{ {1, 0x90000000}, {INDEX_APSR, FLAG_N}, REG_END}, NO_MEMORY},
/* SBC (register) */
	{ "sbc r2, r0", INSTR_10_Rm_Rdn(0x106, 0, 2),
	{ {2, 0x400}, {0, 0x200}, {INDEX_APSR, FLAG_C}, REG_END}, NO_MEMORY,
	{ {2, 0x200}, {INDEX_APSR, FLAG_C}, REG_END}, NO_MEMORY},
/* SBC (register) */
	{ "sbc r2, r0", INSTR_10_Rm_Rdn(0x106, 0, 2),
	{ {2, 0x400}, {0, 0x200}, {INDEX_APSR, 0}, REG_END}, NO_MEMORY,
	{ {2, 0x1ff}, {INDEX_APSR, FLAG_C}, REG_END}, NO_MEMORY},
/* SEV: not implemented. */
/* STM */
	{ "stm r1!, {r0, r6}", INSTR_5_Rn_imm8(0x18, 1, ((1<<0)|(1<<6))),
	{ {1, TESTCASE_PLUGIN_DATA_ADDRESS+400}, {0, 0x12345678}, {6, 0x11223344}, REG_END}, NO_MEMORY,
	{ {1, TESTCASE_PLUGIN_DATA_ADDRESS+400+2*4}, REG_END}, {8, TESTCASE_PLUGIN_DATA_ADDRESS+400, "\x78\x56\x34\x12" "\x44\x33\x22\x11"} },
/* STR (immediate): Encoding T1 */
	{ "str r7, r6, +0x10", INSTR_5_imm5_Rn_Rt(0x0C, 0x10/4, 6, 7),
	{ {6, TESTCASE_PLUGIN_DATA_ADDRESS+400}, {7, 0x99887766}, REG_END}, NO_MEMORY,
	NO_REGISTERS, {4, TESTCASE_PLUGIN_DATA_ADDRESS+400+0x10, "\x66\x77\x88\x99"} },
/* STR (immediate): Encoding T2 */
	{ "str r6, sp, +0x40", INSTR_5_Rt_imm8(0x12, 6, 0x40/4),
	{ {INDEX_SP, TESTCASE_PLUGIN_STACK_ADDRESS-400}, {6, 0x55667788}, REG_END}, NO_MEMORY,
	NO_REGISTERS, {4, TESTCASE_PLUGIN_STACK_ADDRESS-400+0x40, "\x88\x77\x66\x55"} },
/* STR (register) */
	{ "str r5, r6, r7", INSTR_7_Rm_Rn_Rt(0x28, 7, 6, 5),
	{ {5, 0x11992288}, {6, TESTCASE_PLUGIN_DATA_ADDRESS}, {7, 0x200}, REG_END}, NO_MEMORY,
	NO_REGISTERS, {4, TESTCASE_PLUGIN_DATA_ADDRESS+0x200, "\x88\x22\x99\x11" } },
/* STRB (immediate) */
	{ "strb r5, r4, +#30", INSTR_5_imm5_Rn_Rt(0x0E, 30, 4, 5),
	{ {5, 0x11223344}, {4, TESTCASE_PLUGIN_DATA_ADDRESS+0x100}, REG_END}, NO_MEMORY,
	NO_REGISTERS, {1, TESTCASE_PLUGIN_DATA_ADDRESS+0x100+30, "\x44" } },
/* STRB (register) */
	{ "strb r4, r6, r7", INSTR_7_Rm_Rn_Rt(0x2A, 7, 6, 4),
	{ {4, 0x44332211}, {6, TESTCASE_PLUGIN_DATA_ADDRESS+0x30}, {7, 0x40}, REG_END}, NO_MEMORY,
	NO_REGISTERS, {1, TESTCASE_PLUGIN_DATA_ADDRESS+0x30+0x40, "\x11" } },
/* STRH (immediate) */
	{ "strh r5, r3, +#12", INSTR_5_imm5_Rn_Rt(0x10, 12/2, 3, 5),
	{ {5, 0x66778899}, {3, TESTCASE_PLUGIN_DATA_ADDRESS+0x300}, REG_END}, NO_MEMORY,
	NO_REGISTERS, {2, TESTCASE_PLUGIN_DATA_ADDRESS+0x300+12, "\x99\x88"} },
/* STRH (register) */
	{ "strh r4, r5, r6", INSTR_7_Rm_Rn_Rt(0x29, 6, 5, 4),
	{ {4, 0x88776655}, {5, TESTCASE_PLUGIN_DATA_ADDRESS+0x100}, {6, 0x100}, REG_END}, NO_MEMORY,
	NO_REGISTERS, {2, TESTCASE_PLUGIN_DATA_ADDRESS+0x100+0x100, "\x55\x66"} },
/* SUB (immediate) */
	{ "sub r3, r4, #3", INSTR_7_imm3_Rn_Rd(0x0F, 3, 4, 3),
	{ {4, 10}, {INDEX_APSR, 0}, REG_END}, NO_MEMORY,
	{ {3, 7}, {INDEX_APSR, FLAG_C}, REG_END}, NO_MEMORY},
/* SUB (register) */
	{ "sub r2, r5, r3", INSTR_7_Rm_Rn_Rd(0x0D, 3, 5, 2),
	{ {5, 0x80004444}, {3, 0x80004444}, {INDEX_APSR, 0}, REG_END}, NO_MEMORY,
	{ {2, 0}, {INDEX_APSR, FLAG_Z|FLAG_C}, REG_END}, NO_MEMORY},
/* SUB (SP minus immediate) */
	{ "sub sp, sp, #500", INSTR_9_imm7(0x161, 500/4),
	{ {INDEX_SP, TESTCASE_PLUGIN_STACK_ADDRESS}, REG_END}, NO_MEMORY,
	{ {INDEX_SP, TESTCASE_PLUGIN_STACK_ADDRESS-500}, REG_END}, NO_MEMORY},
/* SVC: not implemented. */
/* SXTB */
	{ "sxtb r1, r2", INSTR_10_Rm_Rd(0x2C9, 2, 1),
	{ {2, 0x11223344}, REG_END}, NO_MEMORY,
	{ {1, 0x44}, REG_END}, NO_MEMORY},
/* SXTB */
	{ "sxtb r2, r0", INSTR_10_Rm_Rd(0x2C9, 0, 2),
	{ {0, 0x55667788}, REG_END}, NO_MEMORY,
	{ {2, 0xFFFFFF88}, REG_END}, NO_MEMORY},
/* SXTH */
	{ "sxth r1, r0", INSTR_10_Rm_Rd(0x2C8, 0, 1),
	{ {0, 0x11223344}, REG_END}, NO_MEMORY,
	{ {1, 0x00003344}, REG_END}, NO_MEMORY},
/* SXTH */
	{ "sxth r7, r0", INSTR_10_Rm_Rd(0x2C8, 0, 7),
	{ {0, 0x55669988}, REG_END}, NO_MEMORY,
	{ {7, 0xFFFF9988}, REG_END}, NO_MEMORY},
/* TST */
	{ "tst r3, r7", INSTR_10_Rm_Rn(0x108, 7, 3),
	{ {3, 0xFFFF4444}, {7, 0xF5550000}, {INDEX_APSR, FLAG_Z|FLAG_C|FLAG_V}, REG_END}, NO_MEMORY,
	{ {INDEX_APSR,FLAG_N|FLAG_V}, REG_END}, NO_MEMORY},
/* UDF: not implemented. */
/* UXTB */
	{ "uxtb r4, r2", INSTR_10_Rm_Rd(0x2CB, 2, 4),
	{ {2, 0x11223344}, REG_END}, NO_MEMORY,
	{ {4, 0x00000044}, REG_END}, NO_MEMORY},
/* UXTH */
	{ "uxth r5, r2", INSTR_10_Rm_Rd(0x2CA, 2, 5),
	{ {2, 0x11223344}, REG_END}, NO_MEMORY,
	{ {5, 0x00003344}, REG_END}, NO_MEMORY},
/* WFE: not implemented. */
/* WFI: not implemented. */
/* YIELD: not implemented. */
	// { "yield", 0xBF10, INSTRUCTION_16BIT, NO_REGISTERS, NO_MEMORY, NO_REGISTERS, NO_MEMORY },
/* END */
	{ 0 }
};

#if defined(_MSC_VER)
#pragma pack(push, 1)
#endif
typedef struct
#if !defined(_MSC_VER)
	__attribute__((__packed__))
#endif
{
	uint8_t		VersionMajor;
	uint8_t		VersionMinor;
	uint16_t	FunctionCount;
	uint32_t	GetUptime;
	uint32_t	DebugWriteLine;
	uint32_t	DebugWriteLineHex32;
	uint32_t	WriteScreen;
	uint32_t	WriteScreenDecimal;
	uint32_t	WriteI2C;
	uint32_t	ReadI2C;
} SERVICE_API;
#if defined(_MSC_VER)
#pragma pack(pop)
#endif


/// Program memory.
uint8_t	program_memory[8 * 1024];

/// Data memory.
uint8_t	data_memory[TESTCASE_DATA_MEMORY_SIZE];

/// Service API memory.
const SERVICE_API service_api = {
		1, 1,
		5,
		0x98234983,
		0x09732309,
		0x3243481,
		0x98301021,
		0x0439537,
		0x82309209,
		0x93284831
};
/// Last function called address.
uint32_t	last_function_call = -1;

/** Static emulator state for tests. */
static struct arm_emulator_state emu;

//============================================================
int
arm_emulator_callback_read_program_memory(
	struct arm_emulator_state *state,
	uint8_t *buffer,
	uint32_t address,
	size_t count)
{
	(void)state;
	const uint32_t offset = address - TESTCASE_PLUGIN_API_ADDRESS;
	if (offset < TESTCASE_PLUGIN_API_SIZE && offset + count <= TESTCASE_PLUGIN_API_SIZE)
	{
		memcpy(buffer, program_memory + offset, count);
		return 0;
	}
	else
	{
		return -1;
	}
}

//============================================================
int
arm_emulator_callback_functioncall(
	struct arm_emulator_state *state,
	uint32_t function_address)
{
	(void)state;
	last_function_call = function_address;
	/* No action to be taken. */
	return -1;
}

//============================================================
static const char*	_rnames[16] = {
	"R0", "R1", "R2", "R3", "R4", "R5", "R6", "R7",
	"R8", "R9", "R10", "R11", "R12", "SP", "LR", "PC" };

//============================================================
int
testcase_run(const struct testcase *testcase)
{
	int return_value = 0;
	enum arm_emulator_result arm_r;
	struct arm_emulator_state state_before;
	uint8_t expected_data_memory[TESTCASE_DATA_MEMORY_SIZE];
	unsigned int prepared_registers_mask = 0;
	unsigned int checked_registers_mask = 0;
	uint32_t instruction_address = -1;
	size_t i;

	/* 1. Initialize the simulator. */
	arm_emulator_init(
		&emu,
		&program_memory[0], TESTCASE_PLUGIN_API_ADDRESS, sizeof(program_memory),
		data_memory, TESTCASE_PLUGIN_DATA_ADDRESS, sizeof(data_memory),
		(uint8_t *)&service_api, TESTCASE_SERVICE_API_ADDRESS, sizeof(service_api));

	// 2. Fill memory and registers with random stuff.
	for (i=0; i<sizeof(program_memory); ++i)
	{
		program_memory[i] = rand();
	}
	for (i=0; i<sizeof(data_memory); ++i)
	{
		data_memory[i] = rand();
	}
	for (i = 0; i < ARM_NREGISTERS; ++i)
	{
		emu.R[i] = (rand() << 16) | rand();
	}
	emu.APSR = (rand() & 0x0F) << 28;

	/* 4. Insert bytes expected to be read. */
	if (testcase->expected_reads.count > 0)
	{
		const struct testcase_memory_access *er = &testcase->expected_reads;
		{
			const uint32_t program_offset = er->address - TESTCASE_PLUGIN_API_ADDRESS;
			const uint32_t data_offset = er->address - TESTCASE_PLUGIN_DATA_ADDRESS;
			if (program_offset<sizeof(program_memory) && program_offset+er->count<=sizeof(program_memory))
			{
				memcpy(program_memory+program_offset, er->data, er->count);
			}
			else if (data_offset<sizeof(data_memory) && data_offset+er->count<=sizeof(data_memory))
			{
				memcpy(data_memory+data_offset, er->data, er->count);
			}
			else
			{
				printf("%s: Invalid expected memory address: 0x%04X\n",
					testcase->name, er->address);
				return -1;
			}
		}
	}

	/* 5. Update registers. */
	for (i = 0; i < MAX_REGISTERS && testcase->registers_before[i].index >= 0; ++i)
	{
		const struct testcase_register *rb = testcase->registers_before + i;
		if (rb->index == INDEX_APSR)
		{
			emu.APSR = rb->value;
		}
		else
		{
			emu.R[rb->index] = rb->value;
		}
		prepared_registers_mask |= 1 << rb->index;
	}

	/* 3. Inject instruction */
	if (!(prepared_registers_mask & (1 << INDEX_PC)))
	{
		emu.R[INDEX_PC] = TESTCASE_DEFAULT_PC;
	}
	instruction_address = emu.R[INDEX_PC];
	{
		const uint32_t instruction_offset = instruction_address - TESTCASE_PLUGIN_API_ADDRESS;
		if (instruction_offset>=sizeof(program_memory) || instruction_offset+3>=sizeof(program_memory))
		{
			printf("%s: invalid instruction address 0x%04X\n",
				testcase->name, instruction_address);
			return -1;
		}
		program_memory[instruction_offset+0] = testcase->instruction & 0xFF;
		program_memory[instruction_offset+1] = testcase->instruction >> 8;
		if (testcase->instruction2>=0)
		{
			program_memory[instruction_offset+2] = testcase->instruction2 & 0xFF;
			program_memory[instruction_offset+3] = testcase->instruction2 >> 8;
		}
	}

	/* 6. Run the instruction. */
	state_before = emu;
	memcpy(expected_data_memory, data_memory, sizeof(data_memory));
	arm_r = arm_emulator_execute(&emu, 1);
	if (arm_r != ARM_EMULATOR_OK)
	{
		printf("%s: Emulator error %d.\n",
			testcase->name, arm_r);
		return -1;
	}

	/* 7. Check registers. */
	checked_registers_mask = 0;
	for (i = 0; i < MAX_REGISTERS && testcase->registers_after[i].index >= 0; ++i)
	{
		const struct testcase_register *rb = testcase->registers_after + i;
		checked_registers_mask |= 1 << rb->index;
		if (rb->index == INDEX_APSR)
		{
			if (emu.APSR != rb->value)
			{
				printf("%s: APSR mismatch, expected 0x%04X, got 0x%04X\n",
					testcase->name, rb->value, emu.APSR);
				return_value = -1;
			}
		}
		else
		{
			if (emu.R[rb->index] != rb->value)
			{
				printf("%s: Register %s mismatch, expected 0x%04X, got 0x%04X\n",
					testcase->name, _rnames[rb->index], rb->value, emu.R[rb->index]);
				return_value = -1;
			}
		}
	}
	/* 8. Check remaining registers. PC needs special handling. */
	for (i = 0; i < INDEX_PC; ++i)
	{
		if (!(checked_registers_mask & (1 << i)))
		{
			checked_registers_mask |= 1 << i;
			if (emu.R[i] != state_before.R[i])
			{
				printf("%s: Register %s mismatch, expected 0x%04X, got 0x%04X\n",
					testcase->name, _rnames[i], state_before.R[i], emu.R[i]);
				return_value = -1;
			}
		}
	}
	if (!(checked_registers_mask & (1 << INDEX_PC)))
	{
		const uint32_t expected_pc = testcase->instruction2 >= 0
			? instruction_address + 4
			: instruction_address + 2;
		checked_registers_mask |= 1 << INDEX_PC;
		if (expected_pc != emu.R[INDEX_PC])
		{
			printf("%s: PC mismatch, expected 0x%04X, got 0x%04X\n",
				testcase->name, expected_pc, emu.R[INDEX_PC]);
			return_value = -1;
		}
	}
	if (!(checked_registers_mask & (1 << INDEX_APSR)))
	{
		checked_registers_mask |= 1 << INDEX_APSR;
		if (emu.APSR != state_before.APSR)
		{
			printf("%s: APSR mismatch, expected 0x%04X, got 0x%04X\n",
				testcase->name, state_before.APSR, emu.APSR);
			return_value = -1;
		}
	}

	/* 9. Check the memory... */
	if (testcase->expected_writes.count > 0)
	{
		const struct testcase_memory_access *ew = &testcase->expected_writes;
		{
			const uint32_t data_offset = ew->address - TESTCASE_PLUGIN_DATA_ADDRESS;
			if (data_offset<sizeof(data_memory) && data_offset+ew->count<=sizeof(data_memory))
			{
				memcpy(expected_data_memory+data_offset, ew->data, ew->count);
			}
			else
			{
				printf("%s: Invalid expected write address: 0x%04X\n",
					testcase->name, ew->address);
				return -1;
			}
		}
	}
	for (i = 0; i < TESTCASE_DATA_MEMORY_SIZE; ++i)
	{
		if (expected_data_memory[i] != data_memory[i])
		{
			printf("%s: Data address 0x%04X: expected 0x%02X, got 0x%02X\n",
				testcase->name, (unsigned int)(i + TESTCASE_PLUGIN_DATA_ADDRESS),
				expected_data_memory[i], data_memory[i]);
			return_value = -1;
		}
	}
	/* 10. Sure we are OK? */
	return return_value;
}
