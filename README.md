# Cmos2Tetris

The Hack computer is based on a minimalist 16-bit single-cycle CPU, with separate ROM and RAM, and a memory-mapped screen and keyboard. The system is described in the [Nand2Tetris](https://www.nand2tetris.org) site, and the accompanying [book](https://www.nand2tetris.org/book). There are excellent [Coursera](https://www.coursera.org/learn/build-a-computer) courses based on the same material. The genius of the design is that it keeps everything as simple as possible but no simpler, so this is a non-trivial computer on which you can play Tetris, but still hold the hardware design in your head and visualise it working end to end.

This repo has several experiments based on the Hack architecture and extensions thereof, in various stages of planning and completion.
  * Hack emulator in C
  * Cmos2Tetris: Physical breadboard implementation with 74HC logic ICs
  * Hack++: an extension to expand memory capacity and interrupts, _without any change_ to the CPU and instruction set. This will potentially enable porting of Fuzix and other 16-bit operating systems to the Hack++ platform.
  
  
  ## Hack emulator in C
  The "official" Java Hack emulator is sufficient for the purposes of the course, but it is rather slow, and I was curious to know how fast a C implementation could go. The answer is, pretty fast, around 25MHz (25 million instructions per second) on a Macbook with 2.2GHz i7. Too fast for typical test programs like Pong, which feel more at home at around 2-4MHz.
  
  ### Installation
  Instructions apply for MacOS, but should apply with little modification to Linux. The only external dependency is SDL2.
  `brew install sdl2`
  Run `make` to compile `hackcpu` and `hackio`. `hackcpu` is a version without any IO support, and is useful mainly for testing.
  ### Usage
  ```
  hackio [-c maxcycles] [-r initialram] [-s MHz] [-d] [-i] prog.hack
   -c maxcycles    Number of clock cycles to run. Defaults to 0, runs indefinitely.
   -r initialram   Binary dump file to be loaded into RAM before starting the computer.
   -s MHz          Approximate CPU speed in MHz, defaults to 2. 0 runs as fast as possible.
   -d              Debug mode
   -i              Ignore (memory) errors and continue
   prog.hack       Hack machine language file
   ```
   
   Run the supplied Pong.hack (part of the [Nand2Tetris project files](https://www.nand2tetris.org/course) under [CC BY-NC-SA 3.0](https://www.nand2tetris.org/license)
  
  `./hackio Pong.hack`
  
  Close the window to stop the emulator. To run it at full speed,
  
  `./hackio -s 0 Pong.hack`
  
  
  ### TODO
  No tests!
  
  ## Hack emulator in Python
  Fairly slow, left here for reference. All future work will be done on the C 
  ## Cmos2Tetris, or, Hack on the Breadboard
  No schematics, haphazard implementation in progress
  ## Hack++
  Pie in the sky for now
