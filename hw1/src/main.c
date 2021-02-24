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
	// 	printf("0x%x\n", global_options);
	// }
	// else{
	// 	printf("VALID\n");
	// 	printf("0x%x\n", global_options);
	// }

	//end of testing

	// test bddlookup
	// int x = bdd_lookup(1, 4, 8);
	// printf("%i\n", x);
	// x = bdd_lookup(5, 100, 24);
	// printf("%i\n", x);
	// x = bdd_lookup(1, 4, 8);
	// printf("%i\n", x);
	// x = bdd_lookup(2, 5, 4);
	// printf("%i\n", x);
	// x = bdd_lookup(5, 100, 24);
	// printf("%i\n", x);

	// unsigned char input1[] = {4, 8 ,4, 8}; // Should return 256 because only one node is made
	// BDD_NODE *x = bdd_from_raster(2, 2, input1);
	// // bdd_from_raster(2,2,input1); //256
	// debug("%ld\n", x - bdd_nodes);


 //    unsigned char input2[] = {4,2,12,255};
 //    x = bdd_from_raster(2,2,input2); //259
 //    debug("%ld\n", x - bdd_nodes);

 //    unsigned char input3[] = {4,2,36,49};
 //    x = bdd_from_raster(2,2,input3); //261
 //    debug("%ld\n", x - bdd_nodes);

 //    unsigned char input4[] = {4,2,4,8};
 //    x = bdd_from_raster(2,2,input4); //262
 //    debug("%ld\n", x - bdd_nodes);


    if(validargs(argc, argv))
        USAGE(*argv, EXIT_FAILURE);
    if(global_options & HELP_OPTION)
        USAGE(*argv, EXIT_SUCCESS);

    //program must read input data from stdin and write output data to stdout
    if(global_options == 0x31){
    	if(pgm_to_ascii(stdin, stdout) == 0)
    		return EXIT_SUCCESS;
    }
    if (global_options == 0x32){
    	if(birp_to_ascii(stdin, stdout) == 0)
    		return EXIT_SUCCESS;
    }
    if (global_options == 0x21){
    	if (pgm_to_birp(stdin, stdout) == 0)
    		return EXIT_SUCCESS;
    }
    if (global_options == 0x12){
    	if(birp_to_pgm(stdin, stdout) == 0)
    		return EXIT_SUCCESS;
    }
    if ((global_options & 0xff) == 0x22){
    	if (birp_to_birp(stdin, stdout) == 0)
    		return EXIT_SUCCESS;
    }

    return EXIT_FAILURE;
}

/*
 * Just a reminder: All non-main functions should
 * be in another file not named main.c
 */
