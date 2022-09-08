#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include "pmake.h"

/* This program reads and evaluates a Makefile very much like the program make
   but with some simplifications, and the option to evaluate dependencies in 
   parallel.

   Options:
     "-o"  - print the rules data structure as if it were a Makefile
     "-p"  - create a new process to evaluate each dependency of a rule
     "-f <filename>" - Use "filename" as the input Makefile.  If this option is
             not provided then pmake will use a file called "Makefile"
     "target" - specify the rule in the file to evaluate. If no target is
             provided, pmake will evaluate the first rule.
 */
  //IM SORRY IF I CHANGED SOMETHING I DIDNT MEAN TO

int main(int argc, char **argv) {
    FILE *fp;
    char *filename = "Makefile";
    char *target = NULL;
    int output = 0;
    int parallel = 0;
    char opt;

    while ((opt = getopt(argc, argv, "f:op")) != -1) {
        switch (opt) {
        case 'o':
            output = 1;
            break;
        case 'p':
            parallel = 1;
            break;
        case 'f':
            filename = optarg;
            break;
        default: /* '?' */
            fprintf(stderr, "Usage: %s [-op][-f <filename>] [target]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if(optind < argc) {
        target = argv[optind];
    }
    if((fp = fopen(filename, "r")) == NULL) {
        perror("fopen");
    }

    Rule *rules = parse_file(fp);
    if(output) {
        print_rules(rules);
    }

    run_make(target, rules, parallel);
    
    fclose(fp);

    return 0;
}
