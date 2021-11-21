#ifndef comm_h_
#define comm_h_

#if defined(__cplusplus)
extern "C" {
#endif

/** \file Communications over UART and I2C.
 */
#include <stdint.h> // uint8_t, etc.

#if defined(_MSC_VER)
// hack :)
extern void UARTSend(const uint8_t *BufferPtr, uint32_t Length);
#endif

/// Hexadecimal representation of byte.
extern char*
hex_of_byte(
	char* buffer,
	const unsigned char x);

/// Hexadecimal representation of uint16_t.
extern char*
hex_of_uint16(
	char* 			buffer,
	const uint16_t	x);

/// String of integer.
extern char*
string_of_decimal(
	char*			buffer,
	int32_t			x);

/// Pad string from the left with spaces up to the given width.
extern char*
pad_left(
	char*				s,
	const unsigned int	width);

/// Parse hexdecimal string as byte.
extern uint8_t
uint8_of_hex2(const char* hex);

/// Sleep for the given amount of microseconds.
extern void
_delay_us(const uint32_t	us);

/// uart << msg
extern void
uart_write(const char* msg);

/// uart << prefix << hex_of ui8
extern void
uart_write_hex(const char* prefix, const uint8_t ui8);

/// uart << prefix << hex_of ui16
extern void
uart_write_hex16(const char* prefix, const uint16_t ui16);

/// uart << prefix << hex_of ui32
extern void
uart_write_hex32(const char* prefix, const uint32_t ui32);

/// uart << msg << \r\n
extern void
uart_writeln(const char* msg);

/// uart << 0x0d << 0x0a
extern void
uart_write_crlf(void);

/// uart << prefix << hex_of buffer
extern void
uart_write_buffer(
	const char* prefix,
	const uint8_t* buffer,
	const int nbytes);

#if defined(__cplusplus)
}
#endif


#endif /* comm_h_ */
