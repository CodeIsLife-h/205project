#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <limits.h>

/******************************************************************************
 * Types
 ******************************************************************************/

typedef enum process_status {
	RUNNING = 0,     // Currently executing (max 3)
	READY = 1,       // Eligible to run, waiting for CPU slot
	STOPPED = 2,     // Suspended by the user
	TERMINATED = 3   // Completed or killed
} process_status;

typedef struct process_record {
	pid_t pid;
	process_status status;
	char* command;      // Full command string
	int priority_num;   // Numeric priority (lower = higher priority)
	char* priority_str; // Priority string (P1, P2, etc.)
	time_t arrival_time; // For FCFS tie-breaking
} process_record;

/******************************************************************************
 * Globals
 ******************************************************************************/

enum {
	MAX_PROCESSES = 20,  // Allow more processes total
	MAX_RUNNING = 3      // Max processes running at once
};

process_record process_records[MAX_PROCESSES];
int running_count = 0;

/******************************************************************************
 * Helper Functions
 ******************************************************************************/

// Find an unused slot in the process records
int find_unused_slot(void) {
	for (int i = 0; i < MAX_PROCESSES; ++i) {
		if (process_records[i].pid == 0) {
			return i;
		}
	}
	return -1;
}

// Find a process by PID
int find_process_by_pid(pid_t pid) {
	for (int i = 0; i < MAX_PROCESSES; ++i) {
		if (process_records[i].pid == pid) {
			return i;
		}
	}
	return -1;
}

// Parse priority string (P1, P2, etc.) and return numeric value
int parse_priority(const char* priority_str) {
	if (priority_str == NULL || strlen(priority_str) < 2) {
		return -1;
	}
	
	if (priority_str[0] != 'P') {
		return -1;
	}
	
	int priority = atoi(priority_str + 1);
	if (priority <= 0) {
		return -1;
	}
	
	return priority;
}

void check_completed_processes(void) {
	for (int i = 0; i < MAX_PROCESSES; ++i) {
		process_record *p = &process_records[i];
		if (p->status == RUNNING && p->pid > 0) {
			int status;
			pid_t result = waitpid(p->pid, &status, WNOHANG);
			//more than 0 means there was a state change
			if (result > 0) {
				printf("Process %d completed\n", p->pid);
				
				// Free allocated memory
				if (p->command) {
					free(p->command);
					p->command = NULL;
				}
				if (p->priority_str) {
					free(p->priority_str);
					p->priority_str = NULL;
				}
				
				p->status = TERMINATED;
				running_count--;
				printf("Debug: Running count after completion: %d\n", running_count);
				schedule_processes();
			}
		}
	}
}

// Find the highest priority ready process (lowest priority number, earliest arrival for ties)
int find_highest_priority_ready(void) {
	int best_index = -1;
	int best_priority = INT_MAX;
	time_t best_arrival = 0;
	
	for (int i = 0; i < MAX_PROCESSES; ++i) {
		process_record *p = &process_records[i];
		if (p->status == READY && p->pid > 0) {
			// Lower priority number = higher priority
			if (p->priority_num < best_priority || 
				(p->priority_num == best_priority && p->arrival_time < best_arrival)) {
				best_index = i;
				best_priority = p->priority_num;
				best_arrival = p->arrival_time;
			}
		}
	}
	
	return best_index;
}

// Schedule ready processes to run based on priority
void schedule_processes(void) {
	printf("Debug: Scheduling processes. Running count: %d/%d\n", running_count, MAX_RUNNING);
	while (running_count < MAX_RUNNING) {
		int ready_index = find_highest_priority_ready();
		if (ready_index == -1) {
			printf("Debug: No ready processes found\n");
			break; // No ready processes
		}
		
		process_record *p = &process_records[ready_index];
		printf("Debug: Found ready process %d (Priority: %s)\n", p->pid, p->priority_str);
		if (kill(p->pid, SIGCONT) == 0) {
			p->status = RUNNING;
			running_count++;
			printf("Process %d started (Priority: %s)\n", p->pid, p->priority_str);
		} else {
			printf("Debug: Failed to resume process %d\n", p->pid);
		}
	}
}

// Get status string for display
const char* get_status_string(process_status status) {
	switch (status) {
		case RUNNING: return "0";
		case READY: return "1";
		case STOPPED: return "2";
		case TERMINATED: return "3";
		default: return "?";
	}
}

/******************************************************************************
 * Command Functions
 ******************************************************************************/

