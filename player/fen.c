// Copyright (c) 2022 MIT License by 6.172 / 6.106 Staff

#include "./fen.h"

#include <stdbool.h>
#include <stdio.h>

#include "tbassert.h"
#include "zob.h"

const char* STARTPOS_FEN =
    "1nn6/8/1sw1sw1sw1sw/se1se1se1se1/NE1NE1NE1NE1/1NW1NW1NW1NW/8/3SS4 W";
const char* ENDGAME_FEN = "ss7/8/8/8/8/8/8/7NN W";

static void fen_error(const char* fen, int c_count, const char* msg) {
  fprintf(stderr, "\nError in FEN string:\n");
  fprintf(stderr, "   %s\n  ", fen);  // Indent 3 spaces
  for (int i = 0; i < c_count; ++i) {
    fprintf(stderr, " ");
  }
  fprintf(stderr, "^\n");  // graphical pointer to error character in string
  fprintf(stderr, "%s\n", msg);
}

// parse_fen_board
// Input:   board representation as a fen string
//          unpopulated board position struct
// Output:  index of where board description ends or 0 if parsing error
//          (populated) board position struct
static int parse_fen_board(position_t* p, const char* fen) {
  // Invariant: square (f, r) is last square filled.
  // Fill from last rank to first rank, from first file to last file
  coord_t fil = -1;
  coord_t rnk = BOARD_WIDTH - 1;

  // Current and next characters from input FEN description
  char c, next_c;

  // Invariant: fen[c_count] is next character to be read
  int c_count = 0;

  // Loop also breaks internally if (f, r) == (BOARD_WIDTH-1, 0)
  while ((c = fen[c_count++]) != '\0') {
    piece_t piece;

    switch (c) {
      // ignore whitespace until the end
      case ' ':
      case '\t':
      case '\n':
      case '\r':
        if ((fil == BOARD_WIDTH - 1) && (rnk == 0)) {  // our job is done
          return c_count;
        }
        break;

      // digits
      case '1':
        if (fen[c_count] == '0') {
          c_count++;
          c += 9;
        }
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        while (c > '0') {
          if (++fil >= BOARD_WIDTH) {
            fen_error(fen, c_count, "Too many squares in rank.\n");
            return 0;
          }
          set_piece_at(&p->board, square_of(fil, rnk), (piece_t){.typ = EMPTY});
          c--;
        }
        break;

      // pieces
      case 'N':
        if (++fil >= BOARD_WIDTH) {
          fen_error(fen, c_count, "Too many squares in rank");
          return 0;
        }
        next_c = fen[c_count++];
        piece.color = WHITE;

        if (next_c == 'N') {  // White Monarch facing North
          piece.monarch_ori = cardinal_of_file_dir(NORTH);
          piece.typ = MONARCH;
        } else if (next_c == 'W') {  // White Pawn facing NW
          piece.pawn_ori = make_ordinal(NORTH, WEST);
          piece.typ = PAWN;
        } else if (next_c == 'E') {  // White Pawn facing NE
          piece.pawn_ori = make_ordinal(NORTH, EAST);
          piece.typ = PAWN;
        } else {
          fen_error(fen, c_count + 1, "Syntax error");
          return 0;
        }
        set_piece_at(&p->board, square_of(fil, rnk), piece);
        break;

      case 'n':
        if (++fil >= BOARD_WIDTH) {
          fen_error(fen, c_count, "Too many squares in rank");
          return 0;
        }
        next_c = fen[c_count++];
        piece.color = BLACK;

        if (next_c == 'n') {  // Black Monarch facing North
          piece.monarch_ori = cardinal_of_file_dir(NORTH);
          piece.typ = MONARCH;
        } else if (next_c == 'w') {  // Black Pawn facing NW
          piece.pawn_ori = make_ordinal(NORTH, WEST);
          piece.typ = PAWN;
        } else if (next_c == 'e') {  // Black Pawn facing NE
          piece.pawn_ori = make_ordinal(NORTH, EAST);
          piece.typ = PAWN;
        } else {
          fen_error(fen, c_count + 1, "Syntax error");
          return 0;
        }
        set_piece_at(&p->board, square_of(fil, rnk), piece);
        break;

      case 'S':
        if (++fil >= BOARD_WIDTH) {
          fen_error(fen, c_count, "Too many squares in rank");
          return 0;
        }
        next_c = fen[c_count++];
        piece.color = WHITE;

        if (next_c == 'S') {  // White Monarch facing SOUTH
          piece.monarch_ori = cardinal_of_file_dir(SOUTH);
          piece.typ = MONARCH;
        } else if (next_c == 'W') {  // White Pawn facing SW
          piece.pawn_ori = make_ordinal(SOUTH, WEST);
          piece.typ = PAWN;
        } else if (next_c == 'E') {  // White Pawn facing SE
          piece.pawn_ori = make_ordinal(SOUTH, EAST);
          piece.typ = PAWN;
        } else {
          fen_error(fen, c_count + 1, "Syntax error");
          return 0;
        }
        set_piece_at(&p->board, square_of(fil, rnk), piece);
        break;

      case 's':
        if (++fil >= BOARD_WIDTH) {
          fen_error(fen, c_count, "Too many squares in rank");
          return 0;
        }
        next_c = fen[c_count++];
        piece.color = BLACK;

        if (next_c == 's') {  // Black Monarch facing South
          piece.monarch_ori = cardinal_of_file_dir(SOUTH);
          piece.typ = MONARCH;
        } else if (next_c == 'w') {  // Black Pawn facing SW
          piece.pawn_ori = make_ordinal(SOUTH, WEST);
          piece.typ = PAWN;
        } else if (next_c == 'e') {  // Black Pawn facing SE
          piece.pawn_ori = make_ordinal(SOUTH, EAST);
          piece.typ = PAWN;
        } else {
          fen_error(fen, c_count + 1, "Syntax error");
          return 0;
        }
        set_piece_at(&p->board, square_of(fil, rnk), piece);
        break;

      case 'E':
        if (++fil >= BOARD_WIDTH) {
          fen_error(fen, c_count, "Too many squares in rank");
          return 0;
        }
        next_c = fen[c_count++];

        if (next_c == 'E') {  // White Monarch facing East
          piece.typ = MONARCH;
          piece.color = WHITE;
          piece.monarch_ori = cardinal_of_rank_dir(EAST);
        } else {
          fen_error(fen, c_count + 1, "Syntax error");
          return 0;
        }
        set_piece_at(&p->board, square_of(fil, rnk), piece);
        break;

      case 'W':
        if (++fil >= BOARD_WIDTH) {
          fen_error(fen, c_count, "Too many squares in rank");
          return 0;
        }
        next_c = fen[c_count++];

        if (next_c == 'W') {  // White Monarch facing West
          piece.typ = MONARCH;
          piece.color = WHITE;
          piece.monarch_ori = cardinal_of_rank_dir(WEST);
        } else {
          fen_error(fen, c_count + 1, "Syntax error");
          return 0;
        }
        set_piece_at(&p->board, square_of(fil, rnk), piece);
        break;

      case 'e':
        if (++fil >= BOARD_WIDTH) {
          fen_error(fen, c_count, "Too many squares in rank");
          return 0;
        }
        next_c = fen[c_count++];
        if (next_c == 'e') {  // Black Monarch facing East
          piece.typ = MONARCH;
          piece.color = BLACK;
          piece.monarch_ori = cardinal_of_rank_dir(EAST);
        } else {
          fen_error(fen, c_count + 1, "Syntax error");
          return 0;
        }
        set_piece_at(&p->board, square_of(fil, rnk), piece);
        break;

      case 'w':
        if (++fil >= BOARD_WIDTH) {
          fen_error(fen, c_count, "Too many squares in rank");
          return 0;
        }
        next_c = fen[c_count++];
        if (next_c == 'w') {  // Black Monarch facing West
          piece.typ = MONARCH;
          piece.color = BLACK;
          piece.monarch_ori = cardinal_of_rank_dir(WEST);
        } else {
          fen_error(fen, c_count + 1, "Syntax error");
          return 0;
        }
        set_piece_at(&p->board, square_of(fil, rnk), piece);
        break;

      // end of rank
      case '/':
        if (fil == BOARD_WIDTH - 1) {
          fil = -1;
          if (--rnk < 0) {
            fen_error(fen, c_count, "Too many ranks");
            return 0;
          }
        } else {
          fen_error(fen, c_count, "Too few squares in rank");
          return 0;
        }
        break;

      default:
        fen_error(fen, c_count, "Syntax error");
        return 0;
        break;
    }  // end switch
  }  // end while

  if ((fil == BOARD_WIDTH - 1) && (rnk == 0)) {
    return c_count;
  } else {
    fen_error(fen, c_count, "Too few squares specified");
    return 0;
  }
}

