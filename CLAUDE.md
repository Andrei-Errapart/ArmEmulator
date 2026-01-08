# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ARM Cortex-M0 emulator implementing the Thumb-2 instruction set, specifically designed for the NXP LPC1114 microcontroller. Includes a comprehensive test suite validating all supported instructions.

## Build Commands

**Windows (Visual Studio):**
- Open `TestEmulator.sln` in Visual Studio 2013+
- Build configurations: Debug/Release (Win32)

**Unix/Linux/macOS (manual):**
```bash
gcc -o arm_emulator arm_emulator.c comm.c testcase.c main.c
```

**Running Tests:**
Execute the compiled binary. The test harness in `main.c` iterates through `testcases[]` array and validates each instruction.

## Architecture

### Core Components

| File | Purpose |
|------|---------|
| `arm_emulator.c/h` | Instruction decoder/executor (~1,700 lines) |
| `api.h` | Plugin and service API definitions |
| `comm.c/h` | UART/I2C communication utilities |
| `testcase.c/h` | Comprehensive instruction test suite |
| `main.c` | Test harness entry point |

### Memory Layout

- **Plugin API:** 0x6000-0x7FFF (8KB)
- **Data memory:** 0x10000200+ (1KB default)
- **Service API:** 0x300
- **Default PC:** 0x601C

### Register Model

- R0-R12: General purpose
- R13 (SP): Stack pointer
- R14 (LR): Link register
- R15 (PC): Program counter
- APSR: Condition flags (N, Z, C, V at bits 31-28)

### Supported Instructions

Arithmetic (ADC, ADD, CMN, CMP, RSB, SBC), Logic (AND, BIC, EOR, ORR, TST, MVN), Shifts (ASR, LSL, LSR, ROR), Branches (B, BL, BX, BLX with conditionals), Data movement (MOV, MOVW, MOVT, LDR variants), Stack (PUSH, POP), Memory (LDM, STM, STRB, STRH, STR), Special (ADR, SVC).

### Condition Codes

Constants defined in `arm_emulator.h`: `COND_EQ` (0x0) through `COND_TRUE2` (0xF). Flags: `FLAG_N`, `FLAG_Z`, `FLAG_C`, `FLAG_V`.

## Key Implementation Details

- Uses bit-masking macros for instruction field extraction
- Global `ARM_EMULATOR_STATE global_emulator` provides test access
- Callbacks handle external function calls and program memory reads
- Conditional compilation via `_MSC_VER` for printf vs embedded UART output
- Test cases encode expected register states, memory reads/writes for validation
