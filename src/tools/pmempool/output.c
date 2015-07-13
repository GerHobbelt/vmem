/*
 * Copyright (c) 2014-2015, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * output.c -- definitions of output printing related functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include "common.h"
#include "output.h"

#define	TIME_STR_FMT "%a %b %d %Y %H:%M:%S"
#define	UUID_STR_MAX 37
#define	HEXDUMP_ROW_WIDTH 16
/*
 * 2 chars + space per byte +
 * space after 8 bytes and terminating NULL
 */
#define	HEXDUMP_ROW_HEX_LEN (HEXDUMP_ROW_WIDTH * 3 + 1 + 1)
/* 1 printable char per byte + terminating NULL */
#define	HEXDUMP_ROW_ASCII_LEN (HEXDUMP_ROW_WIDTH + 1)
#define	SEPARATOR_CHAR '-'

static int out_vlevel;
static unsigned int out_column_width = 20;
static FILE *out_fh;
static const char *out_prefix;

#define	STR_MAX 256

/*
 * outv_check -- verify verbosity level
 */
int
outv_check(int vlevel)
{
	return (out_vlevel >= vlevel);
}

/*
 * out_set_col_width -- set column width
 *
 * See: outv_field() function
 */
void
out_set_col_width(unsigned int col_width)
{
	out_column_width = col_width;
}

/*
 * out_set_vlevel -- set verbosity level
 */
void
out_set_vlevel(int vlevel)
{
	out_vlevel = vlevel;
	if (out_fh == NULL)
		out_fh = stdout;
}

/*
 * out_set_prefix -- set prefix to output format
 */
void
out_set_prefix(const char *prefix)
{
	out_prefix = prefix;
}

/*
 * out_set_stream -- set output stream
 */
void
out_set_stream(FILE *stream)
{
	out_fh = stream;
}

/*
 * out_err -- print error message
 */
