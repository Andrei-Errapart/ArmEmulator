// SPDX-License-Identifier: MIT
/**
 * LPC1114 example: Real-world plugin emulation.
 *
 * This example demonstrates:
 * - LPC1114 memory map (plugin at 0x6000, data at 0x10000200, service at 0x300)
 * - Service API structure and function interception
 * - Plugin API header format
 * - Emulating a plugin that calls service functions
 */
#include <stdio.h>
#include <string.h>
#include "arm_emulator.h"
#include "plugin_api.h"

/*
 * LPC1114 Memory Map:
 * 0x00000300 - Service API (provided by host)
 * 0x00006000 - Plugin code (loaded plugin)
 * 0x00007000 - Plugin API header
 * 0x10000200 - Plugin data memory
 */
#define PROGRAM_BASE    0x6000
#define PROGRAM_SIZE    4096
#define DATA_BASE       0x10000200
#define DATA_SIZE       1024

/* Emulator state */
static struct arm_emulator_state emu;

static uint8_t program_memory[PROGRAM_SIZE];
static uint8_t data_memory[DATA_SIZE];

/* Simulated service API at address 0x300 */
static struct service_api sim_service_api;

/* Simulated uptime counter */
static uint32_t uptime_ms = 1000;

/*
 * Service function implementations (called via callback)
 */
static uint32_t sim_get_uptime(void)
{
    uptime_ms += 16;  /* Simulate ~16ms resolution */
    return uptime_ms;
}

static void sim_debug_writeln(const char *s, uint16_t length)
{
    printf("[PLUGIN DEBUG] %.*s\n", length, s);
}

static void sim_debug_writeln_hex32(const char *s, uint16_t length, uint32_t x)
{
    printf("[PLUGIN DEBUG] %.*s 0x%X\n", length, s, x);
}

/*
 * Callback: Handle function calls to service API
 */
int arm_emulator_callback_functioncall(
    struct arm_emulator_state *state,
    uint32_t function_address)
{
    /* Check if calling GetUptime (offset 4 in service_api after header) */
    if (function_address == (uint32_t)(uintptr_t)sim_service_api.GetUptime) {
        state->R[0] = sim_get_uptime();
        printf("  Service: GetUptime() -> %u\n", state->R[0]);
        return 0;
    }

    /* Check if calling DebugWriteLine */
    if (function_address == (uint32_t)(uintptr_t)sim_service_api.DebugWriteLine) {
        /* R0 = string pointer, R1 = length */
        uint32_t str_addr = state->R[0];
        uint16_t length = (uint16_t)state->R[1];
        char buffer[256];

        if (length > sizeof(buffer) - 1) length = sizeof(buffer) - 1;

        /* Read string from emulator memory */
        if (str_addr >= DATA_BASE && str_addr < DATA_BASE + DATA_SIZE) {
            memcpy(buffer, &data_memory[str_addr - DATA_BASE], length);
            buffer[length] = '\0';
            sim_debug_writeln(buffer, length);
        }
        return 0;
    }

    /* Check if calling DebugWriteLineHex32 */
    if (function_address == (uint32_t)(uintptr_t)sim_service_api.DebugWriteLineHex32) {
        /* R0 = string pointer, R1 = length, R2 = hex value */
        uint32_t str_addr = state->R[0];
        uint16_t length = (uint16_t)state->R[1];
        uint32_t value = state->R[2];
        char buffer[256];

        if (length > sizeof(buffer) - 1) length = sizeof(buffer) - 1;

        if (str_addr >= DATA_BASE && str_addr < DATA_BASE + DATA_SIZE) {
            memcpy(buffer, &data_memory[str_addr - DATA_BASE], length);
            buffer[length] = '\0';
            sim_debug_writeln_hex32(buffer, length, value);
        }
        return 0;
    }

    printf("  Unknown function call: 0x%08X\n", function_address);
    return -1;
}

/*
 * Callback: Read program memory
 */
