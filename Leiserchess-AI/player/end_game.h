// Copyright (c) 2022 MIT License by 6.172 / 6.106 Staff

#ifndef END_GAME_H
#define END_GAME_H

#include <stdbool.h>

#include "move_gen.h"
#include "search.h"

typedef struct piece_pos {
  coord_t f;
  coord_t r;
  cardinal_dir_t monarch_ori;
} piece_pos_t;

typedef struct end {
  piece_pos_t pieces[2];
} end_t;

typedef enum { WINNING, LOSING, NA } result_t;

bool player_wins(const position_t* p, color_t c);
bool is_end_game_position(const position_t* p);
bool is_game_over(const position_t* p);
score_t get_end_game_score(const position_t* p, int pov, int ply);

#endif  // END_GAME_H
