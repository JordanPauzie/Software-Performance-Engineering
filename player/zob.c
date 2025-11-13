#include "zob.h"
// Zobrist hashing
//
// https://www.chessprogramming.org/Zobrist_Hashing
//
// NOTE: Zobrist hashing uses piece_t as an integer index into to the zob table.
// So if you change your piece representation, you'll need to recompute what the
// old piece representation is when indexing into the zob table to get the same
// node counts.
static uint64_t zob[ARR_SIZE][1 << PIECE_INDEX_BITS];
static uint64_t zob_color;
uint64_t myrand();
int piece_index(piece_t p) {
  int orientation = 0;
  if (p.typ == PAWN) {
    orientation = ordinal_dir_to_index(p.pawn_ori);
  } else if (p.typ == MONARCH) {
    orientation = cardinal_dir_to_index(p.monarch_ori);
  }
  return (((int)p.color) << 4) | (((int)p.typ) << 2) | orientation;
}

void init_zob() {
  for (int i = 0; i < ARR_SIZE; i++) {
    for (int j = 0; j < (1 << PIECE_INDEX_BITS); j++) {
      zob[i][j] = myrand();
    }
  }
  zob_color = myrand();
}

uint64_t swap_color_to_move(uint64_t zob_key) { return zob_key ^ zob_color; }

uint64_t toggle_piece(uint64_t zob_key, square_t sq, piece_t piece) {
  return zob_key ^ zob[square_to_index(sq)][piece_index(piece)];
}