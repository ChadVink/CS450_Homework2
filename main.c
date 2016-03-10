#include <stdio.h>
#include "constants.h"
#include "parsetools.h"
#include <sys/wait.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

void syserror(const char* s);

int PipeFork(int n,char* cmdArray[][MAX_LINE_WORDS+1], int* PipeType, int RedirectsFound);
int CreateProcess (int in, int out, char* cmd[]);
int CreateRedirect(int input, int output, char* cmd[], int type, char* FileName[]);

int main() {
    // Buffer for reading one line of input
    char line[MAX_LINE_CHARS];
    char* line_words[MAX_LINE_WORDS + 1];
    int countpipes; // Number of pipes that are read in
    int num_words;
    int idx; // index of the word on the line_words
    char* commandArray[MAX_LINE_WORDS + 1][MAX_LINE_WORDS + 1];


    // Loop until user hits Ctrl-D (end of input)
    // or some other input error occurs
    while( fgets(line, MAX_LINE_CHARS, stdin) ) {
        int RedirectsFound = 0;
	countpipes = 0;
        idx = 0;
	int PipeType[100];
        num_words = split_cmd_line(line, line_words);
        int cmdCount = 0;
        memset(commandArray, 0, sizeof commandArray);

         while( idx < num_words ){
	    int check1 = strcmp(line_words[idx], "|");
	    int check2 = strcmp(line_words[idx], ">");
	    int check3 = strcmp(line_words[idx], "<");
            if(check1 != 0 && check2 != 0 && check3 != 0){
		 // if line_words[idx] is not a "|", ">", or "<"
                commandArray[countpipes][cmdCount] = line_words[idx];
                cmdCount++;
            }
            else{
		// check which type of pipe operation should be performed
		// 1 is pipe, 2 and 3 are redirects.
		// Store these in an array in sequential order
		if (check1 == 0)
			PipeType[countpipes] = 0;
		if (check2 == 0){
			PipeType[countpipes] = 1; RedirectsFound++;}
		if (check3 == 0){ RedirectsFound++;
			PipeType[countpipes] = 2;}

                commandArray[countpipes][cmdCount+1] = 0;
                cmdCount = 0;
                countpipes++;
             }
            idx++;
        }

        PipeFork(countpipes+1, commandArray, PipeType, RedirectsFound);
    }
    
	return 0;
}



void syserror(const char *s)
{
    extern int errno;

    fprintf( stderr, "%s\n", s );
    fprintf( stderr, " (%s)\n", strerror(errno) );
    exit( 1 );
}


int CreateRedirect(int input, int output, char* cmd[], int type, char* FileName[]){

	pid_t pid;
	if(type == 2){
	// < redirect
		if ((pid = fork()) == 0)
		{
//			dup2(input, 1);
			int fdIn;
			if ((fdIn = open(FileName[0], O_RDONLY, 0666)) == -1)
				syserror("Could not open file in < redirect");
			dup2(fdIn, 0);
			if(close(fdIn) == -1)
				syserror("Could not close fdin in < redirect");
			return execvp(cmd[0], cmd);
			syserror("Could not execute < redirect");
		}
		else if (pid == -1)
			syserror("could not fork < redirect");
	}
	else{
	// > redirect
		if ((pid = fork()) == 0)
		{
			int fdIn = output;
			dup2(fdIn, 0);
			int fdOut;
			if( (fdOut = open(FileName[0], O_WRONLY|O_CREAT, 0666)) == -1)
				syserror("Could not open file in > redirect");
			dup2(fdOut,1);
			if(close(fdOut) == -1)
				syserror("Could not close fdout in > redirect");
			if(close(fdIn) == -1)
				syserror("Could not close fdin in > redirect");
			
			return execvp(cmd[0], cmd);
			syserror("Could not execute > redirect"); 
		}
		else if (pid == -1)
			syserror("could not create fork for > redirect");
	}
}

int CreateProcess(int input, int output, char* cmd[] )
{

  pid_t pid;
  /* check to make sure process is the child */
  if ((pid = fork()) == 0)
    {
      /* set in and out accordingly*/
      if (input != 0)
        {
          dup2(input, 0);
          if(close(input) == -1)
		syserror("Could not close input in CreateProcess");
        }

      if (output != 1)
        {
          dup2(output, 1);
         if( close(output) == -1)
		syserror("Could not close output in CreateProcess");
        }

      return execvp(cmd[0], cmd);
	syserror("Could not execute pipe!");
    }
  else if (pid == -1)
	syserror("Could not create fork in CreateProcess");
  /* parent returns pid, should not execute */
  return pid;
}

int PipeFork(int params, char* cmdArray[][MAX_LINE_WORDS+1], int* PipeType, int TotalRedirects)
{
  int i;
  pid_t pid;
  int input;
  int fd[2];
  if ((pid = fork()) == 0){
      int RedirectsFound = 0;
      /* first process input end is original fd[0]*/
      input = 0;
      /* pipe EVERYTHING EXCEPT the final pipe here */
      for (i = 0; i < params - 1; i++)
        {
         if ( pipe(fd) == -1 )
		syserror("Pipe failure");

          /* write to fd[1], in is transmitted from previous stage of pipe*/
        if (PipeType[i] == 0) {
	 CreateProcess(input, fd[1], cmdArray[i]);
	}
          /* close write end of pipe and save read end of pipe! */
         if (PipeType[i] == 1 || PipeType[i] == 2){
		RedirectsFound++;
		CreateRedirect(fd[1], input, cmdArray[i], PipeType[i], cmdArray[params-RedirectsFound]);
	}	
	  if(close(fd [1]) == -1)
		syserror("Could not close fd[1]");
          input = fd[0];
        }
	
	
      if (input != 0 && PipeType[params] == 0)
        dup2(input, 0);

      /* exec the command */
      return execvp(cmdArray[i][0], cmdArray[i]);

  }
  else if (pid == -1)
	syserror("Fork Failure");
  return pid;
}
