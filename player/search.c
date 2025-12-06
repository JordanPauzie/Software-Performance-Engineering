// Copyright (c) 2022 MIT License by 6.172 / 6.106 Staff

/*
The search module contains the alpha-beta search utilities for Leiserchess,
consisting of three main functions:

  1. searchRoot() is called from outside the module.
  2. scout_search() performs a null-window search.
  3. searchPV() performs the normal alpha-beta search.

searchRoot() calls scout_search() to order the moves.  searchRoot() then
calls searchPV() to perform the full search.
*/

#include <ctype.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include "end_game.h"
#include "eval.h"
#include "fen.h"
#include "search.h"
#include "simple_mutex.h"
#include "tbassert.h"
#include "tt.h"
#include "util.h"

#if PARALLEL
#include <cilk/cilk.h>
#endif

// -----------------------------------------------------------------------------
// Preprocessor

// Note: parallel_parent_aborted is defined in search_scout.c which is included later

// Structure to pass arguments to parallel move search tasks
typedef struct {
  searchNode* node;
  move_t mv;
  int mv_index;
  killer_info_t killer_a;
  killer_info_t killer_b;
  simple_mutex_t* node_mutex;
  uint64_t* node_count_serial;
  volatile bool* found_cutoff;  // Volatile for atomic abort checking
} jamboree_move_args_t;

#if PARALLEL
// Forward declaration - implementation after includes
static void jamboree_search_move(jamboree_move_args_t* args);
#endif

// -----------------------------------------------------------------------------
// Preprocessor
// -----------------------------------------------------------------------------

#define ABORT_CHECK_PERIOD 0xfff

// -----------------------------------------------------------------------------
// READ ONLY settings (see iopt in leiserchess.c)
// -----------------------------------------------------------------------------

// eval score of a board state that is draw for both players.
int DRAW;

// POSITIONAL WEIGHTS evaluation terms
int HMB;  // having-the-move bonus

// Late-move reduction
int LMR_R1;  // Look at this number of moves full width before reducing 1 ply
int LMR_R2;  // After this number of moves, reduce 2 ply

int USE_NMM;       // Null move margin
int TRACE_MOVES;   // Print moves
int DETECT_DRAWS;  // Detect draws by repetition
int NMOVES_DRAW;   // Number of contiguous non-capture moves after which a draw
                   // is declared (1 move = 2 ply)

// do not set more than 5 ply
int FUT_DEPTH;  // set to zero for no futilty

// From search_scout.c
static score_t searchPV(searchNode* node, int depth,
                        uint64_t* node_count_serial);
static score_t scout_search(searchNode* node, int depth,
                            uint64_t* node_count_serial);

// From search_globals.c
static killer_info_t killer[];
static int best_move_history[];
static void update_transposition_table(searchNode* node);

static void update_best_move_history(position_t* p, int index_of_best,
                                     sortable_move_t* lst, int count);

// From search_common.c
static score_t get_draw_score(position_t* p, int ply);
static void getPV(move_t* pv, char* buf, size_t bufsize);
static void print_move_info(move_t mv, int ply, position_t* pos);
static leafEvalResult evaluate_as_leaf(searchNode* node, searchType_t type);
static moveEvaluationResult evaluateMove(searchNode* node, move_t mv,
                                         killer_info_t killer_a,
                                         killer_info_t killer_b,
                                         searchType_t type,
                                         uint64_t* node_count_serial);
static void sort_insertion(sortable_move_t* move_list, int num_of_moves);
static bool search_process_score(searchNode* node, move_t mv, int mv_index,
                                 moveEvaluationResult* result,
                                 searchType_t type);
static bool should_abort_check();
static int get_sortable_move_list(searchNode* node, sortable_move_t* move_list,
                                  move_t hash_table_move);

// Include common search functions
#include "./search_common.c"
#include "./search_globals.c"
#include "./search_scout.c"