int arm_emulator_callback_read_program_memory(
    struct arm_emulator_state *state,
    uint8_t *buffer,
    uint32_t address,
    size_t count)
{
    (void)state;

    /* Serve reads from program memory at PROGRAM_BASE */
    if (address >= PROGRAM_BASE && address + count <= PROGRAM_BASE + sizeof(program_memory)) {
        memcpy(buffer, &program_memory[address - PROGRAM_BASE], count);
        return 0;
    }

    /* Serve reads from service API at 0x300 */
    if (address >= SERVICE_API_ADDRESS &&
        address + count <= SERVICE_API_ADDRESS + sizeof(struct service_api)) {
        memcpy(buffer, ((uint8_t *)&sim_service_api) + (address - SERVICE_API_ADDRESS), count);
        return 0;
    }

    memset(buffer, 0, count);
    return -1;
}

/*
 * Initialize simulated service API with fake function pointers.
 * These addresses will be intercepted in the callback.
 */
static void init_service_api(void)
{
    sim_service_api.version_major = 1;
    sim_service_api.version_minor = 0;
    sim_service_api.function_count = 7;

    /* Use distinctive addresses that we'll intercept */
    sim_service_api.GetUptime = (service_get_uptime_t)0x1001;
    sim_service_api.DebugWriteLine = (service_debug_writeln_t)0x1002;
    sim_service_api.DebugWriteLineHex32 = (service_debug_writeln_hex32_t)0x1003;
    sim_service_api.WriteScreen = (service_write_screen_t)0x1004;
    sim_service_api.WriteScreenDecimal = (service_write_screen_decimal_t)0x1005;
    sim_service_api.WriteI2C = (service_write_i2c_t)0x1006;
    sim_service_api.ReadI2C = (service_read_i2c_t)0x1007;
}

int main(void)
{
    enum arm_emulator_result result;

    /*
     * Simulated plugin code that:
     * 1. Saves LR (BLX will overwrite it)
     * 2. Loads service API address
     * 3. Loads GetUptime function pointer
     * 4. Calls GetUptime()
     * 5. Restores LR and returns
     *
     * Service API structure starts at 0x300:
     *   +0: version_major (1 byte)
     *   +1: version_minor (1 byte)
     *   +2: function_count (2 bytes)
     *   +4: GetUptime pointer (4 bytes)
     */
    const uint16_t code[] = {
        0xb500,  /* PUSH {LR} - save return address */
        0x4802,  /* LDR R0, [PC, #8] - load service API address */
        0x6841,  /* LDR R1, [R0, #4] - load GetUptime pointer */
        0x4788,  /* BLX R1 - call GetUptime */
        0xbd00,  /* POP {PC} - return (pops saved LR into PC) */
        0x0000,  /* padding for alignment */
        /* Literal pool (word-aligned) */
        0x0300, 0x0000,  /* SERVICE_API_ADDRESS = 0x00000300 */
    };

    /* Initialize */
    init_service_api();
    memset(program_memory, 0, sizeof(program_memory));
    memset(data_memory, 0, sizeof(data_memory));
    memcpy(program_memory, code, sizeof(code));

    /* Put a test string in data memory */
    const char *test_msg = "Hello from plugin!";
    strcpy((char *)data_memory, test_msg);

    printf("=== LPC1114 Plugin Emulation ===\n\n");
    printf("Memory map:\n");
    printf("  Service API: 0x%08X\n", SERVICE_API_ADDRESS);
    printf("  Plugin code: 0x%08X\n", PROGRAM_BASE);
    printf("  Plugin data: 0x%08X\n", DATA_BASE);
    printf("\n");

    arm_emulator_init(
        &emu,
        program_memory, PROGRAM_BASE, sizeof(program_memory),
        data_memory, DATA_BASE, sizeof(data_memory),
        (uint8_t *)&sim_service_api, SERVICE_API_ADDRESS, sizeof(struct service_api)
    );

    printf("Calling plugin entry point...\n\n");
    arm_emulator_start_function_call(&emu, (void *)(PROGRAM_BASE + 1), NULL, 0);

    result = arm_emulator_execute(&emu, 100);

    if (result == ARM_EMULATOR_FUNCTION_RETURNED) {
        printf("\nPlugin returned: %u (uptime in ms)\n",
            arm_emulator_get_function_return_value(&emu));
    } else {
        printf("\nExecution error: %d\n", result);
    }

    return 0;
}
