// SPDX-License-Identifier: MIT
/**
 * Basic example: Initialize emulator, execute simple code, get result.
 *
 * This example demonstrates:
 * - Setting up memory regions
 * - Loading Thumb instructions
 * - Executing code
 * - Reading the return value
 */
#include <stdio.h>
#include <string.h>
#include "arm_emulator.h"

/* Emulator state */
static struct arm_emulator_state emu;

/* Memory regions */
static uint8_t program_memory[1024];
static uint8_t data_memory[256];

/* Required callback: handle external function calls */
int arm_emulator_callback_functioncall(
    struct arm_emulator_state *state,
    uint32_t function_address)
{
    (void)state;
    printf("Function call to 0x%08X\n", function_address);
    return -1;  /* Not handled */
}

/* Callback for reads outside defined memory regions (not needed for basic use) */
int arm_emulator_callback_read_program_memory(
    struct arm_emulator_state *state,
    uint8_t *buffer,
    uint32_t address,
    size_t count)
{
    (void)state;
    (void)buffer;
    (void)address;
    (void)count;
    return -1;  /* No extended memory */
}

int main(void)
{
    enum arm_emulator_result result;

    /*
     * Simple program that computes 5 + 3:
     *   MOV R0, #5      ; 0x2005
     *   MOV R1, #3      ; 0x2103
     *   ADD R0, R0, R1  ; 0x1840
     *   BX LR           ; 0x4770 (return)
     */
    const uint16_t code[] = {
        0x2005,  /* MOV R0, #5 */
        0x2103,  /* MOV R1, #3 */
        0x1840,  /* ADD R0, R0, R1 */
        0x4770,  /* BX LR */
    };

    /* Load code into program memory */
    memcpy(program_memory, code, sizeof(code));

    /* Initialize emulator */
    arm_emulator_reset(
        &emu,
        program_memory, 0x6000, sizeof(program_memory),
        data_memory, 0x10000000, sizeof(data_memory),
        NULL, 0, 0  /* No service memory */
    );

    /* Start execution at address 0x6000 (with Thumb bit set = 0x6001) */
    arm_emulator_start_function_call(&emu, (void *)0x6001, NULL, 0);

    /* Execute up to 100 instructions */
    result = arm_emulator_execute(&emu, 100);

    if (result == ARM_EMULATOR_FUNCTION_RETURNED) {
        uint32_t retval = arm_emulator_get_function_return_value(&emu);
        printf("Function returned: %u (expected: 8)\n", retval);
    } else {
        printf("Execution error: %d\n", result);
    }

    return 0;
}
