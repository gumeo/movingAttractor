/**
 * Strange Attractor Visualizer
 *
 * Based on tutorial "Strange Attractors in C++ and OpenGL" by Nathan Selikoff
 * http://nathanselikoff.com/tutorial-strange-attractors-in-c-and-opengl
 * License: CC BY-NC-SA 3.0
 *
 * Extended with: CLI configuration, multiple attractors, color modes,
 * interactive exploration, and frame export for video generation.
 */

#include <iostream>
#include <iomanip>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>
#include <algorithm>
#include <getopt.h>

#ifdef __APPLE__
#include <OpenGL/OpenGL.h>
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

// ============================================================
// Attractors
// ============================================================

enum AttractorType {
    ATTRACTOR_KINGS_DREAM = 0,
    ATTRACTOR_CLIFFORD,
    ATTRACTOR_DE_JONG,
    ATTRACTOR_BEDHEAD,
    ATTRACTOR_SVENSSON,
    ATTRACTOR_COUNT
};

const char *attractor_names[] = {
    "King's Dream", "Clifford", "De Jong", "Bedhead", "Svensson"
};

// Default a,b,c,d for each attractor type
static const double attractor_defaults[ATTRACTOR_COUNT][4] = {
    { -0.966918,  2.879879,  0.58,    0.813   },  // King's Dream
    { -1.4,       1.6,       1.0,     0.7     },  // Clifford
    {  1.641,     1.902,     0.316,   1.525   },  // De Jong
    { -0.81,     -0.92,      0.0,     0.0     },  // Bedhead (c,d unused)
    {  1.5,      -1.8,       1.6,     0.9     },  // Svensson
};

struct Point2 { double x, y; };

static Point2 step_attractor(Point2 p, AttractorType type, double a, double b, double c, double d) {
    Point2 n;
    switch (type) {
        case ATTRACTOR_KINGS_DREAM:
            n.x = sin(p.y * b) + c * sin(p.x * b);
            n.y = sin(p.x * a) + d * sin(p.y * a);
            break;
        case ATTRACTOR_CLIFFORD:
            n.x = sin(a * p.y) + c * cos(a * p.x);
            n.y = sin(b * p.x) + d * cos(b * p.y);
            break;
        case ATTRACTOR_DE_JONG:
            n.x = sin(a * p.y) - cos(b * p.x);
            n.y = sin(c * p.x) - cos(d * p.y);
            break;
        case ATTRACTOR_BEDHEAD:
            // b must not be zero; clamp to small value
            if (b == 0.0) b = 1e-6;
            n.x = sin(p.x * p.y / b) * p.y + cos(a * p.x - p.y);
            n.y = p.x + sin(p.y) / b;
            break;
        case ATTRACTOR_SVENSSON:
            n.x = d * sin(a * p.x) - sin(b * p.y);
            n.y = c * cos(a * p.x) + cos(b * p.y);
            break;
        default:
            n = p;
    }
    return n;
}

// ============================================================
// Color modes
// ============================================================

enum ColorMode {
    COLOR_MONO = 0,
    COLOR_HUE_CYCLE,
    COLOR_VELOCITY,
    COLOR_ANGLE,
    COLOR_COUNT
};

const char *color_mode_names[] = { "Mono", "Hue Cycle", "Velocity", "Angle" };

static void hsv_to_rgb(float h, float s, float v, float &r, float &g, float &b) {
    if (s == 0.0f) { r = g = b = v; return; }
    int   i = (int)(h * 6.0f);
    float f = h * 6.0f - i;
    float p = v * (1.0f - s);
    float q = v * (1.0f - s * f);
    float t = v * (1.0f - s * (1.0f - f));
    switch (i % 6) {
        case 0: r=v; g=t; b=p; break;
        case 1: r=q; g=v; b=p; break;
        case 2: r=p; g=v; b=t; break;
        case 3: r=p; g=q; b=v; break;
        case 4: r=t; g=p; b=v; break;
        case 5: r=v; g=p; b=q; break;
    }
}

// ============================================================
// Configuration
// ============================================================

