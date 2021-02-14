#include <stdio.h>
#include <stdlib.h>

#include "const.h"
#include "debug.h"

#ifdef _STRING_H
#error "Do not #include <string.h>. You will get a ZERO."
#endif

#ifdef _STRINGS_H
#error "Do not #include <strings.h>. You will get a ZERO."
#endif

#ifdef _CTYPE_H
#error "Do not #include <ctype.h>. You will get a ZERO."
#endif

int main(int argc, char **argv)
{
	//testing part 1
	// if(validargs(argc, argv) == -1){
	// 	printf("INVALID\n");
	// 	printf("%d\n", global_options);
	// }
	// else{
	// 	printf("VALID\n");
	// 	printf("%d\n", global_options);
	// }

	//end of testing

	// test bddlookup
	// int x = bdd_lookup(1, 4, 8);
	// printf("%i\n", x);
	// x = bdd_lookup(5, 100, 24);
	// printf("%i\n", x);
	// x = bdd_lookup(1, 4, 8);
	// printf("%i\n", x);


    if(validargs(argc, argv))
        USAGE(*argv, EXIT_FAILURE);
    if(global_options & HELP_OPTION)
        USAGE(*argv, EXIT_SUCCESS);
    // TO BE IMPLEMENTED
    //program must read input data from stdin and write output data to stdout
    if(global_options == 0x31){
    	if(pgm_to_ascii(stdin, stdout) == 0)
    		return EXIT_SUCCESS;
    }





    return EXIT_FAILURE;
}

/*
 * Just a reminder: All non-main functions should
 * be in another file not named main.c
 */