#if PARALLEL
// Search a single move in parallel (for jamboree search)
// Implementation after includes so it can access all declared functions
static void jamboree_search_move(jamboree_move_args_t* args) {
  searchNode* node = args->node;
  move_t mv = args->mv;
  int mv_index = args->mv_index;
  killer_info_t killer_a = args->killer_a;
  killer_info_t killer_b = args->killer_b;
  simple_mutex_t* node_mutex = args->node_mutex;
  uint64_t* node_count_serial = args->node_count_serial;
  volatile bool* found_cutoff = args->found_cutoff;

  // Fast abort check: if parent found cutoff, abort immediately (lock-free)
  // This prevents wasted computation on subtrees that will be pruned
  if (*found_cutoff) {
    return;
  }

  // Propagate abort flag to child nodes for fast termination
  node->abort = false;  // Initialize, will be set if parent aborts

  // Increment node count atomically
  __sync_fetch_and_add(node_count_serial, 1);

  if (TRACE_MOVES) {
    print_move_info(mv, node->ply, &node->position);
  }

  // Evaluate the move (this may spawn recursive parallel searches)
  moveEvaluationResult result = evaluateMove(node, mv, killer_a, killer_b,
                                             SEARCH_PV, node_count_serial);

  if (result.type == MOVE_ILLEGAL || result.type == MOVE_IGNORE) {
    return;
  }

  // Check for abort after evaluation (another thread may have found cutoff)
  // Use memory barrier to ensure we see the latest value
  __sync_synchronize();
  if (*found_cutoff) {
    return;
  }

  // Acquire mutex to update shared state atomically
  simple_acquire(node_mutex);

  // Double-check abort after acquiring lock (critical section)
  if (*found_cutoff) {
    simple_release(node_mutex);
    return;
  }

  // A legal move, but when we are in quiescence
  // we only want to count moves that has a capture.
  if (result.type == MOVE_EVALUATED) {
    node->legal_move_count++;
  }

  // Process the score and check for beta cutoff
  // If cutoff found, signal all other parallel tasks to abort
  bool cutoff = search_process_score(node, mv, mv_index, &result, SEARCH_PV);
  
  if (cutoff) {
    // Signal abort to all parallel tasks (atomic write with memory barrier)
    __sync_synchronize();  // Ensure all previous writes are visible
    *found_cutoff = true;
    node->abort = true;  // Also set node abort for recursive abort propagation
    __sync_synchronize();  // Ensure abort flag is visible to other threads
  }

  simple_release(node_mutex);
}
#endif

// Initializes a PV (principal variation node)
// https://www.chessprogramming.org/Node_Types#PV-Nodes
static void initialize_pv_node(searchNode* node, int depth) {
  node->type = SEARCH_PV;
  node->alpha = -node->parent->beta;
  node->orig_alpha = node->alpha;  // Save original alpha.
  node->beta = -node->parent->alpha;
  node->subpv[0] = NULL_MOVE;
  node->depth = depth;
  node->legal_move_count = 0;
  node->ply = node->parent->ply + 1;
  node->fake_color_to_move = color_to_move_of(&(node->position));
  // point of view = 1 for White, -1 for Black
  node->pov = 1 - node->fake_color_to_move * 2;
  node->quiescence = (depth <= 0);
  node->best_move_index = 0;
  node->best_score = -INF;
  node->abort = false;
}

