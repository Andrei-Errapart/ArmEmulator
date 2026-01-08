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

/* Memory regions */
static uint8_t program_memory[1024];
static uint8_t data_memory[256];

/* Required callback: handle external function calls */
int arm_emulator_callback_functioncall(
    const uint32_t FunctionAddress,
    ARM_EMULATOR_STATE* State)
{
    (void)State;
    printf("Function call to 0x%08X\n", FunctionAddress);
    return -1;  /* Not handled */
}

/* Callback for reads outside defined memory regions (not needed for basic use) */
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

int main(void)
{
    ARM_EMULATOR_RETURN_VALUE result;

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
        program_memory, 0x6000, sizeof(program_memory),
        data_memory, 0x10000000, sizeof(data_memory),
        NULL, 0, 0  /* No service memory */
    );

    /* Start execution at address 0x6000 (with Thumb bit set = 0x6001) */
    arm_emulator_start_function_call((void*)0x6001, NULL, 0);

    /* Execute up to 100 instructions */
    result = arm_emulator_execute(100);

    if (result == ARM_EMULATOR_FUNCTION_RETURNED) {
        uint32_t retval = arm_emulator_get_function_return_value();
        printf("Function returned: %u (expected: 8)\n", retval);
    } else {
        printf("Execution error: %d\n", result);
    }

    return 0;
}
