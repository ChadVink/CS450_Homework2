#include <stdio.h>
#include "constants.h"
#include "parsetools.h"
#include <sys/wait.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
struct command
{	

	const char ** argv;

};

void syserror(const char* s);

//int CreateProcess(struct command * cmd, int input, int output);
//int PipeFork(int n, struct command * cmd);

int PipeFork(int n, struct command *cmd);
int CreateProcess (int in, int out, struct command *cmd);



int main() {
    // Buffer for reading one line of input
    char line[MAX_LINE_CHARS];
    char* line_words[MAX_LINE_WORDS + 1];
    int countpipes; // Number of pipes that are read in
    int num_words;
    int idx; // index of the word on the line_words
        //char* currentCommand[MAX_LINE_WORDS + 1];

    // Loop until user hits Ctrl-D (end of input)
    // or some other input error occurs
    while( fgets(line, MAX_LINE_CHARS, stdin) ) {
        countpipes = 0;
        idx = 0;
        num_words = split_cmd_line(line, line_words);
        int cmdCount = 0;
        char** commandArray[MAX_LINE_WORDS + 1];

         while( idx < num_words ){
            char* currentCommand[MAX_LINE_WORDS + 1];
            
            while( idx < num_words && strcmp(line_words[idx], "|") != 0){
                &currentCommand[cmdCount] = &line_words[idx];
                cmdCount++;
                idx++;
            }
            currentCommand[cmdCount+1] = 0;
    	    &commandArray[countpipes] = &currentCommand;
            cmdCount = 0;
             if( idx < num_words && strcmp(line_words[idx], "|") == 0){
                countpipes++;          
                idx++;
             }
        }
           //printf("im out\n"); 
            /*if (idx == 0){
                char* currentCommand[MAX_LINE_WORDS + 1] = {};
            }*/

            //if (strcmp(line_words[idx], "|") == 0){
                //currentCommand[++cmdCount] = 0;
    		//commandArray[countpipes++] = currentCommand;
                //cmdCount = 0;
              //  char* currentCommand[MAX_LINE_WORDS +1] = {};
            //}else{
             //   currentCommand[cmdCount++] = line_words[idx];
            //}

      
        int j;
        for( int i = 0; i < countpipes; i++ ){
            j = 0;
            while(commandArray[i][j] != 0){
                printf("%s\n", commandArray[i][j]);
                j++;
            }
        }

        
        //struct command cmd[] = currentCommand;
        //PipeFork(countpipes, cmd);
        
         
         /*for (int i=0; i < num_words; i++){
	    if (strcmp(line_words[i], "|") == 0){
    		countpipes++;
            }

            printf("%s\n", line_words[i]);
	}
        printf("%d\n", countpipes);*/
    }
	



	//	const char *who[] = { "who", 0};
	//	const char *wc[] = {"wc", "-l", 0};
	//	const char *ls[] = {"ls", "-l", 0};
	//	const char *awk[] = {"awk", "{print $1}", 0 };
	//	const char *sort[] = {"sort", 0};
	//	const char *uniq[] = {"uniq", 0};
	//	struct command cmd[] = { {who}, {wc} };
	//	return PipeFork(2, cmd);
	//printf("pipes: %d\n", countpipes);	
}



void syserror(const char *s)
{
    extern int errno;

    fprintf( stderr, "%s\n", s );
    fprintf( stderr, " (%s)\n", strerror(errno) );
    exit( 1 );
}

int CreateProcess (int in, int out, struct command *cmd)
{
  pid_t pid;

  if ((pid = fork ()) == 0)
    {
      if (in != 0)
        {
          dup2 (in, 0);
          close (in);
        }

      if (out != 1)
        {
          dup2 (out, 1);
          close (out);
        }

      return execvp (cmd->argv [0], (char * const *)cmd->argv);
    }

  return pid;
}

int PipeFork(int n, struct command *cmd)
{
  int i;
  pid_t pid;
  int in, fd [2];

  /* The first process should get its input from the original file descriptor 0.  */
  in = 0;

  /* Note the loop bound, we spawn here all, but the last stage of the pipeline.  */
  for (i = 0; i < n - 1; ++i)
    {
      pipe (fd);

      /* f [1] is the write end of the pipe, we carry `in` from the prev iteration.  */
      CreateProcess (in, fd [1], cmd + i);

      /* No need for the write end of the pipe, the child will write here.  */
      close (fd [1]);

      /* Keep the read end of the pipe, the next child will read from there.  */
      in = fd [0];
    }

  /* Last stage of the pipeline - set stdin be the read end of the previous pipe
 *      and output to the original file descriptor 1. */  
  if (in != 0)
    dup2 (in, 0);

  /* Execute the last stage with the current process. */
  return execvp (cmd [i].argv [0], (char * const *)cmd [i].argv);
}


