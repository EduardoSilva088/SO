#include "argus.h"


int* pids;
int pids_count = 0;
int tExecucao = 0;

Tarefa* t;
Tarefa** tarefas;
int nr_tarefas = 0;

void sigAlrm_handler(int signum){
    for(int i = 0; i < pids_count; i++){
        if(pids[i] > 0){
            kill(pids[i],SIGKILL);
        }
    }
    _exit(2);
}

void sigQuit_handler(int signum){
    for(int i = 0; i < pids_count; i++){
        if(pids[i] > 0){
            kill(pids[i],SIGKILL);
        }
    }
    _exit(1);
}

void sigChld_handler(int signum){
    int indice;
    int status;
    int pid;
    pid = wait(&status);
    for(int i = 0; i < nr_tarefas; i++){
        if(pid == tarefas[i]->pid) indice = i;
    }

    if(WIFEXITED(status)){
        switch(WEXITSTATUS(status)){
            case 0:
                tarefas[indice]->modo = strdup("concluida");
                break;
            case 1:
                tarefas[indice]->modo = strdup("user");
                break;
            case 2:
                tarefas[indice]->modo = strdup("max execução");
                break;
        }
    }
}


int executa_comando(char* comando){
    char** exec_args = (char**) malloc(sizeof(char**));
    char* string;
    int i = 0;

    string = strtok(comando, " ");

    while(string!=NULL){
        exec_args[i]=string;
        string=strtok(NULL," ");
        i++;
    }

    exec_args[i] = NULL;

    if(execvp(exec_args[0],exec_args) < 0){
        perror("exec");
    }

    return 0;
}

int executa(char* buf){

    int i, len = strlen(buf);
	for(i = 1; i < len-1; i++){
		buf[i-1] = buf[i];
	}
	buf[i-1]='\0';
    

    if(signal(SIGCHLD,SIG_DFL) == SIG_ERR){
        perror("erro sigChld exec");
        exit(10);
    }

    int nr_comandos = 0;
    char* string;
    char* comando;
    char** comandos = malloc(sizeof(char**));
    int pid = -1;

    string = strdup(buf);
    for(nr_comandos = 0; (comando = strsep(&string,"|")) != NULL; nr_comandos++){
        comandos[nr_comandos] = strdup(comando);
    }
    int pipe_fd[nr_comandos-1][2];
    pids = (int*) malloc(sizeof(int) * nr_comandos);
    pids_count = nr_comandos;
    if(nr_comandos == 1){
        if((pid = fork()) == 0){
            executa_comando(comandos[0]);
            _exit(0);
        }
        else {
            pids[0] = pid;
        }
        wait(NULL);
    }
    else{
        for(int c = 0; c < nr_comandos; c++){
            if(c == 0){
                if(pipe(pipe_fd[0]) < 0){ 
                    perror("pipe");
                    exit(-1);
                }
                if((pid = fork()) == 0){
                    close(pipe_fd[0][0]);
                    dup2(pipe_fd[0][1],1);
                    close(pipe_fd[0][1]);
                    
                    executa_comando(comandos[0]);
                    _exit(0);
                }
                else {
                    close(pipe_fd[0][1]);
                    pids[0] = pid;
                }
            }
            else if(c == nr_comandos -1){
                if((pid = fork()) == 0){
                    dup2(pipe_fd[c-1][0],0);
                    close(pipe_fd[c-1][0]);
                    executa_comando(comandos[c]);
                    _exit(0);
                }
                else{
                    close(pipe_fd[c-1][0]);
                    pids[c] = pid;
                }
            }
            else {
                if(pipe(pipe_fd[c]) < 0){
                    perror("pipe");
                    exit(-1);
                }
                if((pid = fork()) == 0){
                    close(pipe_fd[c][0]);
                    dup2(pipe_fd[c][1],1);
                    close(pipe_fd[c][1]);

                    dup2(pipe_fd[c-1][0],0);
                    close(pipe_fd[c-1][0]);

                    executa_comando(comandos[c]);
                    _exit(0);
                }
                else{
                    close(pipe_fd[c][1]);
					close(pipe_fd[c-1][0]);
                    pids[c] = pid;
                }
            }
        }
        for(int i = 0; i < nr_comandos; i++){
            wait(NULL);
        }
    }

    return 0;
}

