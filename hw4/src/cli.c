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
int operation(int num_args, char** arguments);

//define printer and job from imprimer.h
typedef struct printer {
	//name of printer
	//priner status
}PRINTER;
//create array for printers

typedef struct job {
	//job id
	//type of file
	//job status
}JOB;
//create array for jobs

int num_args = 0;
int num_printers = 0;//make sure it dont go over 32 when adding
int num_jobs = 0;//make sure it dont go over 64 when adding
int quit = 0; //when to exit cli



int run_cli(FILE *in, FILE *out)
{
	char **arguments;
	if(in == NULL)
		return -1;
	//create a buffer to hold the storage of the lines
	if(in != stdin){
		// size_t characters;
		// size_t length = 0;
		// while (characters = getline(&buffer, &length, in))
	}
	else{
		while(!quit){
	    	char* inputcommand = sf_readline("imp> ");
			//if input command is EOF ->return -1//do we stop execution of processes?
	    	arguments = convert_to_commands(inputcommand);
	    	// printf("%s\n", arguments[0]);
	    	operation(num_args, arguments);

	    	//convert char* to char **
	    	//free anything that is needed
			//if quit is called return 0
			for(int i = 0; i < num_args; i++)
				free(arguments[i]);
			free(arguments);
			num_args = 0;
	    }
	}
	if(in != stdin && quit)
		return -1;
	//
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
		arguments[index] = calloc(strlen(word), sizeof(char*));
		strcpy(arguments[index], word);
		index++;
		word = strtok(NULL, " ");
	}
	free(line);
	free(line_duplicate);
	return arguments;
}

int operation(int num_args, char** arguments){
	//first combine characters to arguments and keep track number of args
	if(arguments[0] == NULL){
		//throw an error;
		return -1;
	}
	if(strcmp("help", arguments[0]) == 0){
		if (num_args != 1){
			//throw error
			return -1;
		}
		//print help
		// printf("help");
		//Commands are: help quit type printer conversion printers jobs print cancel disable enable pause resume
		return 0;
	}
	if(strcmp("quit", arguments[0]) == 0){
		if(num_args != 1){
			//throw error
			return -1;
		}
		// printf("quit");
		quit = 1;
		return 0;
	}
	if (strcmp("printers", arguments[0]) == 0){
		//print all printers
		return 0;
	}
	if (strcmp("jobs", arguments[0]) == 0){
		//print all jobs
		return 0;
	}
	if(strcmp("type", arguments[0]) == 0){
		if(num_args != 2){
			//throw error
		}
		// FILE_TYPE *newtype = define_type(arguments[1]);
		return 0;
	}
	if(strcmp("printer", arguments[0]) == 0){
		if(num_args != 3){
			//throw error
			return -1;
		}
		//create printer (make status as idle)
		return 0;
	}

	//arg 1 is a number
	if(strcmp("cancel", arguments[0]) == 0){

	}
	if (strcmp("pause", arguments[0]) == 0){

	}
	if (strcmp("resume", arguments[0]) == 0){

	}

	//arg 1 is a printer_name
	if (strcmp("disable", arguments[0]) == 0){

	}
	if (strcmp("enable", arguments[0]) == 0){

	}

	//ubknown num_args associated
	if(strcmp("print", arguments[0]) == 0){

	}
	if(strcmp("conversion", arguments[0]) == 0){

	}

	//unknown command
	//throw error
	return -1;
}