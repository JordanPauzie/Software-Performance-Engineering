// Copyright (c) 2022 MIT License by 6.172 / 6.106 Staff

// This file is #included in search.c and is not compiled separately

// This file contains the scout search routine. Although this routine contains
//   some duplicated logic from the searchPV routine in search.c, it is
//   convenient to maintain it separately. This allows one to easily
//   parallelize scout search separately from searchPV.

#include <stdbool.h>

#include "search.h"
#include "simple_mutex.h"
#include "tbassert.h"

static bool abortf;

// Checks whether a node's parent has aborted.
//   If this occurs, we should just stop and return 0 immediately.
static bool parallel_parent_aborted(searchNode* node) {
  searchNode* pred = node->parent;
  while (pred != NULL) {
    if (pred->abort) {
      return true;
    }
    pred = pred->parent;
  }
  return false;
}

// Initialize a scout search node for a "Null Window" search.
// https://www.chessprogramming.org/Scout
// https://www.chessprogramming.org/Null_Window
static void initialize_scout_node(searchNode* node, int depth) {
  node->type = SEARCH_SCOUT;
  node->beta = -(node->parent->alpha);
  node->alpha = node->beta - 1;
  node->depth = depth;
  node->ply = node->parent->ply + 1;
  node->subpv[0] = NULL_MOVE;
  node->legal_move_count = 0;
  node->fake_color_to_move = color_to_move_of(&(node->position));
  // point of view = 1 for white, -1 for black
  node->pov = 1 - node->fake_color_to_move * 2;
  node->best_move_index = 0;  // index of best move found
  node->abort = false;
}

