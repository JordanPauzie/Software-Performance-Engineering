// Copyright (c) 2022 MIT License by 6.172 / 6.106 Staff

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include "end_game.h"
#include "eval.h"
#include "fen.h"
#include "move_gen.h"
#include "search.h"
#include "tbassert.h"
#include "util.h"
#include "zob.h"

// -----------------------------------------------------------------------------
// Piece getters and setters (including color, ptype, orientation)
// -----------------------------------------------------------------------------

// which color is moving next
color_t color_to_move_of(const position_t* p) {
  if ((p->ply & 1) == 0) {
    return WHITE;
  } else {
    return BLACK;
  }
}

bool move_eq(move_t a, move_t b) {
  return a.rot == b.rot && square_eq(a.from_sq, b.from_sq) &&
         square_eq(a.to_sq, b.to_sq);
}

sortable_move_t make_sortable(move_t mv) {
  sortable_move_t sortable;
  sortable.key = 0;
  sortable.mv = mv;
  return sortable;
}
// -----------------------------------------------------------------------------
// Move getters and setters
// -----------------------------------------------------------------------------

move_t move_of(rot_t rot, square_t from_sq, square_t to_sq) {
  move_t mv;
  mv.rot = rot;
  mv.from_sq = from_sq;
  mv.to_sq = to_sq;
  return mv;
}

// converts a move to string notation for FEN
void move_to_str(move_t mv, char* buf, size_t bufsize) {
  square_t f = mv.from_sq;  // from-square
  square_t t = mv.to_sq;    // to-square
  rot_t r = mv.rot;         // rotation
  const char* orig_buf = buf;

  buf += square_to_str(f, buf, bufsize);
  if (!square_eq(f, t)) {
    square_to_str(t, buf, bufsize - (buf - orig_buf));
  } else {
    switch (r) {
      case NO_ROT:
        square_to_str(t, buf, bufsize - (buf - orig_buf));
        break;
      case RIGHT:
        snprintf(buf, bufsize - (buf - orig_buf), "R");
        break;
      case UTURN:
        snprintf(buf, bufsize - (buf - orig_buf), "U");
        break;
      case LEFT:
        snprintf(buf, bufsize - (buf - orig_buf), "L");
        break;
      default:
        tbassert(false, "Whoa, now.  Whoa, I say.\n");  // Bad, bad, bad
        break;
    }
  }
}

// -----------------------------------------------------------------------------
// Move generation
// -----------------------------------------------------------------------------

// Returns the square of piece that would be zapped by the laser if fired once,
// or 0 if no such piece exists.
//
// p : Current board state.
// c : Color of monarch shooting laser.
square_t fire_laser(const position_t* p, color_t c) {
  laser_t laser = get_laser_origin(&p->board, c);
  while (!square_eq(laser.sq, INVALID_SQUARE) && !laser.is_hit) {
    step_laser(&p->board, &laser);
  }
  return laser.sq;
}

// Generate all moves from position p.  Returns number of moves.
//
// https://www.chessprogramming.org/Move_Generation
int generate_all(const position_t* p, sortable_move_t* sortable_move_list) {
  color_t color_to_move = color_to_move_of(p);

  int move_count = 0;

  for (coord_t fil = 0; fil < BOARD_WIDTH; fil++) {
    for (coord_t rnk = 0; rnk < BOARD_WIDTH; rnk++) {
      square_t sq = square_of(fil, rnk);
      piece_t piece = get_piece_at(&p->board, sq);

      ptype_t typ = piece.typ;
      color_t color = piece.color;

      if (typ == PAWN || typ == MONARCH) {
        if (color != color_to_move) {  // Wrong color
          continue;
        }

        // directions
        ordinal_dir_t valid_moves[] = {
            make_ordinal(NORTH, WEST), make_ordinal(NORTH, EAST),
            make_ordinal(SOUTH, EAST), make_ordinal(SOUTH, WEST)};
        for (int d = 0; d < (int)(sizeof(valid_moves) / sizeof(*valid_moves));
             d++) {
          ordinal_dir_t dir = valid_moves[d];
          square_t dest = take_ordinal_step(sq, dir);
          if (get_piece_at(&p->board, dest).typ != EMPTY) continue;

          VERBOSE_LOG_MOVE(1, "Before: %s ", move_of(NO_ROT, sq, dest));
          tbassert(move_count < MAX_NUM_MOVES, "move_count: %d\n", move_count);

          move_t mv = move_of(NO_ROT, sq, dest);
          sortable_move_list[move_count++] = make_sortable(mv);

          VERBOSE_LOG_MOVE(1, "After: %s\n",
                           get_move(sortable_move_list[move_count - 1]));
        }
        cardinal_dir_t cardinals[] = {
            cardinal_of_file_dir(NORTH), cardinal_of_file_dir(SOUTH),
            cardinal_of_rank_dir(EAST), cardinal_of_rank_dir(WEST)};
        for (int d = 0; d < (int)COUNT_OF(cardinals); d++) {
          cardinal_dir_t dir = cardinals[d];
          square_t victim_sq = take_cardinal_step(sq, dir);
          if (typ != PAWN)
            continue;  // only pawns can jump for these directions
          if (get_piece_at(&p->board, victim_sq).typ != PAWN ||
              get_piece_at(&p->board, victim_sq).color == color_to_move)
            continue;  // there must be a pawn in next square
          square_t dest =
              take_cardinal_step(victim_sq, dir);  // jump over next square
          bool blocked_jump = false;
          for (int next_dir = 0; next_dir < (int)COUNT_OF(cardinals);
               next_dir++) {
            square_t adjacent = take_cardinal_step(dest, cardinals[next_dir]);
            if (!square_eq(adjacent, victim_sq) &&
                get_piece_at(&p->board, adjacent).typ == PAWN &&
                get_piece_at(&p->board, adjacent).color != color_to_move) {
              blocked_jump = true;
              break;
            }
          }
          if (blocked_jump) {
            continue;
          }
          if (get_piece_at(&p->board, dest).typ != EMPTY) continue;
          move_t mv = move_of(NO_ROT, sq, dest);
          sortable_move_list[move_count++] = make_sortable(mv);
        }

        // For all non-360-degree rotations (i.e., not rot==0, which is NONE)
        for (int rot = 1; rot < NUM_ROT; ++rot) {
          tbassert(move_count < MAX_NUM_MOVES, "move_count: %d\n", move_count);
          move_t mv = move_of((rot_t)rot, sq, sq);
          sortable_move_list[move_count++] = make_sortable(mv);
        }
      }
    }
  }

  // null move
  if (!square_eq(fire_laser(p, color_to_move), INVALID_SQUARE)) {
    square_t monarch = get_monarch(&p->board, color_to_move);
    tbassert(get_piece_at(&p->board, monarch).typ == MONARCH,
             "Detected successful laser fire without monarch");
    move_t mv = move_of(NO_ROT, monarch, monarch);
    sortable_move_list[move_count++] = make_sortable(mv);
  }

  VERBOSE_LOG_MOVE_ARR(1, "\nGenerated moves: ", sortable_move_list,
                       move_count);
  return move_count;
}

