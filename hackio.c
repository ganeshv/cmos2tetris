#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/param.h>
#include <string.h>
#include "hackcpu.h"
#include "SDL.h"

/*
 * Provide video and keyboard support to the Hack computer
 * Snoop on video writes to RAM and propagate the writes to an SDL texture
 *
 * (Approximately) control the rate at which Hack instructions are executed,
 * defaulting to about 2 MIPS, or 2 MHz. Otherwise it is _too_ fast.
 */

int FPS = 60; // frames per second
int KIPS = 2000; // thousand instructions per second, defaults to 2 MIPS
int WIDTH = 512; // screen width
int HEIGHT = 256; // screen height
uint16_t SCREEN_BASE = 16384; // Framebuffer address
uint16_t SCREEN_SIZE = 8192; // Screen size in 16-bit words, 1 bit per pixel
uint16_t KBD = 24576; // Keyboard address
int MAXCYCLES = 0; // Maximum number of cycles to run the CPU, 0 means keep running
bool DEBUG = false;
bool IGNORE = false; // Ignore (memory) errors and continue

char initram[MAXPATHLEN] = "";
char hackfile[MAXPATHLEN] = "";

void process_args(int argc, char **argv);
void usage();
uint16_t keymap(SDL_Keycode key);

void show(SDL_Renderer *renderer, SDL_Texture *texture, Uint32 *pixels) {
    SDL_RenderClear(renderer);
    SDL_UpdateTexture(texture, NULL, pixels, WIDTH * sizeof (Uint32));
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

void gameloop(Computer *c, bool debug, bool ignore) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL init failed: %s\n", SDL_GetError());
        exit(1);
    }
    SDL_Window *window;
    SDL_Renderer *renderer;
    if (SDL_CreateWindowAndRenderer(WIDTH, HEIGHT, 0, &window, &renderer)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window and renderer: %s", SDL_GetError());
        exit(1);
    }

    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
    Uint32 *pixels = calloc(WIDTH * HEIGHT, 4);
    for (int i = 0; i < WIDTH * HEIGHT; i++) {
        pixels[i] = 0xffffff00;
    }
    show(renderer, texture, pixels);

    bool running = true;

    SDL_Event event;
    Uint64 perf_freq = SDL_GetPerformanceFrequency();
    Uint64 begin = SDL_GetPerformanceCounter();
    Uint64 last_frame = begin;
    Uint64 last_instr = begin;
    Uint64 fps_interval = perf_freq / FPS;
    Uint64 kips_interval = KIPS ? perf_freq / (KIPS * 1000) : 0;
    Uint64 delta;
    bool screen_dirty = false;

    uint16_t addressM, outM;
    bool writeM;
    int err;

    computer_reset(c);
    while (running && (!MAXCYCLES || c->cycles < MAXCYCLES)) {
        Uint64 now = SDL_GetPerformanceCounter();
        if (kips_interval && now - last_instr < kips_interval) {
            continue;
        }
        err = computer_ticktock(c, debug, &addressM, &outM, &writeM);
        if (err && !ignore) {
            fprintf(stderr, "Exiting after error %d\n", err);
            goto out;
        }
        if (writeM && addressM >= SCREEN_BASE && addressM < SCREEN_BASE + SCREEN_SIZE ) {
            uint16_t off = addressM - SCREEN_BASE;
            uint16_t row = off / (WIDTH / 16);
            uint16_t col = (off % (WIDTH / 16)) * 16;
            for (int bit = 0; bit < 16; bit++) {
                pixels[row * WIDTH + col + bit] = (outM & (1 << bit)) ? 0x0 : 0xffffffff;
            }
            screen_dirty = true;
        }
        last_instr = now;
        if (now - last_frame > fps_interval) {
            if (screen_dirty) {
                show(renderer, texture, pixels);
                screen_dirty = false;
            }
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    running = false;
                }
                if (event.type == SDL_KEYDOWN) {
                    computer_mmio_set(c, KBD, keymap(event.key.keysym.sym));
                }
                if (event.type == SDL_KEYUP) {
                    computer_mmio_set(c, KBD, 0);
                }
            }
            last_frame = now;
        }
    }

out:
    delta = SDL_GetPerformanceCounter() - begin;
    fprintf(stderr, "%llu cycles %f MIPS\n", c->cycles, (double)c->cycles / 1000000.0 / ((double)delta / (double)perf_freq));

    SDL_DestroyWindow(window);
    SDL_Quit();
}

uint16_t keymap(SDL_Keycode key) {
    if ((key >= 32 && key <= 64) ||
        (key >= 91 && key <= 95)) {
        return key;
    }
    if (key >= SDLK_a && key <= SDLK_z) {
        return key - SDLK_a + 65;
    }
    if (key >= SDLK_F1 && key <= SDLK_F12) {
        return key - SDLK_F1 + 141;
    }
    uint16_t out;
    switch (key) {
        case SDLK_RETURN:
            out = 128;
            break;
        case SDLK_BACKSPACE:
            out = 129;
            break;
        case SDLK_LEFT:
            out = 130;
            break;
        case SDLK_UP:
            out = 131;
            break;
        case SDLK_RIGHT:
            out = 132;
            break;
        case SDLK_DOWN:
            out = 133;
            break;
        case SDLK_HOME:
            out = 134;
            break;
        case SDLK_END:
            out = 135;
            break;
        case SDLK_PAGEUP:
            out = 136;
            break;
        case SDLK_PAGEDOWN:
            out = 137;
            break;
        case SDLK_INSERT:
            out = 138;
            break;
        case SDLK_DELETE:
            out = 139;
            break;
        case SDLK_ESCAPE:
            out = 140;
            break;
        default:
            out = 0;
            break;
    }
    return out;
}

int
main(int argc, char **argv) {
    Computer c;

    process_args(argc, argv);

    computer_init(&c, 32 * 1024, 32 * 1024);
    computer_loadrom(&c, hackfile);
    if (strnlen(initram, MAXPATHLEN) > 0) {
        computer_loadram(&c, initram); 
    }
    gameloop(&c, DEBUG, IGNORE);
}

void
process_args(int argc, char **argv) {
    int ch;

    while ((ch = getopt(argc, argv, "dic:r:s:")) != -1) {
        switch (ch) {
            case 'd':
                DEBUG = true;
                break;
            case 'i':
                IGNORE = true;
                break;
            case 'c':
                MAXCYCLES = atoi(optarg);
                break;
            case 'r':
                strncpy(initram, optarg, MAXPATHLEN);
                break;
            case 's':
                KIPS = atof(optarg) * 1000;
                break;
            default:
                usage();
                break;
        }
    }
    argc -= optind;
    argv += optind;

    if (argc != 1) {
        usage();
    }
    strncpy(hackfile, argv[0], MAXPATHLEN);
}

void usage() {
    fprintf(stderr, "hackio [-c maxcycles] [-r initialram] [-s MHz] [-d] [-i] prog.hack\n");
    fprintf(stderr, "   -c maxcycles    Number of clock cycles to run. Defaults to 0, runs indefinitely.\n");
    fprintf(stderr, "   -r initialram   Binary dump file to be loaded into RAM before starting the computer.\n");
    fprintf(stderr, "   -s MHz          Approximate CPU speed in MHz, defaults to 2. 0 runs as fast as possible.\n");
    fprintf(stderr, "   -d              Debug mode\n");
    fprintf(stderr, "   -i              Ignore (memory) errors and continue\n");
    fprintf(stderr, "   prog.hack       Hack machine language file\n");
    exit(1);
}