int main(int argc, char const *argv[]){

    mkfifo("fifoServer", 0666);
    mkfifo("fifoClient", 0666);

    if (signal(SIGALRM,sigAlrm_handler) == SIG_ERR){
        perror("SigAlrm");
        exit(10);
    }

    if (signal(SIGCHLD,sigChld_handler) == SIG_ERR){
        perror("SigChld");
        exit(10);
    }

    if (signal(SIGQUIT,sigQuit_handler) == SIG_ERR){
        perror("SigQuit");
        exit(10);
    }

    tarefas = (Tarefa**) malloc(sizeof(Tarefa*));

    int fd_Sv, fd_Client;
    if((fd_Sv = open("fifoServer", O_RDONLY)) < 0){
        perror("open");
        exit(1);
    }


    if((fd_Client = open("fifoClient",O_WRONLY)) < 0){
        perror("open fifo");
        exit(1);
    }

    dup2(fd_Client,1);

    while(1){

        char output[256];
        int read_bytes = 0;
        char buf[100];
        if((read_bytes = read(fd_Sv, buf, 100))){
            int pid = -1;
            buf[read_bytes-1] = '\0';
            if(strncmp(buf, "-e", 2) == 0 || strncmp(buf, "executar", 8) == 0){
                if(strncmp(buf, "-e", 2) == 0) memmove(buf,buf+3,strlen(buf+3)+1);
                else memmove(buf,buf+9,strlen(buf+9)+1);
                t = (Tarefa*) malloc(sizeof(Tarefa));
                sprintf(output,"\nNova Tarefa #%d\n",nr_tarefas+1);
                write(fd_Client,output,strlen(output));
                if((pid = fork()) == 0){
                    alarm(tExecucao);
                    _exit(executa(buf));
                }
                t->comando = strdup(buf);
                t->pid = pid;
                tarefas[nr_tarefas++] = t;
            }
            else if(strncmp(buf, "-h",2) == 0 || strncmp(buf,"ajuda",5) == 0){
                char* buf = "\n>tempo-inactividade <segs>\n>tempo-execucao <segs>\n>executar p1 | p2 | ... | pn\n>listar\n>terminar <tarefa>\n>historico\n>ajuda\n";
                write(fd_Client,buf,strlen(buf));
            }
            else if(strncmp(buf, "-m",2) == 0 || strncmp(buf,"tempo-execucao",14) == 0){
                if(strncmp(buf, "-m",2) == 0) memmove(buf,buf+3,strlen(buf+3)+1);
                else memmove(buf,buf+15,strlen(buf+15)+1);
                tExecucao = atoi(buf);
            }
            else if(strncmp(buf, "-t",2) == 0 || strncmp(buf, "terminar",8) == 0){
                int nr;
                if(strncmp(buf, "-t",2) == 0) nr = atoi(memmove(buf,buf+3,strlen(buf+3)+1));
                else nr = atoi(memmove(buf,buf+9,strlen(buf+9)+1));
                kill(tarefas[nr-1]->pid,SIGQUIT);
            }
            else if(strncmp(buf,"-l",2) == 0 || strncmp(buf,"listar",6) == 0){
                write(fd_Client,"Listar\n",8);
                for(int i = 0; i < nr_tarefas; i++){
                    if(kill(tarefas[i]->pid,0) == 0){
                        sprintf(output,"#%d, %s\n",i+1,tarefas[i]->comando);
                        write(fd_Client,output,strlen(output));
                    }
                }
            }
            else if (strncmp(buf,"-r",2) == 0 || strncmp(buf,"historico",2) == 0){
                write(1,"Histórico\n",12);
                for(int i = 0; i < nr_tarefas; i++){
                    if(kill(tarefas[i]->pid,0) != 0){
                        sprintf(output,"#%d, %s: %s\n",i+1,tarefas[i]->modo,tarefas[i]->comando);
                        write(1,output,strlen(output));
                    }
                }
            }

        }
    }
    close(fd_Client);
    close(fd_Sv);
    return 0;
}
