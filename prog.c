#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

/******************************************************************************
 * Global variables
 ******************************************************************************/

static int total_seconds = 0;
static int current_seconds = 0;
static FILE* output_file = NULL;
static volatile sig_atomic_t running = 1;

/******************************************************************************
 * Signal handler for graceful termination
 ******************************************************************************/

void signal_handler(int sig) {
    if (sig == SIGTERM || sig == SIGINT) {
        running = 0;
    }
}

/******************************************************************************
 * Main function
 ******************************************************************************/

int main(int argc, char* argv[]) {
    // Check command line arguments
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <filename> <seconds>\n", argv[0]);
        fprintf(stderr, "Example: %s output.txt 5\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    // Parse arguments
    char* filename = argv[1];
    total_seconds = atoi(argv[2]);
    
    if (total_seconds <= 0) {
        fprintf(stderr, "Error: Number of seconds must be positive\n");
        exit(EXIT_FAILURE);
    }
    
    // Set up signal handlers
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    
    // Open output file
    output_file = fopen(filename, "w");
    if (output_file == NULL) {
        fprintf(stderr, "Error: Cannot open file '%s' for writing\n", filename);
        exit(EXIT_FAILURE);
    }
    
    printf("Process %d starting: writing to '%s' for %d seconds\n", 
           getpid(), filename, total_seconds);
    
    // Main loop: write every second
    while (running && current_seconds < total_seconds) {
        current_seconds++;
        
        // Write progress to file
        fprintf(output_file, "Process ran %d out of %d secs\n", 
                current_seconds, total_seconds);
        fflush(output_file);  // Ensure data is written immediately
        
        // Write progress to stdout for debugging
        printf("Process %d: %d/%d seconds\n", getpid(), current_seconds, total_seconds);
        
        // Sleep for 1 second (unless interrupted)
        if (current_seconds < total_seconds) {
            sleep(1);
        }
    }
    
    // Close file
    fclose(output_file);
    
    // Print final status
    if (running) {
        printf("Process %d completed successfully: %d/%d seconds\n", 
               getpid(), current_seconds, total_seconds);
    } else {
        printf("Process %d terminated early: %d/%d seconds\n", 
               getpid(), current_seconds, total_seconds);
    }
    
    return EXIT_SUCCESS;
}