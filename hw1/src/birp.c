/*
 * BIRP: Binary decision diagram Image RePresentation
 */

#include "image.h"
#include "bdd.h"
#include "const.h"
#include "debug.h"

//prototypes for func
int strcompare(char* str1, char* str2, int size);
int strsize(char* str);
int strToNum(char *str);

int pgm_to_birp(FILE *in, FILE *out) {
    int width = 0;
    int height = 0;
    if(img_read_pgm(in, &width, &height, raster_data, RASTER_SIZE_MAX))
        goto endofpgmtobirp; //reading pgm file unsuccessful

    //call img_write_birp

    return 0;
    endofpgmtobirp:
    return -1;
}

int birp_to_pgm(FILE *in, FILE *out) {
    // TO BE IMPLEMENTED
    return -1;
}

int birp_to_birp(FILE *in, FILE *out) {
    // TO BE IMPLEMENTED
    return -1;
}

int pgm_to_ascii(FILE *in, FILE *out) {
    int width = 0;
    int height = 0;
    if(img_read_pgm(in, &width, &height, raster_data, RASTER_SIZE_MAX))
        goto endofpgmtoascii; //reading pgm file unsuccessful

    int value = 0;
    char print;
    for(int i = 0; i< height; i++){
        for(int j = 0; j <width; j++){
            value = *(raster_data + (i*width) +j);
            if(value > 255 || value < 0)
                goto endofpgmtoascii;
            if(value < 64)
                print = ' ';
            else if(value < 128)
                print = '.';
            else if(value < 192)
                print = '*';
            else if(value < 256)
                print = '@';
            fputc(print, out);
        }
        print = '\n';
        fputc(print, out);
    }
    return 0;
    endofpgmtoascii:
    return -1;
}

int birp_to_ascii(FILE *in, FILE *out) {
    // TO BE IMPLEMENTED
    return -1;
}

/**
 * @brief Validates command line arguments passed to the program.
 * @details This function will validate all the arguments passed to the
 * program, returning 0 if validation succeeds and -1 if validation fails.
 * Upon successful return, the various options that were specifed will be
 * encoded in the global variable 'global_options', where it will be
 * accessible elsewhere int the program.  For details of the required
 * encoding, see the assignment handout.
 *
 * @param argc The number of arguments passed to the program from the CLI.
 * @param argv The argument strings passed to the program from the CLI.
 * @return 0 if validation succeeds and -1 if validation fails.
 * @modifies global variable "global_options" to contain an encoded representation
 * of the selected program options.
 */
