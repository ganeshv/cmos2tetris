import argparse

class CPU():

    def __init__(self):
        self.A = 0
        self.D = 0

        self.PC = 0

    def tick(self, instruction, inM, reset):
        a_instr = instruction & 0x8000 == 0
        c_instr = not a_instr
        jmp = instruction & 0b111
        dst = (instruction & 0b111000) >> 3
        comp = (instruction & 0b111111000000) >> 6
        loadD = c_instr and dst & 0b010 != 0
        writeM = c_instr and dst & 0b001 != 0
        loadPC = False

        y = inM if instruction & 0b0001000000000000 else self.A
        ALUout, zr, ng = ALU(x=self.D, y=y, comp=comp)
        zr1 = 1 if zr else 0
        ng1 = 1 if ng else 0

        if c_instr:
            if (jmp & 0b100 and ng) or \
                (jmp & 0b010 and zr) or \
                (jmp & 0b001 and zr == False and ng == False):
                #print(f'JMP! {jmp:04x} {ALUout:04x} {zr1:04x} {ng1:04x}');
                loadPC = True

        oldA = self.A
        if a_instr:
            self.A = instruction
        elif dst & 0b100:
            self.A = ALUout

        if loadD:
            self.D = ALUout

        if reset:
            self.PC = 0
        elif loadPC:
            self.PC = oldA
        else:
            self.PC = (self.PC + 1) % 65536

        return ALUout, writeM, oldA

    def tock(self):
        return self.PC, self.A

def ALU(x, y,  comp):

    if comp & 0b100000:
        x = 0
    if comp & 0b010000:
        x = ~x & 0xffff
    if comp & 0b001000:
        y = 0
    if comp & 0b000100:
        y = ~y & 0xffff
    out = (x + y) & 0xffff if comp & 0b000010 else x & y & 0xffff
    if comp & 0b000001:
        out = ~out & 0xffff
    zr = out == 0
    ng = out & 0x8000

    return out, zr, ng

def dump(b, filename):
    with open(filename, 'wb') as f:
        f.write(b)
        
class Computer():

    def __init__(self):

        self.rom = bytearray(32 * 1024 * 2)
        self.ram = bytearray(32 * 1024 * 2)
        self.cpu = CPU()

        self.cycles = 0
        self.PC = 0
        self.A = 0

    def loadrom(self, filename):
        with open(filename, 'r') as f:
            i = 0
            for line in f:
                self.rom[i] = int(line[:8], 2)
                self.rom[i + 1] = int(line[8:], 2)
                i = i + 2
        dump(self.rom, 'rom.dump')

    def loadram(self, filename):
        with open(filename, 'rb') as f:
            ramdump = bytes(f.read())
            self.ram[:len(ramdump)] = ramdump

    def reset(self):
        self.cycles = 0
        ALUout, writeM, oldA = self.cpu.tick(instruction=0, inM=0, reset=True)
        self.PC, self.A = self.cpu.tock()

    def mmio_get(self, addr):
        return self.ram[addr * 2] << 8 | self.ram[addr * 2 + 1]

    def mmio_set(self, addr, val):
        self.ram[addr * 2] = (val & 0xffff) >> 8
        self.ram[addr * 2 + 1] = val & 0xff

    def ticktock(self, debug=False):
        instruction = self.rom[self.PC * 2] << 8 | self.rom[self.PC * 2 + 1]
        inM = self.ram[self.A * 2] << 8 | self.ram[self.A * 2 + 1]
        ALUout, writeM, oldA = self.cpu.tick(instruction, inM, reset=False)
        if debug:
            print(f'cycle {self.cycles} PC {self.PC:04x} instruction {instruction:04x} A {self.A:04x} inM {inM:04x}')
        if writeM:
            self.ram[oldA * 2] = (ALUout & 0xffff) >> 8
            self.ram[oldA * 2 + 1] = ALUout & 0xff
            if debug:
                print(f'writeM addressM {oldA:04x} outM {ALUout:04x}')
        self.PC, self.A = self.cpu.tock()
        if debug:
            print(f'tock A {self.A:04x} D {self.cpu.D:04x} nextPC {self.PC:04x}\n')
        self.cycles = self.cycles + 1
        return ALUout, writeM, oldA

    def run(self, maxcycles=1000, ramdump='ram.dump', debug=False):
        self.reset()
        while self.cycles < maxcycles:
            self.ticktock(debug=debug)

        dump(self.ram, ramdump)


def main(args):
    c = Computer()
    c.loadrom(args.hack)
    if args.ram:
        c.loadram(args.ram)
    c.run(maxcycles=args.cycles, debug=args.debug)
    
if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('hack', help="run the .hack file")
    parser.add_argument('-d', '--debug', help='verbose debugging', action='store_true')
    parser.add_argument('-c', '--cycles', help='maximum number of cycles to run', type=int, default=10000)
    parser.add_argument('-r', '--ram', help='load ram from dump')
    args = parser.parse_args()
    main(args)
