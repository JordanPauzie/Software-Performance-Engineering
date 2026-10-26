#include <getopt.h> // optarg, optind
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "../serde.h"
#include "gifenc/gifenc.h"

struct opts {
  const char *out_fn;
  const char *diff_fn;
};

// TODO configurable framerate?

static void usage() { fprintf(stderr, "./diff2gif [-o out] diff\n"); }

static int argparse(int argc, char *const argv[], struct opts *const o) {
  o->out_fn = "out.gif";

  int ch;

  while ((ch = getopt(argc, argv, "o:")) != -1) {
    switch (ch) {
    case 'o':
      o->out_fn = optarg;
      break;
    default:
      goto error;
    }
  }

  argc -= optind;
  argv += optind;

  if (argc != 1) {
    goto error;
  }

  o->diff_fn = argv[0];
  return 0;

error:
  usage();
  return -1;
}

//We're using 3 bits for red and green, only 2 for blue
static uint8_t rgb_to_8bit(int r, int g, int b) {
  r /= 32;
  g /= 32;
  b /= 64;
  uint8_t index = (r<<5) | (g<<2) | b;
  //For some reason index 255 doesn't work properly, so avoid it
  if (index == 255) {
    return 254;
  }
  return index;
}

static uint8_t px2c(const float *px, bool color) {
  if (color) {
    return rgb_to_8bit((int)(*px * 255), (int)(*(px + 1) * 255), (int)(*(px + 2) * 255));
  } else {
    if (*px == 0) {
      return 0; //Correct pixels are white
    } else {
      int BASE_RED = 80; //anything marginally wrong will be at least this red
      //Ref_test already allpies a log scale to the errors [0,1]
      double wrongness = *px;
      //printf("%f\n", wrongness);
      //Scale it into about 0 to 254 - BASE_RED
      int color = (int)(wrongness * (254 - BASE_RED));
      //Clip it to fit in a byte
      //There's some error where 255 appears white, just avoid it
      if (color < 0) {
        color = 0;
      } else if (color > 254 - BASE_RED) {
        color = 254 - BASE_RED;
      }
      return color + BASE_RED;
    }
  }
}

static void write_gif(const char *const gif_fn, const frames_t *const diff,
                      const uint16_t time_per_frame) {
  uint8_t palette[3*256];
  if (diff->is_diff) {
    //Use red-white colorscale for errors
    for (int i = 0; i < 256; i++) {
      palette[i*3] = 255; //red
      palette[i*3 + 1] = 255 - i; //green
      palette[i*3 + 2] = 255 - i; //blue
    }
  } else {
    //Use 8 bit rgb colorscale
    for (int r = 0; r < 8; r++) {
      for (int g = 0; g < 8; g++) {
        for (int b = 0; b < 4; b++) {
          uint8_t index = rgb_to_8bit(r * 32, g * 32, b * 64);
          palette[index * 3] = r * 32;
          palette[index * 3 + 1] = g * 32;
          palette[index * 3 + 2] = b * 64;
        }
      }
    }
  }
  ge_GIF *gif = ge_new_gif(gif_fn, diff->width, diff->height,
                           palette, // use a custom palette
                           8,  // default palette has 256 entries, log2(256) = 8
                           -1, // no transparency
                           0   // infinite loop
  );

  // Draw each frame into the gif.

  // TODO this relies on 1 float per pixel which may not be the case, so check
  // that in the diff generation.
  const size_t floats_per_px = diff->is_diff ? 1 : 3;
  const size_t floats_per_frame = floats_per_px * diff->width * diff->height;
  const size_t total_floats = floats_per_frame * diff->n_frames;
  for (const float *frame = diff->buf; frame < diff->buf + total_floats;
       frame += floats_per_frame) {
    for (size_t y = 0; y < diff->height; y++) {
      for (size_t x = 0; x < diff->width; x++) {
        // Both gifenc and our diff outputs use row-major order so we can just
        // copy over. We do have to convert the float to a palette entry,
        // though.
        const size_t px_i = x + y * diff->width;
        const float *px = frame + px_i * floats_per_px;
        gif->frame[px_i] = px2c(px, !diff->is_diff);
      }
    }

    ge_add_frame(gif, time_per_frame);
  }

  ge_close_gif(gif);
}

int main(int argc, char *const argv[]) {
  struct opts o;

  if (argparse(argc, argv, &o) != 0) {
    return 1;
  }

  FILE *diff_f = fopen(o.diff_fn, "rb");
  if (diff_f == NULL) {
    fprintf(stderr, "diff2gif: could not open diff %s\n", o.diff_fn);
    exit(1);
  }

  frames_t diff;

  if (deser_frames(&diff, diff_f)) {
    fprintf(stderr, "diff2gif: could not deserialize diff file %s\n",
            o.diff_fn);
    exit(1);
  }

  write_gif(o.out_fn, &diff, 10);

  return 0;
}
