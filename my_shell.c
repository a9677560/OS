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
#define MAX_CHAR_COUNT 50
#define LSH_TOK_DELIM " \t\r\n\a"
#define CMD_ARR_COUNT 16


/****************************************
 *Unfinished function:                  * 
 * 1.Pipeline of builtin function       *
 * 2.Get parent and child process id    *
 ****************************************/

//Function declaration for builtin shell commands. 
int lsh_cd(char ***args, int fd_in, int fd_out, int pipes_count, int pipes_fd[][2]);
int lsh_help(char ***args, int fd_in, int fd_out, int pipes_count, int pipes_fd[][2]);
int lsh_exit(char ***args, int fd_in, int fd_out, int pipes_count, int pipes_fd[][2]);
int lsh_echo(char ***args, int fd_in, int fd_out, int pipes_count, int pipes_fd[][2]);
int lsh_record(char ***args, int fd_in, int fd_out, int pipes_count, int pipes_fd[][2]);
int lsh_replay(char ***argv, int fd_in, int fd_out, int pipes_count, int pipes_fd[][2]);
int lsh_mypid(char ***args, int fd_in, int fd_out, int pipes_count, int pipes_fd[][2]);

char *lsh_read_line(void);
char *str_strip(char *str);
char ***lsh_split_line(char *line);
int lsh_execute(char ***args);
char commands[CMD_ARR_COUNT][MAX_CHAR_COUNT], rear = -1, front = 0;

//List of builtin commands
char *builtin_str[] = {
		"cd",
		"help",
		"exit",
		"echo",
		"record",
		"replay",
		"mypid"
};

//Array of builtin functions

int (*builtin_function[]) (char ***, int, int, int, int[][2]) = {
		&lsh_cd,
		&lsh_help,
		&lsh_exit, 
		&lsh_echo,
		&lsh_record,
		&lsh_replay, 
		&lsh_mypid
};

//Include stack push and operation when queue is full
void queue_add(char* str){
		int total_count = rear - front + 1;
		char temp[6];
		if(str == NULL)
				return;
		if(strcmp(strncpy(temp, str, 6), "replay") == 0)
				return;
		if(total_count <= 0 && rear != -1)
				total_count = CMD_ARR_COUNT;

		if(total_count == CMD_ARR_COUNT)
			front = (front + 1) % CMD_ARR_COUNT;
		rear = (rear + 1) % CMD_ARR_COUNT;
		strcpy(commands[rear], str);
}

int lsh_mypid(char ***args, int fd_in, int fd_out, int pipes_count, int pipes_fd[][2]){
		if(args[0][1] == NULL){
			fprintf(stderr, "lsh : expected argument to \"mypid\"\n");			
		} else { 
			if(strcmp(args[0][1], "-i") == 0)
				printf("%d\n", getpid());
		}
		return 1;
}

int lsh_record(char ***args, int fd_in, int fd_out, int pipes_count, int pipes_fd[][2]){
    int i, j;
	printf("history cmd:\n");
	for(i = front, j = 0; j < CMD_ARR_COUNT; i = (i + 1) % CMD_ARR_COUNT, j++){
			printf("%2d: %s\n", j + 1, commands[i]);
	}

	return 1;
}

int lsh_replay(char ***argv, int fd_in, int fd_out, int pipes_count, int pipes_fd[][2]){
		int status = 0;
		int pipe_count = 0;
		if(atoi(argv[0][1]) <= 0 || atoi(argv[0][1]) >= 17){
			fprintf(stderr, "replay: wrong args\n");		
		} else {
			char *line;
			char ***args;
			int index = front + atoi(argv[0][1]) - 1;

			if(index > 16)
					index -= 16;
			line = (char*)malloc(sizeof(commands[index]));
			strcpy(line, commands[index]);

			//Check if pipeline exists
			if(pipes_count > 0){
					for(int i = 1; i < pipe_count; i++){
						int argv_count = 0;
						while(argv[i][argv_count] != NULL)
								argv_count++;

						strcat(line,  " |");
						for(int j = 0; j < argv_count; j++){
							strcat(line, " ");
							strcat(line, argv[i][j]);
						}
					}
			}
			queue_add(line);
			args = lsh_split_line(line);
			status = lsh_execute(args);		//Status flag determine when to terminate.
		}
		return status;
}

int lsh_num_builtins() {
	return sizeof(builtin_str) / sizeof(char*);
}

int lsh_echo(char ***args, int fd_in, int fd_out, int pipes_count, int pipes_fd[][2]){
		//No ability to deal with pipeline
		int args_count = 0;
		
		while(args[0][args_count] != NULL)
				args_count++;

		if(strcmp(args[0][1], "-n") == 0){
			for(int i = 2; i < args_count; i++)
					printf("%s ", args[0][i]);
		} else {
			for(int i = 1; i < args_count; i++){
					printf("%s ", args[0][i]);
					fflush(stdout);
			}
			printf("\n");
		}
		//output file redirection
		int redirection_index = 0, output_flag = 0;
		//if(pipes_count > 0){
			args_count = 0;
			while(args[pipes_count][args_count] != NULL)
					args_count++;
			for(int i = 1; i < args_count; i++){
				if(strcmp(args[pipes_count][i], ">") == 0){
					redirection_index = i;
					output_flag = 1;
					break;
				}
			}
		//}

		if(fd_in != STDIN_FILENO)
			dup2(fd_in, STDIN_FILENO);
		if(fd_out != STDOUT_FILENO)
			dup2(fd_out, STDOUT_FILENO);
		else{
			if(output_flag){
				fd_out = open(args[pipes_count][redirection_index+1], O_RDWR | O_CREAT | O_TRUNC , 0777);
				dup2(fd_out, STDOUT_FILENO);
			}
		}
		return 1;
}

