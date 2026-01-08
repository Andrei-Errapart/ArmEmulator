// SPDX-License-Identifier: MIT
#ifdef __USE_CMSIS
#include "LPC11xx.h"
#endif

#if defined(_MSC_VER) || defined(DESKTOP_BUILD)
#include <stdint.h>	// int8_t, etc.
#include <stdio.h>	// printf, etc.
#else
#include "driver_config.h"
#include "target_config.h"

#include "type.h"
#include "poll_uart.h"
#include "poll_i2c.h"
#endif
#include <string.h>	// strlen
#include <stdlib.h>	// atoi

#include "comm.h"
#include "plugin_api.h" // use some functions from them, too.

#if defined(_MSC_VER) || defined(DESKTOP_BUILD)
void UARTSend(const uint8_t *BufferPtr, uint32_t Length)
{
	uint32_t	i;
	for (i=0; i<Length; ++i)
	{
		printf("%c", BufferPtr[i]);
	}
}
#endif

static const char* _hextable = "0123456789ABCDEF";

//============================================================
static char _toupper(const char c)
{
	return c>='a' && c<='z' ? (c + 'A'-'a') : c;
}

//============================================================
char*
hex_of_byte(
	char* buffer,
	const unsigned char x)
{
	buffer[0] = _hextable[x >> 4];
	buffer[1] = _hextable[x & 0x0F];
	return buffer;
}

//============================================================
char*
hex_of_uint16(
	char* 			buffer,
	const uint16_t	x)
{
	hex_of_byte(buffer, x>>8);
	hex_of_byte(buffer+2, (const uint8_t)x);
	return buffer;
}

//============================================================
char*
string_of_decimal(
	char*			buffer,
	int32_t			x)
{
	uint8_t	have_started = 0; // false
	int		d = x>=0 ? 1000*1000*1000 : -1000*1000*1000;
	char*	ptr = buffer;

	if (x<0)
	{
		*ptr = '-';
		++ptr;
	}
	while (d!=0)
	{
		int32_t	this_round = x / d;
		if (this_round>0 || have_started)
		{
			*ptr = this_round + '0';
			++ptr;
			have_started = 1; // true
		}
		x = x - this_round*d;
		d = d / 10;
	}
	if (!have_started)
	{
		*ptr = '0';
		++ptr;
	}
	*ptr = 0;
	return buffer;
}

//============================================================
char*
pad_left(
	char*				s,
	const unsigned int	width)
{
	const int	length = strlen(s);
	if ((int)width > length)
	{
		// we have something to do!
		const int	nspaces = width - length;
		int i;
		s[width] = 0;
		// 1. shift the content.
		for (i=length-1; i>=0; --i)
		{
			s[i+nspaces] = s[i];
		}
		// 2. add the spaces.
		for (i=nspaces-1; i>=0; --i)
		{
			s[i] = ' ';
		}
	}
	return s;
}

//============================================================
uint8_t
uint8_of_hex2(const char* hex)
{
	const char* pos1 = strchr(_hextable, _toupper(hex[0]));
	const char* pos2 = strchr(_hextable, _toupper(hex[1]));
	if (pos1!=NULL && pos2!=NULL)
	{
		const uint8_t	h = pos1 - _hextable;
		const uint8_t	l = pos2 - _hextable;
		return h<<4 | l;
	}
	else
	{
		return 0x00;
	}
}

#if !defined(_MSC_VER) && !defined(DESKTOP_BUILD)
//============================================================
// Microsecond delay loop-
void
_delay_us(const uint32_t us)
{
	uint32_t CyclestoLoops;

	CyclestoLoops = SystemCoreClock;
	if (CyclestoLoops >= 2000000)
	{
		CyclestoLoops /= 1000000;
		CyclestoLoops *= us;
	}
	else
	{
		CyclestoLoops *= us;
		CyclestoLoops /= 1000000;
	}

	if (CyclestoLoops <= 100) {
		return;
	}
	CyclestoLoops -= 100; // cycle count for entry/exit 100? should be measured
	CyclestoLoops /= 4; // cycle count per iteration- should be 4 on Cortex M0/M3

	if (!CyclestoLoops) {
		return;
	}

	// Delay loop for Cortex M3 thumb2
	asm volatile (
		// Load loop count to register
		" mov r3, %[loops]\n"

		// loop start- subtract 1 from r3
		"loop: sub r3, #1\n"
		// test for zero, loop if not
		" bne loop\n\n"

		: // No output registers
		: [loops] "r" (CyclestoLoops) // Input registers
		: "r3" // clobbered registers
	);
}
#endif

//============================================================
void
uart_write(const char* msg)
{
	const int length = strlen(msg);
	if (length>0)
	{
		UARTSend((unsigned char*)msg, length);
	}
}

//============================================================
void
uart_write_hex(const char* prefix, const uint8_t ui8)
{
	char	xbuf[2];
	const int length = strlen(prefix);

	UARTSend((unsigned char*)prefix, length);
	hex_of_byte(xbuf, ui8);
	UARTSend((unsigned char*)xbuf, 2);
}

//============================================================
void
uart_write_hex16(const char* prefix, const uint16_t ui16)
{
	char	xbuf[4];
	const int length = strlen(prefix);

	UARTSend((unsigned char*)prefix, length);
	hex_of_byte(xbuf, ui16 >> 8);
	hex_of_byte(xbuf+2, ui16 & 0xFF);
	UARTSend((unsigned char*)xbuf, sizeof(xbuf));
}

//============================================================
void
uart_write_hex32(const char* prefix, const uint32_t ui32)
{
	char	xbuf[9];
	const int length = strlen(prefix);

	UARTSend((unsigned char*)prefix, length);
	hex_of_byte(xbuf, ui32 >> 24);
	hex_of_byte(xbuf+2, ui32 >> 16);
	xbuf[4] = '_';
	hex_of_byte(xbuf+5, ui32 >> 8);
	hex_of_byte(xbuf+7, ui32);
	UARTSend((unsigned char*)xbuf, sizeof(xbuf));
}

//============================================================
void
uart_writeln(const char* msg)
{
	uart_write(msg);
	UARTSend((unsigned char*)"\r\n", 2);
}

#if !defined(_MSC_VER)
//============================================================
void
uart_write_buffer(
		const char* prefix,
		const uint8_t* buffer,
		const int nbytes)
{
	char xbuf[nbytes*3+1];
	int i;

	if (prefix)
	{
		uart_write(prefix);
	}
	for (i=0; i<nbytes; ++i)
	{
		hex_of_byte(xbuf+3*i, buffer[i]);
		xbuf[3*i+2] = (i+1==nbytes) ? 0 : ' ';
	}
	uart_write(xbuf);
}
#endif

//============================================================
void
uart_write_crlf(void)
{
	UARTSend((const uint8_t*)"\r\n", 2);
}
