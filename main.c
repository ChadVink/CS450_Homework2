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

//int PipeFork(int params,struct  command* cmd, int* order);
//int CreateRedirect(int in, int out, struct command* cmd, int type, struct command* FileLoc);
//int CreateProcess (int in, int out, struct command* cmd, int type);


int PipeFork(int params, char* cmdArray[][MAX_LINE_WORDS+1], int* order);
int CreateRedirect(int input, int output, char* cmd[], int type, char* fileloc[]);
int CreateProcess(int input, int output, char* cmd[], int type);



int TotalNumRedirects = 0;

int main(){

    char line[MAX_LINE_CHARS];
    char* line_words[MAX_LINE_WORDS + 1]; 
    int countpipes; // Number of pipes that are read in
    int num_words;
    int idx; // index of the word on the line_words
    char* commandArray[MAX_LINE_WORDS + 1][MAX_LINE_WORDS + 1];
    int order[50];
   // int NumRedirects = 0;
   // char* commandArray[MAX_LINE_WORDS + 1][MAX_LINE_WORDS + 1];


    while( fgets(line, MAX_LINE_CHARS, stdin) ) {
        countpipes = 0;
        idx = 0;
        num_words = split_cmd_line(line, line_words);
        int cmdCount = 0;
        //char* commandArray[MAX_LINE_WORDS + 1][MAX_LINE_WORDS + 1];
        memset(commandArray, 0, sizeof commandArray); 
	while( idx < num_words ){
	   if (strcmp(line_words[idx], "|") == 0)
		order[countpipes-1] = 0;
	   if (strcmp(line_words[idx], ">") == 0)
	   {	order[countpipes-1] = 1;
	        TotalNumRedirects++;
	   }
       if (strcmp(line_words[idx], "<") == 0){
		order[countpipes-1] = 2;
		TotalNumRedirects++;
	}
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
	PipeFork(countpipes+1, commandArray, order);
     }
		/*order[0] = 0;
		order[1] = 1;
		order[2] = 0; 
		
		const char *who[] = { "who", 0};
		const char *wc[] = {"wc","-l", 0};
		const char *o[] = {"o.txt", 0};
		const char *u[] = {"i.txt", 0};
		struct command cmd[] ={{who}, {wc} , {o}};
		return PipeFork(3, cmd, order);*/
}



void syserror(const char *s)
{
    extern int errno;

    fprintf( stderr, "%s\n", s );
    fprintf( stderr, " (%s)\n", strerror(errno) );
    exit( 1 );
}

int CreateRedirect(int input, int output, char* cmd[], int type, char* fileloc[]){
	pid_t pid;

	if (type == 2){
		// "<" found
		if ((pid = fork()) == 0)
		{
	           if(input){
	             int fdIn = open(fileloc[0], O_RDONLY, 0666);
	             dup2(fdIn, 0);
		     int fdOut = input;
		     dup2(fdOut, 1);
		     close(fdOut);
	             close(fdIn);
	          //   printf("donezo\n");
	           
		 return  execvp(cmd[0], cmd);
			}
	   	}
	}
	else if (type == 1){
		// ">" found
		if ((pid = fork()) == 0){
	           if (output)
	           {
		     int fdIn = output;
		     dup2(fdIn, 0);
	             int fdOut = open(fileloc[0], O_WRONLY|O_CREAT,0666);
	             dup2 (fdOut, 1);
	             close(fdOut);
		     close(fdIn);
	             printf("cmd %s\n", cmd[0]); 
	                                                                  
	          
		 return execvp(cmd[0], cmd);
		   }		
		}
	}


}


int CreateProcess(int input, int output, char* cmd[], int type)
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
//	printf("nu2: %s\n", cmd->parameters[0]);
      return execvp(cmd[0], cmd);

    }
  return pid;
}

int PipeFork(int params, char* cmdArray[][MAX_LINE_WORDS+1], int* order)
{
  int i;
  pid_t pid;
  int input;
  int fd[2];
  int RedirectIdx[params];
  int NumRedirects = 0; 
  if ((pid = fork()) == 0){
  /* first process input end is original fd[0]*/
  input = 0;
  /* pipe EVERYTHING EXCEPT the final pipe here */
  for (i = 0; i < params - 1; ++i)
  {
      pipe(fd);
	//printf("order %d: %d\n", i, order[i]);
      /* write to fd[1], in is transmitted from previous stage of pipe*/
     if (order[i] == 0){
      CreateProcess(input, fd [1], cmdArray[i], order[i]);
	//close(fd[1]);
	//input = fd[0];
	
	}
     else if (order[i] == 1){
      CreateRedirect(fd[1], input, cmdArray[i] , order[i], cmdArray[(params-i)]);		
	}
     else if (order[i] == 2){
    	CreateRedirect(fd[1], input, cmdArray[i] , order[i], cmdArray[params-i]);
	}
	/* close write end of pipe and save read end of pipe! */
       close(fd [1]);
       input = fd[0];
    }

  /* set stdin as read end of prev pipe and output this to fd*/  
  if (input != 0 && order[i] == 0)
    dup2(input, 0);
 //printf("out: %s\n", cmd[i].parameters[0]);
  return execvp(cmdArray[i][0], cmdArray[i]);
}
return pid;
}
