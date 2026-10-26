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

#include "../utils/utils.h"
#include "my_utils.h"

/********** rotate_bit_matrix ********
 * rotates a bit array clockwise 90 degrees.
 * 
 * Parameters:
 *      uint8_t *img:              1D array of the image of bits
 *      const bits_t N:            size of one side of the NxN bit array
 *
 * Returns:
 *      None, but updates the passed in img
 *
 * Expects:
 *      img is a valid 1D array of bits
 *      img is of size N by N bits
 *      N is a multiple of 64
 *
 ****************************/
void rotate_bit_matrix(uint8_t *img, const bits_t N) {
  // Get the number of bytes per row in `img`
  const uint32_t row_size = bits_to_bytes(N);

  uint32_t w, h, quadrant;

  // Process BLOCK_SIZE x BLOCK_SIZE blocks at a time
  for (h = 0; h < N / 2; h += BLOCK_SIZE) {
    for (w = 0; w < N / 2; w += BLOCK_SIZE) {
      uint32_t i = w, j = h;
      Block tmp_block, next_block;

      // Get the BLOCK_SIZE x BLOCK_SIZE block starting at (i, j)
      // get_rotated_block(img, row_size, i, j, &tmp_block);
      get_block(img, row_size, i, j, &tmp_block);
      rotate_block(&tmp_block);

      // Move a bit from one quadrant to the next and do this
      // for all 4 quadrants of the `img`
      for (quadrant = 0; quadrant < 4; quadrant++) {
        uint32_t next_i = N - j - BLOCK_SIZE, next_j = i;
        
        // get_rotated_block(img, row_size, next_i, next_j, &next_block);
        get_block(img, row_size, next_i, next_j, &next_block);
        rotate_block(&next_block);

        set_block(img, row_size, next_i, next_j, &tmp_block);
        // Update the `i` and `j` with the next quadrant's values and
        // the `next_i` and `next_j` will get the new destination values
        tmp_block = next_block;
        i = next_i;
        j = next_j;
      }
    }
  }

  return;
}