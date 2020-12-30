#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hackcpu.h"

/*
 * Hack CPU emulator. As described in https://www.nand2tetris.org/
 * A minimalist Harvard architecture single-cycle 16-bit CPU created
 * for the Nand2Tetris course.
 */

void
cpu_init(CPU *cpu) {
    cpu->A = 0;
    cpu->D = 0;
    cpu->PC = 0;
}

/*
 * 'Tick' is the rising edge of the clock. We commit all changes to be made
 * to CPU and memory state here.
 *
 * Inputs
 * instruction: contents of the location specified by the program counter
 * inM: contents of the location specified by the A register
 * reset: CPU reset
 *
 * Outputs
 * Return value: boolean specifying whether there was a memory write
 * addressM: Address of the write
 * outM: Value to be written
 */
void
cpu_tick(CPU *cpu, uint16_t instruction, uint16_t inM, bool reset, uint16_t *addressM, uint16_t *outM, bool *writeM) {
    bool a_instr = (instruction & 0x8000) == 0;
    bool c_instr = !a_instr;
    uint16_t jmp = instruction & 0x7;
    uint16_t dst = (instruction & 0x38) >> 3;
    uint16_t comp = (instruction & 0xfc0) >> 6;
    bool loadD = c_instr && (dst & 0x2) != 0;
    bool w = c_instr && (dst & 0x1) != 0;
    bool loadPC = false;
    bool zr = false;
    bool ng = false;
    uint16_t out;

    uint16_t y = (instruction & (1 << 12)) ? inM : cpu->A;
    ALU(cpu->D, y, comp, &out, &zr, &ng);

    if (c_instr) {
        if ((jmp & 0x4 && ng) || (jmp & 0x2 && zr) || (jmp & 0x1 && !zr && !ng)) {
            loadPC = true;
            //printf("JMP! %04hx %04x %04hx %04hx\n", jmp, out, (unsigned short)zr, (unsigned short)ng);
        }
    }

    uint16_t oldA = cpu->A;
    if (a_instr) {
        cpu->A = instruction;
    } else if (dst & 0x4) {
        cpu->A = out;
    }

    if (loadD) {
        cpu->D = out;
    }

    if (reset) {
        cpu->PC = 0;
    } else if (loadPC) {
        cpu->PC = oldA;
    } else {
        cpu->PC += 1;
    }

    *outM = out;
    *addressM = oldA;
    *writeM = w;
}

/*
 * 'Tock' is the falling edge of the clock. We output the PC and A for the next cycle
 *
 * Inputs: none
 *
 * Outputs
 * PC: next instruction to be fetched from ROM
 * addressM: address of next data to be fetched from RAM 
 */
void
cpu_tock(CPU *cpu, uint16_t *PC, uint16_t *addressM) {
    *PC = cpu->PC;
    *addressM = cpu->A;
}

/*
 * ALU
 *
 * Inputs
 * x, y: 
 * comp: controls what happens to x and y and how they are combined
 *
 * Outputs
 * ALUOut: result of computation
 * zr: Flag indicating whether output is 0
 * ng: Flag indicating whether output is negative
 */
void
ALU(uint16_t x, uint16_t y, uint16_t comp, uint16_t *ALUout, bool *zr, bool *ng) {
    uint16_t out;

    if (comp & 0x20) {
        x = 0;
    }
    if (comp & 0x10) {
        x = ~x;
    }
    if (comp & 0x8) {
        y = 0;
    }
    if (comp & 0x4) {
        y = ~y;
    }
    out = (comp & 0x2) ? x + y : x & y;
    if (comp & 0x1) {
        out = ~out;
    }
    *ALUout = out;
    *zr = (out == 0) ? true : false;
    *ng = (out & 0x8000) ? true : false;
}

/*
 * computer_* provide the rest of the Hack computer outside the CPU - RAM, ROM, 
 * etc. External IO is not handled here, we provide helper routines for
 * higher layers to do so.
 */

void
computer_init(Computer *c, int romsize, int ramsize) {
    c->rom = (uint16_t *)calloc(romsize, sizeof (uint16_t));
    c->ram = (uint16_t *)calloc(ramsize, sizeof (uint16_t));
    c->romsize = romsize;
    c->ramsize = ramsize;
    cpu_init(&c->cpu);
    c->cycles = 0;
    c->PC = 0;
    c->A = 0;
}

void
computer_reset(Computer *c) {
    uint16_t discard;
    bool writeM;

    c->cycles = 0;

    cpu_tick(&c->cpu, 0, 0, true, &discard, &discard, &writeM);
    cpu_tock(&c->cpu, &c->PC, &c->A);
}


/*
 * Run one clock cycle. In the tick phase, we receive memory to be written, if any.
 * In the tock phase, we get the addresses of the next instruction and data to be fetched.
 *
 * We also propagate memory writes to the caller, who may snoop on writes to implement the
 * video framebuffer.
 */

