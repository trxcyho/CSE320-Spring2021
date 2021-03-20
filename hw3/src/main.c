#include <stdio.h>
#include "sfmm.h"

int main(int argc, char const *argv[]) {
	printf("main");
    double* ptr = sf_malloc(sizeof(double));

    printf("%p", ptr);
    // *ptr = 320320320e-320;

    // printf("%f\n", *ptr);

    // sf_free(ptr);

    return EXIT_SUCCESS;
}
