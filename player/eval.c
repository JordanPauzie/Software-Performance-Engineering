// Copyright (c) 2022 MIT License by 6.172 / 6.106 Staff

#define _POSIX_C_SOURCE 200809L
#include "eval.h"

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "move_gen.h"
#include "tbassert.h"
#include "util.h"

// -----------------------------------------------------------------------------
// Evaluation
// -----------------------------------------------------------------------------

typedef struct {
  int32_t inner;
} ev_score_t;  // Static evaluator uses "hi res" values

void update_float(ev_score_t* accumulator, float inc) {
  accumulator->inner += (int32_t)(PAWN_EV_VALUE * inc);
}

void update_int(ev_score_t* accumulator, int inc) {
  accumulator->inner += PAWN_EV_VALUE * inc;
}

void update_ev_score(ev_score_t* accumulator, ev_score_t other) {
  accumulator->inner += other.inner;
}

int32_t get(ev_score_t accumulator) { return accumulator.inner; }

ev_score_t apply_weight(ev_score_t accumulator, int weight) {
  return (ev_score_t){(int32_t)(((int64_t)accumulator.inner) *
                                ((int64_t)weight) / ((int64_t)WEIGHT_SCALE))};
}

int RANDOMIZE;

int PMAT_weight = WEIGHT_SCALE;
int PDOPP_weight;
int PDMON_weight;
int MFACE_weight;
int MCEDE_weight;
int LCOVERAGE_weight;
int PMID_weight;
int MMID_weight;
int MOVES_weight;
int PSAFE_weight;
int PBLOCK_weight;
int PISO_weight;
int PBOX_weight;

enum heuristics_t {
  PMAT,
  PDOPP,
  PDMON,
  MFACE,
  MCEDE,
  LCOVERAGE,
  PMID,
  MMID,
  PBLOCK,
  PISO,
  PBOX,
  NUM_HEURISTICS
};
char* heuristic_strs[NUM_HEURISTICS] = {
    "PMAT", "PDOPP", "PDMON",  "MFACE", "MCEDE", "LCOVERAGE",
    "PMID", "MMID",  "PBLOCK", "PISO",  "PBOX",
};

// Heuristics for static evaluation - described in the leiserchess codewalk
// slides

float mface_pair(coord_t fil, coord_t rnk, piece_t piece, square_t opp_sq) {
  int bonus = 0;
  int delta_fil = fil_of(opp_sq) - fil;
  int delta_rnk = rnk_of(opp_sq) - rnk;
  if (piece.monarch_ori.is_file_dir) {
    bonus = piece.monarch_ori.file_dir.is_north ? delta_rnk : -delta_rnk;
  } else {
    bonus = piece.monarch_ori.rank_dir.is_east ? delta_fil : -delta_fil;
  }
  return ((float)bonus) / (abs(delta_rnk) + abs(delta_fil));
}

// MFACE heuristic: bonus (or penalty) for Monarch facing toward the other
// Monarch
float mface(position_t* p, coord_t fil, coord_t rnk) {
  square_t sq = square_of(fil, rnk);
  piece_t piece = get_piece_at(&p->board, sq);
  color_t c = piece.color;
  float total = 0;
  square_t opp_sq = get_monarch(&p->board, opp_color(c));
  if (get_piece_at(&p->board, opp_sq).typ == MONARCH) {
    total += mface_pair(fil, rnk, piece, opp_sq);
  }
  return total;
}

float mcede_pair(coord_t fil, coord_t rnk, square_t opp_sq) {
  int delta_fil = fil_of(opp_sq) - fil;
  int delta_rnk = rnk_of(opp_sq) - rnk;
  int penalty = 0;
  if (delta_fil >= 0 && delta_rnk >= 0) {  // NE quadrant
    penalty = (BOARD_WIDTH - fil) * (BOARD_WIDTH - rnk);
  } else if (delta_fil >= 0 && delta_rnk <= 0) {  // SE quadrant
    penalty = (BOARD_WIDTH - fil) * (rnk + 1);
  } else if (delta_fil <= 0 && delta_rnk <= 0) {  // SW quadrant
    penalty = (fil + 1) * (rnk + 1);
  } else if (delta_fil <= 0 && delta_rnk >= 0) {  // NW quadrant
    penalty = (fil + 1) * (BOARD_WIDTH - rnk);
  } else {
    tbassert(false,
             "You need some direction in life!\n");  // Shouldn't happen.
  }
  return ((float)penalty) / (BOARD_WIDTH * BOARD_WIDTH);
}