void perform_run(char* args[]) {
	if (args[0] == NULL) {
		printf("Usage: run [program] [arguments] [Priority]\n");
		return;
	}
	
	// Find the priority argument (last argument)
	char* priority_str = NULL;
	int arg_count = 0;
	for (int i = 0; args[i] != NULL; i++) {
		arg_count++;
	}
	
	if (arg_count < 2) {
		printf("Error: Priority is required\n");
		return;
	}
	
	priority_str = args[arg_count - 1];
	int priority_num = parse_priority(priority_str);
	if (priority_num == -1) {
		printf("Error: Invalid priority format. Use P1, P2, P3, etc.\n");
		return;
	}
	
	int index = find_unused_slot();
	if (index < 0) {
		printf("Error: No process slots available\n");
		return;
	}
	
	// Create program arguments array (excluding priority)
	char* program_args[20];
	int program_arg_count = 0;
	for (int i = 0; i < arg_count - 1; i++) {
		program_args[program_arg_count++] = args[i];
	}
	program_args[program_arg_count] = NULL; // NULL terminate
	
	pid_t pid = fork();
	if (pid < 0) {
		fprintf(stderr, "Error: fork failed\n");
		return;
	}
	
	if (pid == 0) {
		// Redirect stdout and stderr to /dev/null to keep manager interface clean
		freopen("/dev/null", "w", stdout);
		freopen("/dev/null", "w", stderr);
		
		// Child process - execute the program with correct arguments
		execvp(program_args[0], program_args);
		// Unreachable code unless execution failed.
		fprintf(stderr, "Error: execvp failed\n");
		exit(EXIT_FAILURE);
	}
	
	// Parent process
	process_record *p = &process_records[index];
	p->pid = pid;
	p->priority_num = priority_num;
	p->arrival_time = time(NULL);
	
	// Store priority string
	p->priority_str = malloc(strlen(priority_str) + 1);
	strcpy(p->priority_str, priority_str);
	
	// Build command string for display
	char cmd_buffer[256] = "";
	for (int i = 0; i < arg_count - 1; i++) {
		if (i > 0) strcat(cmd_buffer, " ");
		strcat(cmd_buffer, args[i]);
	}
	p->command = malloc(strlen(cmd_buffer) + 1);
	strcpy(p->command, cmd_buffer);
	
	// Always start the process as stopped initially, then schedule based on priority
	kill(p->pid, SIGSTOP);
	p->status = READY;
	printf("Process %d queued (Priority: %s)\n", p->pid, p->priority_str);
	
	// Schedule processes based on priority
	schedule_processes();
}

void perform_stop(char* args[]) {
	if (args[0] == NULL) {
		printf("Usage: stop [PID]\n");
		return;
	}
	
	const pid_t pid = atoi(args[0]);
	if (pid <= 0) {
		printf("Error: PID must be a positive integer\n");
		return;
	}
	
	int index = find_process_by_pid(pid);
	if (index < 0) {
		printf("Error: Process %d not found\n", pid);
		return;
	}
	
	process_record *p = &process_records[index];
	if (p->status == TERMINATED) {
		printf("Error: Process %d is already terminated\n", pid);
		return;
	}
	
	if (p->status == STOPPED) {
		printf("Error: Process %d is already stopped\n", pid);
		return;
	}
	
	if (p->status != RUNNING) {
		printf("Error: Process %d is not running\n", pid);
		return;
	}
	
	kill(p->pid, SIGSTOP);
	printf("stopping %d\n", p->pid);
	p->status = STOPPED;
	running_count--;
	
	// Schedule ready processes to fill the slot
	schedule_processes();
}

void perform_kill(char* args[]) {
	if (args[0] == NULL) {
		printf("Usage: kill [PID]\n");
		return;
	}
	
	const pid_t pid = atoi(args[0]);
	if (pid <= 0) {
		printf("Error: PID must be a positive integer\n");
		return;
	}
	
	int index = find_process_by_pid(pid);
	if (index < 0) {
		printf("Error: Process %d not found\n", pid);
		return;
	}
	
	process_record *p = &process_records[index];
	if (p->status == TERMINATED) {
		printf("Error: Process %d is already terminated\n", pid);
		return;
	}
	
	kill(p->pid, SIGTERM);
	printf("Process %d terminated\n", p->pid);
	
	// Free allocated memory
	if (p->command) {
		free(p->command);
		p->command = NULL;
	}
	if (p->priority_str) {
		free(p->priority_str);
		p->priority_str = NULL;
	}
	
	if (p->status == RUNNING) {
		running_count--;
	}
	p->status = TERMINATED;
	
	// Schedule ready processes to fill the slot
	schedule_processes();
}

