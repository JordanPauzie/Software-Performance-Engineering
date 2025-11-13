#pragma once
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>

// The current representation of a square is just the index into the padded
// board representation
typedef struct {
  int val;
} square_t;

typedef int coord_t;

// square_t may represent an invalid board coordinate. For example, if a
// function that finds the position of a monarch finds that there is no monarch
#define INVALID_SQUARE ((square_t){.val = 0})

// Directions on the board are represented as a composition of directions that
// run along files (called NORTH and SOUTH) and directions that run along ranks
// (called EAST and WEST). The cardinal directions are parallel to the sides of
// the board (e.g. monarch orientation) while the ordinal directions are at a 45
// degree angle (e.g. pawn orientation).
typedef struct {
  bool is_north;
} file_dir_t;

typedef struct {
  bool is_east;
} rank_dir_t;

typedef struct {
  bool is_file_dir;
  union {
    file_dir_t file_dir;
    rank_dir_t rank_dir;
  };
} cardinal_dir_t;

typedef struct {
  file_dir_t file_component;
  rank_dir_t rank_component;
} ordinal_dir_t;

#define NORTH ((file_dir_t){.is_north = true})
#define SOUTH ((file_dir_t){.is_north = false})
#define EAST ((rank_dir_t){.is_east = true})
#define WEST ((rank_dir_t){.is_east = false})

#define NUM_DIR 4

typedef enum { NO_ROT, RIGHT, UTURN, LEFT, NUM_ROT } rot_t;

square_t square_of(coord_t fil, coord_t rnk);
coord_t fil_of(square_t sq);
coord_t rnk_of(square_t sq);
bool square_eq(square_t a, square_t b);
int square_to_str(square_t sq, char* buf, size_t bufsize);
int square_to_index(square_t sq);
square_t get_midpoint(square_t a, square_t b);
void check_square(square_t sq);
void check_valid_square(square_t sq);

cardinal_dir_t cardinal_of_file_dir(file_dir_t dir);
cardinal_dir_t cardinal_of_rank_dir(rank_dir_t dir);
bool cardinal_dir_eq(cardinal_dir_t a, cardinal_dir_t b);
square_t take_cardinal_step(square_t sq, cardinal_dir_t step_dir);
cardinal_dir_t rotate_cardinal(cardinal_dir_t dir, rot_t rot);
int cardinal_dir_to_index(cardinal_dir_t dir);
char file_dir_to_char(file_dir_t dir, bool capitalize);

ordinal_dir_t make_ordinal(file_dir_t file_dir, rank_dir_t rank_dir);
bool ordinal_dir_eq(ordinal_dir_t a, ordinal_dir_t b);
square_t take_ordinal_step(square_t sq, ordinal_dir_t step_dir);
ordinal_dir_t rotate_ordinal(ordinal_dir_t dir, rot_t rot);
int ordinal_dir_to_index(ordinal_dir_t dir);
char rank_dir_to_char(rank_dir_t dir, bool capitalize);