// returns 0 if no error
static int get_sq_from_str(const char* fen, int* c_count, square_t* sq) {
  char c, next_c;

  while ((c = fen[(*c_count)++]) != '\0') {
    // skip whitespace
    if ((c == ' ') || (c == '\t') || (c == '\n') || (c == '\r')) {
      continue;
    } else {
      break;  // found nonwhite character
    }
  }

  if (c == '\0') {
    *sq = INVALID_SQUARE;
    return 0;
  }

  // get file and rank
  if ((c - 'a' < 0) || (c - 'a') > BOARD_WIDTH) {
    fen_error(fen, *c_count, "Illegal specification of last move");
    return 1;
  }

  next_c = fen[(*c_count)++];
  if (next_c == '\0') {
    fen_error(fen, *c_count, "FEN ended before last move fully specified");
  }

  if ((next_c - '0' < 0) || (next_c - '0') > BOARD_WIDTH) {
    fen_error(fen, *c_count, "Illegal specification of last move");
    return 1;
  }

  *sq = square_of(c - 'a', next_c - '0');
  return 0;
}

// Translate a fen string into a board position struct
// Returns 1 if there is an error
int fen_to_pos(position_t* p, const char* fen) {
  static position_t dmy1, dmy2;

  // these sentinels simplify checking previous
  // states without stepping past null pointers.
  dmy1.victims.count = 1;
  dmy1.history = NULL;

  dmy2.victims.count = 1;
  dmy2.history = &dmy1;

  p->victims.count = 0;  // piece destroyed by shooter
  p->history = &dmy2;    // history
  p->was_played = true;  // mark position as played
  p->nply_since_victim = 0;
  p->board = init_board();

  int c_count = 0;  // Invariant: fen[c_count] is next char to be read

  c_count = parse_fen_board(p, fen);
  if (c_count <= 0) {
    return 1;  // parse error
  }

  // Monarch check
  int monarchs[2] = {0, 0};

  for (coord_t fil = 0; fil < BOARD_WIDTH; ++fil) {
    for (coord_t rnk = 0; rnk < BOARD_WIDTH; ++rnk) {
      square_t sq = square_of(fil, rnk);
      piece_t x = get_piece_at(&p->board, sq);
      ptype_t typ = x.typ;
      if (typ == MONARCH) {
        monarchs[x.color]++;
      }
    }
  }

  if (monarchs[WHITE] == 0) {
    fen_error(fen, c_count, "No White Monarchs");
    return 1;
  } else if (monarchs[WHITE] > MAX_MONARCHS) {
    fen_error(fen, c_count, "Too many White Monarchs");
    return 1;
  } else if (monarchs[BLACK] == 0) {
    fen_error(fen, c_count, "No Black Monarchs");
    return 1;
  } else if (monarchs[BLACK] > MAX_MONARCHS) {
    fen_error(fen, c_count, "Too many Black Monarchs");
    return 1;
  }

  char c;
  bool done = false;
  // Look for color to move and set ply accordingly
  while (!done && (c = fen[c_count++]) != '\0') {
    switch (c) {
      // ignore whitespace until the end
      case ' ':
      case '\t':
      case '\n':
      case '\r':
        break;

      // White to move
      case 'W':
        p->ply = 0;
        done = true;
        break;

      // Black to move
      case 'B':
        p->ply = 1;
        done = true;
        break;

      default:
        fen_error(fen, c_count, "Must specify White (W) or Black (B) to move");
        return 1;
        break;
    }
  }

  // Look for last move, if it exists
  square_t lm_from_sq, lm_to_sq;
  rot_t lm_rot;
  if (get_sq_from_str(fen, &c_count, &lm_from_sq) != 0) {  // parse error
    return 1;
  }
  if (square_eq(lm_from_sq, INVALID_SQUARE)) {  // from-square of last move
    p->last_move = NULL_MOVE;                   // no last move specified
    return 0;
  }

  c = fen[c_count];

  switch (c) {
    case 'R':  // Check for simple rotations
      lm_to_sq = lm_from_sq;
      lm_rot = RIGHT;
      break;
    case 'U':
      lm_to_sq = lm_from_sq;
      lm_rot = UTURN;
      break;
    case 'L':
      lm_to_sq = lm_from_sq;
      lm_rot = LEFT;
      break;

    default:  // Not a rotation
      lm_rot = NO_ROT;
      if (get_sq_from_str(fen, &c_count, &lm_to_sq) != 0) {
        return 1;
      }
      break;
  }
  p->last_move = move_of(lm_rot, lm_from_sq, lm_to_sq);

  return 0;  // everything is okay
}

