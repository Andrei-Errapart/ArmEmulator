// SPDX-License-Identifier: MIT
/**
 * @file plugin_api.h
 * @brief API for plugins and the service interface.
 *
 * Defines the service API (provided by the host) and plugin API
 * (provided by plugins) structures for LPC1114 plugin system.
 */
#ifndef PLUGIN_API_H
#define PLUGIN_API_H

#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(_MSC_VER)
#pragma pack(push, 1)
#define PACKED_ATTR
#elif defined(__GNUC__) || defined(__clang__)
#define PACKED_ATTR __attribute__((__packed__))
#else
#define PACKED_ATTR
#endif

/*
 * Service API function pointer types.
 * These functions are provided by the host and callable by plugins.
 */

/**
 * Get time in 1ms units since system start. Effective resolution is ~16ms.
 */
typedef uint32_t (*service_get_uptime_t)(void);

/**
 * Implementation of service_get_uptime_t.
 * Do not call directly from plugins - use the function pointer in service_api.
 */
extern uint32_t service_get_uptime(void);

/**
 * Write line of text to the serial port.
 * The text is preceded with "PLUGIN:".
 *
 * @param s Text to write.
 * @param length Length of text in bytes.
 */
typedef void (*service_debug_writeln_t)(
	const char *s,
	uint16_t length);

/**
 * Write line of text and hexadecimal number to the serial port.
 * The text is preceded with "PLUGIN:".
 * Upper halfword is omitted if zero.
 *
 * @param s Text to write.
 * @param length Length of text in bytes.
 * @param x Number to write in hexadecimal.
 */
typedef void (*service_debug_writeln_hex32_t)(
	const char *s,
	uint16_t length,
	uint32_t x);

/**
 * Write to the screen, if permitted.
 *
 * @param yx Position: upper halfword=row, lower halfword=column.
 * @param s Text to write.
 * @param length Length of text in bytes.
 */
typedef void (*service_write_screen_t)(
	uint32_t yx,
	const char *s,
	uint16_t length);

/**
 * Write decimal number to the screen, if permitted.
 *
 * @param yx Position: upper halfword=row, lower halfword=column.
 * @param number Number to write.
 * @param nspaces 0 for no padding, >1 for left padding if necessary.
 */
typedef void (*service_write_screen_decimal_t)(
	uint32_t yx,
	int32_t number,
	uint16_t nspaces);

/**
 * Write bytes to an I2C device.
 *
 * @param address_page_register Combined address, use MAKE_ADDRESS_PAGE_REGISTER.
 *        MSB: Zero, Byte 2: Device address, Byte 1: Page (TDA9984), LSB: Register.
 * @param data Data to write.
 * @param nbytes Number of bytes to write.
 * @return 0 on success, negative on error.
 */
typedef int (*service_write_i2c_t)(
	uint32_t address_page_register,
	const uint8_t *data,
	uint32_t nbytes);

/**
 * Compose an I2C address for service_write_i2c and service_read_i2c.
 */
#define MAKE_ADDRESS_PAGE_REGISTER(device_address, page, register_number) \
	((((device_address) & 0xFF) << 16) | \
	 (((page) & 0xFF) << 8) | \
	 ((register_number) & 0xFF))

/**
 * Read bytes from an I2C device.
 *
 * @param buffer Buffer to read into.
 * @param address_page_register Combined address, use MAKE_ADDRESS_PAGE_REGISTER.
 * @param count Number of bytes to read.
 * @return 0 on success, negative on error.
 */
typedef int (*service_read_i2c_t)(
	uint8_t *buffer,
	uint32_t address_page_register,
	uint32_t count);

/**
 * Service API structure.
 * Located at SERVICE_API_ADDRESS (0x300) in memory.
 * Provides host services to plugins.
 */
struct service_api {
	uint8_t version_major;
	uint8_t version_minor;
	uint16_t function_count;
	service_get_uptime_t GetUptime;
	service_debug_writeln_t DebugWriteLine;
	service_debug_writeln_hex32_t DebugWriteLineHex32;
	service_write_screen_t WriteScreen;
	service_write_screen_decimal_t WriteScreenDecimal;
	service_write_i2c_t WriteI2C;
	service_read_i2c_t ReadI2C;
} PACKED_ATTR;

extern const struct service_api service_api;

/**
 * Memory address where the service API structure is located.
 * This is a fixed address in the LPC1114 memory map.
 */
enum {
	SERVICE_API_ADDRESS = 0x300
};

/*
 * Plugin API types and structures.
 */

/**
 * Plugin initialization function.
 * @return 0 on success, negative on error.
 */
typedef int (*plugin_api_init_t)(void);

/**
 * Plugin API structure.
 * Located at PLUGIN_API_ADDRESS (0x7000) in memory.
 * Must be defined by plugins in section ".plugin.header".
 */
struct plugin_api {
	uint8_t version_major;
	uint8_t version_minor;
	uint16_t function_count;
	uint32_t required_memory;  /**< Minimum data memory size required. */
	uint32_t program_address;  /**< Plugin code base address. */
	uint32_t data_address;     /**< Plugin data base address. */
	plugin_api_init_t Init;
} PACKED_ATTR;

extern const struct plugin_api plugin_api;

/**
 * Plugin memory addresses.
 * PLUGIN_API_ADDRESS: Where the plugin_api structure is located.
 * PLUGIN_DATA_ADDRESS: Base address for plugin data memory (SRAM).
 */
enum {
	PLUGIN_API_ADDRESS = 0x7000,
	PLUGIN_DATA_ADDRESS = 0x10000200  /* 0x10000000 + 0x200 */
};

#if defined(_MSC_VER)
#pragma pack(pop)
#endif

/*
 * Section attribute macros for placing plugin code and data.
 */

#if defined(_MSC_VER)
#define PLUGIN_STR(x) ((const char *)(x)), (sizeof(x) - 1)
#define PLUGIN_RODATA(suffix)
#define PLUGIN_FUNCTION()
#define PLUGIN_HEADER()
#else
/** Place string literal in plugin read-only data section. */
#define PLUGIN_STR(s) \
	(__extension__({ \
		static char __c[] __attribute__((section(".plugin.rodata"))) = (s); \
		&__c[0]; \
	})), (sizeof(s) - 1)
/** Place constant in plugin read-only data section. */
#define PLUGIN_RODATA(suffix) __attribute__((section(".plugin.rodata." suffix)))
/** Place function in plugin code section. */
#define PLUGIN_FUNCTION() __attribute__((section(".plugin.functions")))
/** Place plugin header in designated section. */
#define PLUGIN_HEADER() __attribute__((section(".plugin.header")))
#endif

#if defined(__cplusplus)
}
#endif

#endif /* PLUGIN_API_H */
