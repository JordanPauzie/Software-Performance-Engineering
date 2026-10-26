/**
 * Copyright (c) 2024 MIT License by 6.106 Staff
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 **/

#include "my_utils.h"

/********** get_rotated_block ********
 *
 * Updates a passed in 2d array block of an image with defined size. When
 * getting the block it rotates the specific bits within the block by 90 
 * degrees clockwise.
 *
 * Parameters:
 *      uint8_t *img:              1D array of the image of bits
 *      const bytes_t row_size:    size of the image's row to handle 1D storage
 *      uint32_t i:                x coordinate of the block's top left corner
 *      uint32_t j:                y coordinate of the block's top left corner
 *
 * Returns: 
 *      None, but updates the passed in block
 *
 * Expects:
 *      img is a valid 1D array of bits
 *
 ****************************/
__attribute__((__always_inline__)) inline void get_block(uint8_t *img, const bytes_t row_size, uint32_t i, uint32_t j, Block *restrict block) {
    const uint8_t *rowp = img + (size_t)j * row_size + (i >> 3);

    #pragma GCC unroll 16
    for (int r = 0; r < BLOCK_SIZE; r++) {
        block->row[r] = (uint16_t)((rowp[0] << 8) | rowp[1]);
        rowp += row_size;
    }
}

/********** set_block ********
 *
 * Sets a block of an image with defined size to the values in a passed in
 * 2d array block. When setting the block it rotates the specific bits within
 * the block by 90 degrees counter-clockwise.
 *
 * Parameters:
 *      uint8_t *img:              1D array of the image of bits
 *      const bytes_t row_size:    size of the image's row to handle 1D storage
 *      uint32_t i:                x coordinate of the block's top left corner
 *      uint32_t j:                y coordinate of the block's top left corner
 *
 * Returns: 
 *      None, but updates the passed in img
 *
 * Expects:
 *      img is a valid 1D array of bits
 *      block rows are aligned to be a multiple of 8 bits
 *
 ****************************/
__attribute__((__always_inline__)) inline void set_block(uint8_t *img, const bytes_t row_size, uint32_t i, uint32_t j, Block *restrict block) {
    uint8_t *rowp = img + (size_t)j * row_size + (i >> 3);

    #pragma GCC unroll 16
    for (int r = 0; r < BLOCK_SIZE; r++) {
        uint16_t w = block->row[r];
        rowp[0] = (uint8_t)(w >> 8);
        rowp[1] = (uint8_t)(w & 0xFF);
        rowp += row_size;
    }
}

/********** rotate_block ********
 *
 * Rotates a BLOCK_SIZE x BLOCK_SIZE block of bits by 90 degrees clockwise.
 *
 * Parameters:
 *      Block *block:              pointer to the block to be rotated
 *
 * Returns: 
 *      None, but updates the passed in block
 *
 * Expects:
 *      block is a valid pointer to a Block struct
 *      blocks are made up of 16 rows of 16 bits each
 *
 ****************************/
__attribute__((__always_inline__)) inline void rotate_block(Block *restrict block) {
    uint16_t src[BLOCK_SIZE];

    // Read in
    #pragma GCC unroll 16
    for (int r = 0; r < BLOCK_SIZE; ++r) {
        src[r] = block->row[r];
    }

    // Rotate
    #pragma GCC unroll 16
    for (int c = 0; c < BLOCK_SIZE; ++c) {
        const uint16_t mask = (uint16_t)(1u << (15 - c));
        uint16_t col = 0;

        #pragma GCC unroll 16
        for (int r = 0; r < BLOCK_SIZE; ++r) {
            col |= (uint16_t)(((src[r] & mask) != 0) << r);
        }

        block->row[c] = col;
    }
}