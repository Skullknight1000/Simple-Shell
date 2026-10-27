#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include "utils.h"

/*
Purpose: reads a line from stdin, allowing any length
After reading, trim leading/trailing whitespace
Returns heap allocated string or null (on end of file or reading error)
*/
char *read_line(void){
	char *line = NULL; //allocated by getline on heap
	long capacity = 0; //getline determines buffer size
	long numread; //number of bytes read, returns -1 on error

	numread = (long)getline(&line, (unsigned long *)&capacity, stdin);

	//return on read error
	if(numread == -1){
		free(line);
		return NULL;
	}

	/*
	trim trailing '\n' from getline, replace it with '\0'. 
	Do not modify if last byte is not a newline
	*/

	if(numread > 0 && line[numread - 1] == '\n'){
		line[numread - 1] = '\0';
	}

	/*
	keep chars inside quotes.
	pass over the string, track quotes and [first_keep, last_keep) range of chars to keep
	*/
	long first_keep = -1; //no content yet
	long last_keep = 0; //the region to keep is empty
	char quoted = 0;

	//loop over the string
	for(long i = 0; line[i] != '\0'; i++){
		char c = line[i];
		
		//opening quote, keep and start tracking quoted content
		if(!quoted && (c == '"' || c == '\'')){
			quoted = c;
			if(first_keep == -1){
				first_keep = i;
			}
			last_keep = i + 1;
		}
		//closing quote, keep it and not tracking
		else if(quoted && c == quoted){
			quoted = 0;
			last_keep = i + 1;
		}
		//anything inside quotes is kept, including whitespace
		else if(quoted){
			last_keep = i + 1;
		}
		//non-whitespace char outside quotes
		else if(!isspace(c)){
			if(first_keep == -1){
				first_keep = i;
			}
			last_keep = i + 1;
		}
		//whitespace outside quotes are skipped
	}

	
	//compute trimmed length based on what was not trimmed
	long trimmed_len = 0;
	if(first_keep != -1){
		trimmed_len = last_keep - first_keep;
	}

	//free getline buffer 
	char* result = malloc(trimmed_len + 1);
	if(result == NULL){
		free(line);
		return NULL;
	}

	//return the trimmed copy of the input
	if(first_keep != -1){
		for(long i = 0; i < trimmed_len; i++){
			result[i] = line[first_keep + i];
		}
	}
	//add trailing '\0'
	result[trimmed_len] = '\0';
	free(line);
	return result;
}

/*
Purpose: helper function for tokenize()
Copies len chars into new heap buffer and adds \0
Returns new heap buffer size of len + 1
*/
static char *slice_token(const char *start, long len){
	char *out = malloc(len + 1);
	if(out == NULL){
		return NULL;
	}
	for(long i = 0; i < len; i++){
		out[i] = start[i];
	}
	out[len] = '\0';
	return out;
}

/*
Purpose: Counts whitespace-seperated tokens in nullterm string (how many tokens would be produced from user input)
helper function for tokenize()
Does not allocate anything
Uses first_quoted_space() to find boundaries
Returns number of tokens found from input, or 0
*/

static long count_tokens(const char *input){
	long count = 0;
	const char *p = input;

	while(*p != '\0'){

		//skip any whitespace between tokens
		while(*p != '\0' && isspace((unsigned char)*p)){
			p++;
		}
	
		//if there is still content, then it is a token
		if(*p != '\0'){
			count++;
		
			//find length and go past it
			int index = first_unquoted_space(p);
			long raw_len;
			if(index == -1){
				raw_len = 0;
				while(p[raw_len] != '\0'){
					raw_len++;
				}
			}
			else{
				raw_len = index;
			}
			p += raw_len;
			if(*p != '\0'){
				p++;
			}
		}
	}
	return count;
}

/*
Purpose: frees tokens. Helper function of tokenize()
Frees every string in the array, then array itself
*/
void free_tokens(char **tokens){
	if(tokens == NULL) return;

	for(long i = 0; tokens[i] != NULL; i++){
		free(tokens[i]);
	}
	free(tokens);
}

/*
Purpose: Take input string and split it into tokens
Uses first_quoted_space() to find boundaries and unescape() to strip quotes and do escapes
Does two passes, first pass counts the amount of tokens, second slices the tokens and cleans up with unescape
Returns a nullterminated heap array of tokens strings or NULL on allocation error
*/
char **tokenize(const char *input){
	//if input is null
	if(input == NULL){
		return NULL;
	}
	
	//count num of tokens
	long total = count_tokens(input);
	
	//allocate +1 for null terminator
	char **tokens = malloc((total + 1) * sizeof(char *));
	if(tokens == NULL){
		return NULL;
	}

	//slice and clean every token
	long count = 0;
	const char *p = input;

	while(*p != '\0'){
		while(*p != '\0' && isspace((unsigned char)*p)){
			p++;
		}

		if(*p != '\0'){
			//find token length
			int index = first_unquoted_space(p);
			long raw_len;
			if(index == -1){
				raw_len = 0;
				while(p[raw_len] != '\0'){
					raw_len++;
				}
			}
			else{
				raw_len = index;
			}

			//slice raw token
			char *raw = slice_token(p, raw_len);
			if(raw == NULL){
				tokens[count] = NULL;
				free_tokens(tokens);
				return NULL;
			}

			//clean escapes and strip quotes
			char *cleaned = unescape(raw, stderr);
			free(raw);
			if(cleaned == NULL){
				tokens[count] = NULL;
				free_tokens(tokens);
				return NULL;
			}

			tokens[count] = cleaned;
			count++;

			//finished with current token and its trailing whitespace
			p += raw_len;
			if(*p != '\0'){
				p++;
			}
		}
	}

	//nullterminate, count should be same as total (number of tokens in user input)
	tokens[count] = NULL;
	return tokens;
}

