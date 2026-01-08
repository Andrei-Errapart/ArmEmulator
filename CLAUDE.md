# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ARM Cortex-M0 emulator implementing the Thumb-2 instruction set, specifically designed for the NXP LPC1114 microcontroller. Includes a comprehensive test suite validating all supported instructions.

## Build Commands

**Unix/Linux/macOS:**
```bash
make          # Build test executable
make test     # Build and run tests
make lib      # Build libarm_emulator.a static library
make clean    # Remove build artifacts
```

**Windows (Visual Studio):**
- Open `vs/TestEmulator.sln` in Visual Studio 2013+
- Build configurations: Debug/Release (Win32)

## Architecture

### Directory Structure

```
src/           Core emulator library
tests/         Test harness
vs/            Visual Studio project files
```

**Build artifacts (gitignored):** `*.o`, `*.a`, `test_emulator`

### Core Components

| File | Purpose |
|------|---------|
| `src/arm_emulator.c/h` | Instruction decoder/executor (~1,700 lines) |
| `src/api.h` | Plugin and service API definitions |
| `src/comm.c/h` | UART/I2C communication utilities |
| `tests/testcase.c/h` | Comprehensive instruction test suite |
| `tests/main.c` | Test harness entry point |

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
- Conditional compilation via `_MSC_VER` or `DESKTOP_BUILD` for portable C vs embedded ARM assembly
- Test cases encode expected register states, memory reads/writes for validation
