#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include <memory.h>
#include <ifaddrs.h>
#include <net/if.h>
#include "dir.h"
#include "usage.h"

int port;

int writeHelper(int, char*);

// Here is an example of how to use the above function. It also shows
// one how to get the arguments passed on the command line.

int main(int argc, char **argv) {
    
    // Check the command line arguments
    if (argc != 2) {
      usage(argv[0]);
      return -1;
    }

    port = atoi(argv[1]);

    //Socket->Bind->Listen->Accept

    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr , clientAddr;
    int serverAddrLen = sizeof(serverAddr);

    //Create socket
    serverSocket = socket(AF_INET, SOCK_STREAM,0);
    if (serverSocket == -1){
        puts("Socket failed");
        //TODO Error message?
        return -1;
    }
    puts("Socket Successful");

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons( port );
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    //Bind Socket
    if (bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0){
        puts("Bind failed");
        return -1;
    }
    puts("Bind Successful");

    while(1){

    //Listen
    listen(serverSocket, 5);
    puts("Listening...");

    //Accept
    int clilen = sizeof(clientAddr);
    clientSocket = accept(serverSocket, (struct sockaddr *) &clientAddr, &clilen);
    if (clientSocket < 0){
        puts("Accept failed");
        return -1;
    }
    puts("Accept Successful");


    //Handling Variables
    char *serverMessage, clientMessage[2000], cmd[4], param[1000];
    char cwd[1024];
    int read, loggedIn = 0, type = 0, mode = 0, stru = 0;
    int passive = 0;
    int data = -1;
    struct sockaddr_in dataAddr;
    int dataLen = sizeof(dataAddr);
    int pasvSocket, pasvPort;
    struct sockaddr_in pasvAddr;
    getsockname(clientSocket, (struct sockaddr*)&serverAddr, &serverAddrLen);


    //Handling
    writeHelper(clientSocket, "220 - enter username cs317:\n");

    while( (read = recv(clientSocket, clientMessage, 2000, 0)) > 0){

        sscanf(clientMessage, "%s", cmd);
        printf("--> %s \n", clientMessage);

        if (strcasecmp(cmd, "USER") == 0){

            if (!loggedIn){
                sscanf(clientMessage, "%s%s", param, param);
                if (strcasecmp(param, "cs317") == 0){
                    loggedIn = 1;
                    writeHelper(clientSocket, "230 Login Successful\n");
                }
                else {
                    writeHelper(clientSocket, "430 Failed\n");
                }

            }else{
                writeHelper(clientSocket, "430 Failed\n");
            }
        }

        else if (strcasecmp(cmd, "QUIT") == 0){
            writeHelper(clientSocket, "221\n");
            fflush(stdout);
            close(clientSocket);
            close(pasvSocket);
        }

        else if (strcasecmp(cmd, "CWD") == 0){
            if(loggedIn == 1){
                sscanf(clientMessage, "%s%s", param, param);

                if (getcwd(cwd, sizeof(cwd)) != NULL){
                    strcat(cwd,"/");
                    strcat(cwd,param);
                    if (chdir(cwd) < 0)
                        writeHelper(clientSocket, "550 Failed to change directory.\n");
                    else
                        writeHelper (clientSocket, "250 Directory changed.\n"); 
                }
            }
            else{
                writeHelper(clientSocket, "530 Please log in.\n");
            }
        }

        else if (strcasecmp(cmd, "CDUP") == 0){
            if(loggedIn == 1){
                if(!chdir("..")){
                    writeHelper(clientSocket, "200 Successful\n");
                }
            }else{
                writeHelper(clientSocket, "530 Please log in.\n");
            }

        }

        else if (strcasecmp(cmd, "TYPE") == 0){
            if(loggedIn == 1){
                sscanf(clientMessage, "%s%s", param, param);

                if (strcasecmp(param, "A") == 0){
                    if (type == 0){
                        type = 1;
                    }
                    writeHelper(clientSocket, "200 Set to ASCII type\n");
                }
                else if(strcasecmp(param, "I") == 0){
                    if (type == 1){
                        type = 0;
                    }
                    writeHelper(clientSocket, "200 Set to Image type\n");
                }
                else{
                    writeHelper(clientSocket, "504 Only supports Image and ASCII type\n");
                }
            }
            else {
                writeHelper(clientSocket, "530 Please log in.\n");
            }

        }

        else if (strcasecmp(cmd, "MODE") == 0){
            if(loggedIn == 1){

                sscanf(clientMessage, "%s%s", param, param);

                if (strcasecmp(param, "S") == 0) {
                        
                    if (mode == 0) {
                        mode = 1;
                    }
                    writeHelper(clientSocket, "200 Stream mode.\n"); 
                        
                }else {
                    writeHelper(clientSocket, "504 Only supports MODE S.\n");
                }
            }
            else {
                writeHelper(clientSocket, "530 Please log in.\n");
            }
        }

        else if (strcasecmp(cmd, "STRU") == 0){
            if(loggedIn == 1){

                sscanf(clientMessage, "%s%s", param, param);

                if (strcasecmp(param, "F") == 0) {
                        
                    if (stru == 0) {
                        stru = 1;
                    }
                    writeHelper(clientSocket, "200 File Structure.\n"); 
                        
                }else {
                    writeHelper(clientSocket, "504 Only supports STRU F.\n");
                }

            }
            else {
                writeHelper(clientSocket, "530 Please log in.\n");
            }

        }

        else if (strcasecmp(cmd, "RETR") == 0){
            if(loggedIn == 1){

                if (passive == 1) {
                        
                    if (pasvPort > 1024 && pasvPort <= 65535 && pasvSocket >= 0) {
                        type = 0;
                        writeHelper(clientSocket, "150 Opening connection.\n");
                        listen(pasvSocket, 5);
                        data = accept(pasvSocket, (struct sockaddr *)&dataAddr, &dataLen);
                        
                        sscanf(clientMessage, "%s%s", param, param);
                        
                        int transfer, st, i;
                        FILE *f = fopen(param, "rb");
                        if (f) {
                            fseek(f, 0, SEEK_SET);
                            char fileBuffer[1025];
                            while (( i = fread(fileBuffer, 1, 1024, f)) > 0){
                                st = send(data, fileBuffer, i, 0);
                                if (st < 0){
                                    st = -1;
                                }
                                else{
                                    fileBuffer[i] = 0;
                                }
                            }
                            if (st < 0) {
                                st = -2;
                            }
                        }else{
                            st = -1;
                        }
                        transfer = fclose(f);
                        transfer = transfer == 0 ? 0 : -3;
                        
                        if (transfer >= 0) {
                            writeHelper(clientSocket, "226 Successful.\n");
                        }
                        
                        else {
                            writeHelper(clientSocket, "451 File Not Found.\n");
                        }
                        
                        close(data);
                        data = -1;
                        close(pasvSocket);
                        passive = 0;
                        
                    }
                        
                    else {
                        writeHelper(clientSocket, "500 \n");
                    }
                }
                
                else {
                    writeHelper(clientSocket, "425 Must Use PASV First.\n");
                }

            }
            else {
                writeHelper(clientSocket, "530 Please log in.\n");
            }

        }

        else if (strcasecmp(cmd, "PASV") == 0){
            if(loggedIn == 1){
                if (!passive) {
                    while ( bind(pasvSocket,(struct sockaddr *)&pasvAddr , sizeof(pasvAddr)) < 0 ) {
                        pasvPort = (rand() % 64512 + 1024);
                        pasvSocket = socket(AF_INET , SOCK_STREAM , 0);
                        pasvAddr.sin_family = AF_INET;
                        pasvAddr.sin_addr.s_addr = INADDR_ANY;
                        pasvAddr.sin_port = htons( pasvPort );
                        
                    }

                    if (pasvSocket < 0) {
                        writeHelper(clientSocket, "500 Error\n");
                    }
                
                    else {
                        listen(pasvSocket , 1);
                        passive = 1;
                        
                        uint32_t t = serverAddr.sin_addr.s_addr;
                     
                        char formatted[1000];
                        sprintf(formatted, "227 Entering PASV (%d,%d,%d,%d,%d,%d)\n", 
                                        t&0xff,
                                        (t>>8)&0xff,
                                        (t>>16)&0xff,
                                        (t>>24)&0xff,
                                        pasvPort>>8, 
                                        pasvPort & 0xff);
                        writeHelper(clientSocket, formatted);

                        printf("Debug: port = %d \n", pasvPort);
                        
                    }
                }
                else {
                    char formatted[100];
                    sprintf(formatted, "227 Already in passive mode. Port number: %d\n", pasvPort);
                    writeHelper(clientSocket, formatted);
                }
            }
            else {
                writeHelper(clientSocket, "530 Please log in.\n");
            }

        }

        else if (strcasecmp(cmd, "NLST") == 0){
            if (loggedIn == 1) {
            
                if (passive) {
                        
                    if (pasvSocket >= 0 && pasvPort > 1024 && pasvPort <= 65535) {
                        type = 1;
                        writeHelper(clientSocket, "150 Here comes the directory listing.\n");
                        listen(pasvSocket, 10);
                        data = accept(pasvSocket, (struct sockaddr *)&clientAddr, &clilen);
                        getcwd(cwd, sizeof(cwd));
                        listFiles(data, cwd);
                        writeHelper(clientSocket, "226 Transfer Successful.\n");
                        
                        close(data);
                        data = -1;
                        close(pasvSocket);
                        passive = 0;
                    }
                        
                    else {
                        writeHelper(clientSocket, "500 \n");
                    }
                }
                
                else {
                    writeHelper(clientSocket, "425 Must Use PASV.\n");
                }
            }
            
            else {
                writeHelper(clientSocket, "530 Please log in.\n");
            }

        }

        else{
            writeHelper(clientSocket, "530 Not valid\n");
        }

    }
}



    // This is how to call the function in dir.c to get a listing of a directory.
    // It requires a file descriptor, so in your code you would pass in the file descriptor 
    // returned for the ftp server's data connection
}

int writeHelper(int socket, char* msg){
    int len = strlen(msg);
    printf ("<--%s\n",msg);
    return write(socket, msg, len);
}