# Design
(This document is intended for staff maintaining the project in future, but for students, might be useful for understanding how your code is run.)

## Structure
This project is split into submodules:

- `find-tier`, the performance tester
- `libstaff`, the reference implementation
- `libstudent`, the student implementation
- `ref-tester`, the correctness tester
- `specvwr`, a binary for printing out the contents of serialized state files

This structure is used to enable composing together easier-to-understand modules compared to a more monolithic structure. This also means that CLIs have simpler interfaces with fewer arguments, and is intended to make it easier to use the shell to combine tools together if necessary.

Aside from these submodules, there are various libraries that are also important to the project.
`serde` is a serialization/deserialization library for render and simulate states, as well as expected frame outputs. `vtable` is a library for providing tables of function pointers corresponding to various combinations of renderer and simulator implementations. `dbg_types` is a library for displaying the types defined in `common/types.h`.

### `find-tier`
`find-tier` is a binary which runs a linear search through a specified range of tiers with a specified number of blowthroughs. It's structured into two statically-linked libraries, `benchmark` and `fasttime` , and a `main.c`. `fasttime` might be similar to the timing infrastructure used in other projects, while `benchmark` provides the types and methods actually used for benchmarking. Its entry point is `bench_result_t run_benchmark(bench_spec_t \*spec, pass_cb \*on_pass, fail_cb \*on_fail)`, which runs a linear search over the specified tiers and calls the appropriate callback on success and failure (or nothing if NULL is provided). This design was chosen to provide a simple API for displaying pass/fail messages. Considered alternatives were:
- providing functions for registering multiple callbacks
- hardcoding the callback which was run

The former was not chosen due to added complexity when multiple callbacks are likely unnecessary, and the latter was not chosen due to the difficulty of changing what's printed on pass/fail. The actual implementation of the `benchmark` library consists of an array of tier descriptors, `tiers`, and an array of the same size, `initialized`, which indicates which tiers are initialized. A tier descriptor is enough starting state to test a particular tier, so once the tiers are initialized, they're run, which only invokes the implementations in `libstudent`. There's a possible problem where students can discern whether or not  correctness is being tested based on whether or not `libstaff` is loaded, but `main.c` opens `libstaff`  so that students cannot differentiate between if correctness is being tested. An alternative solution may be *also* testing for correctness when testing for performance, but this slows down performance testing as it would either depend on the staff implementation or expected frames for the tiers.

### `libstaff`/`libstudent`
`libstaff` and `libstudent` are structurally identical initially. Each implements `render.h` and `simulate.h`, but they have their own separate `misc_utils.h` to allow students to change the utilities if they'd like. The only distinction between them is how they're linked. `libstudent` is statically linked for improved optimization potential, while `libstaff` is dynamically linked to avoid symbol collisions. Both `libstaff` and `libstudent` only export the symbols in `render.h` and `simulate.h`, which defines the API students must implement.

### `ref-tester`
`ref-tester` is used for correctness checking. It's implemented through 1 statically linked library, `ref-tester` and a `main.c`. `ref-tester`'s entry point is through two functions, `ref_out_t  run_test(const ref_spec_t *start_spec, size_t n_frames)`, and `ref_stats_t  compare_images(const float *ref, const float *test, int height, int width)`. `compare_images` computes summary statistics about the difference between `ref` and `test`, and `run_test` runs a test starting from `start_spec` for the specified frame count `n_frames`.  Test specifications include how re-initialization should occur at the start of each frame. Other approaches were considered:
- a more granular `ref-tester` library that works frame-by-frame
- moving everything together into the `main.c` file

The former was not chosen due to simplicity: we can abstract away the complexity of frame-by-frame comparison, which `main.c` doesn't need to be concerned with. A smaller API surface also means it's easier to avoid breaking changes if we want to change how the reference tester works internally. The latter was not chosen due to maintainability: `main.c`'s only role is currently input parsing, and if `main.c` also handled reference testing, it would be harder to follow.

`vtable.c` does not currently have a way to close the handles it opens, so a tool like Valgrind might think that they're memory leaks. Adding something to close them might be a possible improvement, but isn't particularly important to anything other than Valgrind not showing something that might seem like a false positive to students. Additionally, since Valgrind shows specifics for where each leak comes from, it should be pretty easy to tell that this is both benign and not from their code.

### `specvwr`
`specvwr` is used for displaying renderer and simulator states. It uses `dbg_types.h` for how it displays these states, and `serde.h` for reading them from files. It's not split into separate libraries because its input parsing is quite simple, and selects at runtime which function to use to view the file based on a flag provided by the user. Possible improvements include detecting what type of file was provided based on the first 8 bytes (which is instead used to verify that the file is what the user says it is currently). 

## Makefiles

All binaries and some libraries in the project have a corresponding Makefile. There's also a Makefile in the top-level directory. This top-level Makefile configures common parameters: sanitizers to use, optimization level, and generally other settings that have to be propagated to all others. Makefiles outside the top-level directory are responsible for building their binary or library and can add additional flags for this purpose. Of particular note is the Makefile in `libstudent`, which we expect students to change. It is possible that students might want to change the top-level Makefile to provide some compiler flags, although I am unclear on how that should be accommodated at the moment. Other than the `libstudent`  Makefile and possibly the top-level Makefile, all changes by students can and should be reverted before grading.

This Makefile structure uses sub-make invocations for the depended-upon products. At present, this produces a lot of output, which might be hard to parse. In exchange, it clearly separates the rules for building products so they're clearly related to the respective product. 