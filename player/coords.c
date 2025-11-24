#include "coords.h"

#include <stdlib.h>

#include "board.h"
#include "tbassert.h"
#include "util.h"

square_t square_of(coord_t fil, coord_t rnk) {
  square_t s = {ARR_WIDTH * (FIL_ORIGIN + fil) + RNK_ORIGIN + rnk};
  DEBUG_LOG(1, "Square of (file %d, rank %d) is %d\n", fil, rnk, s);
  check_square(s);
  return s;
}

coord_t fil_of(square_t sq) {
  coord_t fil = sq.val / ARR_WIDTH - FIL_ORIGIN;
  DEBUG_LOG(1, "File of square %d is %d\n", sq.val, fil);
  return fil;
}

coord_t rnk_of(square_t sq) {
  coord_t rnk = sq.val % ARR_WIDTH - RNK_ORIGIN;
  DEBUG_LOG(1, "Rank of square %d is %d\n", sq.val, rnk);
  return rnk;
}

bool square_eq(square_t a, square_t b) { return a.val == b.val; }

// converts a square to string notation, returns number of characters printed
int square_to_str(square_t sq, char* buf, size_t bufsize) {
  return snprintf(buf, bufsize, "%c%d", 'a' + fil_of(sq), rnk_of(sq));
}

square_t get_midpoint(square_t a, square_t b) {
  return (square_t){.val = (a.val + b.val) / 2};
}

void check_square(square_t sq) {
  tbassert(sq.val < ARR_SIZE && sq.val >= 0, "sq: %d\n", sq.val);
  MARK_USED(sq);
}

void check_valid_square(square_t sq) {
  tbassert(sq.val < ARR_SIZE && sq.val > 0, "sq: %d\n", sq.val);
  MARK_USED(sq);
}

cardinal_dir_t cardinal_of_file_dir(file_dir_t dir) {
  return (cardinal_dir_t){.is_file_dir = true, .file_dir = dir};
}

cardinal_dir_t cardinal_of_rank_dir(rank_dir_t dir) {
  return (cardinal_dir_t){.is_file_dir = false, .rank_dir = dir};
}

bool cardinal_dir_eq(cardinal_dir_t a, cardinal_dir_t b) {
  return a.is_file_dir == b.is_file_dir &&
         (a.is_file_dir ? (a.file_dir.is_north == b.file_dir.is_north)
                        : (a.rank_dir.is_east == b.rank_dir.is_east));
}

// helpers for determining offset when taking a step in a direction
int get_file_offset(file_dir_t dir) { return (dir.is_north) ? 1 : -1; }

int get_rank_offset(rank_dir_t dir) {
  return (dir.is_east) ? ARR_WIDTH : -ARR_WIDTH;
}

square_t take_cardinal_step(square_t sq, cardinal_dir_t step_dir) {
  return (square_t){.val = sq.val + ((step_dir.is_file_dir)
                                         ? get_file_offset(step_dir.file_dir)
                                         : get_rank_offset(step_dir.rank_dir))};
}

// helpers for rotating directions
rank_dir_t rotate_file_right(file_dir_t dir) {
  return (rank_dir_t){.is_east = dir.is_north};
}

file_dir_t rotate_rank_right(rank_dir_t dir) {
  return (file_dir_t){.is_north = !dir.is_east};
}

cardinal_dir_t rotate_cardinal(cardinal_dir_t dir, rot_t rot) {
  for (int right_turns = 0; right_turns < (int)rot; right_turns++) {
    dir = dir.is_file_dir
              ? cardinal_of_rank_dir(rotate_file_right(dir.file_dir))
              : cardinal_of_file_dir(rotate_rank_right(dir.rank_dir));
  }
  return dir;
}

int cardinal_dir_to_index(cardinal_dir_t dir) {
  if (dir.is_file_dir) {
    return dir.file_dir.is_north ? 0 : 2;
  } else {
    return dir.rank_dir.is_east ? 1 : 3;
  }
}

char file_dir_to_char(file_dir_t dir, bool capitalize) {
  return capitalize ? (dir.is_north ? 'N' : 'S') : (dir.is_north ? 'n' : 's');
}

ordinal_dir_t make_ordinal(file_dir_t file_dir, rank_dir_t rank_dir) {
  return (ordinal_dir_t){.file_component = file_dir,
                         .rank_component = rank_dir};
}

bool ordinal_dir_eq(ordinal_dir_t a, ordinal_dir_t b) {
  return a.file_component.is_north == b.file_component.is_north &&
         a.rank_component.is_east == b.rank_component.is_east;
}

square_t take_ordinal_step(square_t sq, ordinal_dir_t step_dir) {
  return (square_t){.val = sq.val + get_file_offset(step_dir.file_component) +
                           get_rank_offset(step_dir.rank_component)};
}

ordinal_dir_t rotate_ordinal(ordinal_dir_t dir, rot_t rot) {
  for (int right_turns = 0; right_turns < (int)rot; right_turns++) {
    dir = make_ordinal(rotate_rank_right(dir.rank_component),
                       rotate_file_right(dir.file_component));
  }
  return dir;
}

int ordinal_dir_to_index(ordinal_dir_t dir) {
  return (dir.file_component.is_north ? 0 : 2) +
         (dir.rank_component.is_east ^ dir.file_component.is_north ? 0 : 1);
}

char rank_dir_to_char(rank_dir_t dir, bool capitalize) {
  return capitalize ? (dir.is_east ? 'E' : 'W') : (dir.is_east ? 'e' : 'w');
}

cardinal_dir_t reflect(cardinal_dir_t beam_dir, ordinal_dir_t pawn_ori) {
  if (beam_dir.is_file_dir) {
    if (beam_dir.file_dir.is_north == !pawn_ori.file_component.is_north) {
      return cardinal_of_rank_dir(pawn_ori.rank_component);
    }
  } else {
    if (beam_dir.rank_dir.is_east == !pawn_ori.rank_component.is_east) {
      return cardinal_of_file_dir(pawn_ori.file_component);
    }
  }
  return beam_dir;
}