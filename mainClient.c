#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

struct sockaddr_in getAddr(in_port_t Port);
int sendMes(int sock, char* buffer, ssize_t lenBuff);
void* showChat(void* sock);
void recvPrintMes(int sock);
void getUserName(int sock);


int mainClnt(char* serverPort, char *serverIP){

    in_port_t serPort = atoi(serverPort);
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in servAddr = getAddr(serPort);
    if (inet_pton(AF_INET, serverIP, &servAddr.sin_addr.s_addr) <= 0){
        printf("inet_pton() failed\n");
        exit(1);
    }
    if (connect(sock, (const struct sockaddr*) &servAddr, sizeof(servAddr)) < 0){
        printf("connect() failed\n");
    }

    getUserName(sock);

    pthread_t threadID;
    pthread_create(&threadID, NULL, showChat, &sock);

    while (1){
        char buffer[BUFSIZ];
        if (fgets(buffer, BUFSIZ, stdin) == NULL){
            printf("Error in fgets()\n");
        }
        size_t lenBuff = strlen(buffer);
        if (lenBuff > 0 && buffer[lenBuff - 1] == '\n'){
            buffer[lenBuff - 1] = '\0';
        }
        
        int s = sendMes(sock, buffer, BUFSIZ);
        if (s == 0) break;
    }
    close(sock);
    return 0;
}

struct sockaddr_in getAddr(in_port_t Port){
    struct sockaddr_in servAddr;
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(Port);
    return servAddr;
}

int sendMes(int sock, char* buffer, ssize_t lenBuff){
    size_t numBytesSent = send(sock, buffer, strlen(buffer), 0);
    if (numBytesSent < 0){
        printf("send() failed");
        return -1;
    }else {
        printf("Message has been just sent : %s\n", buffer);
        if (strcmp(buffer, "exit") == 0) {
            printf("Exiting.\n");
            return 0;
        }
    }
    return 1;
}

void* showChat(void* sock){
    int Sock = *(int*)sock;
    while (1){
        recvPrintMes(Sock);
    }
    return NULL;
}

void recvPrintMes(int sock){
    char buffer[BUFSIZ];
    while (1){
        ssize_t Bytes = recv(sock, buffer, BUFSIZ, 0);
        if (Bytes > 0)
        {
            buffer[Bytes] = 0;
            printf("%s\n", buffer);
        }
        if (Bytes == 0) break;
    }

    close(sock);
}

void getUserName(int sock){
    char buffer[BUFSIZ];
    ssize_t Bytes = recv(sock, buffer, BUFSIZ, 0);
    if (Bytes > 0){
        buffer[Bytes] = 0;
        printf("%s ", buffer);
        
        buffer[0] = 0;
        if (fgets(buffer, BUFSIZ, stdin) == NULL){
            printf("Error in fgets()\n");
        }
        size_t lenBuff = strlen(buffer);
        if (lenBuff > 0 && buffer[lenBuff - 1] == '\n'){
            buffer[lenBuff - 1] = '\0';
        }
        sendMes(sock, buffer, lenBuff);
    }
}

int main(void){
    mainClnt((char*)"8080", (char*)"127.0.0.1");
    return 0;
}
