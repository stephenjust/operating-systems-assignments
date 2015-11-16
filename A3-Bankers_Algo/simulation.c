#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct process_t {
	char *name;
	int *needed_resources;
	int arrival_time;
	int simulation_time;
	int start_time;
	int completed;
} process_t;

int num_resource_types;
int *num_each_resource_type;
int *avail_each_resource_type;
int num_processes;
process_t *processes;


void get_resources();
void get_processes();
void run_simulation();
int resources_available(process_t *process);
void allocate_resources(process_t *process);
void release_resources(process_t *process);
int all_processes_completed();
int is_deadlock(int simulation_time);
int system_idle();

int main(int argc, char* argv[]) {
	get_resources();
	get_processes();
	run_simulation();
	return 0;
}

void get_resources() {
	char buffer[80];
	int i;
	/* Loop until input is valid */
	while (1) {
		fprintf(stdout, "?> Number of different resource types: ");
		fgets(buffer, sizeof(buffer), stdin);
		num_resource_types = atoi(buffer);
		/* Check that we have a positive number */
		if (num_resource_types > 0) {
			break;
		} else {
			fprintf(stdout, "Invalid input, try again\n");
		}
	}
	num_each_resource_type = malloc(num_resource_types * sizeof(int));
	avail_each_resource_type = malloc(num_resource_types * sizeof(int));

	/* Loop until input is valid */
	while (1) {
		int pass = 1;
		char *ptr;
		fprintf(stdout, "?> Names of each resource type: ");
		fgets(buffer, sizeof(buffer), stdin);
		/* Check count of values */
		for (i = 0; i < num_resource_types; i++) {
			if (i == 0)
				ptr = strtok(buffer, " ");
			else
				ptr = strtok(NULL, " ");
			/* Check for too few words */
			if (ptr == NULL) {
				fprintf(stdout, "Invalid number of names\n");
				pass = 0;
				break;
			}
		}
		/* Check for too many words */
		ptr = strtok(NULL, " ");
		if (ptr != NULL) {
			fprintf(stdout, "Invalid number of names\n");
			pass = 0;
		}
		if (pass == 1) {
			break;
		}
	}

	/* Loop until input is valid */
	while (1) {
		int pass = 1;
		char *ptr;
		fprintf(stdout, "?> Number of instances of each resource type: ");
		fgets(buffer, sizeof(buffer), stdin);
		/* Check count of values */
		for (i = 0; i < num_resource_types; i++) {
			if (i == 0)
				ptr = strtok(buffer, " ");
			else
				ptr = strtok(NULL, " ");
			/* Check for too few words */
			if (ptr == NULL) {
				fprintf(stdout, "Invalid number of names\n");
				pass = 0;
				break;
			}
			num_each_resource_type[i] = atoi(ptr);
		}
		/* Check for too many words */
		ptr = strtok(NULL, " ");
		if (ptr != NULL) {
			fprintf(stdout, "Invalid number of names\n");
			pass = 0;
		}
		/* Check that all resource counts are valid */
		for (i = 0; i < num_resource_types; i++) {
			if (num_each_resource_type[i] < 0) {
				fprintf(stdout, "Cannot have a negative number of resources\n");
				pass = 0;
			}
		}
		if (pass == 1) {
			break;
		}
	}
	fprintf(stdout, "\n");
}

void get_processes() {
	char buffer[80];
	int i = 0;

	/* Loop until input is valid */
	while (1) {
		fprintf(stdout, "?> Number of processes: ");
		fgets(buffer, sizeof(buffer), stdin);
		num_processes = atoi(buffer);
		/* Check that we have a positive number */
		if (num_processes > 0) {
			break;
		} else {
			fprintf(stdout, "Invalid input, try again\n");
		}
	}
	/* Allocate process structs */
	processes = malloc(num_processes * sizeof(process_t));
	memset(processes, 0, num_processes * sizeof(process_t));

	/* Get info for each process */
	for (i = 0; i < num_processes; i++) {
		processes[i].needed_resources = malloc(num_resource_types *
											   sizeof(int));
		/* Loop until input is valid */
		while (1) {
			int j;
			int pass = 1;
			char *ptr;
			fprintf(stdout, "?> Details of process-%d: ", i+1);
			fgets(buffer, sizeof(buffer), stdin);

			/* Get process name */
			ptr = strtok(buffer, " ");
			processes[i].name = malloc(strlen(ptr) * sizeof(char));
			strcpy(processes[i].name, ptr);

			/* Get resource counts */
			for (j = 0; j < num_resource_types; j++) {
				ptr = strtok(NULL, " ");
				processes[i].needed_resources[j] = atoi(ptr);
				/* Do not allow negative numbers */
				if (processes[i].needed_resources[j] < 0) {
					pass = 0;
				}
				/* Do not allow processes that need more resources than the
				   system contains */
				if (processes[i].needed_resources[j] >
					num_each_resource_type[j]) {
					pass = 0;
				}
			}

			/* Get start time */
			ptr = strtok(NULL, " ");
			processes[i].arrival_time = atoi(ptr);
			if (processes[i].arrival_time < 0) {
				pass = 0;
			}

			/* Get process length */
			ptr = strtok(NULL, " ");
			processes[i].simulation_time = atoi(ptr);
			if (processes[i].simulation_time == 0) {
				pass = 0;
			}

			processes[i].start_time = -1;

			if (pass == 1) {
				break;
			} else {
				fprintf(stdout, "Invalid process parameters, try again\n");
			}
		}
	}
}

