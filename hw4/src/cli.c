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

//protoype
char** convert_to_commands(char* line);
int operation(int num_args, char** arguments, FILE *out);
int string_to_number(char *string);
int valid_printer(char *name);

//define printer and job from imprimer.h
struct printer {
	char *name;
	struct file_type *file;
	PRINTER_STATUS pstatus;
};
//create array for printers
PRINTER *printer_array[MAX_PRINTERS];

struct job {
	char *filename;
	struct file_type *file;
	JOB_STATUS jstatus;
	int eligible;
};
//create array for jobs
JOB *job_array[MAX_JOBS];

int num_args = 0;
int printer_count = 0;
int num_printers = 0;//make sure it dont go over 32 when adding
int num_jobs = 0;//make sure it dont go over 64 when adding
int quit = 0; //when to exit cli



int run_cli(FILE *in, FILE *out)
{
	for(int i = 0; i < MAX_JOBS; i++)
		job_array[i] = NULL;

	char **arguments;
	// signal(SIGTERM, );
	// signal(SIGCOUNT, );
	// signal(SIGSTOP, )
	if(in == NULL)
		return -1;
	//create a buffer to hold the storage of the lines
	if(in != stdin){
		// size_t characters;
		// size_t length = 0;
		// while (characters = getline(&buffer, &length, in) != 0)
			// loop through get line and change any \n to \0
	}
	else{
		while(!quit){
	    	char* inputcommand = sf_readline("imp> ");
			//if input command is NULL ->return -1//do we stop execution of processes, free everything
	    	arguments = convert_to_commands(inputcommand);
	    	// printf("%s\n", arguments[0]);
	    	operation(num_args, arguments, out);

	    	//convert char* to char **
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
			fprintf(out, "PRINTER: id=%d, name=%s, type=%s, status=", i, printer -> name, printer -> file -> name);
			if(printer -> pstatus == 0)
				fprintf(out, "disabled\n");
			else if(printer -> pstatus == 1)
				fprintf(out, "idle\n");
			else
				fprintf(out, "busy\n");
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
		newprinter -> name = arguments[1];
		newprinter -> file = type;
		PRINTER_STATUS stat = PRINTER_DISABLED; //create printer (make status as disabled)
		newprinter -> pstatus = stat;
		sf_printer_defined(arguments[1], arguments[2]);
		//add printer to array
		printer_array[printer_count] = newprinter;
		printer_count++;
		sf_cmd_ok();
		return 0;
	}

	if(strcmp("conversion", arguments[0]) == 0){
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

		char** program_args = calloc(num_args - 3, sizeof(char*));
		int index = 0;
		for(int i = 3; i < num_args; i++){
			program_args[index] = malloc(strlen(arguments[i] + 1));
			strcpy(program_args[index], arguments[i]);
			index++;
		}

		define_conversion(type1 -> name, type2 -> name, program_args);
		sf_cmd_ok();
		return 0;
	}

	if(strcmp("print", arguments[0]) == 0){
		//make sure arg1 is a valid filename for a valid type
		FILE_TYPE *file_type = infer_file_type(arguments[1]);
		if(file_type == NULL){
			sf_cmd_error("type of file not defined");
			return -1;
		}
		//loop through rest of the arguments and make sure its a valid printer
		if(num_args > 2){
			for(int i = 2; i < num_args; i++){
				if(valid_printer(arguments[i]) == -1){
					sf_cmd_error("printer not found");
					return -1;
				}
			}
		}
		//(if num arg > 2) create a job with those printer eligibility, else create job with ffffffff
		sf_cmd_ok();
		return 0;
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
		//cancel a job

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

		sf_cmd_ok();
		return 0;
	}

	//arg 1 is a printer_name
	if (strcmp("disable", arguments[0]) == 0){
		if(num_args != 2){
			sf_cmd_error("arg count");
			return -1;
		}
		if (valid_printer(arguments[1]) == -1){
			sf_cmd_error("printer not found");
			return -1;
		}
		//change printer status to disabled (sf_printer_status(char *name, PRINTER_STATUS status))
		sf_cmd_ok();
		return 0;

	}
	if (strcmp("enable", arguments[0]) == 0){
		if(num_args != 2){
			sf_cmd_error("arg count");
			return -1;
		}
		if (valid_printer(arguments[1]) == -1){
			sf_cmd_error("printer not found");
			return -1;
		}
		PRINTER *printer = printer_array[valid_printer(arguments[1])];
		//change printer status to idle (sf_printer_status(char *name, PRINTER_STATUS status))
		printer -> pstatus = PRINTER_IDLE;
		sf_printer_status(printer->name, printer->pstatus);
		//if able to, look through jobs and fork
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