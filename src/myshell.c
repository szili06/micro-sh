#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
<<<<<<< HEAD
=======
#include <fcntl.h>
>>>>>>> 8c9c6eb (Implementation of IO redirection and piping)

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
<<<<<<< HEAD
	
	// Configure script running
	if(argc > 1) {
		interactive = 0;
		if (freopen(argv[1], "r", stdin) == NULL)
			return -1;
	}
  
  char buf[1024];
=======
	// Configure script running
	if(argc > 1) {
		interactive = 0;
		if(freopen(argv[1], "r", stdin) == NULL)
	    return -1;
	}
  
  char buf[1024];
  signal(SIGINT,SIG_IGN);
>>>>>>> 8c9c6eb (Implementation of IO redirection and piping)
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
<<<<<<< HEAD
		char *token = strtok(buf, " ");
		while(token != NULL && numberOfArguments < 100) {
			arguments[numberOfArguments++] = token;
			token = strtok(NULL, " ");
=======
		char *token = strtok(buf, " \t\r\n");
		while(token != NULL && numberOfArguments < 100) {
			arguments[numberOfArguments++] = token;
			token = strtok(NULL, " \t\r\n");
>>>>>>> 8c9c6eb (Implementation of IO redirection and piping)
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
<<<<<<< HEAD
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
=======

    // Check for empty input (after IO search)
    if(arguments[0] == NULL) continue;
    
    // Splitting pipes per |
    char** cmds[16];
    int cmdCount = 0;
    
    cmds[cmdCount++] = &arguments[0];
    for(int i = 0; i < numberOfArguments; i++) {
      if(arguments[i] != NULL && strcmp(arguments[i], "|") == 0) {
        arguments[i] = NULL;
        cmds[cmdCount++] = &arguments[i+1];
      }
    }
    //Pipes for Pipes
    int pipefds[2 * (cmdCount - 1)];
    for(int i = 0; i < cmdCount - 1; i++) {
      if(pipe(pipefds + i * 2) < 0) {
        perror("pipe");
      }
    }
    //Forking
    pid_t pids[16];
    for(int i = 0; i < cmdCount; i++) {
      pids[i] = fork();

      if(pids[i] == -1) {
        fprintf(stderr, "error forking!\n");
        break;
      }
      else if (pids[i] == 0) {
        // take input from previous pipe
        if (i > 0) {
          dup2(pipefds[(i - 1) * 2], STDIN_FILENO);
        }
        // put output to next pipe
        if(i < cmdCount - 1) {
          dup2(pipefds[i * 2 + 1], STDOUT_FILENO);
        }

        // close duplicate pipes
        for(int j = 0; j < 2 * (cmdCount - 1); j++)
          close(pipefds[j]);
        
        // check for IO redirects
        char *outputFile = NULL;
        char * inputFile = NULL;
        int writeIdx = 0;
        for(int k = 0; cmds[i][k] != NULL; k++) {
          if(strcmp(cmds[i][k], ">") == 0) {
            if(cmds[i][k+1] != NULL) {
              outputFile = cmds[i][k + 1];
              k++;
            }
          } else if (strcmp(cmds[i][k], "<") == 0) {
            if(cmds[i][k+1] != NULL) {
              inputFile = cmds[i][k + 1];
              k++;
            } 
          } else {
            cmds[i][writeIdx++] = cmds[i][k];
          }
        }
        cmds[i][writeIdx] = NULL;

        // handle IO redirects
        if(outputFile != NULL) {
          int fd = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
          if(fd < 0) {
            fprintf(stderr, "Failed to open output file");
            exit(1);
          }
          dup2(fd, STDOUT_FILENO);
          close(fd);
	      }

		    if(inputFile != NULL) {
          int fd = open(inputFile, O_RDONLY);
		      if(fd < 0) {
			      fprintf(stderr, "Failed to open input file");
		        exit(1);
          }
          dup2(fd, STDIN_FILENO);
          close(fd);
        }

	      signal(SIGINT, SIG_DFL);
		    execvp(cmds[i][0], cmds[i]);
		    fprintf(stderr, "Could not execute %s\n", cmds[i][0]);
		    exit(1);
      }
    }

    for(int i = 0; i < 2 * (cmdCount - 1); i++) {
      close(pipefds[i]);
    }

    if(!background) {
      for(int i = 0; i < cmdCount; i++) {
        waitpid(pids[i], NULL, 0);
      }
    }
  }
>>>>>>> 8c9c6eb (Implementation of IO redirection and piping)
	return 0;
}
