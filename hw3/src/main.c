#include <stdio.h>
#include "sfmm.h"

int main(int argc, char const *argv[]) {
	// // printf("main");
 //    double* ptr = sf_malloc(sizeof(double));

 //    // printf("%p\n", ptr);
 //    // sf_show_free_lists();
 //    sf_show_heap();

 //    // *ptr = 320320320e-320;

 //    // printf("%f\n", *ptr);

 //    sf_free(ptr);

    // char * ptr1 = sf_malloc(200);
    // *(ptr1) = 'A';

    // sf_show_blocks();
    // printf("\n");

    // char * ptr2 = sf_malloc(600 * sizeof(double));
    // *(ptr2) = 'A';

    // sf_show_blocks();
    // printf("\n");

    // char * ptr3 = sf_malloc(666);
    // *(ptr3) = 'A';

    // sf_show_blocks();
    // printf("\n");

    // sf_malloc(9000);

    // // printf("%p\n", ptr4);
    // free(ptr1);

    size_t sz_x = 8, sz_y = 200, sz_z = 1;
	/* void *x = */ sf_malloc(sz_x);
	void *y = sf_malloc(sz_y);
	/* void *z = */ sf_malloc(sz_z);

	sf_free(y);
	sf_show_heap();

    sf_show_blocks();
    printf("\n");

    sf_show_free_lists();

    return EXIT_SUCCESS;
}