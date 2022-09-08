#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include "pmake.h"
#define MAXLINE 256


void run_make(char *target, Rule *rules, int pflag){
    // TODO
    Rule *curr = rules;
    //Find the target
	if (target != NULL){
		while (strcmp(curr->target, target) != 0){
			curr = curr->next_rule;
		}
		if (curr == NULL){
			perror("no target found");
			exit(1);
		}
	}
	if (pflag){
		pevaluate(curr);
	} else {
		evaluate(curr);
	}
	free_mem(rules);    
}
/* Parallel verison of evaluate that uses fork to go into each dependency and 
   recursively evaluate to see if we should execute actions.
 */
void pevaluate(Rule *rules){
	Rule *curr = rules;
	int child, status;
	// This is required to avoid duplicate forks of the same rule
	int visited = 0;

	if ((child = fork()) == -1){
		perror("fork");
		exit(0);
	}

	Dependency *curr_dep = curr->dependencies;

	if (child > 0){
		visited = 1;
	}

	while(curr_dep != NULL && visited != 1){
		pevaluate(curr_dep->rule);
		curr_dep = curr_dep->next_dep;
	}
	if (child > 0){
		wait(&status);
		if (WIFEXITED(status)){
			if (WEXITSTATUS(status) == 1){
				Action *curr_act = curr->actions;
				while (curr_act != NULL){
					execute(curr_act->args);
					curr_act = curr_act->next_act;
				}
			} else if (WEXITSTATUS(status) == 2){
				perror("lmt");
				exit(2);
			}
		}
	} else {
		exit(lmt(curr));
	}
}
/* Recusively Evaluate the Rule *rules to see if we should execute the actions */
void evaluate(Rule *rules){
	Rule *curr = rules;
	if (curr->dependencies != NULL){
	//Go to the dependencies and evaluate em
	Dependency *curr_dep = curr->dependencies;
	while(curr_dep != NULL){
		evaluate(curr_dep->rule);
		curr_dep = curr_dep->next_dep;
		}
	} 
	if (curr->actions == NULL){
		//Nothing to do...
		return;
	} else {
		Action *curr_act = curr->actions;
		while (curr_act != NULL){
			if (strcmp(curr_act->args[0], "gcc") == 0 || strstr(curr_act->args[0], "gcc") != NULL){
				if (lmt(curr) == 1){
					execute(curr_act->args);
					curr_act = curr_act->next_act;
				} else {
					curr_act = curr_act->next_act;
					continue;
				}
			} else {
				execute(curr_act->args);
				curr_act = curr_act->next_act;
			}
			
		}
	}
}

/* Execute args by forking and using the child to do the executing */
void execute(char **args){
	int child, status;
	child = fork();
	if (child == -1){
		perror("fork");
		exit(1);
	}
	if (child > 0){
		if (wait(&status) == -1){
			perror("wait");
			exit(1);
		}
		if (WIFEXITED(status)){
			if (WEXITSTATUS(status) == 0){
				return;
			} if (WEXITSTATUS(status) == -1) {
				perror("execvp returned -1");
				exit(1);
			} 
		}
	} else {
		char *buffer = malloc(MAXLINE * sizeof(char));
		printf("%s \n", args_to_string(args,buffer,MAXLINE));
		free(buffer);
		execvp(args[0], args);
		perror("execvp");
	}
}


/* Determines the Last Modified Time (lmt). Returns 0 if it hasn't been changed.
   Returns 1 if the current rule needs to execute because it (the target) doesn't exist 
   or if it has been recently updated
 */
int lmt(Rule *rules){
	Rule *curr = rules;
	//First get the target stats
	struct stat target_stat;
	int lmt = 0;
	if (stat(curr->target, &target_stat) < 0){
		//File isn't alive so lets execute our actions
		lmt = 1;
		return lmt;
	}
	Dependency *curr_dep = curr->dependencies;
	while (curr_dep != NULL){
		struct stat dep_stat;
		if (stat(curr->dependencies->rule->target, &dep_stat) < 0){
			//Dependency doesn't even exist... how did we get here
			perror("lmt Dependency DNE");
			exit(2);
		} 
		struct timespec target_time = target_stat.st_mtim;
		struct timespec dep_time = dep_stat.st_mtim;
		if (dep_time.tv_sec > target_time.tv_sec){
			lmt = 1;
			return lmt;
		} else if (dep_time.tv_sec == target_time.tv_sec) {
			 if (dep_time.tv_nsec > target_time.tv_nsec){
			lmt = 1;
			return lmt;
			}
		}
		curr_dep = curr_dep->next_dep;
	}
	return lmt;

}

/* Frees memory allocated in Rule rules throughout the program
   There is a memory leak, have mercy on me
 */
void free_mem(Rule *rules){
	Rule *curr_rul;
	Dependency *curr_dep;
	Action *curr_act;
	int wc = 0;
	Dependency *prev_dep;
	Action *prev_act;
	while (rules != NULL){
		curr_rul = rules;
		rules = rules->next_rule;
		curr_dep = curr_rul->dependencies;
		while (curr_dep != NULL){
			prev_dep = curr_dep;
			curr_dep = curr_dep->next_dep;
			free(prev_dep);
		}
		curr_act = curr_rul->actions;
		while(curr_act != NULL){
			char *buffer = malloc(MAXLINE * sizeof(char));
			buffer = args_to_string(curr_act->args, buffer, MAXLINE);
			wc = word_count(buffer);
			for (int i = 0; i <= wc; i++){
				free(curr_act->args[i]);
			}
			free(curr_act->args);
			free(buffer);
			prev_act = curr_act;
			curr_act = curr_act->next_act;
			free(prev_act);
		}
		free(curr_rul->target);
		free(curr_rul);
	}
}
