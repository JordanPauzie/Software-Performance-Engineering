This directory contains your implementation of the renderer and simulator. Feel free to change it as you'd like. 
You must implement all the functions in common/render.h and common/simulate.h.
Its initial structure is as follows: (although you might change this structure too)
- include/
    - misc_utils.h
        A collection of functions that might be useful when working with vector_t and other types in types.h.
- src/
    - render.c
        Your implementation of the renderer.
    - simulate.c
        Your implementation of the simulator.

Both files in src/ define an object-like macro called WEAK_SYMBOL. Removing the WEAK_SYMBOL above any function definitions
may cause correctness tests to pass when they should not, which would cause your local tests to differ from staff correctness 
testing. Similarly, changing the definition of the WEAK_SYMBOL macro can cause the same problem.
