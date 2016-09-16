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

/**
 * @file
 *
 * The UTF-8 validator reads chunks of bytes of arbitary length and outputs
 * chunks containing only complete UTF-8 sequences. Sequences overlapping the
 * chunk boundaries are joined. Invalid bytes and sequences are replaced with
 * the replacement glyph � (0xFFFD).
 *
 * The validator uses the checks suggested by Markus G. Kuhn
 * <http://www.cl.cam.ac.uk/~mgk25/> using the test file
 * <http://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-test.txt>.
 *
 * The following is considered to be invalid:
 *
 * - Invalid initial bytes and detached continuation bytes
 * - Incomplete sequences
 * - Overlong glyph representations
 * - Low and high surrogates
 * - Glyphs in the "internal use area"
 *
 * ## Example
 *
 * ```
 * size_t dataSize;
 * uint8_t* data = readFile("UTF-8-test.txt", &dataSize);
 *
 * uint8_t buffer[4096];
 * utf8_validator validator = {0};
 *
 * uint8_t* dataPtr = data;
 * size_t inSize = dataSize;
 * size_t outSize;
 *
 * while (inSize) {
 *     outSize = sizeof(buffer);
 *     utf8_validate(&validator, &dataPtr, &inSize, buffer, &outSize);
 *
 *     if (outSize) {
 *         handleChunk(buffer, outSize);
 *     }
 * }
 * ```
 *
 * The stream end is signalled by giving an empty chunk. This is to check for a
 * possible truncation of the last sequence.
 *
 * ```
 * outSize = sizeof(buffer);
 * utf8_validate(&validator, NULL, NULL, buffer, &outSize);
 *
 * if (outSize) {
 *     handleChunk(buffer, outSize);
 * }
 * ```
 *
 * The output buffer size should be around 4096 bytes. The absolute minimum size
 * is 72 bytes, which is really ineffective.
 */

#pragma once

#include <stdint.h>

/**
 * UTF-8 validator.
 */
typedef struct {
	uint16_t count;   ///< The number of continuation bytes in the current sequence.
	int16_t offset;   ///< The state's current sequence offset.
	uint32_t value;   ///< The state's current glyph value.
	uint8_t frag[7];  ///< The beginning of an incomplete sequence.
	uint8_t fragSize; ///< The size of the incomplete sequence in byte.
} utf8_validator;

/**
 * Validates an UTF-8 encoded stream. The input can be splitted in chunks with
 * arbitary length. Validated chunks are output to the given @p outBuffer and
 * contain only complete UTF-8 sequences.
 *
 * @param validator The validator struct.
 * @param inChunk The input chunk to be validated.
 * @param inSize The size of the input chunk in bytes.
 * @param outBuffer The buffer in which to write the validated output.
 * @param outSize The size of the provided output buffer.
 *
 * @return No errors are defined.
 */
extern void utf8_validate(utf8_validator* validator, uint8_t const** inChunk, size_t* inSize, uint8_t* outBuffer, size_t* outSize);