// Perform a Principal Variation Search
//
// https://www.chessprogramming.org/Principal_Variation_Search
static score_t searchPV(searchNode* node, int depth,
                        uint64_t* node_count_serial) {
  // Initialize the searchNode data structure.
  initialize_pv_node(node, depth);

  // Pre-evaluate the node to determine if we need to search further.
  leafEvalResult pre_evaluation_result = evaluate_as_leaf(node, SEARCH_PV);

  // use some information from the pre-evaluation
  move_t hash_table_move = pre_evaluation_result.hash_table_move;

  if (pre_evaluation_result.type == MOVE_EVALUATED) {
    return pre_evaluation_result.score;
  }
  if (pre_evaluation_result.score > node->best_score) {
    node->best_score = pre_evaluation_result.score;
    if (node->best_score > node->alpha) {
      node->alpha = node->best_score;
    }
  }

  // Get the killer moves at this node.
  killer_info_t killer_a = killer[KMT(node->ply, 0)];
  killer_info_t killer_b = killer[KMT(node->ply, 1)];

  // sortable_move_t move_list
  //
  // Contains a list of possible moves at this node. These moves are "sortable"
  //   and can be compared as integers. This is accomplished by using high-order
  //   bits to store a sort key. To get a better idea of what these sort keys
  //   mean, look at the get_sortable_move_list function in search_common.c.
  //
  // Keep track of the number of moves that we have considered at this node.
  //   After we finish searching moves at this node the move_list array will
  //   be organized in the following way:
  //
  //   m0, m1, ... , m_k-1, m_k, ... , m_N-1
  //
  //  where k = num_moves_tried, and N = num_of_moves
  //
  //  This will allow us to update the best_move_history table easily by
  //  scanning move_list from index 0 to k such that we update the table
  //  only for moves that we actually considered at this node.
  sortable_move_t move_list[MAX_NUM_MOVES];
  int num_of_moves = get_sortable_move_list(node, move_list, hash_table_move);
  int num_moves_tried = 0;

  sort_insertion(move_list, num_of_moves);
  
#if PARALLEL
  // YBW (Young Brothers Wait) parallelization with speculative parallelism
  // Parallelize alpha-beta search: search older brothers (moves after first) in parallel
  // Use smart conditions to avoid overhead on shallow searches
  const int MAX_PARALLEL_MOVES = 8;  // Limit parallel tasks to reduce overhead
  const int MIN_DEPTH_FOR_PARALLEL = 2;  // Lower threshold: parallelize at depth 2+
  const int MIN_MOVES_FOR_PARALLEL = 3;  // Lower threshold: need 3+ moves
  
  // Enable parallelization when conditions are met:
  // - Sufficient depth (amortize overhead) - lowered to 2 for more activation
  // - Enough moves (parallelize multiple moves) - lowered to 3
  // - Not in quiescence (quiescence is fast, overhead hurts)
  // - At reasonable ply (avoid deep recursion overhead) - increased to 30
  bool use_parallel = (node->depth >= MIN_DEPTH_FOR_PARALLEL &&
                       num_of_moves >= MIN_MOVES_FOR_PARALLEL &&
                       !node->quiescence &&
                       node->ply < 30);  // Increased recursion depth limit
  
  if (use_parallel) {
    // YBW: Initialize synchronization for parallel search
    // Use volatile bool for fast, lock-free abort checking
    simple_mutex_t node_mutex;
    init_simple_mutex(&node_mutex);
    volatile bool found_cutoff = false;  // Volatile for atomic reads/writes
    
    // YBW Step 1: Search first move (youngest brother) serially
    // This is usually the best move from hash table or move ordering
    if (num_of_moves > 0) {
      move_t mv = get_move(move_list[0]);
      num_moves_tried++;
      (*node_count_serial)++;

      if (TRACE_MOVES) {
        print_move_info(mv, node->ply, &node->position);
      }
      moveEvaluationResult result = evaluateMove(node, mv, killer_a, killer_b,
                                                 SEARCH_PV, node_count_serial);

      if (result.type != MOVE_ILLEGAL && result.type != MOVE_IGNORE) {
        // A legal move, but when we are in quiescence
        // we only want to count moves that has a capture.
        if (result.type == MOVE_EVALUATED) {
          node->legal_move_count++;
        }

        // Check if we should abort due to time control.
        if (abortf) {
          return 0;
        }

        bool cutoff = search_process_score(node, mv, 0, &result, SEARCH_PV);
        if (cutoff) {
          found_cutoff = true;
          node->abort = true;
        }
      }
    }
    
    // YBW Step 2: After first move completes, search remaining moves (older brothers) in parallel
    // Speculative parallelism: search moves that might be pruned by beta cutoff
    // Limit parallel tasks to avoid overhead, prioritize better moves (earlier in list)
    if (num_of_moves > 1 && !found_cutoff) {
      // Parallelize best moves first (better move ordering = less wasted work)
      // Only parallelize moves that are likely to be searched (not obviously bad)
      int max_parallel = (num_of_moves - 1 < MAX_PARALLEL_MOVES) ? 
                         (num_of_moves - 1) : MAX_PARALLEL_MOVES;
      
      for (int mv_index = 1; mv_index <= max_parallel && mv_index < num_of_moves; mv_index++) {
        // Fast abort check before spawning (lock-free read)
        if (should_abort_check() || found_cutoff) {
          break;
        }
        
        move_t mv = get_move(move_list[mv_index]);
        
        // Prepare arguments for parallel task
        jamboree_move_args_t args;
        args.node = node;
        args.mv = mv;
        args.mv_index = mv_index;
        args.killer_a = killer_a;
        args.killer_b = killer_b;
        args.node_mutex = &node_mutex;
        args.node_count_serial = node_count_serial;
        args.found_cutoff = (volatile bool*)&found_cutoff;  // Cast to match type
        
        // Spawn parallel task to search this move (older brother)
        // This is speculative - the move might be pruned by beta cutoff
        cilk_spawn jamboree_search_move(&args);
        
        num_moves_tried++;
      }
      
      // Wait for all spawned tasks to complete
      // Tasks will abort early if found_cutoff becomes true
      cilk_sync;
      
      // YBW Step 3: Search remaining moves serially (if any)
      // These are searched after parallel tasks complete, reducing contention
      for (int mv_index = MAX_PARALLEL_MOVES + 1; mv_index < num_of_moves; mv_index++) {
        if (should_abort_check() || found_cutoff) {
          break;
        }
        
        move_t mv = get_move(move_list[mv_index]);
        num_moves_tried++;
        (*node_count_serial)++;

        if (TRACE_MOVES) {
          print_move_info(mv, node->ply, &node->position);
        }
        moveEvaluationResult result = evaluateMove(node, mv, killer_a, killer_b,
                                                   SEARCH_PV, node_count_serial);

        if (result.type == MOVE_ILLEGAL || result.type == MOVE_IGNORE) {
          continue;
        }

        if (result.type == MOVE_EVALUATED) {
          node->legal_move_count++;
        }

        if (abortf) {
          return 0;
        }

        bool cutoff = search_process_score(node, mv, mv_index, &result, SEARCH_PV);
        if (cutoff) {
          break;
        }
      }
    } else if (num_of_moves > 1 && !found_cutoff) {
      // Not enough moves for parallelization, search remaining serially
      for (int mv_index = 1; mv_index < num_of_moves; mv_index++) {
        if (should_abort_check() || found_cutoff) {
          break;
        }
        
        move_t mv = get_move(move_list[mv_index]);
        num_moves_tried++;
        (*node_count_serial)++;

        if (TRACE_MOVES) {
          print_move_info(mv, node->ply, &node->position);
        }
        moveEvaluationResult result = evaluateMove(node, mv, killer_a, killer_b,
                                                   SEARCH_PV, node_count_serial);

        if (result.type == MOVE_ILLEGAL || result.type == MOVE_IGNORE) {
          continue;
        }

        if (result.type == MOVE_EVALUATED) {
          node->legal_move_count++;
        }

        if (abortf) {
          return 0;
        }

        bool cutoff = search_process_score(node, mv, mv_index, &result, SEARCH_PV);
        if (cutoff) {
          break;
        }
      }
    }
  }
  
  // Serial search fallback (when parallelization conditions not met)
  if (!use_parallel) {
    for (int mv_index = 0; mv_index < num_of_moves; mv_index++) {
      move_t mv = get_move(move_list[mv_index]);

      num_moves_tried++;
      (*node_count_serial)++;

      if (TRACE_MOVES) {
        print_move_info(mv, node->ply, &node->position);
      }
      moveEvaluationResult result = evaluateMove(node, mv, killer_a, killer_b,
                                                 SEARCH_PV, node_count_serial);

      if (result.type == MOVE_ILLEGAL || result.type == MOVE_IGNORE) {
        continue;
      }

      // A legal move, but when we are in quiescence
      // we only want to count moves that has a capture.
      if (result.type == MOVE_EVALUATED) {
        node->legal_move_count++;
      }

      // Check if we should abort due to time control.
      if (abortf) {
        return 0;
      }

      bool cutoff = search_process_score(node, mv, mv_index, &result, SEARCH_PV);
      if (cutoff) {
        break;
      }
    }
  }
#else
  // Serial search (when PARALLEL is not defined at compile time)
  for (int mv_index = 0; mv_index < num_of_moves; mv_index++) {
    move_t mv = get_move(move_list[mv_index]);

    num_moves_tried++;
    (*node_count_serial)++;

    if (TRACE_MOVES) {
      print_move_info(mv, node->ply, &node->position);
    }
    moveEvaluationResult result = evaluateMove(node, mv, killer_a, killer_b,
                                               SEARCH_PV, node_count_serial);

    if (result.type == MOVE_ILLEGAL || result.type == MOVE_IGNORE) {
      continue;
    }

    // A legal move, but when we are in quiescence
    // we only want to count moves that has a capture.
    if (result.type == MOVE_EVALUATED) {
      node->legal_move_count++;
    }

    // Check if we should abort due to time control.
    if (abortf) {
      return 0;
    }

    bool cutoff = search_process_score(node, mv, mv_index, &result, SEARCH_PV);
    if (cutoff) {
      break;
    }
  }
#endif

  if (node->quiescence == false) {
    update_best_move_history(&(node->position), node->best_move_index,
                             move_list, num_moves_tried);
  }

  tbassert(abs(node->best_score) != -INF, "best_score = %d\n",
           node->best_score);

  // Update the transposition table.
  //
  // Note: This function reads node->best_score, node->orig_alpha,
  //   node->position.board.zob_key, node->depth, node->ply, node->beta,
  //   node->alpha, node->subpv
  update_transposition_table(node);
  return node->best_score;
}

