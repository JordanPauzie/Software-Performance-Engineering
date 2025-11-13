// Copyright (c) 2022 MIT License by 6.172 / 6.106 Staff

// This file is #included in search.c and is not compiled separately

#include "search.h"

// Killer move table
//
// https://www.chessprogramming.org/Killer_Move
// https://www.chessprogramming.org/Killer_Heuristic
//
// FORMAT: killer[ply][id]
static killer_info_t killer[KMT_dim];  // up to 4 killers

killer_info_t killer_info_of(move_t move, ptype_t typ) {
  return (killer_info_t){.move = move, .typ = typ};
}

bool killer_info_eq(killer_info_t a, killer_info_t b) {
  return move_eq(a.move, b.move) && a.typ == b.typ;
}

// Best move history table and lookup function
//
// https://www.chessprogramming.org/History_Heuristic
//
// FORMAT: best_move_history[color_t][piece_t][square_t][orientation]

static int best_move_history[BMH_dim];

void init_best_move_history() {
  memset(best_move_history, 0, sizeof(best_move_history));
}

static void update_best_move_history(position_t* p, int index_of_best,
                                     sortable_move_t* lst, int count) {
  tbassert(ENABLE_TABLES, "Tables weren't enabled.\n");

  int color_to_move = color_to_move_of(p);

  for (int i = 0; i < count; i++) {
    move_t mv = get_move(lst[i]);
    rot_t ro = mv.rot;
    square_t fs = mv.from_sq;
    square_t ts = mv.to_sq;
    piece_t piece = get_piece_at(&p->board, fs);
    ptype_t pce = piece.typ;
    rotate_piece(&piece, ro);
    int ot = pce == PAWN ? ordinal_dir_to_index(piece.pawn_ori)
                         : cardinal_dir_to_index(piece.monarch_ori);
    int s = best_move_history[BMH(color_to_move, pce, ts, ot)];

    if (index_of_best == i) {
      s = s + 11200;  // number will never exceed 1017
    }
    s = s * 0.90;  // decay score over time

    tbassert(s < 102000, "s = %d\n", s);  // or else sorting will fail

    best_move_history[BMH(color_to_move, pce, ts, ot)] = s;
  }
}

static void update_transposition_table(searchNode* node) {
  if (node->type == SEARCH_SCOUT) {
    if (node->best_score < node->beta) {
      tt_hashtable_put(
          node->position.board.zob_key, node->depth,
          tt_adjust_score_for_hashtable(node->best_score, node->ply), UPPER,
          NULL_MOVE);
    } else {
      tt_hashtable_put(
          node->position.board.zob_key, node->depth,
          tt_adjust_score_for_hashtable(node->best_score, node->ply), LOWER,
          node->subpv[0]);
    }
  } else if (node->type == SEARCH_PV) {
    if (node->best_score <= node->orig_alpha) {
      tt_hashtable_put(
          node->position.board.zob_key, node->depth,
          tt_adjust_score_for_hashtable(node->best_score, node->ply), UPPER,
          NULL_MOVE);
    } else if (node->best_score >= node->beta) {
      tt_hashtable_put(
          node->position.board.zob_key, node->depth,
          tt_adjust_score_for_hashtable(node->best_score, node->ply), LOWER,
          node->subpv[0]);
    } else {
      tt_hashtable_put(
          node->position.board.zob_key, node->depth,
          tt_adjust_score_for_hashtable(node->best_score, node->ply), EXACT,
          node->subpv[0]);
    }
  }
}
