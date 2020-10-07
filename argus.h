
#ifndef _ARGUS_H_
#define _ARGUS_H_


#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct tarefa{
    char* modo;
    int pid;
    char* comando;
} Tarefa;






#endif
