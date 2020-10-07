#include "argus.h"

int main(int argc, char *argv[]){
    char comando[100] = "";
    int fd_Sv, fd_Client;
    if((fd_Sv = open("fifoServer",O_WRONLY)) < 0){
        perror("open");
        exit(1);
    }


    char buf[100];
    char sol[256];
    int bytes_read = 0;
    int bytes_read1 = 0;
    if(argc == 1 ){

        if(fork() == 0){
            write(1,"argus$ ",7);
            while((bytes_read = read(0,buf,100)) > 0){
                if(write(fd_Sv,buf,bytes_read) < 0){
                    perror("write");
                    exit(1);
                }
            }
        } else {
            if(fork() == 0){
                if((fd_Client = open("fifoClient",O_RDONLY)) < 0){
                    perror("open");
                    exit(1);
                }
                while((bytes_read1 = read(fd_Client,sol,256)) > 0){
                    write(1,sol,bytes_read1);
                }
                write(1,"argus$ ",7);
                _exit(0);
            }
            wait(NULL);
            wait(NULL);
        }
    }
    else {
        for(int i = 1; i < argc; i++)
            argv[i-1] = argv[i];

        argv[argc-1] = NULL;

        if(argc == 3){
            if(strcmp(argv[0],"-e") == 0){
                strcat(comando,"'");
                strcat(comando,argv[1]);
                strcat(comando,"'");
                strcpy(argv[1],comando);
                strcpy(comando,"");
            }
        }

        for(int i = 0; i < argc-1; i++){
            strcat(comando, argv[i]);
            strcat(comando," ");
        }

        if(write(fd_Sv,comando,strlen(comando)) < 0){
            perror("write");
            exit(1);
        }
        if(fork() == 0){
            if((fd_Client = open("fifoClient",O_RDONLY)) < 0){
                perror("open");
                exit(1);
            }
            while((bytes_read1 = read(fd_Client,sol,256)) > 0){
            write(1,sol,bytes_read1);
            }
        
            close(fd_Client);
        }
    }
    
    close(fd_Sv);
    return 0;
}