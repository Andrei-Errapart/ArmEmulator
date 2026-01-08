# ArmEmulator

A Cortex-M0 emulator implementing the Thumb-2 instruction set, intended for use with the NXP LPC1114 microcontroller.

## Quickstart

```bash
make          # Build test executable
make test     # Build and run tests
make lib      # Build static library
make examples # Build example programs
make clean    # Remove build artifacts
```

## API Overview

### Initialization

```c
#include "arm_emulator.h"

struct arm_emulator_state emu;
uint8_t program_memory[4096];
uint8_t data_memory[1024];

/* Initialize with memory regions */
arm_emulator_init(
    &emu,
    program_memory, 0x6000, sizeof(program_memory),  /* Program */
    data_memory, 0x10000000, sizeof(data_memory),    /* Data */
    NULL, 0, 0                                        /* Service (optional) */
);
```

### Execution

```c
/* Start execution at address (with Thumb bit set) */
arm_emulator_start_function_call(&emu, (void *)0x6001, NULL, 0);

/* Execute up to N instructions */
enum arm_emulator_result result = arm_emulator_execute(&emu, 100);

if (result == ARM_EMULATOR_FUNCTION_RETURNED) {
    uint32_t retval = arm_emulator_get_function_return_value(&emu);
}
```

### Required Callbacks

You must implement these callbacks:

```c
/* Called when code calls an external function */
int arm_emulator_callback_functioncall(
    struct arm_emulator_state *state,
    uint32_t function_address)
{
    /* Return 0 if handled, -1 otherwise */
    return -1;
}

/* Called to read from addresses outside defined memory regions */
int arm_emulator_callback_read_program_memory(
    struct arm_emulator_state *state,
    uint8_t *buffer,
    uint32_t address,
    size_t count)
{
    /* Not needed for basic use - emulator reads from internal buffer */
    return -1;
}
```

For advanced use cases (external ROM, memory-mapped I/O), implement this callback to handle reads from addresses outside the regions passed to `arm_emulator_reset()`. See `examples/callbacks.c`.

## Examples

See `examples/` directory:

- `basic.c` - Initialize, execute, get results
- `callbacks.c` - Handle function calls and memory reads
- `debug.c` - Register inspection and single-stepping
- `lpc1114.c` - LPC1114 plugin emulation with service API

## License

MIT
