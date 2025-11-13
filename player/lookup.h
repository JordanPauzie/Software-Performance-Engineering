// Copyright (c) 2022 MIT License by 6.172 / 6.106 Staff

#ifndef LOOKUP_H
#define LOOKUP_H

#define OPEN_BOOK_DEPTH 2

int lookup_sizes[OPEN_BOOK_DEPTH] = {2, 4};

// Move histories are paired with their best moves in the lookup tables
// IE: lookup_table_depth_n[] = {move_history1, best_move1, move_history2,
// best_move2, ...}
// a move history is the concatenation of the move strings, e.g.
// a lookup_table_depth_3[] could look like: {"move1move2", "move3", ...}
const char* lookup_table_depth_1[] = {"", "h0g1"};
const char* lookup_table_depth_2[] = {"h0g1", "a7b6", "f6e7", "a7b6"};

const char** lookup_tables[OPEN_BOOK_DEPTH] = {lookup_table_depth_1,
                                               lookup_table_depth_2};

#endif  // LOOKUP_H
