#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

static const int MAXclnts = 5;
int CurClnts = 0;

void checkConnections(in_port_t servPort, struct sockaddr_in servAddr, int servSock);
struct sockaddr_in getAddr(in_port_t servPort);
void sendCheck(int clntSock);
int handleClient(struct sockaddr_in clntAddr, int clntSock);
void* HandleClientThread(void* clients);
void sendMesToClient(int clntSock, char* mes, int mesLen);
void* closeSock(void* sock);

typedef struct ClientData{
    int clntSock;
    struct sockaddr_in clntAddr;
    char* userName;
} ClientData;

typedef struct Clients{
    struct ClientData* clients[MAXclnts];
} Clients;
char* recvMesFromClient(ClientData clnt);
void SendMesChat(Clients* carr, char* mes);

int mainSer(char* Port){
    in_port_t servPort = atoi(Port);
    int servSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in servAddr = getAddr(servPort);
    bind(servSock, (const struct sockaddr * ) &servAddr, sizeof(servAddr));

    pthread_t threadId;
    pthread_create(&threadId, NULL, closeSock, &servSock);

    listen(servSock, MAXclnts);
    printf("Listening...\n");
    checkConnections(servPort, servAddr, servSock);
    
    close(servSock);
    return 0;
}

void* closeSock(void* sock){
    int Sock = *(int*)sock;
    while (1){
        char buffer[BUFSIZ];
        if (fgets(buffer, BUFSIZ, stdin) != NULL){
            size_t lenBuff = strlen(buffer);
            if (lenBuff > 0 && buffer[lenBuff - 1] == '\n'){
                buffer[lenBuff - 1] = '\0';
            }
            if (strcmp("close", buffer) == 0){
                close(Sock);
                exit(0);
            }
        }
    }
}

void checkConnections(in_port_t servPort, struct sockaddr_in servAddr, int servSock){
    Clients carr;
    char* getUsrName = "Write your name: ";
    while (1){
        struct sockaddr_in clntAddr;
        socklen_t clntAddrLen = sizeof(clntAddr);

        int clntSock = accept(servSock, (struct sockaddr*) &clntAddr, &clntAddrLen);
        if (clntSock < 0){
            printf("accept() failed\n");
            continue;
        }
        printf("Successfull connection\n");
        CurClnts++;
        char clntName[INET_ADDRSTRLEN];
        if (inet_ntop(AF_INET, &clntAddr.sin_addr.s_addr, clntName, sizeof(clntName)) != 0){
            printf("Handling client %s; Port: %d\n", clntName, ntohs(clntAddr.sin_port));
        }
        else {
            puts("Cannot handle client\n");
            break;
        }
        printf("Clients: %d\n", CurClnts);

        ClientData* data = (ClientData* )malloc(sizeof(ClientData));
        data->clntAddr = clntAddr;
        data->clntSock = clntSock;

        sendMesToClient(data->clntSock, getUsrName, strlen(getUsrName)); ////////////////

        char* name = recvMesFromClient(*data);
        data->userName = name;/////////////
        carr.clients[CurClnts - 1] = data;

        pthread_t threadID;
        if (pthread_create(&threadID, NULL, HandleClientThread, &carr) != 0){
            printf("pthread_create() failed\n");
            break;
        }else {
            pthread_detach(threadID);
        }
    }
}

int handleClient(struct sockaddr_in clntAddr, int clntSock){

    char buffer[BUFSIZ];

    ssize_t numBytes = recv(clntSock, buffer, BUFSIZ - 1, 0);
    if (numBytes < 0) {
        printf("recv() failed\n");
    } else if (numBytes == 0) {
        printf("recv(), connection closed prematurely\n");
    }
    buffer[numBytes] = '\0';
    printf("Recieved message: %s from port: %d\n", buffer, ntohs(clntAddr.sin_port));
    if (strcmp(buffer, "exit") == 0){
        return 0;
    }
    close(clntSock);
    return handleClient(clntAddr, clntSock);
}

void* HandleClientThread(void* clients){

    Clients* carr = (Clients*) clients;

    ClientData* data = carr->clients[CurClnts - 1];

    int clntSock = data->clntSock;
    struct sockaddr_in clntAddr = data->clntAddr;

    while (1){
        char* mes = recvMesFromClient(*data);
        printf("Recieved message: %s from port: %s\n", mes, data->userName);
        
        ssize_t totalLength = strlen(data->userName) + strlen(": ") + strlen(mes) + 1;
        char* formattedMes = (char*)malloc(totalLength);
        strcpy(formattedMes, data->userName);
        strcat(formattedMes, ": ");
        strcat(formattedMes, mes);

        printf("Formatted mes: '%s'\n", formattedMes);

        SendMesChat(carr, formattedMes);
        if (strcmp(mes, "exit") == 0){
            printf("User %s from port %d exited.\n", data->userName, ntohs(clntAddr.sin_port));
            CurClnts--;
            free(data);
            free(mes);
            break;
        } 
        free(mes);
        free(formattedMes);
    }
    close(clntSock);
    return NULL;
}

void SendMesChat(Clients* carr, char* mes){
    for (int i = 0; i < CurClnts; i++){
        sendMesToClient(carr->clients[i]->clntSock, mes, strlen(mes));
        printf("sending: '%s' to client number %d\n", mes, i);
    }
}

struct sockaddr_in getAddr(in_port_t servPort){
    struct sockaddr_in servAddr;
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(servPort);
    return servAddr;
}

void sendCheck(int clntSock){
    char mes[] = "INPUT something: \n";
    ssize_t mesLen = sizeof(mes);

    ssize_t numBytesSent = send(clntSock, mes, mesLen, 0);
    if (numBytesSent < 0){
        printf("send() failed\n");
    } else if (numBytesSent != mesLen) printf("send(), unexpected number of bytes\n");
    
}

void sendMesToClient(int clntSock, char* mes, int mesLen){
    ssize_t numBytesSent = send(clntSock, mes, mesLen, 0);
    if (numBytesSent < 0){
        printf("send() failed\n");
    } else if (numBytesSent != mesLen) printf("send(), unexpected number of bytes\n");
}

char* recvMesFromClient(ClientData clnt){
    char *buffer = malloc(BUFSIZ);
    ssize_t numBytes = recv(clnt.clntSock, buffer, BUFSIZ - 1, 0);
    if (numBytes < 0) {
        printf("recv() failed\n");
        return NULL;
    } else if (numBytes == 0) {
        printf("recv(), connection closed prematurely\n");
        return NULL;
    }
    buffer[numBytes] = '\0';
    return buffer;
}

int main(void){
    mainSer((char*)"8080");
    return 0;
}
