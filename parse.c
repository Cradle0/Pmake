#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <ctype.h>

#include "pmake.h"
#define MAXLINE 256



/* Print the list of actions */
// I've noticed that this function may run into a seg fault if it tries
// to look at act->args[i] when we haven't properly allocated it
// It doesn't happen anymore though....
void print_actions(Action *act) {
    while(act != NULL) {
        if(act->args == NULL) {
            fprintf(stderr, "ERROR: action with NULL args\n");
            act = act->next_act;
            continue;
        }
        printf("\t");

        int i = 0;
        while(act->args[i] != NULL) {
            printf("%s ", act->args[i]) ;
            i++;
        }
        printf("\n");
        act = act->next_act;
    }    
}

/* Print the list of rules to stdout in makefile format. If the output
   of print_rules is saved to a file, it should be possible to use it to 
   run make correctly.
 */
void print_rules(Rule *rules){
    Rule *cur = rules;
    
    while(cur != NULL) {
        if(cur->dependencies || cur->actions) {
            // Print target
            printf("%s : ", cur->target);
            
            // Print dependencies
            Dependency *dep = cur->dependencies;
            while(dep != NULL){
                if(dep->rule->target == NULL) {
                    fprintf(stderr, "ERROR: dependency with NULL rule\n");
                }
                printf("%s ", dep->rule->target);
                dep = dep->next_dep;
            }
            printf("\n");
            
            // Print actions
            print_actions(cur->actions);
        }
        cur = cur->next_rule;
    }
}

/* Create the rules data structure and return it.
   Figure out what to do with each line from the open file fp
     - If a line starts with a tab it is an action line for the current rule
     - If a line starts with a word character it is a target line, and we will
       create a new rule
     - If a line starts with a '#' or '\n' it is a comment or a blank line 
       and should be ignored. 
     - If a line only has space characters ('', '\t', '\n') in it, it should be
       ignored.
 */
Rule *parse_file(FILE *fp) {

    // TODO
    Rule *head_rule = malloc(sizeof(Rule));
    char line[MAXLINE];
    char *target = NULL;
    while (fgets(line, MAXLINE, fp) != NULL){
        if (is_comment_or_empty(line) == 0){
            // First character of the line a letter so make a new rule
            if (isalpha(line[0]) != 0){
                target = build_dependencies(head_rule, line);
            // First char is a tab so its the action of the target          
            } else if (line[0] == '\t') {
                build_actions(head_rule, line, target);
            }
        }   
    }
    return head_rule;   

}


/* Build dependencies in Rule rules for the given line. This includes creating the rules and dependencies for the target
   and linking them all together nicely. Return the target for build_actions.
 */
char *build_dependencies(Rule *rules, char *line){
    Rule *curr = rules;
    // Let's split the line to get the dependency and target
    char *tar_dep[2];
    split_line(line, tar_dep);
    int head = 0;
    // We are at the first node  
    if (curr->target == NULL){
        curr->target = tar_dep[0];
        head = 1;
    } else {
        //Find the target
        while (curr->next_rule != NULL && strcmp(curr->target, tar_dep[0]) != 0){
            curr = curr->next_rule;
        }
    }
    // Check if we even have a dependency
    if (strlen(tar_dep[1]) != 0){
        // We may be at the last node since the target doesn't even exist....
        if (curr->next_rule == NULL && head == 0 && strcmp(curr->target, tar_dep[0]) != 0 ){
            Rule *new_rule = create_rule(tar_dep[0], NULL, NULL, NULL);
            curr->next_rule = new_rule;
            curr = curr->next_rule;
        } 
        int dep_wc = word_count(tar_dep[1]);
        char **depend_list = build_args(tar_dep[1]);
        // original remembers what rule the dependencies are being added to
        Rule *original = curr;
        // we gotta go through the rule list again to see if dependencies already
        // exist
        for (int i = 0; i < dep_wc; i++){
            Rule *curr2 = rules;
            while (curr2 != NULL && strcmp(curr2->target, depend_list[i]) != 0){
                curr2 = curr2->next_rule;
            }
            // The dependency wasn't found so add it 
            if (curr2 == NULL){
                //if dep doesn't exist.... create it
                Rule *new_rule = create_rule(depend_list[i], NULL, NULL, NULL);
                // Add it to the end of the list
                while (curr->next_rule != NULL){
                    curr = curr->next_rule;
                }
                curr->next_rule = new_rule;
                // Now check if the rule we are targeting as dependencies already for it
                // And add them
                if (original->dependencies == NULL){
                    original->dependencies = create_dep(curr->next_rule, NULL);
                    continue;
                } else {
                    while (original->dependencies->next_dep != NULL){
                        original->dependencies = original->dependencies->next_dep;
                    }
                    original->dependencies->next_dep = create_dep(curr->next_rule, NULL);
                }
            } else {
                // The dependency actually exists and curr2 is at it
                // Remember curr is our target too.
                // We need to link the dependency node we will make 
                // to the rule at curr2
                if (original->dependencies == NULL){
                    original->dependencies = create_dep(curr2, NULL);
                    continue;
                } else {
                    while (original->dependencies->next_dep != NULL){
                        original->dependencies = original->dependencies->next_dep;
                    }
                    original->dependencies->next_dep = create_dep(curr2, NULL);
                }
            }
        }
        free(depend_list);

    } else {
        curr->next_rule = create_rule(tar_dep[0], NULL, NULL, NULL);
    }
    return tar_dep[0];
}


/* Build actions in Rule *head_rule for given string line (which starts with a tab)
   and target. Target should exist in head_rule.
 */
void build_actions(Rule *head_rule, char *line, char *target){
    Rule *curr = head_rule;
    // Find the target
    while (strcmp(curr->target, target) != 0){
        curr = curr->next_rule;
    }
    // Target DNE, mistakes were made
    if (curr == NULL){
        perror("Target DNE");
        exit(1);
    }
    char **action_list = build_args(line);
    Action *new_act = create_act(action_list, NULL);
    if (curr->actions == NULL){
        curr->actions = new_act;
        return;
    }
    while (curr->actions->next_act != NULL){
        curr->actions = curr->actions->next_act;
    }
    curr->actions->next_act = new_act;
}

Rule *create_rule(char *target, Dependency *dependency, Action *action, Rule *next_rule){
    Rule *new_rule = malloc(sizeof(Rule));
    new_rule->target = target;
    new_rule->dependencies = dependency;
    new_rule->actions = action;
    new_rule->next_rule = next_rule;
    return new_rule; 
}

Dependency *create_dep(Rule *rule, Dependency *next_dep){
    Dependency *new_dep = malloc(sizeof(Dependency));
    new_dep->rule = rule;
    new_dep->next_dep = next_dep; 
    return new_dep;
}

Action *create_act(char **args, Action *next_act){
    Action *new_act = malloc(sizeof(Action));
    new_act->args = args;
    new_act->next_act = next_act;
    return new_act;
}