int
computer_ticktock(Computer *c, bool debug, uint16_t *addressM, uint16_t *outM, bool *writeM) {
    bool w = false;
    uint16_t instruction = 0;
    uint16_t inM = 0;
    uint16_t addr, out;
    int error = 0;

    if (c->PC < c->romsize) {
        instruction = c->rom[c->PC];
    } else {
        fprintf(stdout, "Invalid instruction read from %04hx\n!", c->PC);
        error |= ERR_READ_ROM;
    }

    if (c->A < c->ramsize) {
        inM = c->ram[c->A];
    } else {
        fprintf(stdout, "Invalid read from %04hx!\n", c->A);
        error |= ERR_READ_RAM;
    }

    cpu_tick(&c->cpu, instruction, inM, false, &addr, &out, &w);
    if (debug) {
        fprintf(stdout, "cycle %llu PC %04hx instruction %04hx A %04hx inM %04hx\n", c->cycles, c->PC, instruction, c->A, inM);
    }
    if (w) {
        if (addr < c->ramsize) {
            c->ram[addr] = out;
        } else {
            fprintf(stdout, "Invalid write to %04hx!\n", addr);
            error |= ERR_READ_RAM;
        }
        if (debug) {
            fprintf(stdout, "writeM addressM %04hx outM %04hx\n", addr, out);
        }
    }

    cpu_tock(&c->cpu, &c->PC, &c->A);
    if (debug) {
        fprintf(stdout, "tock A %04hx D %04hx nextPC %04hx\n\n", c->A, c->cpu.D, c->PC);
    }
    c->cycles += 1;
    *addressM = addr;
    *outM = out;
    *writeM = w;
    return error;
}

/*
 * Helper functions to enable memory mapped IO
 */

uint16_t
computer_mmio_get(Computer *c, uint16_t addr) {
    if (addr < c->ramsize) {
        return c->ram[addr];
    }
    fprintf(stderr, "computer_mmio_get: %04hx beyond RAM size\n", addr);
    return 0;
}

void
computer_mmio_set(Computer *c, uint16_t addr, uint16_t val) {
    if (addr < c->ramsize) {
        c->ram[addr] = val;
    } else {
        fprintf(stderr, "computer_mmio_set: %04hx beyond RAM size\n", addr);
    }
}

void
computer_run(Computer *c, int maxcycles, char *ramdump, bool debug, bool ignore) {
    uint16_t discard;
    bool writeM;
    int err;

    computer_reset(c);
    while (!maxcycles || c->cycles < maxcycles) {
        err = computer_ticktock(c, debug, &discard, &discard, &writeM);
        if (err && !ignore) {
            fprintf(stderr, "Exiting on error %d\n", err);
            exit(1);
        }
    }

    dump(ramdump, c->ram, c->ramsize);
}

/*
 * Load ROM from a text file containing hack machine language
 * Each line is a 16-bit string of 1s and 0s representing an instruction.
 */

void
computer_loadrom(Computer *c, char *filename) {
    FILE *fp;
    char line[100];
    char *err;
    int address = 0;
    unsigned long instruction;

    if ((fp = fopen(filename, "r")) == NULL) {
        fprintf(stderr, "Could not open ROM file %s", filename);
        exit(2);
    }

    while (fgets(line, 100, fp)) {
        line[strcspn(line, "\r\n")] = '\0';
        instruction = strtoul(line, &err, 2);
        if (*err != '\0') {
            fprintf(stderr, "Invalid input in ROM, line %d: %s, starting from %s\n", address + 1, line, err);
            exit(2);
        }
        c->rom[address] = instruction & 0xffff;
        address++;
    }

    dump("rom.dump", c->rom, c->romsize);
}

/*
 * Load RAM from a binary dump file. May be useful to initialise RAM (and R0-R16).
 * This is a simple dump of 16-bit words, must have been created on the same (endianness) machine.
 */

void
computer_loadram(Computer *c, char *filename) {
    FILE *fp;

    if ((fp = fopen(filename, "r")) == NULL) {
        fprintf(stderr, "Could not open RAM file %s", filename);
        exit(2);
    }
    int nread = fread(c->ram, sizeof (uint16_t), c->ramsize, fp);
    if (nread != c->ramsize) {
        fprintf(stderr, "Error: read %d of %d items from %s\n", nread, c->ramsize, filename);
    }
}

/*
 * Dump an array of 16-bit words to a file. No attempt to make it endian-neutral.
 */

void
dump(char *filename, uint16_t *data, int size) {
    FILE *fp;

    if ((fp = fopen(filename, "w")) == NULL) {
        fprintf(stderr, "Could not open dump file %s", filename);
        exit(2);
    }
    int written = fwrite(data, sizeof (uint16_t), size, fp);
    if (written != size) {
        fprintf(stderr, "Error: wrote %d of %d items to %s\n", written, size, filename);
    }
}