struct AppConfig {
    unsigned int width        = 1000;
    unsigned int height       = 1000;
    int          iterations   = 2000000;
    int          init_iters   = 100;
    unsigned int max_frames   = 600;
    double       delta        = 0.1;     // oscillation amplitude for animation

    AttractorType type        = ATTRACTOR_KINGS_DREAM;
    double        a, b, c, d;           // filled from defaults or CLI

    ColorMode color_mode      = COLOR_MONO;
    float     mono_r          = 1.0f;
    float     mono_g          = 1.0f;
    float     mono_b          = 1.0f;
    float     bg_r            = 0.0f;
    float     bg_g            = 0.0f;
    float     bg_b            = 0.0f;
    float     alpha           = 0.02f;
    float     hue_start       = 0.0f;
    float     hue_range       = 1.0f;

    bool      save_frames     = false;
    char      output_dir[512];
    bool      interactive     = false;

    // CLI overrides for a,b,c,d (so --type doesn't clobber explicit values)
    bool      override_a = false, override_b = false;
    bool      override_c = false, override_d = false;

    AppConfig() {
        strcpy(output_dir, ".");
        a = attractor_defaults[ATTRACTOR_KINGS_DREAM][0];
        b = attractor_defaults[ATTRACTOR_KINGS_DREAM][1];
        c = attractor_defaults[ATTRACTOR_KINGS_DREAM][2];
        d = attractor_defaults[ATTRACTOR_KINGS_DREAM][3];
    }
};

// ============================================================
// Global state
// ============================================================

static AppConfig cfg;

// Starting params (used by animation to compute oscillation baseline)
static double a_init, b_init, c_init, d_init;

static unsigned int nframes = 0;
static unsigned int time0   = 0;
static GLubyte     *pixels  = nullptr;
static bool         paused  = false;
static bool         show_hud = true;

// ============================================================
// Screenshot (PPM)
// ============================================================

static void screenshot_ppm(const char *filename) {
    const size_t nc = 3;
    FILE *f = fopen(filename, "w");
    if (!f) { fprintf(stderr, "Cannot open %s for writing\n", filename); return; }
    fprintf(f, "P3\n%u %u\n255\n", cfg.width, cfg.height);
    pixels = (GLubyte *)realloc(pixels, nc * cfg.width * cfg.height);
    glReadPixels(0, 0, cfg.width, cfg.height, GL_RGB, GL_UNSIGNED_BYTE, pixels);
    for (unsigned int i = 0; i < cfg.height; i++) {
        for (unsigned int j = 0; j < cfg.width; j++) {
            size_t cur = nc * ((cfg.height - i - 1) * cfg.width + j);
            fprintf(f, "%3d %3d %3d ", pixels[cur], pixels[cur+1], pixels[cur+2]);
        }
        fprintf(f, "\n");
    }
    fclose(f);
}

// ============================================================
// Animation update (sine-wave oscillation so the loop is seamless)
// ============================================================

static void animation_update() {
    double t = 2.0 * M_PI * nframes / cfg.max_frames;
    cfg.a = a_init + cfg.delta * sin(t);
    cfg.b = b_init + cfg.delta * sin(t + M_PI / 4.0);
    cfg.c = c_init + (cfg.delta / 8.0) * sin(t + M_PI / 2.0);
    cfg.d = d_init + cfg.delta * sin(t + 3.0 * M_PI / 4.0);
}

// ============================================================
// Drawing
// ============================================================

