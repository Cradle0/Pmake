#define MAXLINE 256

/* A line from a makefile is either a target line, an action line, 
or a comment or line to ignore
     "target" line
        - Starts with a target, followed by a colon followed by a 
        space-separated list of dependencies
        - A target is a word that follows the Rule of valid file names  
        (assume that only valid file names are used in testing)
        - The colon has a space on either side of it
        - assume the line will not exceed MAXLINE
        - The list of dependencies follow the same Rule as the target

     "action" line
        - Begins with a tab
        - Contains a line that can be executed. First word is the relative path
        to the executable, remaining tokens are argument to the executable.
        - relative path means that if the executable is in the current working 
        directory, the path begins with "./" (Do not assume "." is in the path)
     
     Comment or empty line
        - contains only spaces and/or tabs
        - contains 0 or more spaces or tabs followed by a '#'.  Any other 
          characters after the '#' are ignored.
 */

typedef struct action_node {
    char **args;  // an array of strings suitable to be passed to execvp
    struct action_node *next_act;
} Action;

typedef struct dep_node {
    struct rule_node *rule;
    struct dep_node *next_dep;
} Dependency;

typedef struct rule_node{
    char *target;
    Dependency *dependencies;
    Action *actions;
    struct rule_node *next_rule;
} Rule;

/* Print the list of rules to stdout in makefile format */
void print_rules(Rule *rules);

/* Read from the open file fp, and creat the linked data structure
   that represents the Makefile contained in the file.
 */
Rule *parse_file(FILE *fp);

/* Return an array of strings where each string is one word in line. The final
   element of the array will contain NULL.
 */
char **build_args(char *line);


/* Return 1 if line contains only space characters, and 0 otherwise */
int is_comment_or_empty(char *line);

/* Concatenates args into a single string and store it in buffer, up to size.
   Return a pointer to buffer.
 */
char *args_to_string(char **args, char *buffer, int size);

/* Count the number of words in a string array line and return count */
int word_count(char *line);

/* Split the line getted by fgets into 2d string array of the target and dependency and store it in buff */
void split_line(char *line, char *buff[2]);

/* Build dependencies for a given string line in the Rule head_rule (which is the head of the rule list).
   Return a pointer to the target for build_actions. 
 */
char *build_dependencies(Rule *rules, char *line);

/* Build actions for a given line (which starts with a tab '\t')  and its target in Rule head_rule  */
void build_actions(Rule *head_rule, char *line, char *target);

/* These are helper functions which will create Rule, Dependency or Action nodes */
Rule *create_rule(char *target, Dependency *dependency, Action *action, Rule *next_rule);
Dependency *create_dep(Rule *rule, Dependency *next_dep);
Action *create_act(char **args, Action *next_act);


/* Evaluate the rule in rules with the target "target"
   If target is NULL, evaluate the first rule in rules.
   If pflag is 1, then create a new process to evaluate each dependency. The
   parent process will wait until all child processes have terminated before
   checking dependecy modfied times to decide whether to execute the actions
 */
void run_make(char *target, Rule *rules, int pflag);


/* The helper function which does the evaluating above on Rule *rules.
   Is recursive, but does not fork. It evaluates rules to see if we should
   update them, if we do execute the actions of that rule.
 */
void evaluate(Rule *rules);

/* The helper function which runs if pflag is 1. Works like evaluate, except
   we fork and recusively call pevaluate to do the same thing as evaluate 
 */
void pevaluate(Rule *rules);

/* Executes the given args */
void execute(char **args);


/* Determines the last modified time for a target and returns 1 if it has changed 
   or if the target doesn't even exist or 0 if it hasn't.
 */
int lmt(Rule *rules);


/* Frees the memory allocated in Rule *rules */
void free_mem(Rule *rules);

