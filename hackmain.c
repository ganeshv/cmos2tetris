#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/param.h>
#include <string.h>
#include "hackcpu.h"

bool debug = false;
bool ignore = false;
int maxcycles = 0;
char initram[MAXPATHLEN] = "";
char hackfile[MAXPATHLEN] = "";

void process_args(int argc, char **argv);
void usage();

int
main(int argc, char **argv) {
    Computer c;

    process_args(argc, argv);

    computer_init(&c, 32 * 1024, 32 * 1024);
    computer_loadrom(&c, hackfile);
    if (strnlen(initram, MAXPATHLEN) > 0) {
        computer_loadram(&c, initram); 
    }
    computer_run(&c, maxcycles, "ram.dump", debug, ignore);
}


void
process_args(int argc, char **argv) {
    int ch;

    while ((ch = getopt(argc, argv, "dic:r:")) != -1) {
        switch (ch) {
            case 'd':
                debug = true;
                break;
            case 'i':
                ignore = true;
                break;
            case 'c':
                maxcycles = atoi(optarg);
                break;
            case 'r':
                strncpy(initram, optarg, MAXPATHLEN);
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
    fprintf(stderr, "hackcpu [-c maxcycles] [-r initialram] [-d] [-i] prog.hack\n");
    exit(1);
}