// Translate a position struct into a fen string
// NOTE: When you use the test framework in search.c, you should modify this
// function to match your optimized board representation in move_gen.c
//
// Input:   (populated) position struct
//          empty string where FEN characters will be written
// Output:  null
void pos_to_fen(const position_t* p, char* fen) {
  int pos = 0;

  for (coord_t rnk = BOARD_WIDTH - 1; rnk >= 0; --rnk) {
    int empty_in_a_row = 0;
    for (coord_t fil = 0; fil < BOARD_WIDTH; ++fil) {
      square_t sq = square_of(fil, rnk);
      piece_t piece = get_piece_at(&p->board, sq);

      if (piece.typ == INVALID) {            // invalid square
        tbassert(false, "Bad news, yo.\n");  // This is bad!
      }

      if (piece.typ == EMPTY) {  // empty square
        empty_in_a_row++;
        continue;
      } else {
        if (empty_in_a_row) {
          fen[pos++] = '0' + empty_in_a_row;
        }
        empty_in_a_row = 0;

        if (piece.typ == MONARCH) {
          cardinal_dir_t dir = piece.monarch_ori;
          char direction_char =
              dir.is_file_dir
                  ? file_dir_to_char(dir.file_dir, piece.color == WHITE)
                  : rank_dir_to_char(dir.rank_dir, piece.color == WHITE);
          fen[pos++] = direction_char;
          fen[pos++] = direction_char;
          continue;
        }

        if (piece.typ == PAWN) {
          ordinal_dir_t dir = piece.pawn_ori;
          fen[pos++] =
              file_dir_to_char(dir.file_component, piece.color == WHITE);
          fen[pos++] =
              rank_dir_to_char(dir.rank_component, piece.color == WHITE);
          continue;
        }
      }
    }
    // assert: for larger boards, we need more general solns
    tbassert(BOARD_WIDTH <= 10, "BOARD_WIDTH = %d\n", BOARD_WIDTH);
    if (empty_in_a_row == 10) {
      fen[pos++] = '1';
      fen[pos++] = '0';
    } else if (empty_in_a_row) {
      fen[pos++] = '0' + empty_in_a_row;
    }
    if (rnk) {
      fen[pos++] = '/';
    }
  }

  // Set the turn
  fen[pos++] = ' ';
  if (p->ply % 2 == 0) {
    fen[pos++] = 'W';
  } else {
    fen[pos++] = 'B';
  }
  fen[pos++] = '\0';
}