int lsh_cd(char ***args, int fd_in, int fd_out, int pipes_count, int pipes_fd[][2]){	
	if(args[0][1] == NULL){
		fprintf(stderr, "lsh : expected argument to \"cd\"\n");			
	} else {
		if(chdir(args[0][1]) != 0){
			perror("lsh");
		}
	}
	return 1;
}

int lsh_help(char ***args, int fd_in, int fd_out, int pipes_count, int pipes_fd[][2]){
		int i;
		printf("The following are built in:\n");

		for(i = 0; i < lsh_num_builtins(); i++)
				printf(" %s\n", builtin_str[i]);
		return 1;
}

int lsh_exit(char ***args, int fd_in, int fd_out, int pipes_count, int pipes_fd[][2]){
		return 0;
}
		
int lsh_launch(char **args, int fd_in, int fd_out, int pipes_count, int pipes_fd[][2]){
	pid_t pid, wpid;
	int status;
	int args_count = 0;
	int input_flag = 0, output_flag = 0;
	int redirection_index = 0;
	
	while(args[args_count] != NULL)
		args_count++;
		//printf("args count is: %d\n", args_count);

	pid = fork();
	//printf("Id of %s is %ld\n", args[0], (long)pid);
	if(pid == 0){
			//Child process
			//Do system call of exec
			
			//Check if there is & character
			if(strcmp(args[args_count - 1], "&") == 0){
					args[args_count - 1] = NULL;
					--args_count;
			}
			
			//Check if there is an redirection	
			for(int i = 0; i < args_count; i++){
					if(strcmp(args[i], ">") == 0){
							output_flag = 1;
							redirection_index = i;
							break;
					}

					if(strcmp(args[i], "<") == 0){
							input_flag = 1;
							redirection_index = i;
							break;
					}
			}

			if(fd_in != STDIN_FILENO)
				dup2(fd_in, STDIN_FILENO);
			else{
				if(input_flag == 1){
					fd_in = open(args[redirection_index+1], O_RDONLY);
					dup2(fd_in, STDIN_FILENO);
					//Clear '<'
					args[redirection_index] = NULL;
				}
			}
			if(fd_out != STDOUT_FILENO)
				dup2(fd_out, STDOUT_FILENO);
			else{
				if(output_flag == 1){
					fd_out = open(args[redirection_index+1], O_RDWR | O_CREAT | O_TRUNC , 0777); 
					dup2(fd_out, STDOUT_FILENO);
					//Clear '>'
					args[redirection_index] = NULL;
				}
			}

			//Close pipelines of child
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
			if(strcmp(args[args_count - 1], "&") == 0){
				printf("[Pid]: %d\n", pid);
				fflush(stdout);
			} else	
				wait(NULL);		//Wait parent process to finish.
	}

	return 1;
}

int lsh_execute(char ***args){
		int i, C, P;
		int status = 0, builtin = 0, builtin_index = 0;
		int cmd_count = 0, pipeline_count = 0;	
		int pipes_fd[MAX_CMD_COUNT][2];

		if(args == NULL)	//An empty command
			return 1;

		//if use sizeof/sizeof, will get count = MAX_CMD_COUNT	
		while(args[cmd_count])
			++cmd_count;
		pipeline_count = cmd_count - 1;

		//Suppose builtin function only happens in first command
		for(i = 0; i < lsh_num_builtins(); i++){
			if(strcmp(args[0][0], builtin_str[i]) == 0){
		    	//status = (*builtin_function[i])(args);
				builtin = 1;
				builtin_index = i;
				break;
			}
		}
		
		for(P = 0; P < pipeline_count; P++){
				if(pipe(pipes_fd[P]) == -1){
						fprintf(stderr, "Error: Unable to create pipe (%d)\n", P);
						exit(EXIT_FAILURE);
				}
		}

		for(C = 0; C < cmd_count; C++){
			int fd_in = (C == 0) ? (STDIN_FILENO) : (pipes_fd[C - 1][0]);
			int fd_out = (C == cmd_count - 1) ? (STDOUT_FILENO) : (pipes_fd[C][1]);
				
			if(C > 0)
				close(pipes_fd[C - 1][1]);
			//Check if builtin function exists.
			if(builtin){	
				status = (*builtin_function[i])(args, fd_in, fd_out, pipeline_count, pipes_fd);
				builtin = 0;
				if(pipeline_count == 0)
					return status;
			}
				status = lsh_launch(args[C], fd_in, fd_out, pipeline_count, pipes_fd);
		}

		for(P = 0; P < pipeline_count; P++){
				close(pipes_fd[P][0]);
				close(pipes_fd[P][1]);
		}

		/*for(C = 0; C < cmd_count; C++){
				int temp;
				wait(&temp);
		}*/
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

	//fputs(">>> $ ", stdout);
	printf(">>> $ ");
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
		queue_add(line);
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

	memset(commands, '\0', sizeof(commands));
	//Print welcome message
	print_hello_message();

	//Run command loop
	lsh_loop();

	return EXIT_SUCCESS;
}