/*
Purpose: Clears .421history on session start
Creates the file if missing, clears it if already exists
*/
void clear_history(void){
	//name of file
	const char *filename = ".history_file";
	

	FILE *filepointer = fopen(filename, "w");
	if(filepointer == NULL){
		perror("unable to open .history_file");
	}
	else{
		fclose(filepointer);
	}
}

/*
Purpose: Add history to .421history
File grows without limit during session
*/
void add_history(const char *line){

	//name of file
	const char *filename = ".history_file";
	
	//if the input to add to .421history is null or blank
	if(line == NULL || line[0] == '\0'){
		return;
	}
	
	//opens the file to add, new line goes to the end
	FILE *filepointer = fopen(filename, "a");
	if(filepointer == NULL){
		perror("could not append to .history_file");
		return;
	}
	
	//writes line into .421history
	fprintf(filepointer, "%s\n", line);
	fclose(filepointer);
}

/*
Purpose: Show the last 10 commands by user
Print past 11 (10 INCLUDING history call) inputs
*/
void show_history(void){
	const char *filename = ".history_file";
	int show_num = 10; //number of past commands to show
	int array_size = show_num + 1; //array that is 1 more than show_num for the history call itself

	//create an array to hold the lines

	char *arr[array_size];
	for(int i = 0; i < array_size; i++){
		arr[i] = NULL;
	}

	//open .421history
	FILE *filepointer = fopen(filename, "r");
	//if .421history couldn't be opened
	if(filepointer == NULL){
		perror("could not read .history_file");
		return;
	}

	//read lines into the array
	int next = 0; //index of array to write next
	int stored = 0; //number of lines read in total
	int abort = 0; //number/flag used to show malloc error to end loop
	char *buffer = NULL;
	long buff_cap = 0;
	long numread = 0; //number of bytes read

	while(numread != -1 && !abort){
		numread = (long)getline(&buffer, (unsigned long *)&buff_cap, filepointer);

		if(numread != -1){
			//strip trailing newline
			if(numread > 0 && buffer[numread - 1] == '\n'){
				buffer[numread - 1] = '\0';
			}

			//get the line length
			int len = 0;
			while(buffer[len] != '\0'){
				len++;
			}

			//create new copy of line at current index
			char *copy = malloc(len + 1);
			if(copy == NULL){
				perror("malloc line copy");
				abort = 1; //abort the loop
			}
			else{
				for (int i = 0; i <= len; i++){
					copy[i] = buffer[i];
				}
		
		
			//free current index before storing
			free(arr[next]);
			arr[next] = copy;

			//move to next index, wrapping at array_size
			next = (next + 1) % array_size;
			stored++;
			}

		}
	}
	free(buffer);
	fclose(filepointer);

	//decide on what to print, skip newest line (current "history")
	int count;
	if(stored < array_size){
		count = stored;
	}
	else{
		count = array_size;
	}

	if(count > 0){
		//skip current 'history' call
		int print_count = count - 1;
		if(print_count > show_num){
			print_count = show_num;
		}
		int arr_start;
		if(stored >= array_size){
			arr_start = next;
		}
		else{
			arr_start = 0;
		}

		int end = count - 1;
		int start = end - print_count;

		for(int i = start; i < end; i++){
			int index = (arr_start + i) % array_size;
			printf("%s\n", arr[index]);
		}
	}

	//free every index in array
	for(int i = 0; i < array_size; i++){
		free(arr[i]);
	}
}

/*
Purpose: Tries to exit the shell.
Returns int value representing if running loop should continue or end
Anything extra after exit should continue the loop
*/
int try_exit(char **tokens){
	//check that user did not anything other arguments after 'exit'
	if(tokens[1] != NULL){
		fprintf(stderr, "ERROR: could not exit, too many arguments\n");
		return 1; //loop should continue
	}
	printf("\n");
	return 0; //loop should end

}

/*
Purpose: View any files within proc filesystem
Prints/outputs these files to user
*/

