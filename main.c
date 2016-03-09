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


int help = 1;
struct command
{	

	const char ** parameters;
	
};

void syserror(const char* s);

int PipeFork(int params,struct  command* cmd, int* order);
int CreateRedirect(int in, int out, struct command* cmd, int type);
int CreateProcess (int in, int out, struct command* cmd, int type);



int main(){

    char line[MAX_LINE_CHARS];
    char* line_words[MAX_LINE_WORDS + 1]; 
    int countpipes; // Number of pipes that are read in
    int num_words;
    int idx; // index of the word on the line_words
    char* commandArray[MAX_LINE_WORDS + 1][MAX_LINE_WORDS + 1];
    int order[50];



    while( fgets(line, MAX_LINE_CHARS, stdin) ) {
        countpipes = 0;
        idx = 0;
        num_words = split_cmd_line(line, line_words);
        int cmdCount = 0;
        char* commandArray[MAX_LINE_WORDS + 1][MAX_LINE_WORDS + 1];
         while( idx < num_words ){
	   if (strcmp(line_words[idx], "|") == 0)
		order[countpipes-1] = 0;
	   if (strcmp(line_words[idx], ">") == 0)
		order[countpipes-1] = 1;
       if (strcmp(line_words[idx], "<") == 0)
		order[countpipes-1] = 2;





	    if(strcmp(line_words[idx], "|") != 0){ // if line_words[idx] is not a "|"
                commandArray[countpipes][cmdCount] = line_words[idx];
                cmdCount++;
            }
            else{
                commandArray[countpipes][cmdCount+1] = 0;
                cmdCount = 0;
                countpipes++;          
             }
            idx++;
        }


        int j;
        for( int i = 0; i < countpipes+1; i++ ){
            j = 0;
            while(commandArray[i][j] != 0){
                printf("%s ", commandArray[i][j]);
                j++;
            }
            printf("\n");
        }
     }
		order[0] = 0;
		order[1] = 0;
		const char *who[] = { "who", 0};
		const char *wc[] = {"wc", "-l", 0};
		struct command cmd[] ={{who},{wc}};
		return PipeFork(2, cmd, order);
}



void syserror(const char *s)
{
    extern int errno;

    fprintf( stderr, "%s\n", s );
    fprintf( stderr, " (%s)\n", strerror(errno) );
    exit( 1 );
}

int CreateRedirect(int input, int output, struct command* cmd, int type){
	pid_t pid;

	if (type == 2){
		// "<" found
		if ((pid = fork()) == 0)
		{
	           if(input){
	             int fdIn = open(cmd->parameters[0], O_RDONLY);
	             dup2(fdIn, 0);
	             close(fdIn);
	             input = 0;
	           }
		  execvp(cmd->parameters[0], (char* const*)cmd->parameters);
	
		}
	}
	else if (type == 1){
		// ">" found
		if ((pid = fork()) == 0){
	           if (output)
	           {
	             int fdOut = open(cmd->parameters[0], O_WRONLY);
	             dup2 (fdOut, 1);
	             close(fdOut);
	             output = 0;
	                                                                      
	          }
		  execvp(cmd->parameters[0], (char* const*)cmd->parameters);
		}
	}


}


int CreateProcess(int input, int output, struct command *cmd, int type)
{  
  pid_t pid;
  /* check to make sure process is the child */
  if ((pid = fork()) == 0)
    {
      /* set in and out accordingly*/
      if (input != 0)
        {
          dup2(input, 0);
          close(input);
        }

      if (output != 1)
        {
          dup2(output, 1);
          close(output);
        }

      return execvp(cmd->parameters[0], (char * const *)cmd->parameters);

    }
  return pid;
}

int PipeFork(int params, struct command *cmd, int* order)
{
  int i;
  pid_t pid;
  int input;
  int fd[2];
 

  /* first process input end is original fd[0]*/
  input = 0;

  /* pipe EVERYTHING EXCEPT the final pipe here */
  for (i = 0; i < params - 1; ++i)
  {
      pipe(fd);

      /* write to fd[1], in is transmitted from previous stage of pipe*/
     if (order[i] == 0){
      CreateProcess(input, fd [1], cmd + i, order[i]);
	//printf("cond met\n");
	}
     else if (order[i] > 0){
      CreateRedirect(input, fd[1], cmd + i, order[i]);
	//printf("cond2 met\n");
	}
	/* close write end of pipe and save read end of pipe! */
      close(fd [1]);
      input = fd[0];
    }

  /* set stdin as read end of prev pipe and output this to fd*/  
  if (input != 0)
    dup2(input, 0);

  /* exec the command */
  return execvp(cmd[i].parameters[0], (char * const *)cmd[i].parameters);
}
