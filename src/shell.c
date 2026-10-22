/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: t; -*- */
/*
 * Minish shell.
 *
 * Σταύρος Ουλεμάνης, ics26177
 * 
*/

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "parser.h"

#define INPUT_SIZE 4096
#define MAX_JOBS 128

struct job {
	pid_t pid;
	int active;
};

// This array can hold information about spawned processes.
// Each array element describes a new process and holds its
// PID and its state in the active variable. This variable
// may take the value 1 if the job is active, or the value
// 0 if the job is not active. The array can be used to
// manage background jobs.
//
// You may use this array if you find it helpful. Otherwise,
// fill free to delete it.
static struct job jobs[MAX_JOBS];

static void print_prompt(void);
static int read_line(char *buffer, size_t size);
static int execute_pipeline(const struct pipeline *pipeline);
static void init_bg_jobs(struct job *jobs);
static void add_bg_job(const pid_t pid);
static void remove_bg_job(const pid_t pid);
static void reap_bg_jobs(void);

// Add the prototypes of helper funcitons here...
static void handle_sigchld(int sig);

static void handle_sigchld(int sig)
{
	(void)sig;
	reap_bg_jobs();
}

/**
 * @brief Run the minish command interpreter.
 *
 * @return Process exit status.
 */
int main(void)
{	
	char line[INPUT_SIZE];
	struct pipeline pipeline;
	struct sigaction chld_action;
	struct sigaction int_action;
	int error;
	int result;

	// Install the required signal handlers here.
	// Add your code here.
	init_bg_jobs(jobs);

	memset(&chld_action, 0, sizeof(chld_action));
	chld_action.sa_handler = handle_sigchld;
	sigemptyset(&chld_action.sa_mask);
	chld_action.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &chld_action, NULL) == -1) {
		perror("minish: sigaction SIGCHLD");
		return EXIT_FAILURE;
	}

	memset(&int_action, 0, sizeof(int_action));
	int_action.sa_handler = SIG_IGN;
	sigemptyset(&int_action.sa_mask);
	if (sigaction(SIGINT, &int_action, NULL) == -1) {
		perror("minish: sigaction SIGINT");
		return EXIT_FAILURE;
	}
	 
	// The main shell loop.
	for (;;) {
		// Print the prompt.
		print_prompt();

		// Read a command line from the user.
		result = read_line(line, sizeof(line));

		// Handle read errors.
		if (result == 0)
			break;

		if (result < 0) {
			if (errno == EINTR)
				continue;

			perror("minish: Cannot read the command line.");
			return EXIT_FAILURE;
		}

		// Parse the line.
		error = parse_line(line, &pipeline);

		// An error occurred during parsing.
		if (error != PARSE_SUCCESS) {
			fprintf(stderr, "minish: %s\n", parse_error_string(error));
			continue;
		}

		// The user just pressed enter. We have nothing to execute at this time.
		if (pipeline.command_count == 0) {
			pipeline_free(&pipeline);
			continue;
		}

		// Execute the command pipeline.
		if ((result = execute_pipeline(&pipeline)) < 0)
			perror("minish");

		// Clear the pipeline.
		pipeline_free(&pipeline);
	}
	return EXIT_SUCCESS;
}

/**
 * @brief Print the interactive shell prompt.
 */
static void print_prompt(void)
{
	if (isatty(STDIN_FILENO) != 0) {
		(void)printf("minish$ ");
		(void)fflush(stdout);
	}
}

/**
 * @brief Read one command line from standard input.
 *
 * @param[out] buffer Destination buffer.
 * @param[in] size Size of the destination buffer.
 *
 * @return 1 when a line was read, 0 on EOF, or -1 on error.
 */
static int read_line(char *buffer, size_t size)
{
	if (fgets(buffer, size, stdin) == NULL) {
		if (feof(stdin) != 0)
			return 0;

		return -1;
	}

	buffer[strcspn(buffer, "\n")] = '\0';

	return 1;
}

void init_bg_jobs(struct job *jobs)
{
	int i;
  
	if (!jobs)
		return;
		
	for (i = 0; i < MAX_JOBS; i++) {		
		jobs[i].pid =-1;
		jobs[i].active = 0;
	}
}

// vriskei tin prwti eleutheri thesi ston jobs kai bazei to neo bg pid
void add_bg_job(const pid_t pid)
{
	int i;

	for (i = 0; i < MAX_JOBS; i++) {
		// Find the first position in the array with active == 0 and add the
		// new job there. Then call break to exit the loop.
        // Add your code here...
		if (jobs[i].active == 0) {
			jobs[i].pid = pid;
			jobs[i].active = 1;
			break;
		}
	}
}

// svinei to bg pid apo ton pinaka jobs bazontas active 0 kai pid -1
void remove_bg_job(const pid_t pid)
{
	int i;

	for (i = 0; i < MAX_JOBS; i++) {
		// Find the first position in the array where its pid matches with the
		// pid parameter of the function and then clean it by setting pid to
		// -1 and active to 0.
        // Add your code here...
		if (jobs[i].pid == pid) {
			jobs[i].pid = -1;
			jobs[i].active = 0;
			break;
		}
	}
}

