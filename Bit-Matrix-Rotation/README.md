# Project 1: Bit Hacks

## Get Started
```bash
cd snailspeed
make

# Rotate a single image
./rotate -t file -f img/speedlimit.bmp -o img/rotated_speedlimit.bmp

# Rotate a randomly-generated matrix of size 2048 and check correctness
./rotate -t generated -N 2048

# Run correctness tests
./rotate -t correctness

# Measure performance tier (does not check correctness)
./rotate -t tiers
```
- see help in `./rotate` for more ways to test
- Note: `tiers` only tests the speed of your code but not correctness. If you want to test for correctness, please use the `correctness` option.
