#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define MAX_ARGS 32
#define MAX_NAME 128


/* Return 1 if the line contains only spaces or a comment (#)
   Return 0 if the line contains any other character before a #

   We want to ignore empty lines or lines that contain only a comment.  This
   helper function identifies such lines.
 */
int is_comment_or_empty(char *line) {
    
    for(int i = 0; i < strlen(line); i++){
        if(line[i] == '#') {
            return 1;
        }
        if(line[i] != '\t' && line[i] != ' ') {
            return 0;
        }
    }
    return 1;
}

/* Convert an array of args to a single space-separated string in buffer.
   Returns buffer.  Note that memory for args and buffer should be allocted
   by the caller.
 */
char *args_to_string(char **args, char *buffer, int size) {
    buffer[0] = '\0';
    int i = 0;
    while(args[i] != NULL) {
        strncat(buffer, args[i], size - strlen(buffer));
        strncat(buffer, " ", size - strlen(buffer));
        i++;
    }
    return buffer;
}

/* Count the number of words in a string array line and return the value/
   Ignore anything past #!
  */
int word_count(char *line){
    int wc = 0;
    int isword = 0;
    int i = 0;
    while (line[i]){
      if (line[i] == ' ' || line[i] == '\t' || line[i] == '\n'){
        isword = 0;
      } else if (line[i] == '#'){
        break;
      } else if (isword == 0){
        isword = 1;
        wc++;
      }
      i++;
    }
    return wc;
}

/* Create an array of arguments suitable for passing into execvp 
   from "line". line is expected to start with a tab and contain a
   action from a Makefile. Remember to ensure that the last element
   of the array is NULL.

   It is fine to use MAX_ARGS to allocate enough space for the arguments
   rather than iterating through line twice. You may want to use strtok to
   split the line into separate tokens.

   Return NULL if there are only spaces and/or tabs in the line. No memory
   should be allocated and the return value will be NULL.

   I've made this usable to split any string array composed of words seperated
   by tab or space. Stop once we see a #.

   I've also done something stupid which is considering cases with folders
   and spaces. This took too long....

   I think most of the memory leaks come from here. I may be copying a string 
   literal into the malloced char arrays.

   Also I do think the folder space part is buggy. There can be weird cases 
   with repeating characters in the path or something dumb. Please have mercy.
 */

