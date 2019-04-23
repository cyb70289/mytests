// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=8 smarttab
/*
 * Ceph - scalable distributed file system
 *
 * Copyright (C) 2011 New Dream Network
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software
 * Foundation.  See file COPYING.
 *
 */
//#include "common/utf8.h"

#define MAX_UTF8_SZ 6
#define INVALID_UTF8_CHAR 0xfffffffful

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>

static int high_bits_set(int c)
{
	int ret = 0;
	while (1) {
		if ((c & 0x80) != 0x080)
			break;
		c <<= 1;
		++ret;
	}
	return ret;
}

/* Encode a 31-bit UTF8 code point to 'buf'.
 * Assumes buf is of size MAX_UTF8_SZ
 * Returns -1 on failure; number of bytes in the encoded value otherwise.
 */
////////////////////////////////////////////////////////////////////////
// Current code
////////////////////////////////////////////////////////////////////////
int encode_utf8(unsigned long u, unsigned char *buf)
{
	static const unsigned long max_val[MAX_UTF8_SZ] = {
		0x0000007ful, 0x000007fful, 0x0000fffful,
		0x001ffffful, 0x03fffffful, 0x7ffffffful
	};
	static const int MAX_VAL_SZ = sizeof(max_val)/sizeof(max_val[0]);

	int i;
	for (i = 0; i < MAX_VAL_SZ; ++i) {
		if (u <= max_val[i])
			break;
	}
	if (i == MAX_VAL_SZ) {
		// This code point is too big to encode.
		return -1;
	}

	if (i == 0) {
		buf[0] = u;
	}
	else {
		signed int j;
		for (j = i; j > 0; --j) {
			buf[j] = 0x80 | (u & 0x3f);
			u >>= 6;
		}

		unsigned char mask = ~(0xFF >> (i + 1));
		buf[0] = mask | u;
	}

	return i + 1;
}

////////////////////////////////////////////////////////////////////////
// Optimized code
////////////////////////////////////////////////////////////////////////
/* XXX: Per https://en.wikipedia.org/wiki/UTF-8#Invalid_code_points,
 * since RFC3629(November 2003), code points after U+10FFFF must be treated
 * as invalid UTF-8 byte sequence.
 * But to be compatible with old code, below implementation still accepts
 * those illegal UTF-8 strings.
 */
int encode_utf8_quick(unsigned long u, unsigned char *buf)
{
	if (u <= 0x0000007F) {
		buf[0] = u;
		return 1;
	} else if (u <= 0x000007FF) {
		buf[0] = 0xC0 | (u >> 6);
		buf[1] = 0x80 | (u & 0x3F);
		return 2;
	} else if (u <= 0x0000FFFF) {
		buf[0] = 0xE0 | (u >> 12);
		buf[1] = 0x80 | ((u >> 6) & 0x3F);
		buf[2] = 0x80 | (u & 0x3F);
		return 3;
	} else if (u <= 0x001FFFFF) {
		buf[0] = 0xF0 | (u >> 18);
		buf[1] = 0x80 | ((u >> 12) & 0x3F);
		buf[2] = 0x80 | ((u >> 6) & 0x3F);
		buf[3] = 0x80 | (u & 0x3F);
		return 4;
	} else {
		/* rare/illegal code points */
		if (u <= 0x03FFFFFF) {
			for (int i = 4; i >= 1; --i) {
				buf[i] = 0x80 | (u & 0x3F);
				u >>= 6;
			}
			buf[0] = 0xF8 | u;
			return 5;
		} else if (u <= 0x7FFFFFFF) {
			for (int i = 5; i >= 1; --i) {
				buf[i] = 0x80 | (u & 0x3F);
				u >>= 6;
			}
			buf[0] = 0xFC | u;
			return 6;
		}
		return -1;
	}
}

/*
 * Decode a UTF8 character from an array of bytes. Return character code.
 * Upon error, return INVALID_UTF8_CHAR.
 */
unsigned long decode_utf8(unsigned char *buf, int nbytes)
{
	unsigned long code;
	int i, j;

	if (nbytes <= 0)
		return INVALID_UTF8_CHAR;

	if (nbytes == 1) {
		if (buf[0] >= 0x80)
			return INVALID_UTF8_CHAR;
		return buf[0];
	}

	i = high_bits_set(buf[0]);
	if (i != nbytes)
		return INVALID_UTF8_CHAR;
	code = buf[0] & (0xff >> i);
	for (j = 1; j < nbytes; ++j) {
		if ((buf[j] & 0xc0) != 0x80)
			    return INVALID_UTF8_CHAR;
		code = (code << 6) | (buf[j] & 0x3f);
	}

	// Check for invalid code points
	if (code == 0xFFFE)
	    return INVALID_UTF8_CHAR;
	if (code == 0xFFFF)
	    return INVALID_UTF8_CHAR;
	if (code >= 0xD800 && code <= 0xDFFF)
	    return INVALID_UTF8_CHAR;

	return code;
}

