// Copyright (c) 2022 MIT License by 6.172 / 6.106 Staff

#ifndef EVAL_H
#define EVAL_H

#include <stdbool.h>

#include "move_gen.h"
#include "search.h"

#define EV_SCORE_RATIO 100  // Ratio of ev_score_t values to score_t values

// ev_score_t values
#define PAWN_EV_VALUE (PAWN_VALUE * EV_SCORE_RATIO)

// shorter version of PAWN_EV_VALUE to fit in table
#define P_EV_VAL PAWN_EV_VALUE

#define WEIGHT_SCALE (1 << 20)

score_t eval(position_t* p, bool verbose);

#endif  // EVAL_H