// MCEDE heuristic: penalty for ceding opp Monarch room to move.
float mcede(position_t* p, coord_t fil, coord_t rnk) {
  piece_t piece = get_piece_at(&p->board, square_of(fil, rnk));
  color_t c = piece.color;
  float total = 0;
  tbassert(piece.typ == MONARCH, "piece.typ = %d\n", piece.typ);

  square_t opp_sq = get_monarch(&p->board, opp_color(c));
  if (get_piece_at(&p->board, opp_sq).typ == MONARCH) {
    total += mcede_pair(fil, rnk, opp_sq);
  }
  return total;
}

// Marks the path/line-of-sight of the laser until it hits a piece or goes off
// the board.
// Increment for each time you touch a square with the laser
//
// p : Current board state.
// c : Color of monarch shooting laser.
// laser_map : End result will be stored here. Every square on the
//             path of the laser is marked with mark_mask.
// mark_mask : What each square is marked with.
void add_laser_path(position_t* p, color_t c, float* laser_map) {
  laser_t laser = get_laser_origin(&p->board, c);
  int length = 1;
  while (!square_eq(laser.sq, INVALID_SQUARE) && !laser.is_hit) {
    step_laser(&p->board, &laser);
    // set laser map to min
    if (laser_map[square_to_index(laser.sq)] > length) {
      laser_map[square_to_index(laser.sq)] = length;
    }
    length++;
  }
}

float mult_dist(square_t a, square_t b) {
  float delta_fil = abs(fil_of(a) - fil_of(b));
  float delta_rnk = abs(rnk_of(a) - rnk_of(b));
  if (delta_fil == 0 && delta_rnk == 0) {
    return 2;
  }
  float x = (1 / (delta_fil + 1)) * (1 / (delta_rnk + 1));
  return x;
}

// Manhattan distance
int manhattan_dist(square_t a, square_t b) {
  int delta_fil = abs(fil_of(a) - fil_of(b));
  int delta_rnk = abs(rnk_of(a) - rnk_of(b));
  return delta_fil + delta_rnk;
}

float laser_coverage(position_t* p, color_t color) {
  position_t np;
  sortable_move_t moves[MAX_NUM_MOVES];
  int num_moves = generate_all_with_color(p, moves, color);
  int i;

  float coverage_map[ARR_SIZE];

  // initialization
  for (int i = 0; i < ARR_SIZE; ++i) {
    coverage_map[i] = FLT_MAX;
  }

  // increment laser path for each possible move
  for (i = 0; i < num_moves; i++) {
    move_t mv = get_move(moves[i]);
    move_piece(p, &np, mv);                    // make the move
    add_laser_path(&np, color, coverage_map);  // increment laser path
  }

  // initialize coverage map
  float result = 0;

  // get square of monarch
  square_t monarch_sq = get_monarch(&p->board, color);
  // get square of opposing monarch
  square_t opp_monarch_sq = get_monarch(&p->board, opp_color(color));
  // if both monarchs are alive
  if (get_piece_at(&p->board, monarch_sq).typ == MONARCH &&
      get_piece_at(&p->board, opp_monarch_sq).typ == MONARCH) {
    // add in everything on board
    for (coord_t fil = 0; fil < BOARD_WIDTH; ++fil) {
      for (coord_t rnk = 0; rnk < BOARD_WIDTH; ++rnk) {
        square_t sq = square_of(fil, rnk);
        if (coverage_map[square_to_index(sq)] < FLT_MAX) {
          tbassert(mult_dist(sq, opp_monarch_sq) > 0,
                   "mult_distance must be positive");
          result += (((manhattan_dist(monarch_sq, sq)) /
                      (coverage_map[square_to_index(sq)]))) *
                    mult_dist(sq, opp_monarch_sq);
        }
      }
    }
  }

  // add in off-board weights
  // add in top row
  if (get_piece_at(&p->board, opp_monarch_sq).typ == MONARCH) {
    coord_t rnk = -1;
    coord_t fil;
    for (fil = -1; fil < BOARD_WIDTH + 1; fil++) {
      result += mult_dist(square_of(fil, rnk), opp_monarch_sq);
    }
    // add in bottom row
    rnk = BOARD_WIDTH;
    for (fil = -1; fil < BOARD_WIDTH + 1; fil++) {
      result += mult_dist(square_of(fil, rnk), opp_monarch_sq);
    }

    // add in left col (minus top and bottom)
    fil = -1;
    for (rnk = 0; rnk < BOARD_WIDTH; rnk++) {
      result += mult_dist(square_of(fil, rnk), opp_monarch_sq);
    }

    // add in right col (minus top and bottom)
    fil = BOARD_WIDTH;
    for (rnk = 0; rnk < BOARD_WIDTH; rnk++) {
      result += mult_dist(square_of(fil, rnk), opp_monarch_sq);
    }
  }

  return result;
}

