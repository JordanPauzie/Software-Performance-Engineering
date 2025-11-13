// Copyright (c) 2022 MIT License by 6.172 / 6.106 Staff

#include "end_game.h"

#include <stdlib.h>

#include "fen.h"
#include "tbassert.h"

bool player_wins(const position_t* p, color_t c) {
  for (coord_t fil = 0; fil < BOARD_WIDTH; fil++) {
    for (coord_t rnk = 0; rnk < BOARD_WIDTH; rnk++) {
      piece_t piece = get_piece_at(&p->board, square_of(fil, rnk));
      if (piece.color != c && piece.typ == MONARCH) {
        return false;
      }
    }
  }
  return true;
}

// Goal is to start your turn with more monarchs that your opponent.
// So, if at the start of their turn, either player has more monarchs,
// the game is over.
// Additionally, the game ends when either player has 0 monarchs.
bool is_game_over(const position_t* p) {
  return player_wins(p, WHITE) || player_wins(p, BLACK);
}

// check the victim pieces returned by the move to determine if it's a
// game-over situation.  If so, also calculate the score depending on
// the pov (which player's point of view)
static score_t get_game_over_score(const position_t* p, int pov, int ply) {
  score_t score = 0;
  if (player_wins(p, BLACK)) {
    score = -WIN * pov;
  } else if (player_wins(p, WHITE)) {
    score = WIN * pov;
  } else {
    tbassert(false, "Should not happen.\n");
  }
  if (score < 0) {
    score += ply;
  } else {
    score -= ply;
  }
  return score;
}

// Here's a great spot to add in a closing book/end game table if you choose to
// implement one.

// In this function, determine if the position is in the end game table or not.
bool is_end_game_position(const position_t* p) { return is_game_over(p); }

// In this function, read the end game table and return the score of this
// position, accounting for who wins and how many moves away this end game
// position is.
//
// The score of an end-game position is dependent on:
//   - Whose point of view the search is occurring from
//   - How many layers deep this position occurs in the search
//   - How many more moves it will take the game to end once it reaches this
//   position.
score_t get_end_game_score(const position_t* p, int pov, int ply) {
  // Note: pov and ply are for the move that *generated* this position, not the
  // position itself.
  //       This means you might have an off-by-one error when reading from your
  //       closing book.
  return get_game_over_score(p, pov, ply);
}