void procread(char **tokens){
	//count number of arguments that user provided
	int args = 0;
	while(tokens[args] != NULL){
		args++;
	}

	//case where there is no argument, only procread was typed
	if(args == 1){
		fprintf(stderr, "ERROR: procread requires an argument\n");
		return;
	}

	//case where users gives too many arguments
	if(args > 2){
		fprintf(stderr, "ERROR: procread only takes one argument\n");
		return;
	}

	//args should be exactly 2, tokens[0] is "procread" and tokens[1] is whatever user inputed
	const char *procfile = tokens[1];

	//check if user inputed a leading /
	if(procfile[0] == '/'){
		fprintf(stderr, "ERROR: file cannot start with '/' \n");
		return;
	}

	fflush(stdout); //ensure that parent text is flushed before child starts, avoid duplicate output
	//fork, fork() returns 0 to child, child's pid to parent
	int pid = fork();

	//check that fork worked
	if(pid < 0){
		perror("fork failed");
		return;
	}

	/*
	build path "/proc/" + user inputed path, open, and print it
	print a message and exit with _exit on any error
	*/
	//in child process
	if(pid == 0){
	        //build "/proc/" + user input into buffer
	        const char *prefix = "/proc/";

	        //manually get the length to get the amount to malloc
	        int prefix_len = 0;
	        while(prefix[prefix_len] != '\0'){
        	        prefix_len++;
	        }

	        int arg_len = 0;
	        while(procfile[arg_len] != '\0'){
        	        arg_len++;
	        }

	        //malloc the amount for the path, + 1 for \0
	        char *path = malloc(prefix_len + arg_len + 1);
	        //check that path was allocated correctly
	        if(path == NULL){
        	        fprintf(stderr, "ERROR: malloc error\n");
	                _exit(1);
	        }      

		//copy prefix and arg into path
		int i = 0;
		for(; i < prefix_len; i++){
			path[i] = prefix[i];
		}
		for(int j = 0; j < arg_len; j++){

			path[i + j] = procfile[j];
		}
		path[prefix_len + arg_len] = '\0';

		//try to open the file, _exit if it does not exist
		FILE *filepointer = fopen(path, "r");
		if(filepointer == NULL){
			fprintf(stderr, "ERROR: command failed, does file exist?\n");
			free(path);
			_exit(1);
		}

		//print file contents
		int c;
		//prints one char at a time until fgetc returns EOF
		while((c = fgetc(filepointer)) != EOF){
			fprintf(stdout, "%c", c);
		}

		fclose(filepointer);
		free(path);
		//force buffered output from stdout
		fflush(stdout);
		_exit(0);
	}	

	//parent process, wait for child to finish
	int status;
	//waitpid blocks until child exists/terminated
	waitpid(pid, &status, 0);
}

/*
Purpose: Execute user inputed command.
Takes command that was already split into tokens and runs it
Run tokens[0] as the command, pass tokens[1], tokens[2], etc as arguments
*/

void system_command(char **tokens){
	fflush(stdout); //ensure that parent text is flushed before child starts, avoid duplicate output
	//create copy of current process
	int pid = fork();

	//check if fork failed
	if(pid < 0){
		perror("fork failed");
		return;
	}

	//in child process, become user's program
	if(pid == 0){
		
		/*
		tokens[0] is program name/path, other elements in tokens are arguments
		child process is replaced by program
		*/
		execvp(tokens[0], tokens);

		//execvp fails, could not execute user command
		perror(tokens[0]);
		_exit(1);
	}

	//parent process, wait for child to finish
	int status;
	waitpid(pid, &status, 0);
	
}


int main(void) {
	int running = 1; //int to act as flag if the loop should continue
	clear_history();
	while (running) {
		printf("SimpleShell $ ");
	        fflush(stdout);

        	char *line = read_line();

		//EOF error
	        if (line == NULL) {
	            printf("\n[EOF]\n");
        	    running = 0;
	        }
		//blank line, skip it
	        else if (line[0] == '\0') {
        	    free(line);
	        }
		//user entered input
	        else {
		
			add_history(line);
			//if user lead with " or '\'
			if (line[0] == '"' || line[0] == '\'') {
				fprintf(stderr, "ERROR: command not found!\n");
	                        free(line);
                	}
			else{
				//split the input into tokens
				char **tokens = tokenize(line);

				//if tokenize returns NULL
				if(tokens == NULL){
					free(line);
				}

				//empty token list
				else if(tokens[0] == NULL){
					free_tokens(tokens);
					free(line);
				}

				//using history (nothing else after 'history')
				else if ((strcmp(tokens[0], "history") == 0) && tokens[1] == NULL){
					show_history();
					free_tokens(tokens);
					free(line);
				}
				//trying to exit
				else if(strcmp(tokens[0], "exit") == 0){
					running = try_exit(tokens);
					free_tokens(tokens);
					free(line);
				}
				//using procread
				else if(strcmp(tokens[0], "procread") == 0){
					procread(tokens);
					free_tokens(tokens);
					free(line);
				}
				//deal with anything else
				else{
					//run as system command
					system_command(tokens);
					free_tokens(tokens);
					free(line);
				}
			}
		}
	}
	return 0;
}