static void draw_attractor() {
    glClear(GL_COLOR_BUFFER_BIT);

    // Warm up: run init_iters to settle into attractor basin
    Point2 p = {0.1, 0.1};
    for (int i = 0; i < cfg.init_iters; i++)
        p = step_attractor(p, cfg.type, cfg.a, cfg.b, cfg.c, cfg.d);

    // For MONO mode, set color once before the loop for performance
    const bool per_point_color = (cfg.color_mode != COLOR_MONO);
    if (!per_point_color)
        glColor4f(cfg.mono_r, cfg.mono_g, cfg.mono_b, cfg.alpha);

    glBegin(GL_POINTS);
    for (int i = 0; i < cfg.iterations; i++) {
        Point2 next = step_attractor(p, cfg.type, cfg.a, cfg.b, cfg.c, cfg.d);

        if (per_point_color) {
            float r, g, b;
            switch (cfg.color_mode) {
                case COLOR_HUE_CYCLE: {
                    float hue = fmodf((float)i / cfg.iterations * cfg.hue_range + cfg.hue_start, 1.0f);
                    hsv_to_rgb(hue, 1.0f, 1.0f, r, g, b);
                    break;
                }
                case COLOR_VELOCITY: {
                    float dx = (float)(next.x - p.x), dy = (float)(next.y - p.y);
                    float vel = sqrtf(dx*dx + dy*dy) * 1.5f;
                    float hue = fmodf(vel * cfg.hue_range + cfg.hue_start, 1.0f);
                    hsv_to_rgb(hue, 1.0f, 1.0f, r, g, b);
                    break;
                }
                case COLOR_ANGLE: {
                    float dx = (float)(next.x - p.x), dy = (float)(next.y - p.y);
                    float hue = fmodf((atan2f(dy, dx) / (2.0f*(float)M_PI) + 0.5f) * cfg.hue_range + cfg.hue_start, 1.0f);
                    hsv_to_rgb(hue, 1.0f, 1.0f, r, g, b);
                    break;
                }
                default: r = g = b = 1.0f;
            }
            glColor4f(r, g, b, cfg.alpha);
        }

        glVertex2f((float)next.x, (float)next.y);
        p = next;
    }
    glEnd();
}

static void draw_string(float x, float y, const char *s) {
    glRasterPos2f(x, y);
    for (const char *p = s; *p; p++)
        glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *p);
}

static void draw_hud() {
    if (!show_hud) return;

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, cfg.width, 0, cfg.height);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_BLEND);
    glColor3f(0.1f, 1.0f, 0.2f);

    char buf[256];
    float ty = (float)cfg.height - 14.0f;
    float tx = 6.0f;

    snprintf(buf, sizeof(buf), "Attractor: %s  [1-5 to switch]", attractor_names[(int)cfg.type]);
    draw_string(tx, ty, buf); ty -= 14.0f;

    snprintf(buf, sizeof(buf), "Color: %s  [c to cycle]", color_mode_names[(int)cfg.color_mode]);
    draw_string(tx, ty, buf); ty -= 14.0f;

    snprintf(buf, sizeof(buf), "a=%.5f  [Q/q]   b=%.5f  [W/w]", cfg.a, cfg.b);
    draw_string(tx, ty, buf); ty -= 14.0f;

    snprintf(buf, sizeof(buf), "c=%.5f  [E/e]   d=%.5f  [R/r]", cfg.c, cfg.d);
    draw_string(tx, ty, buf); ty -= 14.0f;

    snprintf(buf, sizeof(buf), "iter=%d  [+/-]   alpha=%.3f  [A/a]", cfg.iterations, cfg.alpha);
    draw_string(tx, ty, buf); ty -= 14.0f;

    draw_string(tx, ty, "H: toggle HUD   S: save frame   0: reset params   ESC: exit");

    glEnable(GL_BLEND);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

// ============================================================
// GLUT callbacks
// ============================================================

static void myinit() {
    glClearColor(cfg.bg_r, cfg.bg_g, cfg.bg_b, 1.0f);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glViewport(0, 0, cfg.width, cfg.height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-2.0f, 2.0f, -2.0f, 2.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_POINT_SMOOTH);
    glPointSize(1.0f);
    time0 = glutGet(GLUT_ELAPSED_TIME);
}

static void mydisplay() {
    if (!cfg.interactive && !paused)
        animation_update();

    draw_attractor();

    if (cfg.interactive)
        draw_hud();

    glutSwapBuffers();

    if (!cfg.interactive) {
        if (cfg.save_frames) {
            char filename[600];
            snprintf(filename, sizeof(filename), "%s/%05u.ppm", cfg.output_dir, nframes);
            screenshot_ppm(filename);
        }
        nframes++;
        if (nframes >= cfg.max_frames)
            exit(EXIT_SUCCESS);
    }
}

static void idle() {
    if (!paused)
        glutPostRedisplay();
}

