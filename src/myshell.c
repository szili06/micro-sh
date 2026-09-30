#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

void sigchld_handler(int sig) {
  int savedErrno = errno;
  while(waitpid(-1, NULL, WNOHANG) > 0 );
  errno = savedErrno;
}

int main(int argc, char* argv[]) {
  struct sigaction sa;
  sa.sa_handler = sigchld_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
  sigaction(SIGCHLD, &sa, NULL);

	int interactive = 1;
	
	// Configure script running
	if(argc > 1) {
		interactive = 0;
		if (freopen(argv[1], "r", stdin) == NULL)
			return -1;
	}
  
  char buf[1024];
	while(1) {
		if(interactive) printf("$ ");

		// Read input
		if(fgets(buf, 1024, stdin) == NULL) exit(0);
	
		// Get rid of new-line
		char * nl = strchr(buf, '\n');
		if (nl) *nl = 0;

    // Get rid of comments
    char * hash = strchr(buf, '#');
    if(hash) *hash = 0;
		
		// Tokenization of arguments
		int numberOfArguments = 0;
		char * arguments[100];
		char *token = strtok(buf, " ");
		while(token != NULL && numberOfArguments < 100) {
			arguments[numberOfArguments++] = token;
			token = strtok(NULL, " ");
		}
    // Check for overflow on number of arguemnts
    if (numberOfArguments == 100) {
      fprintf(stderr, "Too many arguments for command! (Use less than 100)\n");
      continue;
    }
		arguments[numberOfArguments] = 0;

    // Check for empty input
    if(arguments[0] == NULL) continue;

    // Check for "exit"
		if (strcmp(arguments[0], "exit") == 0) exit(0);

    // Check for "cd"
    if (strcmp(arguments[0], "cd") == 0) {
      if (arguments[1] == NULL) arguments[1] = getenv("HOME");
      if(chdir(arguments[1]) != 0) fprintf(stderr, "Error on running cd!\n");
      continue;
    }

    int background = 0;
    // Check for background process (&)
    if(strcmp(arguments[numberOfArguments - 1], "&") == 0) {
      background = 1;
      arguments[--numberOfArguments] = 0;
    }

    // Check for empty input (after deleting &)
    if(arguments[0] == NULL) continue;
		int pid = fork();
	
    if(pid == -1){
      fprintf(stderr, "Error forking!\n");
    }
    else if(pid != 0) {
      if(!background) waitpid(pid, NULL, 0);
		}
		else {
			execvp(arguments[0], arguments);
			fprintf(stderr, "Could not execute %s\n", arguments[0]);
		  exit(1);
    }
	}
	return 0;
}