////////////////////////////////////////////////////////////////////////
// Benchmark
////////////////////////////////////////////////////////////////////////
static void bench(unsigned long *decoded, int n_decoded,
		unsigned char *encoded, int n_encoded,
		int (*encode)(unsigned long, unsigned char *))
{
	const int loops = 1024*1024*1024/n_decoded;
	double time, size;
	struct timeval tv1, tv2;

	gettimeofday(&tv1, 0);
	for (int i = 0; i < loops; ++i) {
		unsigned char *encoded2 = encoded;
		for (int i = 0; i < n_decoded; ++i)
			encoded2 += encode(decoded[i], encoded2);
	}
	gettimeofday(&tv2, 0);

	time = tv2.tv_usec - tv1.tv_usec;
	time = time / 1000000 + tv2.tv_sec - tv1.tv_sec;
	size = ((double)n_decoded * loops) / (1024*1024);
	printf("time: %.4f s\n", time);
	printf("data: %.0f Mchars\n", size);
	printf("BW: %.2f Mchars/s\n", size / time);

	/* Only to make sure compiler won't over optimize */
	int ret = 0;
	for (int i = 0; i < n_encoded; ++i)
		ret |= encoded[i];
	printf(ret ? "OK\n": "BAD\n");
}

static unsigned char *load_test_file(int *len)
{
	unsigned char *data;
	int fd;
	struct stat stat;

	fd = open("./UTF-8-demo.txt", O_RDONLY);
	if (fd == -1) {
		printf("Failed to open UTF-8-demo.txt!\n");
		exit(1);
	}
	if (fstat(fd, &stat) == -1) {
		printf("Failed to get file size!\n");
		exit(1);
	}

	*len = stat.st_size;
	data = malloc(*len);
	if (read(fd, data, *len) != *len) {
		printf("Failed to read file!\n");
		exit(1);
	}

	close(fd);

	return data;
}

/* Fill test buf with patterns */
static unsigned char *load_test_buf(int len)
{
	/* code len = 1, 2, 3, 4 */
	const char utf8[] = "\x55\xC7\x99\xEF\x88\xBF\xF0\x90\xBF\x80";
	/* code len = 5, 6 */
	//const char utf8[] = "\xF9\x81\x82\x83\x84\xFD\x81\x82\x83\x84\x85";
	const int utf8_len = sizeof(utf8)/sizeof(utf8[0]) - 1;

	unsigned char *data = malloc(len);
	unsigned char *p = data;

	while (len >= utf8_len) {
		memcpy(p, utf8, utf8_len);
		p += utf8_len;
		len -= utf8_len;
	}

	while (len--)
		*p++ = 0x7F;

	return data;
}

int main(void)
{
	unsigned char *data;
	int len = 4096;

#if 1
	data = load_test_buf(len);
#else
	data = load_test_file(&len);
#endif

	/* Validate quick method */

	/* Decode */
	unsigned char *data2 = data, *data2_end = data + len;
	int n_decoded = 0;
	unsigned long *decoded = (unsigned long *)malloc(len*8);
	unsigned long *decoded2 = decoded;
	while (data2 < data2_end) {
		int n = high_bits_set(*data2);
		n += !n;	/* 0 -> 1, others -> unchanged */
		*decoded2 = decode_utf8(data2, n);
		assert(*decoded2 != INVALID_UTF8_CHAR);
		++decoded2;
		++n_decoded;
		data2 += n;
	}
	/* Encode back with quick method and check against original sample */
	int len2 = len;
	unsigned char *encoded = (unsigned char *)malloc(len);
	unsigned char *encoded2 = encoded;
	for (int i = 0; i < n_decoded; ++i) {
		int n = encode_utf8_quick(decoded[i], encoded2);
		assert(n > 0);
		len2 -= n;
		encoded2 += n;
	}
	assert(len2 == 0 && memcmp(encoded, data, len) == 0);
	fprintf(stderr, "validation ok\n");

	/* Benchmark */
#ifdef BENCH_QUICK
	fprintf(stderr, "\nbench optimized code: encode_utf8_quick... ");
	bench(decoded, n_decoded, encoded, len, encode_utf8_quick);
#else
	fprintf(stderr, "\nbench current code: encode_utf8... ");
	bench(decoded, n_decoded, encoded, len, encode_utf8);
#endif

	return 0;
}