static void mykey(unsigned char key, int /*mx*/, int /*my*/) {
    const double step = 0.02;

    switch (key) {
        case 27: exit(0);  // ESC

        // Switch attractor type (resets params to defaults for that type)
        case '1': case '2': case '3': case '4': case '5': {
            int idx = key - '1';
            cfg.type = (AttractorType)idx;
            cfg.a = a_init = attractor_defaults[idx][0];
            cfg.b = b_init = attractor_defaults[idx][1];
            cfg.c = c_init = attractor_defaults[idx][2];
            cfg.d = d_init = attractor_defaults[idx][3];
            break;
        }

        // Cycle color mode
        case 'c': case 'C':
            cfg.color_mode = (ColorMode)(((int)cfg.color_mode + 1) % COLOR_COUNT);
            break;

        // Param tweaks: uppercase = increase, lowercase = decrease
        case 'Q': cfg.a += step; a_init = cfg.a; break;
        case 'q': cfg.a -= step; a_init = cfg.a; break;
        case 'W': cfg.b += step; b_init = cfg.b; break;
        case 'w': cfg.b -= step; b_init = cfg.b; break;
        case 'E': cfg.c += step; c_init = cfg.c; break;
        case 'e': cfg.c -= step; c_init = cfg.c; break;
        case 'R': cfg.d += step; d_init = cfg.d; break;
        case 'r': cfg.d -= step; d_init = cfg.d; break;

        // Iterations
        case '+': case '=':
            cfg.iterations = std::min(cfg.iterations + 100000, 5000000); break;
        case '-':
            cfg.iterations = std::max(cfg.iterations - 100000, 10000); break;

        // Alpha
        case 'A': cfg.alpha = std::min(cfg.alpha + 0.005f, 1.0f); break;
        case 'a': cfg.alpha = std::max(cfg.alpha - 0.005f, 0.001f); break;

        // HUD toggle
        case 'h': case 'H': show_hud = !show_hud; break;

        // Save current frame
        case 's': case 'S': {
            char filename[600];
            snprintf(filename, sizeof(filename), "%s/frame_%05u.ppm", cfg.output_dir, nframes);
            screenshot_ppm(filename);
            fprintf(stderr, "Saved %s\n", filename);
            nframes++;
            break;
        }

        // Reset params to attractor defaults
        case '0':
            cfg.a = a_init = attractor_defaults[(int)cfg.type][0];
            cfg.b = b_init = attractor_defaults[(int)cfg.type][1];
            cfg.c = c_init = attractor_defaults[(int)cfg.type][2];
            cfg.d = d_init = attractor_defaults[(int)cfg.type][3];
            break;

        // Pause/resume (animation mode)
        case ' ': paused = !paused; break;
    }

    glutPostRedisplay();
}

static void deinit() {
    if (nframes > 0)
        printf("FPS = %.2f\n", 1000.0 * nframes / (double)(glutGet(GLUT_ELAPSED_TIME) - time0));
    free(pixels);
}

// ============================================================
// CLI parsing
// ============================================================

