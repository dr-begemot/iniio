#define DEBUG_MSG 1

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sqlite3.h>

#include "iniio.h"

int main (void)
{
    printf("ini_load\n");
    ini_t* ini = ini_load ("test.ini");
    printf("ini_print\n");
    ini_print(ini);
    printf("ini_save\n");
    ini_save (ini, "test2.ini");
    printf("end\n");

    return 0;
}
