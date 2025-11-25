#include "board.h"

#include "tbassert.h"
#include "util.h"
#include "zob.h"

board_t init_board() {
  // temporarily initialize positions with no square found yet
  board_t result = (board_t){.color_to_move = WHITE, .zob_key = 0,
                              .white_monarch = INVALID_SQUARE,
                              .black_monarch = INVALID_SQUARE};
  for (int i = 0; i < ARR_SIZE; i++) {
    result.pieces[i] = NULL_PIECE;
    result.pieces[i].typ = INVALID;
  }
  for (coord_t fil = 0; fil < BOARD_WIDTH; fil++) {
    for (coord_t rnk = 0; rnk < BOARD_WIDTH; rnk++) {
      square_t sq = square_of(fil, rnk);
      result.pieces[sq.val].typ = EMPTY;
      result.zob_key =
          toggle_piece(result.zob_key, sq, result.pieces[square_to_index(sq)]);
    }
  }

  // find monarchs once at board initialization
  for (coord_t fil=0; fil < BOARD_WIDTH; fil++) {
    for (coord_t rnk=0; rnk < BOARD_WIDTH; rnk++) {
      square_t sq = square_of(fil, rnk);
      piece_t piece = get_piece_at(&result, sq);
      if (piece.typ == MONARCH) {
        if (piece.color == WHITE) {
          result.white_monarch = sq;
        } else {
          result.black_monarch = sq;
        }
      }
    }
  }
  return result;
}

void set_piece_at(board_t* board, square_t sq, piece_t piece) {
  int idx = sq.val;
  piece_t old_piece = board->pieces[idx];

  // update monarch cache if old piece is a monarch
  if (old_piece.typ == MONARCH) {
    if (old_piece.color == WHITE && square_eq(board->white_monarch, sq)) {
      board->white_monarch = INVALID_SQUARE;
    } else if (old_piece.color == BLACK && square_eq(board->black_monarch, sq)) {
      board->black_monarch = INVALID_SQUARE;
    }
  }

  // add new monarch to cache if incoming piece is monarch
  if (piece.typ == MONARCH) {
    if (piece.color == WHITE) {
      board->white_monarch = sq;
    } else {
      board->black_monarch = sq;
    }
  } else {
    // Invalidate monarch pos if different piece is being placed on monarch
    if (square_eq(board->white_monarch, sq)) {
      board->white_monarch = INVALID_SQUARE;
    }
    if (square_eq(board->black_monarch, sq)) {
      board->black_monarch = INVALID_SQUARE;
    }
  }

  board->zob_key = toggle_piece(
      toggle_piece(board->zob_key, sq, old_piece), sq, board->pieces[square_to_index(sq)]);
  board->pieces[idx] = piece;
}

void toggle_color_to_move(board_t* board) {
  board->color_to_move = opp_color(board->color_to_move);
  board->zob_key = swap_color_to_move(board->zob_key);
}

uint64_t recompute_zob(const board_t* board) {
  uint64_t recomputed_key = 0;
  for (coord_t fil = 0; fil < BOARD_WIDTH; fil++) {
    for (coord_t rnk = 0; rnk < BOARD_WIDTH; rnk++) {
      square_t sq = square_of(fil, rnk);
      recomputed_key =
          toggle_piece(recomputed_key, sq, get_piece_at(board, sq));
    }
  }
  if (board->color_to_move == BLACK) {
    recomputed_key = swap_color_to_move(recomputed_key);
  }
  return recomputed_key;
}

void check_board_zob(const board_t* board) {
  tbassert(board->zob_key == recompute_zob(board),
           "board->zob_key: %" PRIu64 ", zob-key: %" PRIu64 "\n",
           board->zob_key, recompute_zob(board));
  MARK_USED(board);
}

square_t get_monarch(const board_t* board, color_t c) {
  // Use cached monarch position
  square_t cached = (c == WHITE) ? board->white_monarch : board->black_monarch;
  
  // Validate cache
  if (!square_eq(cached, INVALID_SQUARE)) {
    piece_t piece = get_piece_at(board, cached);
    if (piece.typ == MONARCH && piece.color == c) {
      return cached;
    }
    // Cache is invalid - fall through to scan and repair
  }
  
  // fallback ie original scanning the board
  for (coord_t fil = 0; fil < BOARD_WIDTH; fil++) {
    for (coord_t rnk = 0; rnk < BOARD_WIDTH; rnk++) {
      square_t sq = square_of(fil, rnk);
      piece_t piece = get_piece_at(board, sq);
      if (piece.typ == MONARCH && piece.color == c) {
        // Repair cache
        if (c == WHITE) {
          ((board_t*)board)->white_monarch = sq;
        } else {
          ((board_t*)board)->black_monarch = sq;
        }
        return sq;
      }
    }
  }
  return INVALID_SQUARE;
}

void rotate_piece(piece_t* p, rot_t rot) {
  if (p->typ == PAWN) {
    p->pawn_ori = rotate_ordinal(p->pawn_ori, rot);
  } else {
    p->monarch_ori = rotate_cardinal(p->monarch_ori, rot);
  }
}

laser_t get_laser_origin(const board_t* board, color_t c) {
  square_t sq = get_monarch(board, c);
  piece_t piece = get_piece_at(board, sq);
  if (piece.typ == EMPTY || piece.typ == INVALID) {
    return (laser_t){.sq = INVALID_SQUARE};
  }
  tbassert(piece.typ == MONARCH, "ptype: %d\n", piece.typ);
  return (laser_t){.sq = sq, .beam_dir = piece.monarch_ori, .is_hit = false};
}

void step_laser(const board_t* board, laser_t* laser) {
  if (laser->is_hit || square_eq(laser->sq, INVALID_SQUARE)) return;
  laser->sq = take_cardinal_step(laser->sq, laser->beam_dir);
  piece_t piece = get_piece_at(board, laser->sq);
  switch (piece.typ) {
    case EMPTY:
      break;
    case PAWN: {
      cardinal_dir_t inverted_beam = rotate_cardinal(laser->beam_dir, UTURN);
      cardinal_dir_t mirror_rank_dir =
          cardinal_of_rank_dir(piece.pawn_ori.rank_component);
      cardinal_dir_t mirror_file_dir =
          cardinal_of_file_dir(piece.pawn_ori.file_component);
      if (cardinal_dir_eq(inverted_beam, mirror_rank_dir)) {
        laser->beam_dir = mirror_file_dir;
      } else if (cardinal_dir_eq(inverted_beam, mirror_file_dir)) {
        laser->beam_dir = mirror_rank_dir;
      } else {
        laser->is_hit = true;
      }
      break;
    }
    case MONARCH:
      laser->is_hit = true;
      break;
    case INVALID:  // Ran off edge of board
      laser->sq = INVALID_SQUARE;
      break;
    default:  // Shouldn't happen, man!
      tbassert(false, "Like porkchops and whipped cream.\n");
      break;
  }
}

static const char* color_strs[3] = {"White", "Black"};
const char* color_to_str(color_t c) { return color_strs[c]; }

color_t opp_color(color_t c) {
  if (c == WHITE) {
    return BLACK;
  } else {
    return WHITE;
  }
}