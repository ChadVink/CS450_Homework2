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

//int CreateProcess(struct command * cmd, int input, int output);
//int PipeFork(int n, struct command * cmd);

int PipeFork(int n,char* cmdArray[][MAX_LINE_WORDS+1], int* order);
int CreateProcess (int in, int out, char* cmd[]);
int CreateRedirect(int input, int output, char* cmd[], int type, char* fileloc[]);



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
        countpipes = 0;
        idx = 0;
	int order[100];
        num_words = split_cmd_line(line, line_words);
        int cmdCount = 0;
        memset(commandArray, 0, sizeof commandArray);
        //char* commandArray[MAX_LINE_WORDS + 1][MAX_LINE_WORDS + 1] = {};


         while( idx < num_words ){
	    int check1 = strcmp(line_words[idx], "|");
	    int check2 = strcmp(line_words[idx], ">");
	    int check3 = strcmp(line_words[idx], "<");
            if(strcmp(line_words[idx], "|") != 0 && check2 != 0 && check3 != 0){ // if line_words[idx] is not a "|"
                commandArray[countpipes][cmdCount] = line_words[idx];
                cmdCount++;
            }
            else{
		if (check1 == 0)
			order[countpipes] = 0;
		if (check2 == 0)
			order[countpipes] = 1;
		if (check3 == 0)
			order[countpipes] = 2;

                commandArray[countpipes][cmdCount+1] = 0;
                cmdCount = 0;
                countpipes++;
             }
            idx++;
        }

       /* int j;
        for( int i = 0; i < countpipes+1; i++ ){
            j = 0;
            while(commandArray[i][j] != 0){
                printf("%s ", commandArray[i][j]);
                j++;
            }
            printf("\n");
        }*/
        PipeFork(countpipes+1, commandArray, order);
    }
    
   //    printf("here");

        //struct command cmd[] = currentCommand;
        

         /*for (int i=0; i < num_words; i++){
	    if (strcmp(line_words[i], "|") == 0){
    		countpipes++;
            }
            printf("%s\n", line_words[i]);
	}
        printf("%d\n", countpipes);*/

//		const char *who[] = { "who", 0};
//		const char *wc[] = {"wc", "-l", 0};
	//	const char *ls[] = {"ls", "-l", 0};
	//	const char *awk[] = {"awk", "{print $1}", 0 };
	//	const char *sort[] = {"sort", 0};
	//	const char *uniq[] = {"uniq", 0};
//		struct command cmd[] = { {who}, {wc}};
		//struct command cmd[2];
		//cmd[0].parameters = who;
		//cmd[1].parameters = wc;
		// pass the number of commands to PipeFork as well as
		// the cmd struct with a char** array pointing to
		// array of command/parameters. Above is an example in hard
		// code. We have to take all the inputs from the user and pass
		// them to the PipeFork command.
		//return PipeFork(2, cmd);
	//printf("pipes: %d\n", countpipes);
	return 0;
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
	if(type == 2){
		if ((pid = fork()) == 0)
		{
			int fdIn = open(fileloc[0], O_RDONLY, 0666);
			dup2(fdIn, 0);
			close(fdIn);
//			int fdOut = output;
//			dup2(fdOut, 1);
			return execvp(cmd[0], cmd);
		}
	}
	else{
		if ((pid = fork()) == 0)
		{
			int fdIn = output;
			dup2(fdIn, 0);
			int fdOut = open(fileloc[0], O_WRONLY|O_CREAT, 0666);
			dup2(fdOut,1);
			close(fdOut);
			close(fdIn);

			return execvp(cmd[0], cmd);
		}
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
          close(input);
        }

      if (output != 1)
        {
          dup2(output, 1);
          close(output);
        }

      return execvp(cmd[0], cmd);
    }
  /* parent returns pid, should not execute */
  return pid;
}

int PipeFork(int params, char* cmdArray[][MAX_LINE_WORDS+1], int* order)
{
  int i;
  pid_t pid;
  int input;
  int fd[2];

  if ((pid = fork()) == 0){

      /* first process input end is original fd[0]*/
      input = 0;

      /* pipe EVERYTHING EXCEPT the final pipe here */
      for (i = 0; i < params - 1; i++)
        {
          pipe(fd);

          /* write to fd[1], in is transmitted from previous stage of pipe*/
        if (order[i] == 0) 
	 CreateProcess(input, fd [1], cmdArray[i]);
          /* close write end of pipe and save read end of pipe! */
         if (order[i] == 1 || order[i] == 2)
		CreateRedirect(fd[1], input, cmdArray[i], order[i], cmdArray[i+1]);

	  close(fd [1]);
          input = fd[0];
        }
	

      /* set stdin as read end of prev pipe and output this to fd*/
      if (input != 0)
        dup2(input, 0);

      /* exec the command */
      return execvp(cmdArray[i][0], cmdArray[i]);

  }
  return pid;
}