// mazeyei ola ta termatismena bg paidia me waitpid kai ta vgazei apo to jobs
// The function below should be called from a signal handler.
void reap_bg_jobs(void) {
	pid_t pid;
	int status;	
	int i;

	for (i = 0; i < MAX_JOBS; i++) {
		// Call pid = waitpid() system call properly for each array slot.
		// If this call returns a positive pid, then this means that we just
		// found a terminated background process. Call remove_bg_job()
		// properly to remove this process from the jobs array.
          		
        // Add your code here...
		if (jobs[i].active == 1) {
			pid = waitpid(jobs[i].pid, &status, WNOHANG);
			if (pid > 0)
				remove_bg_job(pid);
		}
	}	
}

/**
 * @brief Execute all commands in a parsed pipeline.
 *
 * @param[in] pipeline Parsed pipeline.
 *
 * @return 0 on success, -1 on error, -2 if exit was requested.
 */
static int execute_pipeline(const struct pipeline *pipeline)
{
	pid_t *pids;	
	int result = 0;
	int pipe_fds[2];
	int prev_fd = -1;
	size_t i, j;
	int fd;
	struct sigaction dfl;
	
	//char *directory; // pou thelo na paw
	char *directory = getenv("HOME");
	
	// First, we have to handle the internal commands. To do this, we need to
	// check if our command count is one, and if the name of the command matches
	// an internal command. Remember that the command name is always stored in
	// argv[0].

	// argc ean input cd argc = 1 ean input cd /home/user argc = 2

	// Add your code here...
	if (pipeline->command_count == 1) {
		if (pipeline->commands[0].argv != NULL && pipeline->commands[0].argv[0] != NULL) {
			if (!strcmp(pipeline->commands[0].argv[0], "exit")) {
				pipeline_free((struct pipeline *)pipeline); //to (struct pipeline *) agnoei to const gia na min einai read only 
				exit(0);
			}
			if (!strcmp(pipeline->commands[0].argv[0], "cd")) {
				if (pipeline->commands[0].argc > 1)
					directory = pipeline->commands[0].argv[1]; // parnei px to /tmp apo to cd /tmp tin deuteri leksi
				//else 
					//directory = getenv("HOME"); // se paei home ean grapsei mono cd
				
				if (directory == NULL) {
					fprintf(stderr, "minish: cd: HOME not set\n");
					return 0;
				}

				if (chdir(directory) < 0)
					perror("minish: cd");

				return 0;
			}
		}
	}

	// No internal command found. We have to execute a new program for each
	// command in the pipeline.

	// Allocate an array to store the PID of each child process.
    // You may use the array below, if you find it helpful.    
	pids = calloc(pipeline->command_count, sizeof(*pids));
	if (pids == NULL)
		return -1;

	// Process each command in the pipeline. Remember that the command_count
	// tells us the number of commands in the pipeline.
	// E.g., cmd1 | cmd2 : 2 commands.
	// E.g., cmd1 : 1 command.

	// Add your code here...
	for (i = 0; i < pipeline->command_count; i++) {
		// anoigma pipe an yparxei epomeni entoli
		if (i < pipeline->command_count - 1) {
			if (pipe(pipe_fds) < 0) {
				perror("minish: pipe");
				free(pids);
				return -1;
			}
		}

		pids[i] = fork();
		if (pids[i] < 0) {
			perror("minish: fork");
			free(pids);
			return -1;
		}

		// paidi
		if (pids[i] == 0) {
			if (pipeline->background == 0) {
				memset(&dfl, 0, sizeof(dfl));
				dfl.sa_handler = SIG_DFL;
				sigemptyset(&dfl.sa_mask);
				sigaction(SIGINT, &dfl, NULL);
			}

			if (prev_fd != -1) {
				dup2(prev_fd, STDIN_FILENO);
				close(prev_fd);
			}

			if (i < pipeline->command_count - 1) {
				close(pipe_fds[0]);
				dup2(pipe_fds[1], STDOUT_FILENO);
				close(pipe_fds[1]);
			}

			// diaxeirisi redirects
			for (j = 0; j < pipeline->commands[i].redirect_count; j++) {
				if (pipeline->commands[i].redirects[j].type == REDIRECT_INPUT) {
					fd = open(pipeline->commands[i].redirects[j].path, O_RDONLY);
					if (fd < 0) {
						perror("minish");
						exit(1);
					}
					dup2(fd, STDIN_FILENO);
					close(fd);
				} else if (pipeline->commands[i].redirects[j].type == REDIRECT_OUTPUT) {
					fd = open(pipeline->commands[i].redirects[j].path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
					if (fd < 0) {
						perror("minish");
						exit(1);
					}
					dup2(fd, STDOUT_FILENO);
					close(fd);
				} else if (pipeline->commands[i].redirects[j].type == REDIRECT_APPEND) {
					fd = open(pipeline->commands[i].redirects[j].path, O_WRONLY | O_CREAT | O_APPEND, 0644);
					if (fd < 0) {
						perror("minish");
						exit(1);
					}
					dup2(fd, STDOUT_FILENO);
					close(fd);
				}
			}

			execvp(pipeline->commands[i].argv[0], pipeline->commands[i].argv);
			perror("minish");
			exit(1);
		}

		// pateras
		if (prev_fd != -1)
			close(prev_fd);

		if (i < pipeline->command_count - 1) {
			close(pipe_fds[1]);
			prev_fd = pipe_fds[0];
		}
	}

	// background h wait
	if (pipeline->background == 1) {
		for (i = 0; i < pipeline->command_count; i++)
			add_bg_job(pids[i]);
	} else {
		for (i = 0; i < pipeline->command_count; i++)
			waitpid(pids[i], NULL, 0);
	}
        
	free(pids);
	return result;
}