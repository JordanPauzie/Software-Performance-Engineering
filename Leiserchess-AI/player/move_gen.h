// Copyright (c) 2022 MIT License by 6.172 / 6.106 Staff

#ifndef MOVE_GEN_H
#define MOVE_GEN_H

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>

#include "board.h"

// The MAX_NUM_MOVES is just an estimate
#define MAX_NUM_MOVES 256      // real number = 8 * (11) + 1 = 89
#define MAX_PLY_IN_SEARCH 100  // up to 100 ply
#define MAX_PLY_IN_GAME 4096   // long game!  ;^)

// Used for debugging and display
#define MAX_CHARS_IN_MOVE 16  // Could be less
#define MAX_CHARS_IN_TOKEN 64

// -----------------------------------------------------------------------------
// Board
// -----------------------------------------------------------------------------

typedef struct {
  ptype_t typ;
  rot_t rot;
  square_t from_sq;
  square_t to_sq;
} move_t;

#define NULL_MOVE ((move_t){0})

typedef uint32_t sort_key_t;
typedef struct {
  sort_key_t key;
  move_t mv;
} sortable_move_t;
#define MAX_ZAPPED MAX_PAWNS
typedef struct victims_t {
  int count;
  piece_t removed[MAX_ZAPPED];
} victims_t;

// returned by make move in illegal situation
#define ILLEGAL_ZAPPED -1

// -----------------------------------------------------------------------------
// Position
// -----------------------------------------------------------------------------

typedef struct position {
  board_t board;
  struct position* history;  // history of position
  int ply;                // ply since beginning of game (even=White, odd=Black)
  int nply_since_victim;  // number of ply since last capture
  move_t last_move;       // move that led to this position
  victims_t victims;      // pieces destroyed by shooter
  bool was_played;        // TRUE: position was actually played;
                          // FALSE: position is being considered in search
} position_t;

// -----------------------------------------------------------------------------
// Function prototypes
// -----------------------------------------------------------------------------

color_t color_to_move_of(const position_t* p);

move_t move_of(rot_t rot, square_t from_sq, square_t to_sq);
void move_to_str(move_t mv, char* buf, size_t bufsize);
bool move_eq(move_t a, move_t b);
sortable_move_t make_sortable(move_t mv);

int generate_all(const position_t* p, sortable_move_t* sortable_move_list);
int generate_all_with_color(position_t* p, sortable_move_t* sortable_move_list,
                            color_t color_to_move);

void do_perft(position_t* gme, int depth);
void move_piece(position_t* old, position_t* out, move_t mv);
void execute_turn(position_t* old, position_t* out, move_t mv,
                  bool mark_as_played);

void display(const position_t* p);

victims_t ILLEGAL();

bool is_ILLEGAL(victims_t victims);
bool zero_victims(victims_t victims);
bool victim_exists(victims_t victims);

void update_victim(position_t* p, square_t victim_sq);

#endif  // MOVE_GEN_H
