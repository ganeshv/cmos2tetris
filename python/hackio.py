import time
import pygame
from pygame.locals import *
import argparse
from hackcpu import Computer

FPS = 40
WIDTH = 512
HEIGHT = 256
SCREEN_BASE = 16384
SCREEN_SIZE = 8192
KBD = 24576
MAXCYCLES = 100

def gameloop(computer, debug=False):
    running = True

    screen = pygame.display.set_mode([WIDTH, HEIGHT])
    screen.fill((255, 0, 255))
    pygame.display.flip()

    shadow = pygame.Surface([WIDTH, HEIGHT])

    computer.reset()
    last = time.perf_counter()
    fps_interval = 1 / FPS
    screen_dirty = False
    screen_top = SCREEN_BASE + SCREEN_SIZE
    row_bytes = WIDTH // 16

    pixels = pygame.PixelArray(shadow)
    pixels[:,:] = 0xFFFFFF
    while running:
        while time.perf_counter() - last < fps_interval:
            c = 0
            while c < MAXCYCLES:
                c += 1
                ALUout, writeM, A = computer.ticktock(debug=debug)
                if writeM and A >= SCREEN_BASE and A < screen_top:
                    off = A - SCREEN_BASE
                    row = off // row_bytes
                    col = (off % row_bytes) * 16
                    screen_dirty = True
                    p = 0
                    while p < 16:
                        pixels[col + p, row] = 0x000000 if ALUout & (1 << p) else 0xFFFFFF
                        p += 1

        last = time.perf_counter()
        if screen_dirty:
            del pixels
            screen.blit(shadow, (0, 0))
            pygame.display.flip()
            pixels = pygame.PixelArray(shadow)
            screen_dirty = False

        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
            if event.type == pygame.KEYDOWN:
                computer.mmio_set(KBD, keymap(event.key))
            if event.type == pygame.KEYUP:
                computer.mmio_set(KBD, 0)

KEYMAP = {
    K_RETURN: 128,
    K_BACKSPACE: 129,
    K_LEFT: 130,
    K_UP: 131,
    K_RIGHT: 132,
    K_DOWN: 133,
    K_HOME: 134,
    K_END: 135,
    K_PAGEUP: 136,
    K_PAGEDOWN: 137,
    K_INSERT: 138,
    K_DELETE: 139,
    K_ESCAPE: 140
}

def keymap(key):
    out = 0
    if (key >= 32 and key <= 64) or\
        (key >= 91 and key <= 95):
        out = key
    elif key >= K_a and key <= K_z:
        out = key - K_a + 65
    elif key >= K_F1 and key <= K_F12:
        out = key - K_F1 + 141
    elif key in KEYMAP:
        out = KEYMAP[key]

    return out

def main(args):
    c = Computer()
    c.loadrom(args.hack)
    if args.ram:
        c.loadram(args.ram)
    pygame.init()
    gameloop(c, debug=args.debug)
    pygame.quit()

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('hack', help="run the .hack file")
    parser.add_argument('-d', '--debug', help='verbose debugging', action='store_true')
    parser.add_argument('-c', '--cycles', help='maximum number of cycles to run', type=int, default=10000)
    parser.add_argument('-r', '--ram', help='load ram from dump')
    args = parser.parse_args()
    main(args)