static void print_usage(const char *prog) {
    fprintf(stdout,
        "Usage: %s [OPTIONS]\n"
        "\n"
        "Options:\n"
        "  -W, --width N          Window/render width  (default: 1000)\n"
        "  -H, --height N         Window/render height (default: 1000)\n"
        "  -i, --iterations N     Iterations per frame (default: 2000000; interactive: 300000)\n"
        "  -f, --frames N         Frames for animation/export (default: 600)\n"
        "  -t, --type TYPE        Attractor type (default: kings_dream)\n"
        "                           kings_dream | clifford | de_jong | bedhead | svensson\n"
        "  -C, --color MODE       Color mode (default: mono)\n"
        "                           mono | hue_cycle | velocity | angle\n"
        "  -s, --save             Save each frame as PPM (use with --output)\n"
        "  -o, --output DIR       Output directory for PPM frames (default: .)\n"
        "  -I, --interactive      Interactive mode — live keyboard parameter control\n"
        "      --a FLOAT          Initial param a\n"
        "      --b FLOAT          Initial param b\n"
        "      --c FLOAT          Initial param c\n"
        "      --d FLOAT          Initial param d\n"
        "      --delta FLOAT      Animation oscillation amplitude (default: 0.1)\n"
        "      --alpha FLOAT      Point alpha transparency (default: 0.02)\n"
        "      --hue-start FLOAT  Starting hue, 0-1 (default: 0.0)\n"
        "      --hue-range FLOAT  Hue range, 0-1 (default: 1.0)\n"
        "      --mono-r FLOAT     Mono mode red channel   (default: 1.0)\n"
        "      --mono-g FLOAT     Mono mode green channel (default: 1.0)\n"
        "      --mono-b FLOAT     Mono mode blue channel  (default: 1.0)\n"
        "  -h, --help             Show this help\n"
        "\n"
        "Interactive mode keys:\n"
        "  1-5        Switch attractor type\n"
        "  c          Cycle color mode\n"
        "  Q/q        Increase/decrease a   W/w  b\n"
        "  E/e        Increase/decrease c   R/r  d\n"
        "  +/-        Increase/decrease iterations\n"
        "  A/a        Increase/decrease alpha\n"
        "  H          Toggle HUD overlay\n"
        "  S          Save current frame to output dir\n"
        "  0          Reset params to attractor defaults\n"
        "  Space      Pause/resume (animation mode)\n"
        "  ESC        Exit\n"
        "\n"
        "Video generation example:\n"
        "  mkdir frames\n"
        "  %s --save --output frames --frames 600 --color hue_cycle --width 1920 --height 1080\n"
        "  ffmpeg -r 30 -i frames/%%05d.ppm -c:v libx264 -pix_fmt yuv420p -crf 18 output.mp4\n"
        "\n",
        prog, prog);
}

static AttractorType parse_attractor_type(const char *s) {
    if (strcmp(s, "clifford")    == 0) return ATTRACTOR_CLIFFORD;
    if (strcmp(s, "de_jong")     == 0) return ATTRACTOR_DE_JONG;
    if (strcmp(s, "bedhead")     == 0) return ATTRACTOR_BEDHEAD;
    if (strcmp(s, "svensson")    == 0) return ATTRACTOR_SVENSSON;
    return ATTRACTOR_KINGS_DREAM;
}

static ColorMode parse_color_mode(const char *s) {
    if (strcmp(s, "hue_cycle") == 0) return COLOR_HUE_CYCLE;
    if (strcmp(s, "velocity")  == 0) return COLOR_VELOCITY;
    if (strcmp(s, "angle")     == 0) return COLOR_ANGLE;
    return COLOR_MONO;
}

// Long-option codes for options that have no short form
enum LongOptCode {
    OPT_A         = 300,
    OPT_B,
    OPT_C,
    OPT_D,
    OPT_DELTA,
    OPT_ALPHA,
    OPT_HUE_START,
    OPT_HUE_RANGE,
    OPT_MONO_R,
    OPT_MONO_G,
    OPT_MONO_B,
    OPT_BG_R,
    OPT_BG_G,
    OPT_BG_B,
};

static const struct option long_opts[] = {
    {"width",       required_argument, 0, 'W'},
    {"height",      required_argument, 0, 'H'},
    {"iterations",  required_argument, 0, 'i'},
    {"frames",      required_argument, 0, 'f'},
    {"type",        required_argument, 0, 't'},
    {"color",       required_argument, 0, 'C'},
    {"save",        no_argument,       0, 's'},
    {"output",      required_argument, 0, 'o'},
    {"interactive", no_argument,       0, 'I'},
    {"a",           required_argument, 0, OPT_A},
    {"b",           required_argument, 0, OPT_B},
    {"c",           required_argument, 0, OPT_C},
    {"d",           required_argument, 0, OPT_D},
    {"delta",       required_argument, 0, OPT_DELTA},
    {"alpha",       required_argument, 0, OPT_ALPHA},
    {"hue-start",   required_argument, 0, OPT_HUE_START},
    {"hue-range",   required_argument, 0, OPT_HUE_RANGE},
    {"mono-r",      required_argument, 0, OPT_MONO_R},
    {"mono-g",      required_argument, 0, OPT_MONO_G},
    {"mono-b",      required_argument, 0, OPT_MONO_B},
    {"bg-r",        required_argument, 0, OPT_BG_R},
    {"bg-g",        required_argument, 0, OPT_BG_G},
    {"bg-b",        required_argument, 0, OPT_BG_B},
    {"help",        no_argument,       0, 'h'},
    {0, 0, 0, 0}
};