// Harmonic-ish distance: 1/(|dx|+1) + 1/(|dy|+1)
float h_dist(square_t a, square_t b) {
  int delta_fil = abs(fil_of(a) - fil_of(b));
  int delta_rnk = abs(rnk_of(a) - rnk_of(b));
  float x = (1.0 / (delta_fil + 1)) + (1.0 / (delta_rnk + 1));
  return x;
}

int get_centrality(coord_t fil, coord_t rnk) {
  int centrality = 0;
  if (rnk < BOARD_WIDTH - 1 - rnk) {
    centrality += rnk;
  } else {
    centrality += BOARD_WIDTH - 1 - rnk;
  }
  if (fil < BOARD_WIDTH - 1 - fil) {
    centrality += fil;
  } else {
    centrality += BOARD_WIDTH - 1 - fil;
  }
  return centrality;
}

// Static evaluation.  Returns score
score_t eval(position_t* p, bool verbose) {
  // seed rand_r with a value of 1, as per
  // https://linux.die.net/man/3/rand_r
  static __thread unsigned int seed = 1;
  // verbose = true: print out components of score
  ev_score_t score[2][NUM_HEURISTICS] = {0};
  char sq_str[MAX_CHARS_IN_MOVE];

  square_t white_monarch = get_monarch(&p->board, WHITE);
  square_t black_monarch = get_monarch(&p->board, BLACK);

  for (coord_t fil = 0; fil < BOARD_WIDTH; fil++) {
    for (coord_t rnk = 0; rnk < BOARD_WIDTH; rnk++) {
      square_t sq = square_of(fil, rnk);
      piece_t piece = get_piece_at(&p->board, sq);
      color_t c = piece.color;
      if (verbose) {
        square_to_str(sq, sq_str, MAX_CHARS_IN_MOVE);
      }

      int centrality = get_centrality(fil, rnk);

      switch (piece.typ) {
        case EMPTY:
          break;
        case PAWN: {
          // PMATerial heuristic: Bonus for each Pawn
          update_int(&score[c][PMAT], 1);
          if (verbose) {
            printf("PMATerial bonus for %s Pawn on %s\n", color_to_str(c),
                   sq_str);
          }

          // Pawn Distance to MONarch heuristic: sum of harmonic distance of
          // player's pieces to their monarch
          square_t my_monarch = (c == WHITE) ? white_monarch : black_monarch;
          if (!square_eq(my_monarch, INVALID_SQUARE)) {
            float prox_bonus = h_dist(sq, my_monarch);
            update_float(&score[c][PDMON], prox_bonus);
            if (verbose) {
              printf("PDMON bonus %.2f for %s Pawn on %s\n", prox_bonus,
                     color_to_str(c), sq_str);
            }
          }
          // Pawn Distance to OPPonent heuristic: sum of harmonic distance of
          // player's pieces to opp monarch
          square_t opp_monarch = (c == WHITE) ? black_monarch : white_monarch;
          if (!square_eq(opp_monarch, INVALID_SQUARE)) {
            float prox_bonus = h_dist(sq, opp_monarch);
            update_float(&score[c][PDOPP], prox_bonus);
            if (verbose) {
              printf("PDMON bonus %.2f for %s Pawn on %s\n", prox_bonus,
                     color_to_str(c), sq_str);
            }
          }

          // PMID heuristic: Pawns should try to be close to the center of
          // the board to maximize usefulness
          update_float(&score[c][PMID], centrality);
          if (verbose) {
            printf("PMID bonus %d for %s Pawn on %s\n", centrality,
                   color_to_str(c), sq_str);
          }

          // PBLOCK heuristic: number of potential jumps that are blocked by the
          // pawn touch rule
          int blocked_squares = 0;
          cardinal_dir_t cardinals[] = {
              cardinal_of_file_dir(NORTH), cardinal_of_file_dir(SOUTH),
              cardinal_of_rank_dir(EAST), cardinal_of_rank_dir(WEST)};
          for (int jump_dir = 0; jump_dir < (int)COUNT_OF(cardinals);
               jump_dir++) {
            square_t dest = take_cardinal_step(sq, cardinals[jump_dir]);
            for (int adjacent_dir = 0; adjacent_dir < (int)COUNT_OF(cardinals);
                 adjacent_dir++) {
              square_t adjacent =
                  take_cardinal_step(dest, cardinals[adjacent_dir]);
              if (!square_eq(adjacent, sq) &&
                  get_piece_at(&p->board, adjacent).typ == PAWN &&
                  get_piece_at(&p->board, adjacent).color == c) {
                blocked_squares++;
                break;
              }
            }
          }
          update_int(&score[c][PBLOCK], blocked_squares);

          // PISO heuristic: number of pawns that are not protected in any way
          // by another pawn
          if (blocked_squares == 0) {
            update_int(&score[c][PISO], -1);
          }

          // PBOX heuristic: number of pawns in bounding box containing both
          // monarchs
          if (!square_eq(white_monarch, INVALID_SQUARE) &&
              !square_eq(black_monarch, INVALID_SQUARE)) {
            if ((rnk_of(sq) <= rnk_of(white_monarch) ||
                 rnk_of(sq) <= rnk_of(black_monarch)) &&
                (rnk_of(sq) >= rnk_of(white_monarch) ||
                 rnk_of(sq) >= rnk_of(black_monarch)) &&
                (fil_of(sq) <= fil_of(white_monarch) ||
                 fil_of(sq) <= fil_of(black_monarch)) &&
                (fil_of(sq) >= fil_of(white_monarch) ||
                 fil_of(sq) >= fil_of(black_monarch))) {
              update_int(&score[c][PBOX], 1);
            }
          }
          break;
        }
        case MONARCH: {
          // MFACE heuristic
          float kface_bonus = mface(p, fil, rnk);
          update_float(&score[c][MFACE], kface_bonus);
          if (verbose) {
            printf("MFACE bonus %.2f for %s Monarch on %s\n", kface_bonus,
                   color_to_str(c), sq_str);
          }

          // MCEDE heuristic
          float penalty = mcede(p, fil, rnk);
          update_float(&score[c][MCEDE], penalty);
          if (verbose) {
            printf("MCEDE penalty %.2f for %s Monarch on %s\n", penalty,
                   color_to_str(c), sq_str);
          }

          // MMID heuristic
          update_int(&score[c][MMID], centrality);
          if (verbose) {
            printf("MMID bonus %d for %s Monarch on %s\n", centrality,
                   color_to_str(c), sq_str);
          }

          break;
        }
        case INVALID:
          break;

        default:
          tbassert(false, "Jose says: no way!\n");  // No way, Jose!
      }
    }
  }

  // LASER_COVERAGE heuristic
  update_float(&score[WHITE][LCOVERAGE], laser_coverage(p, WHITE));
  update_float(&score[BLACK][LCOVERAGE], laser_coverage(p, BLACK));

  int weights[NUM_HEURISTICS] = {PMAT_weight,  PDOPP_weight, PDMON_weight,
                                 MFACE_weight, MCEDE_weight, LCOVERAGE_weight,
                                 PMID_weight,  MMID_weight,  PBLOCK_weight,
                                 PISO_weight,  PBOX_weight};
  ev_score_t total_score[2] = {0};
  for (color_t c = 0; c <= 1; c++) {
    for (int i = 0; i < NUM_HEURISTICS; i++) {
      ev_score_t bonus = apply_weight(score[c][i], weights[i]);
      if (verbose) {
        printf("Total %s score of %d for %s\n", heuristic_strs[i],
               get(score[c][i]), color_to_str(c));
      }
      update_ev_score(&total_score[c], bonus);
      if (verbose) {
        printf("Final %s contribution of %d for %s\n", heuristic_strs[i],
               get(bonus), color_to_str(c));
      }
    }
  }

  int32_t tot = get(total_score[WHITE]) - get(total_score[BLACK]);

  if (RANDOMIZE) {
    int z = rand_r(&seed) % (RANDOMIZE * 2 + 1);
    int randomness = z - RANDOMIZE;
    tot = tot + randomness;
    if (verbose) {
      printf("Randomness of %d added\n",
             randomness * (1 - 2 * color_to_move_of(p)));
    }
  }

  if (color_to_move_of(p) == BLACK) {
    tot = -tot;
  }

  return tot / EV_SCORE_RATIO;
}
