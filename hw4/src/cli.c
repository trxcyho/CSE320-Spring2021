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

}PRINTER;

typedef struct job {

}JOB;

int quit = 0; //when to exit cli

int run_cli(FILE *in, FILE *out)
{
	// int copy_stdin = dup(STDIN_FILENO);//copy_stdin is original
	// dup2(fileno(in), STDIN_FILENO);
	// dup2(copy_stdin, 0); //revert back to original

	while(!quit){
    	// char* inputcommand = sf_readline("imp> ");
    	//convert char* to char **
    	//free anything that is needed


    }
    // if(in== stdin) -> kill the processes

    abort();
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

}