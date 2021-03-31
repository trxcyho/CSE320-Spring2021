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

int quit = 0; //when to exit cli

int run_cli(FILE *in, FILE *out)
{
    fprintf(out, "imp> ");

    abort();
}

//Commands are: help quit type printer conversion printers jobs print cancel disable enable pause resume
