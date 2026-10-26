% FIND-TIER(1) find-tier 0.1.0
% Jay Hilton
% April 2022

# NAME
find-tier - test the performance of libstudent

# SYNOPSIS
**find-tier** [**-m** *min_tier*] [**-M** *max_tier*] [**-b** *blowthroughs*] 

# DESCRIPTION
**find-tier** determines the performance tier of libstudent via pre-determined initial simulator
and renderer states. 

# OPTIONS
**-h**
: Displays usage information.

**-m *min_tier***
: Sets the minimum tier to start performance tests from, defaults to 0.

**-M *max_tier***
: Sets the maximum tier to end performance tests at, defaults to 90.

**-b *blowthroughs***
: Sets the number of tiers to allow to fail before stopping performance testing.

# EXIT VALUES
**0**
: Success

**1**
: Malformed file provided

# COPYRIGHT
TODO