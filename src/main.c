#include "chronotask.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
    const char* config_file = "config.yaml";
    
    if (argc > 1) {
        config_file = argv[1];
    }
    
    printf("Starting ChronoTask...\n");
    int result = run_chronotask(config_file);
    printf("ChronoTask exited with result: %d\n", result);
    return result;
}

