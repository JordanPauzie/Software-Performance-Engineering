# Software Performance Engineering

Projects from Software Performance Engineering (Tufts CS 151, Fall 2025), a course built on
MIT 6.106. Each project hands you working but slow C code and a performance benchmark;
the job is to make it fast without breaking correctness. All work was done in teams.

**Tools:** C, OpenCilk (fork-join parallelism), perf, Linux, TACC Stampede3

---

## 1. Bit-matrix rotation
*Team: Jordan Pauzie, [TEAMMATES]*

Rotate a bit-packed square image by 90° in place, as fast as possible.

- Replaced bit-by-bit rotation with a block-based approach that rotates
  BLOCK_SIZE × BLOCK_SIZE tiles and cycles each tile through all four quadrants in one pass
- [BIT-HACK TECHNIQUES USED, e.g. word-level masks, shifts, loop unrolling]
- **Result:** [TIER / SPEEDUP]

## 2. N-body physics simulation and renderer
*Team: Jordan Pauzie, [TEAMMATES]*

Speed up a gravitational simulation of colliding spheres and the renderer that draws each frame.

- **Rendering:** replaced per-pixel ray tracing with a projection algorithm. Spheres are
  depth-sorted, culled outside the view, and drawn only inside a screen-space bounding box,
  with already-filled pixels skipped. Then ray setup was precomputed and the pixel loop
  parallelized with OpenCilk.
- **Physics:** used Newton's third law to compute each pairwise force once instead of twice
  (n²/2 instead of n²), while keeping the reference summation order so results stay bit-exact
  for correctness tests
- **Parallelism:** parallel force accumulation and collision detection, each written race-free
  by giving every task its own output slot
- **Algorithms and micro-optimizations:** parallel merge sort that falls back to insertion sort
  on small inputs, replaced `pow()` with multiplication, removed repeated distance calculations,
  inlined hot vector math by hand

**Result:** performance tier **0 → 64** (target: 30)

| Version | Tier | Change |
|---|---|---|
| Original | 0 | — |
| 1 | 24 | Projection rendering instead of ray tracing |
| 2 | 32 | Symmetric force computation |
| 3 | 47 | Parallelized rendering |
| 4 | 49 | Parallel merge sort |
| 5 | 50 | Removed redundant `pow` and distance calls |
| 6 | 52 | Manual inlining and loop fixes |
| 7 | 64 | Parallelized collision detection |

## 3. Memory allocator
*Team: Jordan Pauzie, [TEAMMATES]*

Write a `malloc`/`free`/`realloc` replacement scored on both throughput (against libc) and
memory utilization.

- Segregated free lists with 60 size classes growing geometrically (×1.5) from a 32-byte minimum
- Large blocks are carved down to the requested size, and the leftover is recursively split
  into smaller size classes instead of wasted
- `realloc` grows in place when the existing block already fits
- Wrote a heap validator and consistency checker to catch corruption early
- **Result:** [SCORE / UTILIZATION / THROUGHPUT]

## 4. Leiserchess game engine
*Team: Jordan Pauzie, [TEAMMATES]*

Make a game-playing engine for Leiserchess, a laser-chess variant, both faster and stronger.
It was evaluated in automated head-to-head matches with Elo ratings.

- **My focus, profiling and hot paths:** profiled every engine command with `perf` and inlined
  the hottest functions. `get_piece_at()` dropped from 21.2% to 6.2% of CPU time in move-generation
  benchmarks, and `square_to_index()` from 7.5% to 1.8%. With the team's monarch caching and merge
  sort, the beta engine ran **77% faster** than the baseline.
- **Debugging the metric:** the team found that a search reporting depths over 7,000 was a bug,
  not a speedup. A broken Zobrist hash update made the search deep but almost empty. Fixing it
  and switching to Elo-based testing put the engine back on track.
- **Parallel search:** principal-variation search with the Young Brothers Wait Concept and
  Jamboree-style parallel sibling search using Cilk work stealing, with early abort on beta cutoffs
- **Result:** **60% win rate** against the baseline engine over 20 games, and depth 8 versus 7
  with a 30-second search. Also won our course's mini-tournament when every group's Leiserchess engine
  was pitted against each other.

**Lesson:** parallelizing the search added overhead without a net speedup at our search depths.
Measuring with the right metric mattered more than any single optimization.
