#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <jpeglib.h>
#include <signal.h>
#include <errno.h>
#include <arpa/inet.h>

#include "cameraAPI.h"


int init_socket(int port, char* address){
    //--------------
    //Function that creates a socket from a port and an address given
    //--------------

    int socket_service = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_service == -1){
        perror("Unable to create the socket");
        exit(1);
    }
    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    struct hostent* host, *gethostbyname();
    host = gethostbyname(address);
    bcopy(host->h_addr, &sin.sin_addr.s_addr,host->h_length);
    struct timeval tv;
    tv.tv_sec = 5;
    setsockopt(socket_service, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    if (connect(socket_service,(struct sockaddr*) &(sin), sizeof(sin)) != 0){
        printf("Cannot connect to server\n");
        exit(1);
    }
    return socket_service;
}

int cameraAPI_snapshot(CAMERA* myCam)
{
    //--------------
    //API function to take a snapshot
    //--------------

    int command = 0;
    write(myCam->fd, &command, sizeof(int));
/*    printf("Sending snapshot order\n");*/
    int value = read(myCam->fd, &(myCam->status), sizeof(int));
    if (value < 1) myCam->status = -1;
/*    printf("Camera status: %d\n", myCam->status);*/
    if (myCam->status != -1){
        // MSG_WAITALL should block until all data has been received. From the manual page on recv.
        int a= recv(myCam->fd, myCam->lastImage, sizeof(char)*height*width*3,MSG_WAITALL);
/*        printf("%d\n",a);*/
    }
    return myCam->status;
}

int cameraAPI_video_init(CAMERA* myCam)
{
    //--------------
    //API function to init a streaming session
    //--------------

    int command = 2;
    write(myCam->fd, &command, sizeof(int));
    read(myCam->fd, &(myCam->status), sizeof(int));
    return myCam->status;
}

int cameraAPI_video(CAMERA* myCam, int stop)
{
    //--------------
    //API function to get the image stream
    //--------------
        // MSG_WAITALL should block until all data has been received. From the manual page on recv.
    int len = recv(myCam->fd, myCam->lastImage, sizeof(char)*height*width*3,MSG_WAITALL);
    if (len==3*height*width)
    {
        write(myCam->fd, &stop, sizeof(int));
    }
    else myCam->status=-1;
    return myCam->status;
}


void* cameraAPI_getIP(void* arg)
{
    //--------------
    //API function to catched the Camera's IP address from broadcast (IPV4 necessary)
    //--------------

    CAMERA* myCam = (CAMERA *) arg;
    int server_fd;
    int data_received = 0;
    struct timeval tv;
    tv.tv_sec = 60;
    tv.tv_usec = 0;

    struct sockaddr_in address, add_other; 

    // Creating socket file descriptor 
    if ((server_fd = socket(AF_INET, SOCK_DGRAM, 0)) == 0) 
    { 
        printf("socket failed\n"); 
        return NULL;
    } 
    int broadcastEnable=1;
    // Forcefully attaching socket to the port

    if (setsockopt(server_fd,  SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv))<0) 
    { 
        printf("setsockopt 2 error\n");
        return NULL;
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable
                                                 , sizeof(broadcastEnable))<0) 
    { 
        printf("setsockopt 1 error\n"); 
        return NULL;
    }



    address.sin_family = AF_INET; 
    address.sin_addr.s_addr = inet_addr("255.255.255.255"); 
    address.sin_port = htons(BROADCAST_PORT); 

    if (bind(server_fd, (struct sockaddr *) &address, sizeof(address)) < 0)
    {
        printf("Bind failed\n");
        return NULL;
    }

    printf("Start getting IP address on port %d during %ld seconds\n",BROADCAST_PORT, tv.tv_sec);
    data_received = recvfrom(server_fd , myCam->IPaddress , 50*sizeof(char) , 0, NULL, 0);
    close(server_fd);
    if (data_received>0)
    {
        cameraAPI_init(myCam);
        printf("\tCamera detected at : %s\n",myCam -> IPaddress);
        printf("Getting ended\n");
    }
    else
    {
        printf("\t Camera not found\n");
    }
    return NULL;
}

int cameraAPI_init(CAMERA* myCam)
{
    //--------------
    //API function to initialize a Camera
    //--------------
    myCam->status = -1;
    myCam->lastImage = malloc(sizeof(char)*height*width*3);
    myCam->fd = init_socket(PORT, myCam->IPaddress);
    read(myCam->fd, &(myCam->status), sizeof(int));
    return myCam->status;
}

int cameraAPI_destroy(CAMERA* myCam)
{
    //--------------
    //API function to close a Session
    //--------------
    int command = 1;
    write(myCam->fd, &command, sizeof(int));
    close(myCam->fd);
    free(myCam->lastImage);
    return myCam->status;
}

