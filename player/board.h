#pragma once

#include "coords.h"

// The board is centered in a ARR_WIDTH x ARR_WIDTH array, with the
// (ample) excess height and width being used for sentinels.
#define ARR_WIDTH 16
#define ARR_SIZE (ARR_WIDTH * ARR_WIDTH)

// Board is BOARD_WIDTH x BOARD_WIDTH
#define BOARD_WIDTH 8
#define FIL_ORIGIN ((ARR_WIDTH - BOARD_WIDTH) / 2)
#define RNK_ORIGIN ((ARR_WIDTH - BOARD_WIDTH) / 2)

// -----------------------------------------------------------------------------
// Pieces
// -----------------------------------------------------------------------------

#define MAX_PAWNS (BOARD_WIDTH * BOARD_WIDTH)  // maximum number of Pawns total
#define MAX_MONARCHS 1  // maximum number of Monarchs per color

#define PIECE_INDEX_BITS 6  // Bitwidth of index returned by piece_index()

typedef enum { EMPTY, PAWN, MONARCH, INVALID } ptype_t;
typedef enum { WHITE = 0, BLACK = 1 } color_t;

typedef struct {
  color_t color;
  ptype_t typ;
  union {
    ordinal_dir_t pawn_ori;
    cardinal_dir_t monarch_ori;
  };
} piece_t;

#define NULL_PIECE ((piece_t){0})

// Board representation is square-centric with sentinels.
//
// https://www.chessprogramming.org/Board_Representation
// https://www.chessprogramming.org/Mailbox
// https://www.chessprogramming.org/10x12_Board

typedef struct {
  piece_t pieces[ARR_SIZE];
  color_t color_to_move;
  uint64_t zob_key;
} board_t;

typedef struct {
  square_t sq;
  cardinal_dir_t beam_dir;
  bool is_hit;
} laser_t;

board_t init_board();
piece_t get_piece_at(const board_t* board, square_t sq);
void set_piece_at(board_t* board, square_t sq, piece_t piece);
void toggle_color_to_move(board_t* board);
void check_board_zob(const board_t* board);
square_t get_monarch(const board_t* board, color_t c);

void rotate_piece(piece_t* p, rot_t rot);

laser_t get_laser_origin(const board_t* board, color_t c);
void step_laser(const board_t* board, laser_t* laser);

const char* color_to_str(color_t c);
color_t opp_color(color_t c);