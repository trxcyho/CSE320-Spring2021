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
//define printer and job from imprimer.h
typedef struct printer {
	//name of printer
	//priner status
	//next printer
}PRINTER;

typedef struct job {
	//job id

	//job status
	//next job
}JOB;

int num_printers = 0;//make sure it dont go over 32 when adding
int num_jobs = 0;//make sure it dont go over 64 when adding
int quit = 0; //when to exit cli

int run_cli(FILE *in, FILE *out)
{
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
	    	// char* inputcommand = sf_readline("imp> ");
			//if input command is EOF ->return -1//do we stop execution of processes?

	    	//convert char* to char **
	    	//free anything that is needed
			//if quit is called return 0
	    }
	}
	if(in != stdin && quit)
		return -1;
	//
	return 0;
}

//have a function that converts char* to char ** then call operation


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
		//Commands are: help quit type printer conversion printers jobs print cancel disable enable pause resume
		return 0;
	}
	if(strcmp("quit", arguments[0]) == 0){
		if(num_args != 1){
			//throw error
			return -1;
		}
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
		FILE_TYPE *newtype = define_type(arguments[1]);
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
	if(strcmp("cancel", arguments[0]) == 0){

	}
	if (strcmp("pause", arguments[0]) == 0){

	}
	if (strcmp("resume", arguments[0]) == 0){

	}
	if (strcmp("disable", arguments[0]) == 0){

	}
	if (strcmp("enable", arguments[0]) == 0){

	}

	//unknown command
	//throw error
	return -1;
}