void
out_err(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	fprintf(stderr, "error: ");
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

/*
 * _out_prefix -- print prefix if defined
 */
static void
_out_prefix(void)
{
	if (out_prefix)
		fprintf(out_fh, "%s: ", out_prefix);
}

/*
 * outv -- print message taking into account verbosity level
 */
void
outv(int vlevel, const char *fmt, ...)
{
	va_list ap;

	if (outv_check(vlevel)) {
		_out_prefix();
		va_start(ap, fmt);
		vfprintf(out_fh, fmt, ap);
		va_end(ap);
	}
}

/*
 * outv_field -- print field name and value in specified format
 *
 * Field name will have fixed width which can be changed by
 * out_set_column_width() function.
 * vlevel - verbosity level
 * field  - field name
 * fmt    - format form value
 */
void
outv_field(int vlevel, const char *field, const char *fmt, ...)
{
	va_list ap;

	if (outv_check(vlevel)) {
		_out_prefix();
		va_start(ap, fmt);
		fprintf(out_fh, "%-*s : ", out_column_width, field);
		vfprintf(out_fh, fmt, ap);
		fprintf(out_fh, "\n");
		va_end(ap);
	}
}

/*
 * out_get_percentage -- return percentage string
 */
const char *
out_get_percentage(double perc)
{
	static char str_buff[STR_MAX] = {0, };

	if (perc > 0.0 && perc < 0.0001) {
		snprintf(str_buff, STR_MAX, "%e %%", perc);
	} else {
		int decimal = 0;
		if (perc >= 100.0 || perc == 0.0)
			decimal = 0;
		else
			decimal = 6;

		snprintf(str_buff, STR_MAX, "%.*f %%", decimal, perc);
	}

	return str_buff;
}

/*
 * out_get_size_str -- return size string
 *
 * human - if 1 return size in human-readable format
 *         if 2 return size in bytes and human-readable format
 * otherwise return size in bytes.
 */
const char *
out_get_size_str(uint64_t size, int human)
{
	static char str_buff[STR_MAX] = {0, };
	char units[] = {
		'K', 'M', 'G', 'T', '\0'
	};
	const int nunits = sizeof (units) / sizeof (units[0]);

	if (!human) {
		snprintf(str_buff, STR_MAX, "%ld", size);
	} else {
		int i = -1;
		double dsize = (double)size;
		double csize = size;

		while (csize >= 1024 && i < nunits) {
			csize /= 1024;
			dsize /= 1024.0;
			i++;
		}

		if (i >= 0 && i < nunits)
			if (human == 1)
				snprintf(str_buff, STR_MAX, "%.1f%c",
					dsize, units[i]);
			else
				snprintf(str_buff, STR_MAX, "%.1f%c [%lu]",
					dsize, units[i], size);
		else
			snprintf(str_buff, STR_MAX, "%ld",
					size);
	}

	return str_buff;
}

/*
 * out_get_uuid_str -- returns uuid in human readable format
 */
const char *
out_get_uuid_str(uuid_t uuid)
{
	static char uuid_str[UUID_STR_MAX] = {0, };

	uuid_unparse(uuid, uuid_str);

	return uuid_str;
}

/*
 * out_get_time_str -- returns time in human readable format
 */
const char *
out_get_time_str(time_t time)
{
	static char str_buff[STR_MAX] = {0, };
	struct tm *tm = localtime(&time);

	if (tm)
		strftime(str_buff, STR_MAX, TIME_STR_FMT, tm);
	else
		snprintf(str_buff, STR_MAX, "unknown");

	return str_buff;
}

/*
 * out_get_printable_ascii -- convert non-printable ascii to dot '.'
 */
static char
out_get_printable_ascii(char c)
{
	return isprint(c) ? c : '.';
}

/*
 * out_get_ascii_str -- get string with printable ASCII dump buffer
 *
 * Convert non-printable ASCII characters to dot '.'
 * See: out_get_printable_ascii() function.
 */
static int
out_get_ascii_str(char *str, size_t str_len, const uint8_t *datap, size_t len)
{
	int c = 0;
	int i;
	char pch;

	if (str_len < len)
		return -1;

	for (i = 0; i < len; i++) {
		pch = out_get_printable_ascii(datap[i]);
		c += snprintf(str + c, str_len - c, "%c", pch);
	}

	return c;
}

/*
 * out_get_hex_str -- get string with hexadecimal dump of buffer
 *
 * Hexadecimal bytes in format %02x, each one followed by space,
 * additional space after every 8th byte.
 */
static int
out_get_hex_str(char *str, size_t str_len, const uint8_t *datap, size_t len)
{
	int c = 0;
	int i;

	if (str_len < (3 * len + 1))
		return -1;

	for (i = 0; i < len; i++) {
		/* add space after n*8 byte */
		if (i && (i % 8) == 0)
			c += snprintf(str + c, str_len - c, " ");
		c += snprintf(str + c, str_len - c, "%02x ", datap[i]);
	}

	return c;
}

/*
 * outv_hexdump -- print buffer in canonical hex+ASCII format
 *
 * Print offset in hexadecimal,
 * sixteen space-separated, two column, hexadecimal bytes,
 * followed by the same sixteen bytes converted to printable ASCII characters
 * enclosed in '|' characters.
 */
void
outv_hexdump(int vlevel, const void *addr, size_t len, size_t offset, int sep)
{
	if (!outv_check(vlevel) || len <= 0)
		return;

	const uint8_t *datap = (uint8_t *)addr;
	uint8_t row_hex_str[HEXDUMP_ROW_HEX_LEN] = {0, };
	uint8_t row_ascii_str[HEXDUMP_ROW_ASCII_LEN] = {0, };
	size_t curr = 0;
	size_t prev = 0;
	int repeated = 0;
	int n = 0;

	while (len) {
		size_t curr_len = min(len, HEXDUMP_ROW_WIDTH);

		/*
		 * Check if current row is the same as the previous one
		 * don't check it for first and last rows.
		 */
		if (len != curr_len && curr &&
				!memcmp(datap + prev, datap + curr, curr_len)) {
			if (!repeated) {
				/* print star only for the first repeated */
				fprintf(out_fh, "*\n");
				repeated = 1;
			}
		} else {
			repeated = 0;

			/* row with hexadecimal bytes */
			int rh = out_get_hex_str((char *)row_hex_str,
				HEXDUMP_ROW_HEX_LEN, datap + curr, curr_len);
			/* row with printable ascii chars */
			int ra = out_get_ascii_str((char *)row_ascii_str,
				HEXDUMP_ROW_ASCII_LEN, datap + curr, curr_len);

			if (ra && rh)
				n = fprintf(out_fh, "%08lx  %-*s|%-*s|\n",
					curr + offset,
					HEXDUMP_ROW_HEX_LEN, row_hex_str,
					HEXDUMP_ROW_WIDTH, row_ascii_str);
			prev = curr;
		}

		len -= curr_len;
		curr += curr_len;
	}

	if (sep && n) {
		while (--n)
			fprintf(out_fh, "%c", SEPARATOR_CHAR);
		fprintf(out_fh, "\n");
	}
}

/*
 * out_get_checksum -- return checksum string with result
 */
const char *
out_get_checksum(void *addr, size_t len, uint64_t *csump)
{
	static char str_buff[STR_MAX] = {0, };
	uint64_t csum = *csump;

	/* validate checksum and get correct one */
	int valid = util_validate_checksum(addr, len, csump);

	if (valid)
		snprintf(str_buff, STR_MAX, "0x%08x [OK]", (uint32_t)csum);
	else
		snprintf(str_buff, STR_MAX,
			"0x%08x [wrong! should be: 0x%08x]",
			(uint32_t)csum, (uint32_t)*csump);

	/* restore original checksum */
	*csump = csum;

	return str_buff;
}

/*
 * out_get_btt_map_entry -- return BTT map entry with flags strings
 */
const char *
out_get_btt_map_entry(uint32_t map)
{
	static char str_buff[STR_MAX] = {0, };

	int is_init = (map & ~BTT_MAP_ENTRY_LBA_MASK) == 0;
	int is_zero = (map & ~BTT_MAP_ENTRY_LBA_MASK) ==
		BTT_MAP_ENTRY_ZERO;
	int is_error = (map & ~BTT_MAP_ENTRY_LBA_MASK) ==
		BTT_MAP_ENTRY_ERROR;
	int is_normal = (map & ~BTT_MAP_ENTRY_LBA_MASK) ==
		BTT_MAP_ENTRY_NORMAL;

	uint32_t lba = map & BTT_MAP_ENTRY_LBA_MASK;

	snprintf(str_buff, STR_MAX, "0x%08x state: %s", lba,
			is_init ? "init" :
			is_zero ? "zero" :
			is_error ? "error" :
			is_normal ? "normal" : "unknown");

	return str_buff;
}

/*
 * out_get_pool_type_str -- get pool type string
 */
const char *
out_get_pool_type_str(pmem_pool_type_t type)
{
	switch (type) {
	case PMEM_POOL_TYPE_LOG:
		return "log";
	case PMEM_POOL_TYPE_BLK:
		return "blk";
	case PMEM_POOL_TYPE_OBJ:
		return "obj";
	default:
		return "unknown";
	}
}
