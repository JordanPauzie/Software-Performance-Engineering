% SPECVWR(1) specvwr 0.1.0
% Jay Hilton
% April 2022

# NAME
specvwr - view initial simulator or renderer state

# SYNOPSIS
**specvwr** [-r|-s] *state*

# DESCRIPTION
**specvwr** accepts a binary file containing the initial simulator or renderer state, and
pretty-prints it. The provided flag, either -r or -s, indicates if the provided file is 
a renderer intial state or simulator initial state.

# OPTIONS
**-h**
: Displays usage information.

**-r**
: Causes **specvwr** to interpret the provided file as an initial renderer state.

**-s**
: Causes **specvwr** to interpret the provided file as an initial simulator state.

# EXAMPLES
**specvwr**
: Displays usage information, then exits.

**specvwr -r *file***
: Opens file, which is an initial renderer state, and pretty-prints it.

**specvwr -s *file***
: Opens file, which is an initial simulator state, and pretty-prints it.

# EXIT VALUES
**0**
: Success

**1**
: Malformed file provided

# COPYRIGHT
TODO