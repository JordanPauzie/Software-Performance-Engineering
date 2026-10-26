# 6.106 Project 2
## Author:
- Isabel Rosa, <isrosa@mit.edu>
- Jay Hilton, <jhilton@mit.edu>

## Getting started

Run
```
cd project2
make
````
to generate some helpful binaries in the `project2/bin/` folder. Instructions for running the binaries can be seen below. In this project, you will be tasked with optimizing the code inside `project2/libstudent/`. You can find documented function prototypes and type definitions in `project2/common/`. **You should only change the code inside `libstudent/`. Changes to files outside `libstudent/` will be reverted when your project is being graded!**

## Instructions for performance testing:

Run
```
cd project2
make
telerun ./bin/find-tier
```
to performance test your implementation in `libstudent/` against a series of tiers. 

You can also run
```
telerun --cores 8 ./bin/find-tier
```
to test your parallelized version on a machine with more cores.


## Instructions for testing for correctness:
Run
```
cd project2
make
./bin/ref-test <path_to_sim_file> <path_to_render_file>
```
to test your implementation for correctness.

Simulation specs are named `s`, and render specs are named `r`, in their respective directories in `tiers` and `simulations`. For example, to test your implementation with the specifications for tier 0, you would run the following commands:

```
./bin/ref-test tiers/0/s tiers/0/r
```

Note, `<path_to_sim_file>` and `<path_to_render_file>` must be relative to your current working directory.

## Makefile Options:

Run program with cilkscale by building with command `make CILKSCALE=1`. You can check for races by building with `make CILKSAN=1`. You can check for undefined behavior using `make UBSAN=1` or out of bounds memory accesses with `make ASAN=1`. You should only use one of these options at the same time as they may interfere with each other. We also reccomend running the sanitizers locally as they often time out on telerun.

## Instructions for Making Tests:

Run
```
./bin/convert-test test_file N
```
where test_file is a file in the text-based format specified in the handout and N defines the renderer viewport to be N x N.
The output binary test files will be at `r_out` and `s_out`.
The text-based format is as follows: 
- the first line is formatted as "g num-spheres", where g is a non-negative floating-point
number representing the gravitational acceleration and num-spheres is a positive integer. There must be some whitespace between g
and num-spheres.
- each other line is formatted as "r mass pos-x pos-y pos-z vel-x vel-y vel-z mat-diffuse-red
mat-diffuse-green mat-diffuse-blue mat-reflection" where pos is the position of the sphere,
vel is the velocity of the sphere, and mat is the material of the sphere. x, y, and z refer to the respective components of the vector. mat-diffuse-red, mat-diffuse-green, mat-diffuse-blue,
and mat-reflection must all be between 0 and 1.
- there must be num-spheres lines defining spheres in the text file.

##  Instructions for diff visualization:
If you are having correctness issues, you can generate a diff file that you can visualize. For example, you can generate a diff file by running 
```
./bin/ref-test -o diff_file tiers/0/s tiers/0/r
```
 and then generate the GIF `/project2/out.gif` using 
 ```
 ./bin/diff2gif diff_file
 ``` 
 This GIF is white where your output was correct, and darker shades of red where it was more wrong. The color scaling is done so even tiny errors will still be visibly light red. If you want to mess with the contrast or colors look at `px2c()` in `diff2gif/main.c`.


##  Instructions for saving output:
You can use `gen_eframes` to run a simulation and save the rendered output for later use. You can specify to use any combination of your or the staff renderer or simulator. For example to run the staff code on tier zero:
 ```
 ./bin/gen_eframes tiers/0/s tiers/0/r correct_output
 ``` 
 To run your simulator with the staff renderer:
  ```
 ./bin/gen_eframes -s tiers/0/s tiers/0/r my_output
 ``` 
 You can now compare the outputs using:
  ```
 cmp my_output correct_output
  ``` 
  And you won't need to rerun the staff code every time you test your own code!
  
## Viewing renders:
To just look at what you've rendered, run
 ```
 ./bin/gen_eframes -s tiers/0/s tiers/0/r my_output
 ./bin/diff2gif my_output -o render.gif
 ``` 
and gaze upon your work in any gif viewer, including VSCode.

## Viewing specifications
To see a render specification (with image size and light info) or the simuliation specification (gravity and all sphere info) run:
```
./bin/specvwr -r tiers/0/r
./bin/specvwr -s tiers/0/s
```

## File overview:
Feel free to look around, but your performance grade will only depend on changes to files in the `libstudent/` directory. All other files will be reverted for grading.



```
project
|   asprintf(.c/.h): heap-allocating sprintf    
|   dbg_types(.c/.h): methods for debugging types defined in common/types.h
|   DESIGN.md: documentation on why this project is structured how it is
|   Makefile: top-level Makefile, delegates to Makefiles in other binaries
|   misc_utils(.c/.h): utilities for operating on types defined in common/types.h
|   README.md: this file!
|   serde(.c/.h): methods for serializing and deserializing types in common/types.h
|   vtable(.c/.h): methods for obtaining combinations of staff/student implementations
└───bin: place where completed binaries are stored
└───common: files used in infrastructure + staff/student implementations
    │   render.h: renderer methods that an implementation must provide
    │   simulate.h: simulator methods that an implementation must provide
    |	types(.h/.c): types used within the raytracer and simple operations on them
└───convert_test: module for converting tests from text-based to binary file-based
    |	main.c
    |	Makefile
└───find-tier: module for performance testing
    │   benchmark(.c/.h): library for performance testing
    │	fasttime(.c/.h): library for timing
    |	main.c
    |	Makefile
└───libstaff: staff reference implementation
	| Makefile
	| README.md
	└───include: headers
		| misc_utils.h: utilities for operating on the types in common/types.h
	└───src: implementation files
		| misc_utils.c
		| render.c: staff render implementation
		| simulate.c: staff simulate implementation
|
└───libstudent: your implementation!
	| Makefile
	| README.md
	└───include: headers
		| misc_utils.h: utilities for operating on the types in common/types.h
	└───src: implementation files
		| misc_utils.c
		| render.c: student render implementation
		| simulate.c: student simulate implementation
└───ref-tester: module for correctness testing by comparing to reference
	|	main.c
	|	Makefile
	|	ref_tester(.c/.h): library for reference testing
	|	serde(.c/.h): library for serializing / deserializing ref_out_t, defined in types.h
	|	types(.c/.h): type definitions and operations for reference testing
└───simulations: smaller tests useful for correctness testing
└───specvwr: module for viewing renderer / simulator spec files
	| 	main.c
	|	Makefile
└───tiers: tiers for performance testing
└───diff2gif: Generate gif heatmaps of where in the image errors are or view renders
└───gen_eframes: Run with a set combination of student/staff renderer/simulator and save the output
```
