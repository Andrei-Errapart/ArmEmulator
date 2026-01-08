// SPDX-License-Identifier: MIT
/**
 * Callbacks example: Implement custom function call and memory handlers.
 *
 * This example demonstrates:
 * - Handling function calls via arm_emulator_callback_functioncall()
 * - Custom program memory reads via arm_emulator_callback_read_program_memory()
 * - Intercepting calls to external APIs
 */
#include <stdio.h>
#include <string.h>
#include "arm_emulator.h"

/* Emulator state */
static struct arm_emulator_state emu;

/* Memory regions */
static uint8_t program_memory[1024];
static uint8_t data_memory[256];

/* Simulated external ROM at 0x8000 */
static const uint8_t external_rom[] = {
    0x70, 0x47,  /* BX LR - simple return */
};

/* Track calls for demonstration */
static int call_count = 0;

/*
 * Handle external function calls.
 * When emulated code calls a function outside the program memory,
 * this callback is invoked.
 */
int arm_emulator_callback_functioncall(
    struct arm_emulator_state *state,
    uint32_t function_address)
{
    /* Mask off Thumb bit for address comparison */
    uint32_t addr = function_address & ~1;

    call_count++;
    printf("Callback #%d: Function call to 0x%08X\n", call_count, function_address);

    /* Example: Intercept call to address 0x100 (simulated "print" function) */
    if (addr == 0x100) {
        /* R0 contains the value to "print" */
        printf("  -> Simulated print: value = %u\n", state->R[0]);
        /* Set return value in R0 */
        state->R[0] = 0;  /* Success */
        return 0;  /* Function handled */
    }

    /* Example: Intercept call to address 0x104 (simulated "add" function) */
    if (addr == 0x104) {
        /* R0 and R1 contain operands */
        uint32_t a = state->R[0];
        uint32_t b = state->R[1];
        printf("  -> Simulated add: %u + %u = %u\n", a, b, a + b);
        state->R[0] = a + b;
        return 0;  /* Function handled */
    }

    return -1;  /* Not handled - emulator will try to execute at address */
}

/*
 * Handle reads from program memory.
 * Also demonstrates serving external ROM at a different address.
 */
int arm_emulator_callback_read_program_memory(
    struct arm_emulator_state *state,
    uint8_t *buffer,
    uint32_t address,
    size_t count)
{
    (void)state;

    /* Read from main program memory at 0x6000 */
    if (address >= 0x6000 && address + count <= 0x6000 + sizeof(program_memory)) {
        memcpy(buffer, &program_memory[address - 0x6000], count);
        return 0;
    }

    /* Serve reads from external ROM at 0x8000 */
    if (address >= 0x8000 && address + count <= 0x8000 + sizeof(external_rom)) {
        memcpy(buffer, &external_rom[address - 0x8000], count);
        return 0;
    }

    memset(buffer, 0, count);
    return -1;  /* Address not mapped */
}

int main(void)
{
    enum arm_emulator_result result;

    /*
     * Program that calls external functions.
     * Uses PUSH/POP to preserve LR across function calls.
     *
     * The callback intercepts calls to addresses 0x100 (print) and 0x104 (add).
     */
    const uint16_t code[] = {
        /* 0x6000 */ 0xb500,  /* PUSH {LR} */
        /* 0x6002 */ 0x202A,  /* MOV R0, #42 */
        /* 0x6004 */ 0x4903,  /* LDR R1, [PC, #12] -> 0x6014 */
        /* 0x6006 */ 0x4788,  /* BLX R1 - call print */
        /* 0x6008 */ 0x200A,  /* MOV R0, #10 */
        /* 0x600A */ 0x2114,  /* MOV R1, #20 */
        /* 0x600C */ 0x4A02,  /* LDR R2, [PC, #8] -> 0x6018 */
        /* 0x600E */ 0x4790,  /* BLX R2 - call add */
        /* 0x6010 */ 0xbd00,  /* POP {PC} - return */
        /* 0x6012 */ 0x0000,  /* padding for alignment */
        /* Literal pool (word-aligned at 0x6014) */
        /* 0x6014 */ 0x0101, 0x0000,  /* 0x00000101 (print @ 0x100) */
        /* 0x6018 */ 0x0105, 0x0000,  /* 0x00000105 (add @ 0x104) */
    };

    memcpy(program_memory, code, sizeof(code));

    arm_emulator_reset(
        &emu,
        program_memory, 0x6000, sizeof(program_memory),
        data_memory, 0x10000000, sizeof(data_memory),
        NULL, 0, 0
    );

    arm_emulator_start_function_call(&emu, (void *)0x6001, NULL, 0);

    printf("Executing with callbacks...\n\n");
    result = arm_emulator_execute(&emu, 100);

    if (result == ARM_EMULATOR_FUNCTION_RETURNED) {
        printf("\nFunction returned: %u\n", arm_emulator_get_function_return_value(&emu));
        printf("Total callbacks: %d\n", call_count);
    } else {
        printf("Execution error: %d\n", result);
    }

    return 0;
}