int generate_all_with_color(position_t* p, sortable_move_t* sortable_move_list,
                            color_t color) {
  color_t color_to_move = color_to_move_of(p);
  if (color_to_move != color) {
    p->ply++;
  }

  int num_moves = generate_all(p, sortable_move_list);

  if (color_to_move != color) {
    p->ply--;
  }

  return num_moves;
}

// -----------------------------------------------------------------------------
// Move execution
// -----------------------------------------------------------------------------

void update_victim(position_t* p, square_t sq) {
  VERBOSE_LOG_POSITION(1, "Before:\n", p);
  check_valid_square(sq);

  piece_t piece = get_piece_at(&p->board, sq);
  // remove victim
  p->victims.removed[p->victims.count++] = piece;
  set_piece_at(&p->board, sq, NULL_PIECE);

  check_board_zob(&p->board);
  VERBOSE_LOG_POSITION(1, "After:\n", p);
}

void move_piece(position_t* old, position_t* out, move_t mv) {
  tbassert(!move_eq(mv, NULL_MOVE), "mv was zero.\n");
  VERBOSE_LOG_MOVE(1, "move_piece: %s\n", mv);
  check_board_zob(&old->board);
  VERBOSE_LOG_POSITION(1, "Before:\n", old);

  square_t from_sq = mv.from_sq;
  square_t to_sq = mv.to_sq;
  rot_t rot = mv.rot;

  VERBOSE_LOG(1, "move_piece 2:\n");
  VERBOSE_LOG_SQUARE(1, "from_sq: %s\n", from_sq);
  VERBOSE_LOG_SQUARE(1, "to_sq: %s\n", to_sq);
  VERBOSE_LOG_ROT(1, "rot: %s\n", rot);

  *out = *old;

  out->history = old;
  out->last_move = mv;
  out->victims.count = 0;

  check_valid_square(from_sq);
  check_valid_square(to_sq);

  toggle_color_to_move(&out->board);

  piece_t from_piece = get_piece_at(&out->board, from_sq);

  if (!square_eq(to_sq, from_sq)) {  // move, not rotation
    set_piece_at(&out->board, from_sq, NULL_PIECE);
    set_piece_at(&out->board, to_sq, from_piece);

    // if from_sq and to_sq are on the same row or column, it must have jumped
    if (fil_of(from_sq) == fil_of(to_sq) || rnk_of(from_sq) == rnk_of(to_sq)) {
      square_t mid_sq = get_midpoint(from_sq, to_sq);
      tbassert(get_piece_at(&out->board, mid_sq).typ == PAWN,
               "oh no. must jump over another pawn");
      update_victim(out, mid_sq);
    }
  } else {  // rotation
    // remove from_piece from from_sq in hash
    rotate_piece(&from_piece, rot);  // rotate from_piece
    set_piece_at(&out->board, from_sq,
                 from_piece);  // place rotated piece on board
  }

  // Increment ply
  out->ply++;

  check_board_zob(&out->board);
  VERBOSE_LOG_POSITION(1, "After:\n", out);
}

