#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LSH_RL_BUFSIZE 1024
#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"

//Function declaration for builtin shell commands. 
int lsh_cd(char **args);
int lsh_help(char **args);
int lsh_exit(char **args);


//List of builtin commands
char *builtin_str[] = {
		"cd",
		"help",
		"exit"
};

//Array of builtin functions
int (*builtin_func[]) (char **) = {
		&lsh_cd,
		&lsh_help,
		&lsh_exit
};

int lsh_num_builtins() {
	return sizeof(builtin_str) / sizeof(char*);
}

int lsh_cd(char **args){	
	if(args[1] == NULL){
		fprintf(stderr, "lsh : expected argument to \"cd\"\n");			
	} else {
		if(chdir(args[1]) != 0){
			perror("lsh");
		}
	}
	return 1;
}

int lsh_help(char **args){
		int i;
		printf("The following are built in:\n");

		for(i = 0; i < lsh_num_builtins(); i++)
				printf(" %s\n", builtin_str[i]);
		return 1;
}

int lsh_exit(char **args){
		return 0;
}
		
int lsh_launch(char **args){
	pid_t pid, wpid;
	int status;

	pid = fork();
	if(pid == 0){
			//Child process
			//Do system call of exec
			if(execvp(args[0], args) == -1){
					perror("lsh");
			}
			exit(EXIT_FAILURE);
	} else if (pid < 0){
			//Error forking
			perror("lsh");
	} else {	//Success
			//Parent process
			do {
				//Wait for process's state to change
				wpid = waitpid(pid, &status, WUNTRACED);
			} while(!WIFEXITED(status) && !WIFSIGNALED(status));

			return 1;
	}
}

int lsh_execute(char **args){
		int i;
		int args_num = sizeof(args) / sizeof(char*);

		if(args[0] == NULL){
				//An empty command
				return 1;
		}

		//TODO: check if pipeline
		for(i = 0; i < args_num; i++){
				if(strcmp(args[0], "|") == 0){
						//return lsh_pipeline();
				}
		}
		
		for(i = 0; i < lsh_num_builtins(); i++){
			if(strcmp(args[0], builtin_str[i]) == 0)
					return (*builtin_func[i])(args);
		}

		return lsh_launch(args);
}

char **lsh_split_line(char *line){
	int bufsize = LSH_TOK_BUFSIZE, position = 0;
	char **tokens = malloc(bufsize * sizeof(char*));
	char *token;

	if(!tokens){
		fprintf(stderr, "lsh : allocation error\n");
		exit(EXIT_FAILURE);
	}

	token = strtok(line, LSH_TOK_DELIM);
	while(token != NULL) {
			tokens[position] = token;
			position++;

			if(position >= bufsize){
					bufsize += LSH_TOK_BUFSIZE;
					tokens = realloc(tokens, bufsize * sizeof(char*));
					if(!tokens){
							fprintf(stderr, "lsh : allocation error\n");
							exit(EXIT_FAILURE);
					}
			}

			token = strtok(NULL, LSH_TOK_DELIM);
	}
	tokens[position] = NULL;
	return tokens;
}

char *lsh_read_line(void){
	int bufsize = LSH_RL_BUFSIZE;
	int position = 0;
	char *buffer = malloc(sizeof(char) * bufsize);
	int temp;

	if(!buffer){
		fprintf(stderr, "lsh : allocation error\n");
		exit(EXIT_FAILURE);
	}

	while (1){
		//Read a character
		temp = getchar();

		//Replace last character with a null character
		if( temp == EOF || temp == '\n' ){
			buffer[position] = '\0';
			return buffer;
		} else 
			buffer[position] = temp;
		position++;
		
		//If exceeds size of buffer, reallocate
		if (position >= bufsize) {
			bufsize += LSH_RL_BUFSIZE;
			buffer = realloc(buffer, bufsize);
			if(!buffer){
					fprintf(stderr, "lsh : allocation error\n");
					exit(EXIT_FAILURE);
			}
		}
		
	}
}

void lsh_loop(void){
	char *line;	//string
	char **args;	//array of string
	int status = 0;

	do {
		printf(">>> $");
		line = lsh_read_line();	//Read command line
		args = lsh_split_line(line);
		status = lsh_execute(args);		//Status flag determine when to terminate.
		free(line);
		free(args);
	} while(status);
}

void print_hello_message(){

	printf(
	"================================================\n"
	"* Welcome to my little shell:                  *\n"
	"*                                              *\n"
	"* Type \"help\" to see builtin functions.        *\n"
	"*                                              *\n"
	"* If you want to do things below:              *\n"
	"* + redirection: \">\" or \"<\"                    *\n"
	"* + pipe: \"|\"                                  *\n"
	"* + background: \"&\"                            *\n"
	"* Make sure they are seperated by \"(space)\".   *\n"

	"*                                              *\n"
	"* Have fun!!                                   *\n"
	"================================================\n"
	);
}

int main(int argc, char** argv){

	//Print welcome message
	print_hello_message();

	//Run command loop
	lsh_loop();

	return EXIT_SUCCESS;
}

