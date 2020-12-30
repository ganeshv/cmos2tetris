#ifndef _HACKCPU_H
#define _HACKCPU_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint16_t A;
    uint16_t D;
    uint16_t PC;
} CPU;

typedef struct {
    uint16_t *rom;
    uint16_t *ram;
    int romsize;
    int ramsize;
    CPU cpu;

    unsigned long long cycles;
    uint16_t PC;
    uint16_t A;
} Computer;

#define ERR_READ_ROM 0x1
#define ERR_READ_RAM 0x2
#define ERR_WRITE_RAM 0x4

void cpu_init(CPU *cpu);
void cpu_tick(CPU *cpu, uint16_t instruction, uint16_t inM, bool reset, uint16_t *addressM, uint16_t *outM, bool *writeM);
void cpu_tock(CPU *cpu, uint16_t *PC, uint16_t *addressM);
void ALU(uint16_t x, uint16_t y, uint16_t comp, uint16_t *out, bool *zr, bool *ng);

void computer_init(Computer *c, int romsize, int ramsize);
void computer_loadrom(Computer *c, char *filename);
void computer_loadram(Computer *c, char *filename);
void computer_reset(Computer *c);
int computer_ticktock(Computer *c, bool debug, uint16_t *addressM, uint16_t *outM, bool *writeM);
uint16_t computer_mmio_get(Computer *c, uint16_t addr);
void computer_mmio_set(Computer *c, uint16_t addr, uint16_t val);
void computer_reset(Computer *c);
void computer_run(Computer *c, int maxcycles, char *ramdump, bool debug, bool ignore);

void dump(char *filename, uint16_t *data, int size);
#endif