void run_simulation() {
	int simulation_time = 0;
	int i;
	/* All resources are available at start */
	memcpy(avail_each_resource_type, num_each_resource_type,
		   num_resource_types * sizeof(int));
	/* Loop until the simulation is complete */
	while (1) {
		fprintf(stdout, "\n");
		fprintf(stdout, "?> Simulation time: %d\n", simulation_time);
		/* Check if any process is completed before trying to start new
		   processes */
		for (i = 0; i < num_processes; i++) {
			process_t *process = &processes[i];
			if (simulation_time - process->start_time ==
				process->simulation_time && process->start_time != -1) {
				fprintf(stdout, "?> Process %s has just finished execution\n",
					process->name);
				release_resources(process);
				process->completed = 1;
			}
		}
		/* Try to start new processes */
		for (i = 0; i < num_processes; i++) {
			process_t *process = &processes[i];
			/* Notify when process has arrived */
			if (process->arrival_time == simulation_time) {
				fprintf(stdout, "?> Process %s has just arrived "
						"at the system\n", process->name);
			}
			
			/* Check for running processes */
			if (process->start_time != -1 && process->completed == 0) {
				fprintf(stdout, "?> Process %s is running\n", process->name);
			}
			
			/* Check if the process can be started */
			if (process->arrival_time <= simulation_time &&
				process->start_time == -1) {
				if (resources_available(process)) {
					/* resources are available - allocate them */
					allocate_resources(process);
					fprintf(stdout, "?> Process %s has just started "
							"execution\n", process->name);
					process->start_time = simulation_time;
				} else {
					/* resources are unavailable */
					fprintf(stdout, "?> Process %s is idle\n", process->name);
				}
			}
		}
		if (system_idle()) {
			fprintf(stdout, "?> No processes running. System is idle\n");
		}
		if (all_processes_completed()) {
			fprintf(stdout, "?> No processes available for execution\n\n"
					"?> Simulation is ended.\n");
			break;
		} else if (is_deadlock(simulation_time)) {
			fprintf(stdout, "?> No process is running. Available resources are "
					"not sufficient to run any idle process or to avoid "
					"deadlocks. Aborting\n\n?> Simulation is ended\n");
			break;
		}
		simulation_time++;
	}
}

int resources_available(process_t *process) {
	int i;
	for (i = 0; i < num_resource_types; i++) {
		if (process->needed_resources[i] > avail_each_resource_type[i])
			return 0;
	}
	return 1;
}

void allocate_resources(process_t *process) {
	int i;
	for (i = 0; i < num_resource_types; i++) {
		avail_each_resource_type[i] -= process->needed_resources[i];
	}
}

void release_resources(process_t *process) {
	int i;
	for (i = 0; i < num_resource_types; i++) {
		avail_each_resource_type[i] += process->needed_resources[i];
	}
}

int all_processes_completed() {
	int done = 1;
	int i = 0;
	for (i = 0; i < num_processes; i++) {
		if (processes[i].completed != 1)
			done = 0;
	}
	return done;
}

int is_deadlock(int simulation_time) {
	int i = 0;
	for (i = 0; i < num_processes; i++) {
		/* ignore completed processes */
		if (processes[i].completed) continue;
		/* no deadlock if any process is running */
		if (processes[i].start_time != -1 && !processes[i].completed) return 0;
		/* no deadlock if there are sufficient resources for a process */
		if (resources_available(&processes[i])) return 0;
	}
	return 1;
}

int system_idle() {
	int i = 0;
	for (i = 0; i < num_processes; i++) {
		if (processes[i].completed) continue;
		if (processes[i].start_time != -1) return 0;
	}
	return 1;
}
