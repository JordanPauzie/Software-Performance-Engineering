#pragma once
#include "board.h"

void init_zob();
uint64_t swap_color_to_move(uint64_t zob_key);
uint64_t toggle_piece(uint64_t zob_key, square_t sq, piece_t piece);