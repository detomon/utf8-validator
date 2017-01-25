/*
 * Copyright (c) 2016 Simon Schoenenberger
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <assert.h>
#include <string.h>
#include "utf8-validator.h"

/**
 * Get minimum of two values.
 */
#define MIN(a, b) ((a) < (b) ? (a) : (b))

/**
 * Worst case chunk size fitting into the buffer when the input chunk only
 * consists of invalid sequences.
 */
#define DECOCDED_SIZE(size) (((size) / 3) - 16)

/**
 * Minimum valid range for sequence sizes. Value is expressed as shift.
 *
 * ```
 * 0XXXXXXX
 * 110XXXXX 10XXXXXX                                      (1 << 7)
 * 1110XXXX 10XXXXXX 10XXXXXX                             (1 << 11)
 * 11110XXX 10XXXXXX 10XXXXXX 10XXXXXX                    (1 << 16)
 * 111110XX 10XXXXXX 10XXXXXX 10XXXXXX 10XXXXXX           (1 << 22)
 * 1111110X 10XXXXXX 10XXXXXX 10XXXXXX 10XXXXXX 10XXXXXX  (1 << 29)
 * ```
 */
#define VALID_RANGES ((7UL << 5) | (11UL << 10) | (16UL << 15) | (22UL << 20) | (29UL << 25))

/**
 * Calculate minimum valid value for sequence with `count` continuation bytes.
 */
#define MIN_VALID_VALUE(count) (1UL << (((VALID_RANGES >> ((count) * 5)) & 0x1F)))

/**
 * Maximum valid Unicode value.
 */
#define MAX_VALUE 0x10FFFF

/**
 * Contains information for a byte.
 */
typedef union {
	uint8_t value;       ///< Field for initialization.
	struct {
		uint8_t init:5;  ///< Initial glyph value.
		uint8_t count:3; ///< Number of following bytes.
		                 ///< If 0, byte is no valid starting byte.
	};
} byte_info;

/**
 * UTF-8 initial bytes from 0x80 to 0xFF. A value contains the number of
 * continuation bytes in the upper 3 bits (0b11100000) and the initial value in
 * the lower 5 bits (0b00011111).
 */

static byte_info const lookup_table[128] = {
	{0x00}, {0x00}, {0x00}, {0x00}, {0x00}, {0x00}, {0x00}, {0x00},
	{0x00}, {0x00}, {0x00}, {0x00}, {0x00}, {0x00}, {0x00}, {0x00},
	{0x00}, {0x00}, {0x00}, {0x00}, {0x00}, {0x00}, {0x00}, {0x00},
	{0x00}, {0x00}, {0x00}, {0x00}, {0x00}, {0x00}, {0x00}, {0x00},
	{0x00}, {0x00}, {0x00}, {0x00}, {0x00}, {0x00}, {0x00}, {0x00},
	{0x00}, {0x00}, {0x00}, {0x00}, {0x00}, {0x00}, {0x00}, {0x00},
	{0x00}, {0x00}, {0x00}, {0x00}, {0x00}, {0x00}, {0x00}, {0x00},
	{0x00}, {0x00}, {0x00}, {0x00}, {0x00}, {0x00}, {0x00}, {0x00},
	{0x20}, {0x21}, {0x22}, {0x23}, {0x24}, {0x25}, {0x26}, {0x27},
	{0x28}, {0x29}, {0x2A}, {0x2B}, {0x2C}, {0x2D}, {0x2E}, {0x2F},
	{0x30}, {0x31}, {0x32}, {0x33}, {0x34}, {0x35}, {0x36}, {0x37},
	{0x38}, {0x39}, {0x3A}, {0x3B}, {0x3C}, {0x3D}, {0x3E}, {0x3F},
	{0x40}, {0x41}, {0x42}, {0x43}, {0x44}, {0x45}, {0x46}, {0x47},
	{0x48}, {0x49}, {0x4A}, {0x4B}, {0x4C}, {0x4D}, {0x4E}, {0x4F},
	{0x60}, {0x61}, {0x62}, {0x63}, {0x64}, {0x65}, {0x66}, {0x67},
	{0x80}, {0x81}, {0x82}, {0x83}, {0xA0}, {0xA1}, {0x00}, {0x00},
};

/**
 * Write replacement glyph into buffer and return new cursor.
 */
static inline uint8_t* write_replacement_glyph(uint8_t* buffer) {
	*buffer++ = 0xEF;
	*buffer++ = 0xBF;
	*buffer++ = 0xBD;

	return buffer;
}

