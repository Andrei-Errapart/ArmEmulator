/** \file API for plugin and the services. */
#ifndef plugin_api_h_
#define plugin_api_h_

#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(_MSC_VER)
#pragma pack(push,1)
#endif

/** Get time, in 1ms units, since system start. Effective resolution is ~16ms.
 */
typedef uint32_t	(*service_get_uptime_t)(void);

/** Implementation of service_get_uptime_t.
 * Do not use in the update!
 */
extern uint32_t
service_get_uptime(void);

/** Write line of text to the serial port. The text is preceeded with "PLUGIN:" */
typedef void (*service_debug_writeln_t) (
		const char*		s,
		const uint16_t	length);

/** Write line of text and the hexadecimal number to the serial port.
 * The text is preceeded with "PLUGIN:"
 * Upper halfword is ommited if zero.
 * */
typedef void (*service_debug_writeln_hex32_t) (
		const char*		s,
		const uint16_t	length,
		const uint32_t	x);

/** Write to the screen, if permitted.
 * yx:		Upper halfword=Row, Lower halfword=Column
 * s:		Text to be written.
 * length:	Length of the text.
 * */
typedef void (*service_write_screen_t)(
		const uint32_t	yx,
		const char*		s,
		const uint16_t	length);

/** Write decimal number to the screen, if permitted.
 * yx:		Upper halfword=Row, Lower halfword=Column
 * number: Number to be written.
 * nspaces: 0 for no padding, >1 for padding on the left if necessary.
 * */
typedef void (*service_write_screen_decimal_t)(
		const uint32_t	yx,
		const int32_t	number,
		const uint16_t	nspaces);

/** Write to the given I2C device the given bytes.
 * address_page_register: Use macro MAKE_ADDRESS_REGISTER for composing addresses.
 * 			MSB: Zero.
 * 			Byte 2: Device address.
 * 			Byte 1: Page number, if applicable (on a TDA9984).
 * 			LSB: Register number.
 * Return value: 0=ok, <0=Error.
 * */
typedef int (*service_write_i2c_t)(
		const uint32_t	address_page_register,
		const uint8_t*	data,
		const uint32_t	nbytes);

#define	MAKE_ADDRESS_PAGE_REGISTER(device_address, page, register_number)	\
	(		  ((device_address) << 16) \
			| ((page) << 8) \
			| (register_number) \
		)

/** Read the given number of bytes from I2C port.
 * buffer:	Buffer to write to.
 * address_page_register: Use macro MAKE_ADDRESS_REGISTER for composing addresses.
 * 			MSB: Zero.
 * 			Byte 2: Device address.
 * 			Byte 1: Page number, if applicable (on a TDA9984).
 * 			LSB: Register number.
 * count:	Number of bytes to be read.
 * Return value: 0=ok, <0=Error.
 * */
typedef int (*service_read_i2c_t)(
		uint8_t*		buffer,
		const uint32_t	address_page_register,
		const uint32_t	count);

typedef struct
#if defined(__GNUC__)
	__attribute__((__packed__))
#endif
{
	uint8_t					VersionMajor;
	uint8_t					VersionMinor;
	uint16_t				FunctionCount;
	service_get_uptime_t	GetUptime;
	service_debug_writeln_t	DebugWriteLine;
	service_debug_writeln_hex32_t
							DebugWriteLineHex32;
	service_write_screen_t	WriteScreen;
	service_write_screen_decimal_t
							WriteScreenDecimal;
	service_write_i2c_t		WriteI2C;
	service_read_i2c_t		ReadI2C;
} SERVICE_API;

extern const SERVICE_API service_api;
enum {
	SERVICE_API_ADDRESS = 0x300
};

/** Initialize the plugin.
* Return values:
* 0=OK
* <0: Error.
*/
typedef int (*plugin_api_init_t)(void);

typedef struct
#if defined(__GNUC__)
	__attribute__((__packed__))
#endif
{
	uint8_t						VersionMajor;
	uint8_t						VersionMinor;
	uint16_t					FunctionCount;
	/// Data memory size, at least :)
	uint32_t					RequiredMemory;
	/// Program address.
	uint32_t					ProgramAddress;
	/// Data address.
	uint32_t					DataAddress;
	plugin_api_init_t			Init;
} PLUGIN_API;

/// This has to be defined in the section ".plugin.header"
extern const PLUGIN_API	plugin_api;
enum {
	PLUGIN_API_ADDRESS = 0x7000,
	PLUGIN_DATA_ADDRESS = (0x10000000 + 0x200)
};

#if defined(_MSC_VER)
#pragma pack(pop)
#endif

#if defined(_MSC_VER)
# define	PLUGIN_STR(x)			((const char*)(x)), (sizeof(x)-1)
# define	PLUGIN_RODATA(suffix)
# define	PLUGIN_FUNCTION()
# define	PLUGIN_HEADER()
#else
/// Use PSTR for constant argument strings in order to put them into appropriate section.
# define	PLUGIN_STR(s) 			(__extension__({static char __c[] __attribute__((section(".plugin.rodata"))) = (s); &__c[0];})), (sizeof(s)-1)
/// Add this to read-only constants.
# define	PLUGIN_RODATA(suffix)	__attribute__((section(".plugin.rodata." suffix)))
# define	PLUGIN_FUNCTION()		__attribute__((section(".plugin.functions")))
# define	PLUGIN_HEADER()			__attribute__((section(".plugin.header")))
#endif


#if defined(__cplusplus)
}
#endif

#endif /* plugin_api_h_ */