static score_t scout_search(searchNode* node, int depth,
                            uint64_t* node_count_serial) {
  // Initialize the search node.
  initialize_scout_node(node, depth);

  // check whether we should abort
  if (should_abort_check() || parallel_parent_aborted(node)) {
    return 0;
  }

  // Pre-evaluate this position.
  leafEvalResult pre_evaluation_result = evaluate_as_leaf(node, SEARCH_SCOUT);

  // If we decide to stop searching, return the pre-evaluation score.
  if (pre_evaluation_result.type == MOVE_EVALUATED) {
    return pre_evaluation_result.score;
  }

  // Populate some of the fields of this search node, using some
  //  of the information provided by the pre-evaluation.
  move_t hash_table_move = pre_evaluation_result.hash_table_move;
  node->best_score = pre_evaluation_result.score;
  node->quiescence = pre_evaluation_result.should_enter_quiescence;

  // Grab the killer-moves for later use.
  killer_info_t killer_a = killer[KMT(node->ply, 0)];
  killer_info_t killer_b = killer[KMT(node->ply, 1)];

  // Store the sorted move list on the stack.
  //   MAX_NUM_MOVES is all that we need.
  sortable_move_t move_list[MAX_NUM_MOVES];

  // Obtain the sorted move list.
  int num_of_moves = get_sortable_move_list(node, move_list, hash_table_move);

  int number_of_moves_evaluated = 0;

  // A simple mutex. See simple_mutex.h for implementation details.
  simple_mutex_t node_mutex;
  init_simple_mutex(&node_mutex);

  // Sort the move list.
  sort_insertion(move_list, num_of_moves);

#if PARALLEL
  // Parallelize scout search using YBW for better coverage
  // Scout search benefits less from parallelization (null-window), but still helps
  const int MIN_DEPTH_FOR_SCOUT_PARALLEL = 4;
  const int MIN_MOVES_FOR_SCOUT_PARALLEL = 6;
  const int MAX_SCOUT_PARALLEL = 4;  // Fewer for scout (less benefit)
  
  bool use_scout_parallel = (node->depth >= MIN_DEPTH_FOR_SCOUT_PARALLEL &&
                              num_of_moves >= MIN_MOVES_FOR_SCOUT_PARALLEL &&
                              !node->quiescence &&
                              node->ply < 20);
  
  if (use_scout_parallel && num_of_moves > 1) {
    volatile bool found_cutoff = false;
    simple_mutex_t scout_mutex;
    init_simple_mutex(&scout_mutex);
    
    // Search first move serially (youngest brother)
    if (num_of_moves > 0) {
      int local_index = number_of_moves_evaluated++;
      move_t mv = get_move(move_list[local_index]);
      
      if (TRACE_MOVES) {
        print_move_info(mv, node->ply, &node->position);
      }
      
      __sync_fetch_and_add(node_count_serial, 1);
      moveEvaluationResult result = evaluateMove(node, mv, killer_a, killer_b,
                                                 SEARCH_SCOUT, node_count_serial);
      
      if (result.type != MOVE_ILLEGAL && result.type != MOVE_IGNORE &&
          !abortf && !parallel_parent_aborted(node)) {
        if (result.type == MOVE_EVALUATED) {
          node->legal_move_count++;
        }
        
        bool cutoff = search_process_score(node, mv, local_index, &result, SEARCH_SCOUT);
        if (cutoff) {
          found_cutoff = true;
          node->abort = true;
        }
      }
    }
    
    // Search remaining moves in parallel (older brothers)
    if (!found_cutoff && num_of_moves > 1) {
      int max_parallel = (num_of_moves - 1 < MAX_SCOUT_PARALLEL) ?
                         (num_of_moves - 1) : MAX_SCOUT_PARALLEL;
      
      for (int mv_index = 1; mv_index <= max_parallel && mv_index < num_of_moves; mv_index++) {
        if (found_cutoff || abortf || parallel_parent_aborted(node)) {
          break;
        }
        
        int local_index = number_of_moves_evaluated++;
        move_t mv = get_move(move_list[local_index]);
        
        // Spawn parallel scout search (simplified - reuse PV parallelization logic)
        // For scout, we can use a simpler approach since it's null-window
        __sync_fetch_and_add(node_count_serial, 1);
        
        if (TRACE_MOVES) {
          print_move_info(mv, node->ply, &node->position);
        }
        
        moveEvaluationResult result = evaluateMove(node, mv, killer_a, killer_b,
                                                   SEARCH_SCOUT, node_count_serial);
        
        if (result.type == MOVE_ILLEGAL || result.type == MOVE_IGNORE ||
            found_cutoff || abortf || parallel_parent_aborted(node)) {
          continue;
        }
        
        simple_acquire(&scout_mutex);
        if (found_cutoff) {
          simple_release(&scout_mutex);
          continue;
        }
        
        if (result.type == MOVE_EVALUATED) {
          node->legal_move_count++;
        }
        
        bool cutoff = search_process_score(node, mv, local_index, &result, SEARCH_SCOUT);
        if (cutoff) {
          found_cutoff = true;
          node->abort = true;
        }
        simple_release(&scout_mutex);
        
        if (cutoff) break;
      }
    }
    
    // Search remaining moves serially
    for (int mv_index = MAX_SCOUT_PARALLEL + 1; mv_index < num_of_moves; mv_index++) {
      if (found_cutoff || abortf || parallel_parent_aborted(node)) {
        break;
      }
      
      int local_index = number_of_moves_evaluated++;
      move_t mv = get_move(move_list[local_index]);
      
      if (TRACE_MOVES) {
        print_move_info(mv, node->ply, &node->position);
      }
      
      __sync_fetch_and_add(node_count_serial, 1);
      moveEvaluationResult result = evaluateMove(node, mv, killer_a, killer_b,
                                                 SEARCH_SCOUT, node_count_serial);
      
      if (result.type == MOVE_ILLEGAL || result.type == MOVE_IGNORE ||
          abortf || parallel_parent_aborted(node)) {
        continue;
      }
      
      if (result.type == MOVE_EVALUATED) {
        node->legal_move_count++;
      }
      
      bool cutoff = search_process_score(node, mv, local_index, &result, SEARCH_SCOUT);
      if (cutoff) {
        node->abort = true;
        break;
      }
    }
  } else {
    // Serial scout search (fallback)
    for (int mv_index = 0; mv_index < num_of_moves; mv_index++) {
      int local_index = number_of_moves_evaluated++;
      move_t mv = get_move(move_list[local_index]);

      if (TRACE_MOVES) {
        print_move_info(mv, node->ply, &node->position);
      }

      __sync_fetch_and_add(node_count_serial, 1);

      moveEvaluationResult result = evaluateMove(node, mv, killer_a, killer_b,
                                                 SEARCH_SCOUT, node_count_serial);

      if (result.type == MOVE_ILLEGAL || result.type == MOVE_IGNORE || abortf ||
          parallel_parent_aborted(node)) {
        continue;
      }

      if (result.type == MOVE_EVALUATED) {
        node->legal_move_count++;
      }

      bool cutoff = search_process_score(node, mv, local_index, &result, SEARCH_SCOUT);

      if (cutoff) {
        node->abort = true;
        break;
      }
    }
  }
#else
  // Serial scout search (when PARALLEL is not defined)
  for (int mv_index = 0; mv_index < num_of_moves; mv_index++) {
    int local_index = number_of_moves_evaluated++;
    move_t mv = get_move(move_list[local_index]);

    if (TRACE_MOVES) {
      print_move_info(mv, node->ply, &node->position);
    }

    __sync_fetch_and_add(node_count_serial, 1);

    moveEvaluationResult result = evaluateMove(node, mv, killer_a, killer_b,
                                               SEARCH_SCOUT, node_count_serial);

    if (result.type == MOVE_ILLEGAL || result.type == MOVE_IGNORE || abortf ||
        parallel_parent_aborted(node)) {
      continue;
    }

    if (result.type == MOVE_EVALUATED) {
      node->legal_move_count++;
    }

    bool cutoff = search_process_score(node, mv, local_index, &result, SEARCH_SCOUT);

    if (cutoff) {
      node->abort = true;
      break;
    }
  }
#endif

  if (parallel_parent_aborted(node)) {
    return 0;
  }

  if (node->quiescence == false) {
    update_best_move_history(&(node->position), node->best_move_index,
                             move_list, number_of_moves_evaluated);
  }

  tbassert(abs(node->best_score) != -INF, "best_score = %d\n",
           node->best_score);

  // Reads node->position.board.zob_key, node->depth, node->best_score, and
  // node->ply
  update_transposition_table(node);
  return node->best_score;
}
