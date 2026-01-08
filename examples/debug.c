// SPDX-License-Identifier: MIT
/**
 * Debug example: Register inspection and single-stepping.
 *
 * This example demonstrates:
 * - Using arm_emulator_dump() for register inspection
 * - Single-stepping through instructions
 * - Accessing emulator state directly via global_emulator
 */
#include <stdio.h>
#include <string.h>
#include "arm_emulator.h"

/* Memory regions */
static uint8_t program_memory[1024];
static uint8_t data_memory[256];

/* Required callbacks */
int arm_emulator_callback_functioncall(
    const uint32_t FunctionAddress,
    ARM_EMULATOR_STATE* State)
{
    (void)FunctionAddress;
    (void)State;
    return -1;
}

int arm_emulator_callback_read_program_memory(
    uint8_t* Buffer,
    const uint32_t Address,
    const uint8_t Count)
{
    (void)Buffer;
    (void)Address;
    (void)Count;
    return -1;  /* No extended memory */
}

/* Helper to print register state */
static void print_registers(const char* label)
{
    printf("\n=== %s ===\n", label);
    printf("R0=%08X  R1=%08X  R2=%08X  R3=%08X\n",
        global_emulator.R[0], global_emulator.R[1],
        global_emulator.R[2], global_emulator.R[3]);
    printf("R4=%08X  R5=%08X  R6=%08X  R7=%08X\n",
        global_emulator.R[4], global_emulator.R[5],
        global_emulator.R[6], global_emulator.R[7]);
    printf("SP=%08X  LR=%08X  PC=%08X  APSR=%08X\n",
        global_emulator.R[13], global_emulator.R[14],
        global_emulator.R[15], global_emulator.APSR);

    /* Decode APSR flags */
    printf("Flags: %c%c%c%c\n",
        (global_emulator.APSR & (1 << 31)) ? 'N' : '-',
        (global_emulator.APSR & (1 << 30)) ? 'Z' : '-',
        (global_emulator.APSR & (1 << 29)) ? 'C' : '-',
        (global_emulator.APSR & (1 << 28)) ? 'V' : '-');
}

int main(void)
{
    ARM_EMULATOR_RETURN_VALUE result;
    int step = 0;

    /*
     * Program that manipulates several registers:
     *   MOV R0, #100    ; 0x2064
     *   MOV R1, #50     ; 0x2132
     *   ADD R2, R0, R1  ; 0x1882
     *   SUB R3, R0, R1  ; 0x1A43
     *   CMP R2, R3      ; 0x429A (sets flags)
     *   BX LR           ; 0x4770
     */
    const uint16_t code[] = {
        0x2064,  /* MOV R0, #100 */
        0x2132,  /* MOV R1, #50 */
        0x1882,  /* ADD R2, R0, R1 */
        0x1A43,  /* SUB R3, R0, R1 */
        0x429A,  /* CMP R2, R3 */
        0x4770,  /* BX LR */
    };

    memcpy(program_memory, code, sizeof(code));

    arm_emulator_reset(
        program_memory, 0x6000, sizeof(program_memory),
        data_memory, 0x10000000, sizeof(data_memory),
        NULL, 0, 0
    );

    arm_emulator_start_function_call((void*)0x6001, NULL, 0);

    printf("Single-stepping through program...\n");
    print_registers("Initial state");

    /* Single-step through each instruction */
    while (1) {
        step++;
        result = arm_emulator_execute(1);  /* Execute exactly 1 instruction */

        printf("\n--- After step %d ---", step);
        print_registers("Registers");

        /* Also use the built-in dump function */
        printf("\nBuilt-in dump:\n");
        arm_emulator_dump();

        if (result == ARM_EMULATOR_FUNCTION_RETURNED) {
            printf("\n=== Function returned at step %d ===\n", step);
            printf("Return value (R0): %u\n", arm_emulator_get_function_return_value());
            break;
        }

        if (result == ARM_EMULATOR_ERROR) {
            printf("\n=== Error at step %d ===\n", step);
            break;
        }

        if (step >= 10) {
            printf("\n=== Max steps reached ===\n");
            break;
        }
    }

    return 0;
}
