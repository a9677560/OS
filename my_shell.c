#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>

#define LSH_RL_BUFSIZE 1024
#define MAX_ARG_COUNT 15
#define MAX_CMD_COUNT 5
#define LSH_TOK_DELIM " \t\r\n\a"

//Function declaration for builtin shell commands. 
int lsh_cd(char **args);
int lsh_help(char **args);
int lsh_exit(char **args);

char *str_strip(char *str);

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
		
int lsh_launch(char **args, int fd_in, int fd_out, int pipes_count, int pipes_fd[][2]){
	pid_t pid, wpid;
	int status;

	pid = fork();
	if(pid == 0){
			//Child process
			//Do system call of exec
			
			//Check if pipeline exists.
			if(fd_in != STDIN_FILENO)
					dup2(fd_in, STDIN_FILENO);
			if(fd_out != STDOUT_FILENO)
					dup2(fd_out, STDOUT_FILENO);
			
			for(int P = 0; P < pipes_count; P++){
					close(pipes_fd[P][0]);	
					close(pipes_fd[P][1]);
			}

			if(execvp(args[0], args) == -1){
					perror("lsh");
					exit(EXIT_FAILURE);
			}

			exit(EXIT_FAILURE);
	} else if (pid < 0){
			//Error forking
			perror("Error: Unable to fork.\n");
			exit(EXIT_FAILURE);
	} else {	
			/* This will cause pipeline to stuck in some case.
			 * EX: cat text.txt | tail -2
			 *
			//Parent process
			do {
				//Wait for process's state to change
				wpid = waitpid(pid, &status, WUNTRACED);
			} while(!WIFEXITED(status) && !WIFSIGNALED(status));
			*/
	}

	return 1;
}

int lsh_execute(char ***args){
		int i, C, P;
		int status = 0;
		int cmd_count = 0, pipeline_count = 0;
		
		if(args == NULL)	//An empty command
				return 1;

		//Temparorily not deal with pipeline of builtin command.
		for(i = 0; i < lsh_num_builtins(); i++){
			if(strcmp(args[0][0], builtin_str[i]) == 0)
					return (*builtin_func[i])(args[0]);
		}
		
		//if use sizeof/sizeof, will get count = MAX_CMD_COUNT	
		while(args[cmd_count])
				++cmd_count;
		pipeline_count = cmd_count - 1;

		int pipes_fd[MAX_CMD_COUNT][2];

		for(P = 0; P < pipeline_count; P++){
				if(pipe(pipes_fd[P]) == -1){
						fprintf(stderr, "Error: Unable to create pipe (%d)\n", P);
								exit(EXIT_FAILURE);
				}
		}

		for(C = 0; C < cmd_count; C++){
				int fd_in = (C == 0) ? (STDIN_FILENO) : (pipes_fd[C - 1][0]);
				int fd_out = (C == cmd_count - 1) ? (STDOUT_FILENO) : (pipes_fd[C][1]);

				status = lsh_launch(args[C], fd_in, fd_out, pipeline_count, pipes_fd);
		}

		for(P = 0; P < pipeline_count; P++){
				close(pipes_fd[P][0]);
				close(pipes_fd[P][1]);
		}

		for(C = 0; C < cmd_count; C++){
				int temp;
				wait(&temp);
		}
		return status;
}

/* TODO: Convert a string to args array array */
char ***lsh_split_line(char *line){
	int i, j;


	if(line == NULL)
			return NULL;
	static char *cmds[MAX_CMD_COUNT + 1];
	memset(cmds, '\0', sizeof(cmds));

	//Split cmds
	cmds[0] = str_strip(strtok(line, "|"));
	for(i = 1; i <= MAX_CMD_COUNT; i++){
			cmds[i] = str_strip(strtok(NULL, "|"));
			if(cmds[i] == NULL)	//No more cmds
					break;
	}

	static char *args_array[MAX_CMD_COUNT + 1][MAX_ARG_COUNT + 1];
	static char **args[MAX_CMD_COUNT + 1];

	memset(args_array, '\0', sizeof(args_array));
	memset(args, '\0', sizeof(args));

	for(i = 0; cmds[i]; i++){
		args[i] = args_array[i];

		args[i][0] = strtok(cmds[i], LSH_TOK_DELIM);
		for(j = 1; j <= MAX_ARG_COUNT; j++){
				args[i][j] = strtok(NULL, LSH_TOK_DELIM);
				
				if(args[i][j] == NULL)
						break;
		}
	}

	return args;
}

char *lsh_read_line(void){
	static char cmd_seq_buffer[LSH_RL_BUFSIZE];

	fputs(">>> $ ", stdout);
	fflush(stdout);

	memset(cmd_seq_buffer, '\0', sizeof(cmd_seq_buffer));
	fgets(cmd_seq_buffer, sizeof(cmd_seq_buffer), stdin);

	if (feof(stdin))
			return NULL;

	char *line = str_strip(cmd_seq_buffer);
	if(strlen(line) == 0)
			return NULL;

	return line;
}

char *str_strip(char *str){
		if(!str)
				return str;	//NULL

		while(isspace(*str))
				++str;

		char *last = str;
		while(*last != '\0') 
				++last;
		last--;		//move to the character before '\0'

		while(isspace(*last)) 
				*(last--) = '\0';	//replace space with \0

		return str;
}

void lsh_loop(void){
	char *line;	//string
	char ***args;	//array of array of string
	int status = 0;

	do {
		//printf(">>> $");
		line = lsh_read_line();	//Read command line
		args = lsh_split_line(line);
		status = lsh_execute(args);		//Status flag determine when to terminate.
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