static bool parse_args(int argc, char **argv) {
    bool iterations_set = false;
    int opt, idx = 0;

    while ((opt = getopt_long(argc, argv, "W:H:i:f:t:C:so:Ih", long_opts, &idx)) != -1) {
        switch (opt) {
            case 'W': cfg.width        = (unsigned)atoi(optarg); break;
            case 'H': cfg.height       = (unsigned)atoi(optarg); break;
            case 'i': cfg.iterations   = atoi(optarg); iterations_set = true; break;
            case 'f': cfg.max_frames   = (unsigned)atoi(optarg); break;
            case 't': {
                cfg.type = parse_attractor_type(optarg);
                // Apply type defaults (user a/b/c/d overrides applied below)
                cfg.a = attractor_defaults[(int)cfg.type][0];
                cfg.b = attractor_defaults[(int)cfg.type][1];
                cfg.c = attractor_defaults[(int)cfg.type][2];
                cfg.d = attractor_defaults[(int)cfg.type][3];
                break;
            }
            case 'C': cfg.color_mode   = parse_color_mode(optarg); break;
            case 's': cfg.save_frames  = true; break;
            case 'o': snprintf(cfg.output_dir, sizeof(cfg.output_dir), "%s", optarg); break;
            case 'I': cfg.interactive  = true; break;
            case OPT_A: cfg.a = atof(optarg); cfg.override_a = true; break;
            case OPT_B: cfg.b = atof(optarg); cfg.override_b = true; break;
            case OPT_C: cfg.c = atof(optarg); cfg.override_c = true; break;
            case OPT_D: cfg.d = atof(optarg); cfg.override_d = true; break;
            case OPT_DELTA:     cfg.delta     = atof(optarg); break;
            case OPT_ALPHA:     cfg.alpha     = (float)atof(optarg); break;
            case OPT_HUE_START: cfg.hue_start = (float)atof(optarg); break;
            case OPT_HUE_RANGE: cfg.hue_range = (float)atof(optarg); break;
            case OPT_MONO_R:    cfg.mono_r    = (float)atof(optarg); break;
            case OPT_MONO_G:    cfg.mono_g    = (float)atof(optarg); break;
            case OPT_MONO_B:    cfg.mono_b    = (float)atof(optarg); break;
            case OPT_BG_R:      cfg.bg_r      = (float)atof(optarg); break;
            case OPT_BG_G:      cfg.bg_g      = (float)atof(optarg); break;
            case OPT_BG_B:      cfg.bg_b      = (float)atof(optarg); break;
            case 'h': print_usage(argv[0]); return false;
            default:  print_usage(argv[0]); return false;
        }
    }

    if (cfg.interactive && !iterations_set)
        cfg.iterations = 300000;

    return true;
}

// ============================================================
// main
// ============================================================

int main(int argc, char **argv) {
    // Parse our args BEFORE glutInit (which may consume argv entries)
    if (!parse_args(argc, argv))
        return 0;

    // Store animation baseline (used by animation_update sinusoid)
    a_init = cfg.a; b_init = cfg.b; c_init = cfg.c; d_init = cfg.d;

    if (cfg.save_frames) {
        printf("Saving frames to: %s/\n", cfg.output_dir);
        printf("After rendering, stitch with:\n");
        printf("  ffmpeg -r 30 -i %s/%%05d.ppm -c:v libx264 -pix_fmt yuv420p -crf 18 output.mp4\n\n",
               cfg.output_dir);
    }

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
    glutInitWindowSize((int)cfg.width, (int)cfg.height);
    glutCreateWindow("Strange Attractor Visualizer");
    glutDisplayFunc(mydisplay);
    glutIdleFunc(idle);
    atexit(deinit);
    glutKeyboardFunc(mykey);
    myinit();
    glutMainLoop();

    return 0;
}