// -----------------------------------------------------------------------------
// searchRoot
//
// This handles scout search logic for the first level of the search tree
// -----------------------------------------------------------------------------
static void initialize_root_node(searchNode* node, score_t alpha, score_t beta,
                                 int depth, int ply, position_t* p) {
  node->type = SEARCH_ROOT;
  node->alpha = alpha;
  node->beta = beta;
  node->depth = depth;
  node->ply = ply;
  node->position = *p;
  node->fake_color_to_move = color_to_move_of(&(node->position));
  node->best_score = -INF;
  node->pov =
      1 - node->fake_color_to_move * 2;  // pov = 1 for White, -1 for Black
  node->abort = false;
}

score_t searchRoot(position_t* p, score_t alpha, score_t beta, int depth,
                   int ply, move_t* pv, uint64_t* node_count_serial,
                   FILE* OUT) {
  static int num_of_moves = 0;  // number of moves in list
  // hopefully, more than we will need
  static sortable_move_t move_list[MAX_NUM_MOVES];

  if (depth == 1) {
    // we are at depth 1; generate all possible moves
    num_of_moves = generate_all(p, move_list);
    // shuffle the list of moves
   #if DETERMINISTIC

   #else
    // shuffle the list of moves
    for (int i = 0; i < num_of_moves; i++) {
        int r = myrand() % num_of_moves;
        sortable_move_t tmp = move_list[i];
        move_list[i] = move_list[r];
        move_list[r] = tmp;
    }
    #endif

  }

  searchNode rootNode;
  rootNode.parent = NULL;
  initialize_root_node(&rootNode, alpha, beta, depth, ply, p);

  tbassert(rootNode.best_score == alpha, "root best score not equal alpha");

  searchNode next_node;
  next_node.subpv[0] = NULL_MOVE;
  next_node.parent = &rootNode;

  score_t score;

  for (int mv_index = 0; mv_index < num_of_moves; mv_index++) {
    move_t mv = get_move(move_list[mv_index]);

    if (TRACE_MOVES) {
      print_move_info(mv, ply, &rootNode.position);
    }

    (*node_count_serial)++;

    // make the move.
    execute_turn(&(rootNode.position), &(next_node.position), mv, false);
    victims_t x = next_node.position.victims;

    if (is_ILLEGAL(x)) {
      continue;  // not a legal move
    }

    if (is_end_game_position(&(next_node.position))) {
      score =
          get_end_game_score(&(next_node.position), rootNode.pov, rootNode.ply);
      next_node.subpv[0] = NULL_MOVE;
      goto scored;
    }

    if (is_draw(&(next_node.position))) {
      score = get_draw_score(&(next_node.position), rootNode.ply);
      next_node.subpv[0] = NULL_MOVE;
      goto scored;
    }

    if (mv_index == 0 || rootNode.depth == 1) {
      // We guess that the first move is the principal variation
      score = -searchPV(&next_node, rootNode.depth - 1, node_count_serial);

      // Check if we should abort due to time control.
      if (abortf) {
        return 0;
      }
    } else {
      score = -scout_search(&next_node, rootNode.depth - 1, node_count_serial);

      // Check if we should abort due to time control.
      if (abortf) {
        return 0;
      }

      // If its score exceeds the current best score,
      if (score > rootNode.alpha) {
        score = -searchPV(&next_node, rootNode.depth - 1, node_count_serial);
        // Check if we should abort due to time control.
        if (abortf) {
          return 0;
        }
      }
    }

  scored:
    // only valid for the root node:
    tbassert((score > rootNode.best_score) == (score > rootNode.alpha),
             "score = %d, best = %d, alpha = %d\n", score, rootNode.best_score,
             rootNode.alpha);
    if (score > rootNode.best_score) {
      tbassert(score > rootNode.alpha, "score: %d, alpha: %d\n", score,
               rootNode.alpha);

      rootNode.best_score = score;
      pv[0] = mv;
      memcpy(pv + 1, next_node.subpv, sizeof(move_t) * (MAX_PLY_IN_SEARCH - 1));
      pv[MAX_PLY_IN_SEARCH - 1] = NULL_MOVE;

      // Print out based on UCI (universal chess interface)
      double et = elapsed_time();
      char pvbuf[MAX_PLY_IN_SEARCH * MAX_CHARS_IN_MOVE];
      getPV(pv, pvbuf, MAX_PLY_IN_SEARCH * MAX_CHARS_IN_MOVE);
      if (et < 0.00001) {
        et = 0.00001;  // hack so that we don't divide by 0
      }

      uint64_t nps = 1000 * *node_count_serial / et;
      fprintf(OUT,
              "info depth %d move_no %d time (microsec) %d nodes %" PRIu64
              " nps %" PRIu64 "\n",
              depth, mv_index + 1, (int)(et * 1000), *node_count_serial, nps);
      fprintf(OUT, "info score cp %d pv %s\n", score, pvbuf);

      // Slide this move to the front of the move list
      for (int j = mv_index; j > 0; j--) {
        move_list[j] = move_list[j - 1];
      }
      move_list[0] = make_sortable(mv);
    }

    // Normal alpha-beta logic: if the current score is better than what the
    // maximizer has been able to get so far, take that new value.  Likewise,
    // score >= beta is the beta cutoff condition
    if (score > rootNode.alpha) {
      rootNode.alpha = score;
    }
    if (score >= rootNode.beta) {
      tbassert(0, "score: %d, beta: %d\n", score, rootNode.beta);
      break;
    }
  }

  return rootNode.best_score;
}