char **build_args(char *line) {
    // All actions must begin with a tab. This tab counter will ensure a '/0' will be added 
    // to the action args.
    int tab = 0;
    //Check if the line is NULL
    if (is_comment_or_empty(line) == 1){
      return NULL;
    } else {
      if (line[0] == '\t'){
        tab = 1;
      }
      int i = 0;
      int slashcounter = 0;
      while (line[i]){
      	if (line[i] == '/'){
      		slashcounter++;
      	}
      	i++;
      }
      if (slashcounter == 1 || slashcounter == 0){
	      int wc = word_count(line) + tab;
	      char **args = malloc(wc * sizeof(char *));
	      for (int i = 0; i < wc; i++){
	        args[i] = malloc(MAX_ARGS * sizeof(char));
	      }
	      int j,k, isword;
	      j = 0;
	      k = 0;
	      isword = 0;
	      i = 0;
	      while (line[i]){
	        if (line[i] == ' ' || line[i] == '\t' || line[i] == '\0' || line[i] == '\n'){
	          if (isword == 1){
	            args[j][k] = '\0';
	            isword = 0;
	            j++;
	          }
	          k = 0;
	        } else if (line[i] == '#'){
	          break;
	        } else {	          
	          isword = 1;
	          args[j][k] = line[i];
	          k++;
	        }
	        i++;
	      }
	      if (tab == 1){
	      	if (slashcounter == 1 && args[0][0] != '.'){
	      		// The exec is in the current working directory so add a dot to the front
	      		char temp[MAX_ARGS] = ".";
	      		strncpy(temp+1, args[0], MAX_ARGS);
	      		strncpy(args[0], temp,MAX_ARGS);
	        	} 
	        args[wc-1] = '\0';
	      }
	      return args;
      } else {
      	// There were multiple slashes, its time to build it differently
      	// This scenario is so insane please have mercy
      	// Some cases the buffer will overflow....
      	// So it wont work correctly
      	char *buffer = malloc(MAX_ARGS * sizeof(char));
      	// First check if there are any spaces
      	// In between the /s 
      	// I can only assume... that there wont be anything really messed up like the example below
      	// /usr/bin/gcc flags /this is a spaced out folder/exec.c
      	// This only takes care if there are spaces in the 'first' arg
      	if (line[0] == '\t'){
      		line++;
      		char *temp = malloc(MAX_ARGS * sizeof(char));
      		if (strchr(strrchr(line, '/'), ' ') != NULL){
      			strncpy(temp, strchr(strrchr(line, '/'), ' '), MAX_ARGS);
      		} else { 
      			strncpy(temp, strrchr(line, '/'), MAX_ARGS);
      		}
      		strncpy(buffer, line, strlen(line) - strlen(temp));
      		if (strchr(temp, '/') != NULL){
      			strncat(buffer, temp, MAX_ARGS);
      		}
      		buffer[MAX_ARGS-1] = '\0';
      		temp[MAX_ARGS-1] = '\0';
      		i = 0;
      		//temp will have the args for the executable....
      		if (strstr(buffer, temp) == NULL){
      			int wc = word_count(temp);
      			//One for buffer and one for the null term
      			wc = wc + 2;
			      char **args = malloc(wc * sizeof(char *));
			      for (int i = 0; i < wc; i++){
			        args[i] = malloc(MAX_ARGS * sizeof(char));
			      }
			      int j,k, isword;
			      j = 1;
			      k = 0;
			      isword = 0;
			      while (temp[i]){
			        if (temp[i] == ' ' || temp[i] == '\t' || temp[i] == '\0' || temp[i] == '\n'){
			          if (isword == 1){
			            args[j][k] = '\0';
			            isword = 0;
			            j++;
			          }
			          k = 0;
			        } else if (temp[i] == '#'){
			          break;
			        } else {	          
			          isword = 1;
			          args[j][k] = temp[i];
			          k++;
			        }
			        i++;      			 
      			}
      			args[wc-1] = '\0';
      			args[0] = buffer;
      			return args;
      		} else {
      			// temp is a substring of buffer so it is probably just an executable
      			char **args = malloc(2 * sizeof(char *));
      			// clean up buffer
      			int i = 0;
      			while (buffer[i]){
      				if (buffer[i] == '\n'){
      					buffer[i] = '\0';
      				}
      				i++;
      			}
      			args[0] = buffer;
      			args[1] = '\0';
      			return args;
      		}
      	} else {
      		perror("help");
      		exit(1);
      	}
    }
	}
}
/* Splits a string array line into a 2d string array consisting 
   of the target and dependecies and stores into provided 2x2 char array buff. 
   buf[0] = args[0] = target which is on heap
   args[1] = dependencies
  */

void split_line(char *line, char *buff[2]){

    char *args[2];
    args[1] = strchr(line, ':');
    int len = strlen(line) - strlen(args[1]);
    args[0] = malloc((len+1) * sizeof(char));
    strncpy(args[0], line, len);
    //Remove any trailing whitespace in the target
    char *temp = args[0] + strlen(args[0]) - 1;
    while (temp > args[0] && isspace((char)*temp)){
    	temp--;
    }
    temp[1] = '\0';
    //Remove colon from the split
    args[1]++;
    int i = 0;
    //Remove whitespace in the front
    while (isspace(args[1][i])){
      args[1]++;
      i++;
    }
    buff[0] = args[0];
    buff[1] = args[1];
  }