int validargs(int argc, char **argv) {
    global_options = 0;

    if(argc <= 1 || argc > 7) //max args is 7
        return -1;
    //read the second arg and make sure size is 2
    char *arg1 = *(argv + 1);
    if(strsize(arg1) != 2)
        return -1;
    //if -h is provided
    if(strcompare(arg1, "-h", 2) == 1){
        global_options = 0x80000000;
        return 0;
    }

    //flags for loop
    int i_flag = 0;
    int o_flag = 0;
    int add_flag = 0;
    int invalid_flag = 0;
    for(int i = 1; i < argc; i+=2){
        char *arg1 = *(argv + i);
        if(strsize(arg1) != 2){
            invalid_flag = 1;
            break;
        }
        if(argc == i+1){
            if(i_flag == 0)
                global_options = global_options | 0x2;
            if(o_flag == 0)
                global_options = global_options | 0x20;
            if(global_options != 0x22){
                invalid_flag = 1;
                break;
            }
            if(strcompare(arg1, "-n", 2) == 1){
                global_options = global_options | 0x100;
                add_flag = 1;
                return 0;
            }
            if(strcompare(arg1, "-r", 2) == 1){
                global_options = global_options | 0x400;
                add_flag = 1;
                return 0;
            }
            global_options = 0;
            return -1;

        }
        char *arg2 = *(argv + i + 1);
        if(strcompare(arg1, "-i", 2) == 1){
            if(i_flag == 1 || add_flag == 1){
                invalid_flag = 1;
                break;
            }
            if(strsize(arg2) == 3){
                if(strcompare(arg2, "pgm", 3) == 1){
                    global_options = global_options | 0x1;
                    i_flag = 1;
                }
            }
            else if(strsize(arg2) == 4){
                if(strcompare(arg2, "birp", 4) == 1){
                    global_options = global_options | 0x2;
                    i_flag = 1;
                }
            }
            else{
                invalid_flag = 1;
                break;
            }
        }
        else if(strcompare(arg1, "-o", 2) == 1){
            if(o_flag == 1 || add_flag == 1){
                invalid_flag = 1;
                break;
            }
            if(strsize(arg2) == 3){
                if(strcompare(arg2, "pgm", 3) == 1){
                    global_options = global_options | 0x10;
                    o_flag = 1;
                }
            }
            else if(strsize(arg2) == 4){
                if(strcompare(arg2, "birp", 4) == 1){
                    global_options = global_options | 0x20;
                    o_flag = 1;
                }
            }
            else if(strsize(arg2) == 5){
                if(strcompare(arg2, "ascii", 5) == 1){
                    global_options = global_options | 0x30;
                    o_flag = 1;
                }
            }
            else{
                invalid_flag = 1;
                break;
            }
        }
        else if(strcompare(arg1, "-t", 2) == 1){
            if(add_flag == 1){
                invalid_flag = 1;
                break;
            }
            if(i_flag == 0)
                global_options = global_options | 0x2;
            if(o_flag == 0)
                global_options = global_options | 0x20;
            if(global_options != 0x22){
                invalid_flag = 1;
                break;
            }
            int value = strToNum(arg2);
            if(value < 0 || value > 255){
                invalid_flag = 1;
                break;
            }
            else {
                value = value <<16;
                global_options = global_options | value;
                global_options = global_options | 0x200;
                add_flag = 1;
            }
        }
        else if(strcompare(arg1, "-z", 2) == 1){
            if(add_flag == 1){
                invalid_flag = 1;
                break;
            }
            if(i_flag == 0)
                global_options = global_options | 0x2;
            if(o_flag == 0)
                global_options = global_options | 0x20;
            if(global_options != 0x22){
                invalid_flag = 1;
                break;
            }
            int value = strToNum(arg2);
            if(value < 0 || value > 16){
                invalid_flag = 1;
                break;
            }
            else {
                value *= -1;
                value = value <<24;
                value = value >> 8;
                global_options = global_options | value;
                global_options = global_options | 0x300;
                add_flag = 1;
            }
        }
        else if(strcompare(arg1, "-Z", 2) == 1){
            if(add_flag == 1){
                invalid_flag = 1;
                break;
            }
            if(i_flag == 0)
                global_options = global_options | 0x2;
            if(o_flag == 0)
                global_options = global_options | 0x20;
            if(global_options != 0x22){
                invalid_flag = 1;
                break;
            }
            int value = strToNum(arg2);
            if(value < 0 || value > 16){
                invalid_flag = 1;
                break;
            }
            else {
                value = value <<16;
                global_options = global_options | value;
                global_options = global_options | 0x300;
                add_flag = 1;
            }
        }
        else{
            invalid_flag = 1;
            break;
        }

    }
    if(invalid_flag == 0){
        if(i_flag == 0)
            global_options = global_options | 0x2;
        if(o_flag == 0)
            global_options = global_options | 0x20;
        return 0;
    }
    global_options = 0;
        return -1;
}

int strsize(char *str){
    if(str == NULL)
        return -1;
    int x = 0;
    while(*(str + x)){
        x++;
    }
    return x;
}

int strcompare(char *str1, char *str2, int size){
    if(str1 == NULL || str2 == NULL)
        return -1;

    for(int i = 0; i < size; i++){
        if(*(str1 + i) != *(str2 + i))
            return 0;
    }
    return 1;
}

int strToNum(char *str){
    int total = 0;
    for(str = str; *str != '\0'; str++){
        if(*str < '0' || *str > '9')
            return -1;
        total = total * 10 + (*str - '0');
    }
    return total;
}
