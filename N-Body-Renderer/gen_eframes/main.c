#include "../common/types.h"
#include "../libstaff/include/misc_utils.h"
#include "../serde.h"
#include "../vtable.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h> 
#ifdef GRAPHICS
#include <GL/glut.h>
#include <math.h>
#endif

struct opts {
  vtable_t vtable;
  const char *s_filename;
  const char *r_filename;
  const char *out_filename;
  bool hash_only;
  size_t n_frames;
  bool realtime_graphics;
};

static void usage() { fprintf(stderr, "./gen_eframes [-n num_frames] [-r -s] [-g | -h] sim_spec renderer_spec output_file\nBy default uses staff simulator and renderer, -r and -s switch to student versions.\n"); }

static int argparse(int argc, char *const argv[], struct opts *const o) {
  bool staff_renderer = true;
  bool staff_simulator = true;
  o->realtime_graphics = false;
  o->hash_only = false;
  o->n_frames = 12;
  int ch;

  while ((ch = getopt(argc, argv, "n:rsgh")) != -1) {
    switch (ch) {
    case 'n':
      if (1 != sscanf(optarg, "%zu", &o->n_frames))
        goto error;
      break;
    case 'r':
      staff_renderer = false;
      break;
    case 's':
      staff_simulator = false;
      break;
    case 'h':
      o->hash_only = true;
      o->n_frames = 12;
      break;
    case 'g':
      o->realtime_graphics = true;
      break;
    default:
      goto error;
    }
  }

  argc -= optind;
  argv += optind;

  if (argc != 3 && !(o->realtime_graphics && argc == 2)) {
    goto error;
  }
  o->s_filename = argv[0];
  o->r_filename = argv[1];
  o->out_filename = argv[2];
  if (staff_renderer && staff_simulator) {
    o->vtable = staff_all();
  } else if (!staff_renderer && staff_simulator) {
    o->vtable = student_renderer();
  } else if (staff_renderer && !staff_simulator) {
    o->vtable = student_simulator();
  } else {
    o->vtable = student_all();
  }
  return 0;

error:
  usage();
  return -1;
}

#ifdef GRAPHICS
FILE *glut_s_f;
FILE *glut_r_f;
simulator_spec_t *glut_s_spec;
renderer_spec_t *glut_r_spec;
vtable_t *glut_vtable;
float *glut_img;
float *glut_empty;
int glut_frame_counter = 0;
int glut_n_frames;

void display(void) {
  // reset drawing window
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Draw the pixel array
  glDrawPixels(glut_r_spec->resolution, glut_r_spec->resolution, GL_RGB, GL_FLOAT, glut_img);

  // Reset buffer for next frame
  glutSwapBuffers();
}


void idle(void) {
  if (glut_frame_counter++ > glut_n_frames) {
    free(glut_empty);
    destroy_simulator_spec(glut_s_spec);
    destroy_renderer_spec(glut_r_spec);
    destroy_impl(glut_vtable);
    fclose(glut_s_f);
    fclose(glut_r_f);
    exit(0);
  }

  glutPostRedisplay();
  sphere_t* spheres = glut_vtable->simulate(glut_vtable->simulate_this);
  glut_img = glut_vtable->render(glut_vtable->renderer_this, spheres, glut_s_spec->n_spheres);
}

// Allows use of arrow keys to control movement
void special(int key, int x, int y) {
  vector_t e = glut_r_spec->eye;
  switch (key) {
  case GLUT_KEY_UP:
    glut_r_spec->eye = (vector_t) {e.x, e.y, e.z + 1};
    break;
  case GLUT_KEY_DOWN:
    glut_r_spec->eye = (vector_t) {e.x, e.y, e.z - 1};
    break;
  case GLUT_KEY_LEFT:
    glut_r_spec->eye = (vector_t) {e.x, e.y + 1, e.z};
    break;
  case GLUT_KEY_RIGHT:
    glut_r_spec->eye = (vector_t) {e.x, e.y - 1, e.z};
    break;
  }

  glutPostRedisplay();
}

// Allows keyboard commands to control parameters and movement
void keyboard(unsigned char key, int x, int y) {
  vector_t e = glut_r_spec->eye;
  switch (key) {
  case 'h':
    printf("HELP\n");
    printf("----\n");
    printf("f - move forward\n");
    printf("b - move backward\n");
    printf("c - rotate clockwise\n");
    printf("right arrow - move right\n");
    printf("left arrow - move left\n");
    printf("up arrow - move up\n");
    printf("down arrow - move down\n");
    break;
  case 'f':
    glut_r_spec->eye = (vector_t){e.x - 1, e.y, e.z};
    break;
  case 'b':
    glut_r_spec->eye = (vector_t){e.x + 1, e.y, e.z};
    break;
  case 'c':;
    float angle;
    vector_t viewDirection = qcross_libstaff(glut_r_spec->proj_plane_u, glut_r_spec->proj_plane_v);
    if (viewDirection.x != 0) {
      angle = atan(viewDirection.y / viewDirection.x);
    } else if (viewDirection.y < 0) {
      angle = -3.14 / 2;
    } else if (viewDirection.y > 0) {
      angle = 3.14 / 2;
    } else {
      break;
    }
    if (viewDirection.x < 0) {
      angle = 3.14 + angle;
    }
    viewDirection =
        (vector_t) {cos(angle + 0.1), sin(angle + 0.1), viewDirection.z};
    // calculate basis vectors
    vector_t up = (vector_t){0, 0, 1};
    vector_t w = scale(1 / qsize_libstaff(viewDirection), viewDirection);
    vector_t u = scale(1 / qsize_libstaff(qcross_libstaff(up, w)), qcross_libstaff(up, w));
    vector_t v = qcross_libstaff(w, u);
    glut_r_spec->proj_plane_u = u;
    glut_r_spec->proj_plane_v = v;
    break;
  }

  glutPostRedisplay();
}
#endif

