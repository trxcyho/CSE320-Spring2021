/*
 * Imprimer: Command-line interface
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "imprimer.h"
#include "conversions.h"
#include "sf_readline.h"

//added things
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>

//protoype
char *readfile(FILE *in);
char** convert_to_commands(char* line);
int operation(int num_args, char** arguments, FILE *out);
int string_to_number(char *string);
int valid_printer(char *name);
void starting_job(PRINTER *printer, JOB *job);
void look_for_jobs();
void sigchild_handler(int sig);
int job_index_from_pid(pid_t pid);
int printer_index_from_pid(pid_t pid);
void free_memory();


//define printer and job from imprimer.h
struct printer {
	char *name;
	struct file_type *file;
	PRINTER_STATUS pstatus;
};
//create array for printers
PRINTER *printer_array[MAX_PRINTERS];
pid_t printer_pid[MAX_PRINTERS];

struct job {
	char *filename;
	struct file_type *file;
	JOB_STATUS jstatus;
	int eligible;
	time_t todelete;
};
//create array for jobs
JOB *job_array[MAX_JOBS];
pid_t job_pid[MAX_JOBS];

int num_args = 0;
int printer_count = 0; //make sure it dont go over 32 when adding
// int num_jobs = 0;//make sure it dont go over 64 when adding
int quit = 0; //when to exit cli
int intialization = 0;



int run_cli(FILE *in, FILE *out)
{
	if(in == NULL)
		return -1;
	if(intialization == 0){
		for(int i = 0; i < MAX_JOBS; i++){
			job_array[i] = NULL;
			job_pid[i] = 0;
		}
		for(int i = 0; i < MAX_PRINTERS; i++)
			printer_pid[i] = 0;
	}
	intialization = 1;

	char **arguments;
	signal(SIGCHLD, sigchild_handler);
	if(in == NULL)
		return -1;
	//create a buffer to hold the storage of the lines
	if(in != stdin){
		char *filecommand = readfile(in);
		while (filecommand != NULL){
			look_for_jobs();
			arguments = convert_to_commands(filecommand);
			operation(num_args, arguments, out);
			//reset everything
			for(int i = 0; i < num_args; i++)
				free(arguments[i]);
			free(arguments);
			num_args = 0;
			//read next line
			filecommand = readfile(in);
		}
	}
	else{
		while(!quit){
	    	look_for_jobs();
	    	char* inputcommand = sf_readline("imp> ");
			//if input command is NULL ->return -1//do we stop execution of processes, free everything
			if(inputcommand == NULL){//if sf_readline is EOF
				//call a function that frees everything
				free_memory();
				return -1;
			}
	    	arguments = convert_to_commands(inputcommand);
	    	// printf("%s\n", arguments[0]);
	    	operation(num_args, arguments, out);

	    	//free anything that is needed
			//if quit is called return 0
			for(int i = 0; i < num_args; i++)
				free(arguments[i]);
			free(arguments);
			num_args = 0;
	    }
	}
	if(in != stdin && quit == 1)
		return -1;

	return 0;
}

char *readfile(FILE *in){
	int buffersize = 32, counter = 0;
	char *buffer = (char *)calloc(buffersize, sizeof(char));
	int c;
	while(1){
		c = fgetc(in);
		if(c == EOF)
			return NULL;
		if(c == '\n'){
			buffer[counter] = '\0';
			return buffer;
		}
		buffer[counter] = c;
		counter++;
		//increase buffersize by 1
		if(counter == buffersize){
			buffersize *= 2;
			buffer = realloc(buffer, buffersize);
		}
	}
}

//have a function that converts char* to char ** then call operation
char** convert_to_commands(char* line){
	char *line_duplicate = malloc(strlen(line) + 1);
	strcpy(line_duplicate, line);
	char *word = strtok(line, " ");
	while(word != NULL){
		num_args++;
		word = strtok(NULL, " ");
	}
	char **arguments = calloc(num_args, sizeof(char*));
	word = strtok(line_duplicate, " ");
	int index = 0;
	while (word != NULL){
		arguments[index] = calloc(strlen(word) + 1, sizeof(char));
		strcpy(arguments[index], word);
		index++;
		word = strtok(NULL, " ");
	}
	free(line);
	free(line_duplicate);
	return arguments;
}

int operation(int num_args, char** arguments, FILE *out){
	if(arguments[0] == NULL){
		//throw an error;
		return -1;
	}
	if(strcmp("help", arguments[0]) == 0){
		if (num_args != 1){
			sf_cmd_error("arg count");
			return -1;
		}
		//print help
		fprintf(out, "Commands are: help quit type printer conversion printers jobs print cancel disable enable pause resume\n");
		sf_cmd_ok();
		return 0;
	}
	if(strcmp("quit", arguments[0]) == 0){
		if(num_args != 1){
			sf_cmd_error("arg count");
			return -1;
		}
		//free all the printers and jobs here?
		free_memory();
		quit = 1;
		sf_cmd_ok();
		return 0;
	}
	if (strcmp("printers", arguments[0]) == 0){
		if(num_args != 1){
			sf_cmd_error("arg count");
			return -1;
		}
		//print all printers
		//id, name, type, status
		for(int i = 0; i < printer_count; i++){
			PRINTER *printer = printer_array[i];
			fprintf(out, "PRINTER: id=%d, name=%s, type=%s, status=%s\n", i, printer -> name,
				printer -> file -> name, printer_status_names[printer -> pstatus]);
		}

		sf_cmd_ok();
		return 0;
	}
	if (strcmp("jobs", arguments[0]) == 0){
		if(num_args != 1){
			sf_cmd_error("arg count");
			return -1;
		}
		//print all jobs
		//type, status, eligible printers, file
		for(int i = 0; i < MAX_JOBS; i++){
			JOB *job = job_array[i];
			if(job != NULL){
				fprintf(out, "JOB[%d]: type=%s, eligible=%x, file=%s, status=%s\n", i, job -> file -> name,
					job -> eligible, job -> filename, job_status_names[job -> jstatus]);
			}
		}
		sf_cmd_ok();
		return 0;
	}
	if(strcmp("type", arguments[0]) == 0){
		if(num_args != 2){
			sf_cmd_error("arg count");
			return -1;
		}
		FILE_TYPE *type = find_type(arguments[1]);
		if(type != NULL){
			sf_cmd_error("type defined already");
			return -1;
		}
		define_type(arguments[1]); //define a new filetype
		sf_cmd_ok();
		return 0;
	}
	if(strcmp("printer", arguments[0]) == 0){
		if(num_args != 3){
			sf_cmd_error("arg count");
			return -1;
		}
		if(printer_count >= MAX_PRINTERS){
			sf_cmd_error("max printers reached");
			return -1;
		}

		//check to make sure name is unique first
		if(valid_printer(arguments[1]) != -1){
			sf_cmd_error("printer name not unique");
			return -1;
		}
		//make sure filetype is valid
		FILE_TYPE *type = find_type(arguments[2]);
		if(type == NULL){
			sf_cmd_error("type not defined");
			return -1;
		}
		PRINTER *newprinter = malloc(sizeof(PRINTER));
		char *newname = malloc(strlen(arguments[1]) + 1);
		strcpy(newname, arguments[1]);
		newprinter -> name = newname;
		newprinter -> file = type;
		newprinter -> pstatus = PRINTER_DISABLED;
		sf_printer_defined(arguments[1], arguments[2]);
		//add printer to array
		printer_array[printer_count] = newprinter;
		printer_count++;
		sf_cmd_ok();
		return 0;
	}

	if(strcmp("conversion", arguments[0]) == 0){
		if(num_args < 4){
			sf_cmd_error("arg count");
			return -1;
		}
		//make sure arg[1] and arg[2] are actual file types
		FILE_TYPE *type1 = find_type(arguments[1]);
		if(type1 == NULL){
			sf_cmd_error("type not defined");
			return -1;
		}
		FILE_TYPE *type2 = find_type(arguments[2]);
		if(type2 == NULL){
			sf_cmd_error("type not defined");
			return -1;
		}

		char** program_args = calloc(num_args - 3 + 1, sizeof(char*));
		int index = 0;
		for(int i = 3; i < num_args; i++){
			program_args[index] = malloc(strlen(arguments[i]) + 1);
			strcpy(program_args[index], arguments[i]);
			index++;
		}
		program_args[index] = NULL;
		define_conversion(type1 -> name, type2 -> name, program_args);
		for(int i =0; i < num_args -3 + 1; i++)
			free(program_args[i]);
		free(program_args);
		sf_cmd_ok();
		return 0;
	}

	if(strcmp("print", arguments[0]) == 0){
		if(num_args < 2){
			sf_cmd_error("arg count");
			return -1;
		}
		//make sure arg1 is a valid filename for a valid type
		FILE_TYPE *file_type = infer_file_type(arguments[1]);
		if(file_type == NULL){
			sf_cmd_error("type of file not defined");
			return -1;
		}
		//loop through rest of the arguments and make sure its a valid printer
		int eligible_printers = 0;
		if(num_args > 2){
			for(int i = 2; i < num_args; i++){
				int index = valid_printer(arguments[i]);
				if(index == -1){
					sf_cmd_error("printer not found");
					return -1;
				}
				eligible_printers = eligible_printers | (0x1 << index);
			}

		}
		else
			eligible_printers = 0xffffffff;
		JOB *newjob = malloc(sizeof(JOB));
		char *nameoffile = malloc(strlen(arguments[1]) + 1);
		strcpy(nameoffile, arguments[1]);
		newjob -> filename = nameoffile;
		newjob -> file = file_type;
		newjob -> jstatus = JOB_CREATED;
		newjob -> eligible = eligible_printers;
		newjob -> todelete = 0;
		for(int i = 0; i < MAX_JOBS; i++){
			if(job_array[i] == NULL){
				sf_job_created(i, newjob -> filename, newjob -> file -> name);
				job_array[i] = newjob;
				sf_cmd_ok();
				return 0;
			}
		}
		//if comes out of for loop, job array is full
		sf_cmd_error("max jobs reached");
		return -1;
	}

	//arg 1 is a number
	if(strcmp("cancel", arguments[0]) == 0){
		if(num_args != 2){
			sf_cmd_error("arg count");
			return -1;
		}
		int jnum = string_to_number(arguments[1]);
		if(jnum < 0){
			sf_cmd_error("job num invalid");
			return -1;
		}
		if(job_array[jnum] == NULL){
			sf_cmd_error("job not defined");
			return -1;
		}
		//cancel a job
		int outcome = killpg(job_pid[jnum], SIGTERM);
		if(outcome < 0){
			sf_cmd_error("signal not sucessfully sent to child");
			return -1;
		}
		//TODO: call sigcont?
		sf_cmd_ok();
		return 0;
	}
	if (strcmp("pause", arguments[0]) == 0){
		if(num_args != 2){
			sf_cmd_error("arg count");
			return -1;
		}
		int jnum = string_to_number(arguments[1]);
		if(jnum < 0){
			sf_cmd_error("job num invalid");
			return -1;
		}
		if(job_array[jnum] == NULL){
			sf_cmd_error("job not defined");
			return -1;
		}
		int outcome = killpg(job_pid[jnum], SIGSTOP);
		if(outcome < 0){
			sf_cmd_error("signal not sucessfully sent to child");
			return -1;
		}
		sf_cmd_ok();
		return 0;
	}
	if (strcmp("resume", arguments[0]) == 0){
		if(num_args != 2){
			sf_cmd_error("arg count");
			return -1;
		}
		int jnum = string_to_number(arguments[1]);
		if(jnum < 0){
			sf_cmd_error("job num invalid");
			return -1;
		}
		if(job_array[jnum] == NULL){
			sf_cmd_error("job not defined");
			return -1;
		}
		int outcome = killpg(job_pid[jnum], SIGCONT);
		if(outcome < 0){
			sf_cmd_error("signal not sucessfully sent to child");
			return -1;
		}
		sf_cmd_ok();
		return 0;
	}

	//arg 1 is a printer_name
	if (strcmp("disable", arguments[0]) == 0){
		if(num_args != 2){
			sf_cmd_error("arg count");
			return -1;
		}
		int indexofprinter = valid_printer(arguments[1]);
		if (indexofprinter == -1){
			sf_cmd_error("printer not found");
			return -1;
		}
		//change printer status to disabled (sf_printer_status(char *name, PRINTER_STATUS status))
		PRINTER *printer = printer_array[indexofprinter];
		printer -> pstatus = PRINTER_DISABLED;
		sf_printer_status(printer->name, printer->pstatus);
		sf_cmd_ok();
		return 0;

	}
	if (strcmp("enable", arguments[0]) == 0){
		if(num_args != 2){
			sf_cmd_error("arg count");
			return -1;
		}
		int indexofprinter = valid_printer(arguments[1]);
		if (indexofprinter == -1){
			sf_cmd_error("printer not found");
			return -1;
		}
		PRINTER *printer = printer_array[indexofprinter];
		//change printer status to idle (sf_printer_status(char *name, PRINTER_STATUS status))
		if(printer -> pstatus != PRINTER_DISABLED){
			sf_cmd_error("printer was not disabled in first place");
			return -1;
		}
		printer -> pstatus = PRINTER_IDLE;
		sf_printer_status(printer->name, printer->pstatus);
		//if able to, look through jobs, fork, pipe, and fork
		look_for_jobs();

		sf_cmd_ok();
		return 0;
	}

	//unknown command
	sf_cmd_error("unknown command");
	return -1;
}

int string_to_number(char *string){
	char *ptr = NULL;
	long value = strtol(optarg, &ptr, 10);
	if(strcmp(ptr, "\0") != 0) {
      return -1;
   }
   return (int)value;
}

int valid_printer(char *name){
	//returns index of printer, -1 if no such printer
	for(int i = 0; i < printer_count; i++){
		PRINTER *printer = printer_array[i];
		if(strcmp(name, printer -> name) == 0)
			return i;
	}
	return -1;
}

void starting_job(PRINTER *printer, JOB *job){
	//return pid of the fork
	setpgid(0, 0);
	int fd[2];
	int status;
	//with the pipeline
	//find conversion path
	//loop through conversion to see number args before bull (if 0 just cat, else convert)
	CONVERSION **convert1 = find_conversion_path(job -> file -> name, printer -> file -> name);
	int counter = 0;
	while(convert1[counter] != NULL)
		counter++;
	if(counter == 0){
		//just cat and print
		pid_t processid = fork();
		if(processid < 0)
			exit(1);
		else if(processid == 0){
			//make an array or char** (bin/cat, rest null)
			char * executearray[] = {(char *)"/bin/cat", NULL};
			dup2(imp_connect_to_printer(printer -> name, printer->file -> name, PRINTER_NORMAL), STDOUT_FILENO);
			dup2(open(job -> filename, O_RDONLY) , STDIN_FILENO);
			execvp(executearray[0], executearray);
			exit(0);
		} else{
			waitpid(processid, &status, 0);
			if(WIFEXITED(status))
				if(WEXITSTATUS(status) == 0)
					exit(0);
				else
					exit(1);
			else
				exit(1);
		}
	}
	else{
		int trackprev = 0;
		for(int i = 0; i < counter; i++){
			pipe(fd);
			pid_t processid = fork();
			if(processid < 0)
				exit(1);
			else if(processid == 0){
				if(i == 0){
					dup2(open(job -> filename, O_RDONLY) , STDIN_FILENO);
				}
				else {
					dup2(trackprev, STDIN_FILENO);
				}
				//stdout
				if(i == (counter - 1)){
					dup2(imp_connect_to_printer(printer -> name, printer->file -> name, PRINTER_NORMAL), STDOUT_FILENO);
				}
				else{
					dup2(fd[1], STDOUT_FILENO);
				}
				execvp(convert1[i] -> cmd_and_args[0], convert1[i] -> cmd_and_args);
				exit(0);
			} else {

				waitpid(processid, &status, 0);
				trackprev = fd[0];
				if(WIFEXITED(status))
					if(WEXITSTATUS(status) == 0)
						exit(0);
					else
						exit(1);
				else
					exit(1);
			}
		}
	}
}

void look_for_jobs(){
	for(int i = 0; i < printer_count; i++){
		PRINTER *loopprinter = printer_array[i];
		if(loopprinter -> pstatus == PRINTER_IDLE){
			for(int j = 0; j < MAX_JOBS; j++){
				if(job_array[j] != NULL){
					int eligibility = job_array[j] -> eligible;
					if((eligibility & (0x1 << i)) == 1){
						//check if conversion between types
						CONVERSION ** convert = find_conversion_path(job_array[j] -> file -> name,
						 						loopprinter -> file -> name);
						if(convert != NULL){
							//change status of printer and job and then fork
							loopprinter -> pstatus = PRINTER_BUSY;
							sf_printer_status(loopprinter -> name, PRINTER_BUSY);
							job_array[j] -> jstatus = JOB_RUNNING;
							sf_job_status(j, JOB_RUNNING);
							pid_t pid = fork();
							if(pid < 0){
								continue;//check next job
							}
							//child process
							else if(pid == 0){
								starting_job(loopprinter, job_array[j]);
							}
							//main process
							else {
								int counter = 0;
								while(convert[counter] != NULL)
									counter++;

								char * path[counter + 1];
								counter = 0;
								while(convert[counter] != NULL){
									path[counter] = convert[counter] -> cmd_and_args[0];
									counter ++;
								}
								path[counter] = NULL;
								sf_job_started(j, loopprinter -> name, pid, path);
								printer_pid[i] = pid;
								job_pid[j] = pid;
							}
						}

					}
				}
			}
		}
	}
}

void sigchild_handler(int sig){
	//sent from child process to main to kill itself LOL
	//handle if child dies(cancel/error/finish), stops(pause), continues(resume)
	int status;
	pid_t pid_master;
	while((pid_master = waitpid (-1, &status, WNOHANG)) > 0){//tells you which pid finished and set status = exit status
		if(sig == SIGCHLD){
			//find the job and printer from pid and then changed its stauses based on value of status(0=finish 1=aborted)
			int printerindex = printer_index_from_pid(pid_master);
			int jobindex = job_index_from_pid(pid_master);
			if(printerindex == -1 || jobindex == -1)
				continue;//skip this pid??

			if(WIFSTOPPED(status)){//process paused
				job_array[jobindex] -> jstatus = JOB_PAUSED;
				sf_job_status(jobindex, JOB_PAUSED);

			}
			if(WIFCONTINUED(status)){ //process resumed
				job_array[jobindex] -> jstatus = JOB_RUNNING;
				sf_job_status(jobindex, JOB_RUNNING);
			}

			if(WIFSIGNALED(status) || WIFEXITED(status)){ //child was terminated
				if(WEXITSTATUS(status) == 0){//child terminate normally
					printer_array[printerindex] -> pstatus = PRINTER_IDLE;
					sf_printer_status(printer_array[printerindex] -> name, PRINTER_IDLE);
					job_array[jobindex] -> jstatus = JOB_FINISHED;
					sf_job_status(jobindex, JOB_FINISHED);
					sf_job_finished(jobindex, 0);
					//for job_delete
					job_array[jobindex] -> todelete = time(NULL);
				}
				else{
					printer_array[printerindex] -> pstatus = PRINTER_IDLE;
					sf_printer_status(printer_array[printerindex] -> name, PRINTER_IDLE);
					job_array[jobindex] -> jstatus = JOB_ABORTED;
					sf_job_status(jobindex, JOB_ABORTED);
					sf_job_finished(jobindex, 1);
					//for job_delete
					job_array[jobindex] -> todelete = time(NULL);
				}
			}
		}
	}
}

int job_index_from_pid(pid_t pid){
	//return index of the job from pid else -1 if pid not found
	for(int i = 0; i < MAX_JOBS; i++){
		if(job_pid[i] != 0){
			if(job_pid[i] == pid)
				return i;
		}
	}
	return -1;
}

int printer_index_from_pid(pid_t pid){
	//return index of printer from pid else -1 if pid not found
	for(int i = 0; i < printer_count; i++){
		if(printer_pid[i] != 0){
			if(printer_pid[i] == pid)
				return i;
		}
	}
	return -1;
}

void free_memory(){
	for(int i = 0; i < printer_count; i++){
		PRINTER *printer = printer_array[i];
		free(printer -> name);
		free(printer);
	}
	for(int i = 0; i < MAX_JOBS; i++){
		JOB *job = job_array[i];
		if(job != NULL){
			free(job -> filename);
			free(job);
		}
	}

}