// returns victim pieces (does not mark position as actually played)
void execute_turn(position_t* old, position_t* out, move_t mv,
                  bool mark_as_played) {
  tbassert(!move_eq(mv, NULL_MOVE), "mv was zero.\n");

  // move phase 1 - moving a piece
  move_piece(old, out, mv);

  // move phase 2 - shooting the laser (s)
  square_t victim_sq = fire_laser(out, color_to_move_of(old));

  if (!square_eq(victim_sq, INVALID_SQUARE)) {
    VERBOSE_LOG_SQUARE(1, "Zapping piece on %s\n", victim_sq);
    // We definitely hit something with the laser.  Remove it from the board.
    update_victim(out, victim_sq);
  }

  if (out->victims.count == 0) {
    // increment ply counter since last victim
    out->nply_since_victim = old->nply_since_victim + 1;
  } else {
    // reset ply counter since last victim
    out->nply_since_victim = 0;
  }

  out->was_played = mark_as_played;
}

// -----------------------------------------------------------------------------
// Move path enumeration (perft)
// -----------------------------------------------------------------------------

// Helper function for do_perft() (for root call, use ply 0).
static uint64_t perft_search(position_t* p, int depth, int ply) {
  uint64_t node_count = 0;
  position_t np;
  sortable_move_t lst[MAX_NUM_MOVES];
  int num_moves;
  int i;

  if (depth == 0) {
    return 1;
  }

  num_moves = generate_all(p, lst);

  if (depth == 1) {
    return num_moves;
  }

  for (i = 0; i < num_moves; i++) {
    move_t mv = get_move(lst[i]);
    execute_turn(p, &np, mv, false);

    if (is_game_over(&np)) {
      // do not expand further: hit a Monarch
      node_count++;
      continue;
    }

    uint64_t partialcount = perft_search(&np, depth - 1, ply + 1);
    node_count += partialcount;
  }

  return node_count;
}

// Debugging function to perform a sanity check that the move generator is
// working correctly.  Not a thorough test, but quick.
//
// https://www.chessprogramming.org/Perft
void do_perft(position_t* gme, int depth) {
  for (int d = 0; d <= depth; d++) {
    printf("info perft %2d ", d);
    uint64_t j = perft_search(gme, d, 0);
    printf("%" PRIu64 "\n", j);
  }
}

// -----------------------------------------------------------------------------
// Position display
// -----------------------------------------------------------------------------

void display(const position_t* p) {
  char buf[MAX_CHARS_IN_MOVE];

  printf("info Ply: %d\n", p->ply);
  printf("info Color to move: %s\n", color_to_str(color_to_move_of(p)));

  square_to_str(get_monarch(&p->board, WHITE), buf, MAX_CHARS_IN_MOVE);
  printf("info White Monarch: %s\n", buf);
  square_to_str(get_monarch(&p->board, BLACK), buf, MAX_CHARS_IN_MOVE);
  printf("info Black Monarch: %s\n", buf);
  printf("info");
  for (coord_t rnk = BOARD_WIDTH - 1; rnk >= 0; --rnk) {
    printf("\ninfo %1d ", rnk);
    for (coord_t fil = 0; fil < BOARD_WIDTH; ++fil) {
      square_t sq = square_of(fil, rnk);
      piece_t piece = get_piece_at(&p->board, sq);

      if (piece.typ == EMPTY) {  // empty square
        printf(" --");
      } else if (piece.typ == MONARCH) {
        cardinal_dir_t dir = piece.monarch_ori;
        char direction_char =
            dir.is_file_dir
                ? file_dir_to_char(dir.file_dir, piece.color == WHITE)
                : rank_dir_to_char(dir.rank_dir, piece.color == WHITE);
        printf(" %c%c", direction_char, direction_char);
      } else if (piece.typ == PAWN) {
        ordinal_dir_t dir = piece.pawn_ori;
        printf(" %c%c",
               file_dir_to_char(dir.file_component, piece.color == WHITE),
               rank_dir_to_char(dir.rank_component, piece.color == WHITE));
      } else {
        tbassert(false, "found invalid piece on board\n");
      }
    }
  }

  printf("\ninfo    ");
  for (coord_t fil = 0; fil < BOARD_WIDTH; ++fil) {
    printf(" %c ", 'a' + fil);
  }
  printf("\n");
  printf("DoneDisplay\n");  // DO NOT DELETE THIS LINE!!! It is needed for the
                            // autotester.
  printf("\n");
}

// -----------------------------------------------------------------------------
// Illegal move signalling
// -----------------------------------------------------------------------------

victims_t ILLEGAL() {
  victims_t v;
  v.count = ILLEGAL_ZAPPED;
  return v;
}

bool is_ILLEGAL(victims_t victims) { return (victims.count == ILLEGAL_ZAPPED); }

bool zero_victims(victims_t victims) { return (victims.count == 0); }

bool victim_exists(victims_t victims) { return (victims.count > 0); }
