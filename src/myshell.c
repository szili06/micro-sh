#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>

void sigchld_handler(int sig) {
  int savedErrno = errno;
  while(waitpid(-1, NULL, WNOHANG) > 0 );
  errno = savedErrno;
}

void stringCleanup(char* buf) {
  // Get rid of new-line
  char * nl = strchr(buf, '\n');
  if (nl) {
    *nl = 0;
  }
  // Get rid of comments
  char * hash = strchr(buf, '#');
  if(hash) {
   *hash = 0;
  }
}

void tokenization(char* buf, char* arguments[], int* numberOfArguments) {
	char *token = strtok(buf, " \t\r\n");
	while(token != NULL && *numberOfArguments < 100) {
		arguments[(*numberOfArguments)++] = token;
		token = strtok(NULL, " \t\r\n");
	}
}

int checkBackground(char* arguments[], int* numberOfArguments) {
  if(*numberOfArguments > 0 && strcmp(arguments[(*numberOfArguments) - 1], "&") == 0) {
    arguments[--(*numberOfArguments)] = 0;
    return 1;
  }
  return 0;
}

void splitPipes(char* arguments[], int numberOfArguments, char** cmds[], int* cmdCount) {
  cmds[(*cmdCount)++] = &arguments[0];
  for(int i = 0; i < numberOfArguments; i++) {
    if(arguments[i] != NULL && strcmp(arguments[i], "|") == 0) {
      arguments[i] = NULL;
      cmds[(*cmdCount)++] = &arguments[i+1];
    }
  }
}

void initPipes(int cmdCount, int pipefds[]) {
  for(int i = 0; i < cmdCount - 1; i++) {
    if(pipe(pipefds + i * 2) < 0) {
      perror("pipe");
    }
  }
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
		if(freopen(argv[1], "r", stdin) == NULL)
	    return -1;
	}
  
  char buf[1024];
  signal(SIGINT,SIG_IGN);
	while(1) {
		if(interactive) printf("\n$ ");

		// Read input
		if(fgets(buf, 1024, stdin) == NULL) exit(0);
	
	  stringCleanup(buf);	
		int numberOfArguments = 0;
	  char *arguments[101];
	  tokenization(buf, arguments, &numberOfArguments);

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

    int background = checkBackground(arguments, &numberOfArguments);

    // Check for empty input (after deleting &)
    if(arguments[0] == NULL) continue;

    // Splitting pipes per |
    char** cmds[16];
    int cmdCount = 0;

    splitPipes(arguments, numberOfArguments, cmds, &cmdCount);

    //Pipes for Pipes
    int pipefds[cmdCount > 1 ? 2 * (cmdCount - 1) : 1];
    if(cmdCount > 1) {
      initPipes(cmdCount, pipefds);
    }
   
    //Blocking SIGCHLD
    sigset_t mask, prev_mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);

    if(!background) {
      sigprocmask(SIG_BLOCK, &mask, &prev_mask);
    }

    //Forking
    pid_t pids[16] = {0};
    for(int i = 0; i < cmdCount; i++) {
      pids[i] = fork();

      if(pids[i] == -1) {
        fprintf(stderr, "error forking!\n");
        break;
      }
      else if (pids[i] == 0) {
        // reset SIGCHLD
        if(!background) {
          sigprocmask(SIG_SETMASK, &prev_mask, NULL);
        } 
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
            perror("Failed to open output file");
            exit(1);
          }
          dup2(fd, STDOUT_FILENO);
          close(fd);
	      }

		    if(inputFile != NULL) {
          int fd = open(inputFile, O_RDONLY);
		      if(fd < 0) {
			      perror("Failed to open input file");
		        exit(1);
          }
          dup2(fd, STDIN_FILENO);
          close(fd);
        }

	      signal(SIGINT, SIG_DFL);
        if(cmds[i][0] == NULL) {
          exit(0);
        }
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
        if(pids[i] > 0) {
          waitpid(pids[i], NULL, 0);
        }
      }
      sigprocmask(SIG_SETMASK, &prev_mask, NULL);
    }
  }
	return 0;
}