/**
 * Save current state for continuation.
 */
static void save_state(utf8_validator* validator, uint32_t count, int32_t offset, uint32_t value, uint8_t* outPtr) {
	validator->count = count;
	validator->offset = offset;
	validator->value = value;

	validator->fragSize = offset + 1;
	memcpy(validator->frag, outPtr - validator->fragSize, validator->fragSize);
}

/**
 * Check if glyph value is valid.
 */
static int check_glyph_value(uint32_t value, uint32_t count) {
	if (value >= 0xD800) {
		// check if low or high surrogate
		// matches range from 0xD8nn to 0xDCnn
		if ((value & ~0x07FF) == 0xD800) {
			return 0;
		}
		// U+nFFFE U+nFFFF (for n = 1..10)
		else if ((value & 0xFFFE) == 0xFFFE) {
			return 0;
		}
		// other non-characters
		else if (value >= 0xFDD0 && value <= 0xFDEF) {
			return 0;
		}
		// check for maximum value
		else if (value > MAX_VALUE) {
			return 0;
		}
	}

	// check for overlong sequences
	if (value < MIN_VALID_VALUE(count)) {
		return 0;
	}

	return 1;
}

/**
 * Validates an UTF-8 encoded byte chunk by replacing invalid sequences with the
 * replacement glyph � (U+FFFD). Marks the last complete sequence found in the
 * chunk. If a sequence cannot be completed, the current state is saved and
 * continued when the next chunk is given.
 */
static uint8_t* parse_chunk(utf8_validator* validator, uint8_t const* inPtr, uint8_t const* end, uint8_t* outPtr) {
	uint32_t count;
	int32_t offset;
	uint32_t value;
	size_t size;

	// continue from previous truncated sequence
	if (validator->count) {
		count = validator->count;
		offset = validator->offset;
		value = validator->value;
		size = MIN(count, offset + end - inPtr);
		validator->count = 0;

		// last glyph is truncated
		if (end - inPtr == 0) {
			goto invalid_sequence;
		}

		// continue previous glyph
		goto continue_glyph;
	}

	while (inPtr < end) {
		int c = *inPtr++;

		if (c <= 0x7F) {
			*outPtr++ = c;
		}
		else {
			byte_info info = lookup_table[c - 128];

			count = info.count;
			value = info.init;

			// invalid starting character
			if (!count) {
				offset = -1;
				goto invalid_sequence;
			}

			size = MIN(count, end - inPtr);
			offset = 0;

			*outPtr++ = c;

			// read continuation bytes
			for (; offset < size; offset++) {
				continue_glyph:
				c = *inPtr++;

				if ((c & 0xC0) != 0x80) {
					inPtr--;
					goto invalid_sequence;
				}

				value = (value << 6) | (c & 0x3F);
				*outPtr++ = c;
			}

			// save state and abort if sequence is truncated
			if (offset < count) {
				save_state(validator, count, offset, value, outPtr);
				break;
			}

			if (!check_glyph_value(value, count)) {
				goto invalid_sequence;
			}
		}

		continue;

		invalid_sequence: {
			outPtr -= offset + 1;
			outPtr = write_replacement_glyph(outPtr);
		}
	}

	return outPtr;
}

void utf8_validate(utf8_validator* validator, uint8_t const** inChunk, size_t* inSize, uint8_t* outBuffer, size_t* outSize) {
	size_t chunkSize;
	size_t size = inSize ? *inSize : 0;
	size_t bufferSize = *outSize;
	uint8_t const* inPtr = inChunk ? *inChunk : NULL;
	uint8_t const* subchunkEnd;
	uint8_t* outPtr = outBuffer;

	assert(bufferSize >= 72);

	if (validator->fragSize) {
		memcpy(outBuffer, validator->frag, validator->fragSize);

		bufferSize -= validator->fragSize;
		outPtr += validator->fragSize;
		validator->fragSize = 0;
	}

	chunkSize = DECOCDED_SIZE(bufferSize);
	subchunkEnd = inPtr + MIN(chunkSize, size);

	outPtr = parse_chunk(validator, inPtr, subchunkEnd, outPtr);
	*outSize = (outPtr - outBuffer) - validator->fragSize;

	if (inChunk) {
		*inChunk = subchunkEnd;
		*inSize -= subchunkEnd - inPtr;
	}
}
