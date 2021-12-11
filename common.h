#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>

void usage(char *program_name, char *msg) {
    fprintf(stderr, "%s <%s>\n", program_name, msg);
    exit(1);
}

void err(char *msg) {
    char *errmsg = (char *) malloc(strlen(msg) + 1);
    strcpy(errmsg, msg);
    fprintf(stderr, "ERROR: %s\n", errmsg);
    free(errmsg);
    exit(1);
}

#endif