int main(int argc, char * argv[]) {

  struct opts o;
  if (argparse(argc, argv, &o)) {
    return -1;
  }


  FILE *s_f = fopen(o.s_filename, "rb");
  FILE *r_f = fopen(o.r_filename, "rb");

  if (s_f == NULL | r_f == NULL) {
    fprintf(stderr, "gen_eframes: could not open %s or %s\n", o.s_filename, o.r_filename);
    exit(1);
  }

  simulator_spec_t s_spec;
  renderer_spec_t r_spec;

  if (deser_renderer_spec(&r_spec, r_f)) {
    fprintf(stderr, "gen_eframes: could not read render spec from %s\n", o.r_filename);
    exit(1);
  }

  if (deser_simulator_spec(&s_spec, s_f)) {
    fprintf(stderr, "gen_eframes: could not read simulate spec from %s\n",
            o.s_filename);
    exit(1);
  }

  // Run for n_frames frames and output the serialized results to out.

  init_impl(&o.vtable, &r_spec, &s_spec);

  if (o.realtime_graphics) {
#ifdef GRAPHICS
    glut_s_spec = &s_spec;
    glut_r_spec = &r_spec;
    glut_vtable = &o.vtable;
    glut_n_frames = o.n_frames;
    glut_s_f = s_f;
    glut_r_f = r_f;
    // prevents a segfault when display is called before render
    glut_empty = calloc(r_spec.resolution * r_spec.resolution * 3, sizeof(float)); 
    glut_img = glut_empty;
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(r_spec.resolution, r_spec.resolution);

    glutCreateWindow("Project 2 Graphical Display");
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special);
    glutIdleFunc(idle);
    glutMainLoop();
#else
    fprintf(stderr, "compile with GRAPHICS=1 at top level.\n");
    exit(1);
#endif
  } else {
    
    frames_t out = {
      .height = r_spec.resolution,
      .width = r_spec.resolution,
      .is_diff = false,
      .n_frames = o.n_frames,
    };

    const size_t floats_per_frame = 3 * r_spec.resolution * r_spec.resolution;
    if (!o.hash_only) {
      out.buf = malloc(sizeof(float) * floats_per_frame * out.n_frames);
      if (out.buf == NULL) {
        fprintf(stderr, "gen_eframes: oom\n");
        exit(1);
      }
    }

    FILE *out_f = fopen(o.out_filename, o.hash_only ? "w" : "wb");
    if (out_f == NULL) {
      fprintf(stderr, "gen_eframes: could not open %s as writeable\n", o.out_filename);
      exit(1);
    }

    for (size_t f = 0; f < out.n_frames; f++) {
      sphere_t *spheres = o.vtable.simulate(o.vtable.simulate_this);
      const float *frame =
          o.vtable.render(o.vtable.renderer_this, spheres, s_spec.n_spheres);
      // Modification starts here: 
      if (o.hash_only) {
        uint64_t hash = 0xcbf29ce484222325;
#define HASH(x, sz)                                                            \
  do {                                                                         \
    for (size_t i = 0; i < sz; i++) {                                          \
      hash *= 0x00000100000001B3;                                              \
      hash ^= *((uint8_t *)x + i);                                             \
    }                                                                          \
  } while (0)
        HASH(&out.is_diff, sizeof(out.is_diff));
        HASH(&out.height, sizeof(out.height));
        HASH(&out.width, sizeof(out.width));
        HASH(frame, floats_per_frame * sizeof(float));
        fprintf(out_f, "%016lx\n", hash);
#undef HASH
      } else {
        float *buf = out.buf + (f * floats_per_frame);
        memcpy(buf, frame, floats_per_frame * sizeof(float));
      }
    }
  
    if (!o.hash_only) {
      if (ser_frames(out_f, &out)) {
        fprintf(stderr, "gen_eframes: could not serialize frames\n");
        exit(1);
      }
      free(out.buf);
    }
    else {
      fprintf(stderr, "gen_eframes: hash generated for %s\n", o.out_filename);
    }
    fclose(out_f);
  }

  destroy_simulator_spec(&s_spec);
  destroy_renderer_spec(&r_spec);
  destroy_impl(&o.vtable);

  fclose(s_f);
  fclose(r_f);
  return 0;
}