void perform_resume(char* args[]) {
	if (args[0] == NULL) {
		printf("Usage: resume [PID]\n");
		return;
	}
	
	const pid_t pid = atoi(args[0]);
	if (pid <= 0) {
		printf("Error: PID must be a positive integer\n");
		return;
	}
	
	int index = find_process_by_pid(pid);
	if (index < 0) {
		printf("Error: Process %d not found\n", pid);
		return;
	}
	
	process_record *p = &process_records[index];
	if (p->status == TERMINATED) {
		printf("Error: Process %d is terminated and cannot be resumed\n", pid);
		return;
	}
	
	if (p->status != STOPPED) {
		printf("Error: Process %d is not stopped\n", pid);
		return;
	}
	
	printf("resuming %d\n", p->pid);
	
	if (running_count < MAX_RUNNING) {
		kill(p->pid, SIGCONT);
		p->status = RUNNING;
		running_count++;
	} else {
		p->status = READY;
	}
}

void perform_list(void) {
	printf("PID\t\tSTATE\tPRIORITY\n");
	
	bool anything = false;
	for (int i = 0; i < MAX_PROCESSES; ++i) {
		process_record *p = &process_records[i];
		if (p->pid > 0) {
			printf("%d\t\t%s\t\t%s\n", 
				p->pid, get_status_string(p->status), p->priority_str);
			anything = true;
		}
	}
	
	if (!anything) {
		printf("No processes to list\n");
	}
}

void perform_exit(void) {
	printf("Terminating all processes...\n");
	
	// Kill all non-terminated processes
	for (int i = 0; i < MAX_PROCESSES; ++i) {
		process_record *p = &process_records[i];
		if (p->pid > 0 && p->status != TERMINATED) {
			kill(p->pid, SIGTERM);
		}
	}
	
	// Wait for all processes to terminate and free memory
	for (int i = 0; i < MAX_PROCESSES; ++i) {
		process_record *p = &process_records[i];
		if (p->pid > 0) {
			waitpid(p->pid, NULL, 0);
			
			// Free allocated memory
			if (p->command) {
				free(p->command);
				p->command = NULL;
			}
			if (p->priority_str) {
				free(p->priority_str);
				p->priority_str = NULL;
			}
		}
	}
	
	printf("bye!\n");
}

char * get_input(char * buffer, char * args[], int args_count_max) {
	// capture a command
	printf("\x1B[34m" "cs205" "\x1B[0m" "$ ");
	fgets(buffer, 79, stdin);
	for (char* c = buffer; *c != '\0'; ++c) {
		if ((*c == '\r') || (*c == '\n')) {
			*c = '\0';
			break;
		}
	}
	strcat(buffer, " ");
	// tokenize command's arguments
	char * p = strtok(buffer, " ");
	int arg_cnt = 0;
	while (p != NULL) {
		args[arg_cnt++] = p;
		if (arg_cnt == args_count_max - 1) {
			break;
		}
		p = strtok(NULL, " ");
	}
	args[arg_cnt] = NULL;
	return args[0];
}

/******************************************************************************
 * Entry point
 ******************************************************************************/

int main(void) {
	char buffer[80];
	// NULL-terminated array
	char * args[20];
	const int args_count = sizeof(args) / sizeof(*args);
	
	// Initialize process records
	for (int i = 0; i < MAX_PROCESSES; ++i) {
		process_records[i].pid = 0;
		process_records[i].status = TERMINATED;
		process_records[i].command = NULL;
		process_records[i].priority_str = NULL;
		process_records[i].priority_num = 0;
		process_records[i].arrival_time = 0;
	}
	
	while (true) {
		// Check for completed processes before each command
		check_completed_processes();
		
		char * const cmd = get_input(buffer, args, args_count);
		if (strcmp(cmd, "kill") == 0) {
			perform_kill(&args[1]);
		} else if (strcmp(cmd, "run") == 0) {
			perform_run(&args[1]);
		} else if (strcmp(cmd, "stop") == 0) {
			perform_stop(&args[1]);
		} else if (strcmp(cmd, "resume") == 0) {
			perform_resume(&args[1]);
		} else if (strcmp(cmd, "list") == 0) {
			perform_list();
		} else if (strcmp(cmd, "exit") == 0) {
			perform_exit();
			break;
		} else {
			printf("invalid command\n");
		}
	}
	return EXIT_SUCCESS;
}