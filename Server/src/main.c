// To compile:
//      gcc -Wall main.c libCamera.c gpioManager.c -o server -lv4l2 -ljpeg -pthread
//
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "libCamera.h"
#include "gpioManager.h"


#define OK 0
#define KO -1

#define SNAPSHOT 0
#define CLOSE 1

#define BROADCAST_PORT 5678

int port = 32424;

typedef struct threadarg{
    int socket;
} THREAD_ARG;

int socket_RV;
GPIO* ledStatus;

CAMERA myCam = {0};

void sigint_handler(int sig){
    printf("Signal caught\n");
    close(socket_RV);
    free(myCam.lastImage);
    closeGPIO(ledStatus);
    exit(0);
}





void getIPaddress(char host[50])
{
    //--------------
    //Function that get the IP address from the 'eth0' interface (IPV4 necessary)
    //--------------

    printf("Get IP address:\n");
    struct ifaddrs *ifaddr, *ifa;
    int s;

    if (getifaddrs(&ifaddr) == -1) 
    {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }


    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) 
    {
        if (ifa->ifa_addr == NULL)
            continue;  

        s=getnameinfo(ifa->ifa_addr,sizeof(struct sockaddr_in),host, 50, NULL, 0, NI_NUMERICHOST);

        if((strcmp(ifa->ifa_name,"eth0")==0)&&(ifa->ifa_addr->sa_family==AF_INET))
        {
            if (s != 0)
            {
                printf("getnameinfo() failed: %s\n", gai_strerror(s));
                exit(EXIT_FAILURE);
            }
            printf("\tInterface : <%s>\n",ifa->ifa_name );
            printf("\t  Address : <%s>\n", host); 
        }
    }

    freeifaddrs(ifaddr);

}

void* broadcastIPaddress(void* arg)
{
    //--------------
    //Function called by a thread that broadcasts the IP address of the Camera (IPV4 necessary).
    // The IP is broadcasted during 30s, 30 times.
    //--------------

    int server_fd; 
    struct sockaddr_in address; 

    char IPaddress[50]; 

    getIPaddress(IPaddress);

    // Creating socket file descriptor 
    if ((server_fd = socket(AF_INET, SOCK_DGRAM, 0)) == 0) 
    { 
        printf("Socket for broadcasting failed\n"); 
        return NULL; 
    } 
    int broadcastEnable=1;
    // Forcefully attaching socket to the port
    if (setsockopt(server_fd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable
                                                 , sizeof(broadcastEnable))<0) 
    { 
        printf("Setsockopt for broadcasting failed\n"); 
        return NULL; 
    }

    address.sin_family = AF_INET; 
    address.sin_addr.s_addr = inet_addr("255.255.255.255"); 
    address.sin_port = htons(BROADCAST_PORT); 
      
    printf("Start broadcasting IP address on port %d\n",BROADCAST_PORT);
    int LEDval = writeGPIO(ledStatus, HIGH);
    for(int n=0; n<30; n++)
    {
        sendto(server_fd , IPaddress , 50*sizeof(char) , 0,(struct sockaddr *)  &address, sizeof(address) );
        sleep(1);
        LEDval = writeGPIO(ledStatus, !(LEDval));
    }
    printf("Broadcasting ended\n");
    writeGPIO(ledStatus, HIGH);
    return NULL;

}

int initSocketServer(int port, struct sockaddr_in* sinp){
    //--------------
    //Function that initializes a socket RV.
    //--------------

    int sock_RV = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_RV == -1){
        perror("Unable to create the socket");
        exit(1);
    }
    struct sockaddr_in sin;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    if (bind(sock_RV, (struct sockaddr*) &sin, sizeof(sin)) == -1){
        perror("Unable to bind socket");
        exit(1);
    }
    if (listen(sock_RV,5) == -1){
        perror("Unable to listen on the socket");
        exit(1);
    }
    *sinp = sin;
    return sock_RV;
}

void* clientRoutine(void* arg){
    //--------------
    //Function called after a client is connected.
    //--------------

    THREAD_ARG* threadArg = (THREAD_ARG*) arg;
    int end = 0;
    int command;
    while (!end){
        printf("Waiting for order\n");
        command = -1;
        read(threadArg->socket, &command, sizeof(int));
        switch (command){
            case SNAPSHOT:
                printf("Received snapshot order\n");
                writeGPIO(ledStatus,LOW);
                capture_image(&myCam);
                writeGPIO(ledStatus,HIGH);
                printf("Image taken with status %d\n", myCam.status);
                write(threadArg->socket, &myCam.status, sizeof(int));
                if (myCam.status == 0){
                    send(threadArg->socket, myCam.lastImage, sizeof(char)*width*height*3,0);
                }
                break;
            case CLOSE:
                printf("Client disconnected\n");
                close(threadArg->socket);
                end = 1;
                break;
        }
    }
    printf("Client routine ended\n");
    return NULL;
}

void sendStatus(int socket){
    //--------------
    //Function that send the Camera status
    //--------------

    write(socket, &(myCam.status), sizeof(int));

}

void waitForConnection(int socket_RV, struct sockaddr_in* sin){
    //--------------
    //Function that calls the Client Routine when a new client is connected
    //--------------

    socklen_t length = sizeof(*sin);
    pthread_t clientThread;
    THREAD_ARG threadArg;
    int socket = accept(socket_RV, (struct sockaddr*) sin, &length);
    printf("Connection of client\n");
    threadArg.socket = socket;
    sendStatus(socket);
    pthread_create(&clientThread, NULL, clientRoutine, (void*) &threadArg);
}

int main(int argc, char* argv[])

{
    signal(SIGINT, sigint_handler);

    printf("Default PORT : %d\n",port);

    if (argc == 2){
        port = atoi(argv[1]);
    }

    initCamera(&myCam);
    pthread_t broadcastThread;

    ledStatus = initGPIO(18, "out", 1);
    int LEDval = writeGPIO(ledStatus, HIGH);

    printf("Camera status: %d\n", myCam.status);
    if (myCam.status==OK)
    {
        struct sockaddr_in sin = {0};

        socket_RV = initSocketServer(port, &sin);
        pthread_create(&broadcastThread, NULL, broadcastIPaddress, NULL);

        while (1){
            waitForConnection(socket_RV, &sin);
        }
    }
    else
    {
        while(1){
            LEDval = writeGPIO(ledStatus,!LEDval);
            usleep(200000);
        }
    }

    return 0;
}
