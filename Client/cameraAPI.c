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


void jpegWrite(unsigned char* img, char* jpegFilename)
{
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;

    JSAMPROW row_pointer[1];
    FILE *outfile = fopen( jpegFilename, "wb" );

    // try to open file for saving
    if (!outfile) {
        fprintf(stderr, "%s error %d, %s\n", "jpeg", errno, strerror(errno));
    }

    // create jpeg data
    cinfo.err = jpeg_std_error( &jerr );
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, outfile);

    // set image parameters
    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_YCbCr;

    // set jpeg compression parameters to default
    jpeg_set_defaults(&cinfo);
    // and then adjust quality setting
    jpeg_set_quality(&cinfo, 120, TRUE);

    // start compress
    jpeg_start_compress(&cinfo, TRUE);

    // feed data
    while (cinfo.next_scanline < cinfo.image_height) {
        row_pointer[0] = &img[cinfo.next_scanline * cinfo.image_width *  cinfo.input_components];
        jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    // finish compression
    jpeg_finish_compress(&cinfo);

    // destroy jpeg data
    jpeg_destroy_compress(&cinfo);

    // close output file
    fclose(outfile);
}

int init_socket(int port, char* address){
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
    if (connect(socket_service,(struct sockaddr*) &(sin), sizeof(sin)) != 0){
        printf("Cannot connect to server\n");
        exit(1);
    }
    return socket_service;
}

int cameraAPI_snapshot(CAMERA* myCam)
{
    int command = 0;
    write(myCam->fd, &command, sizeof(int));
/*    printf("Sending snapshot order\n");*/
    read(myCam->fd, &(myCam->status), sizeof(int));
/*    printf("Camera status: %d\n", myCam->status);*/
    if (myCam->status != -1){
        // MSG_WAITALL should block until all data has been received. From the manual page on recv.
        int a= recv(myCam->fd, myCam->lastImage, sizeof(char)*height*width*3,MSG_WAITALL);
/*        printf("%d\n",a);*/
    }
    return myCam->status;
}

void* cameraAPI_getIP(void* arg)
{
    CAMERA* myCam = (CAMERA *) arg;
    int server_fd;
    int data_received = 0;
    struct timeval tv;
    tv.tv_sec = 30;
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
        printf("\t Camera not found");
    }
    return NULL;
}

int cameraAPI_init(CAMERA* myCam)
{

    myCam->status = -1;
    myCam->lastImage = malloc(sizeof(char)*height*width*3);
    myCam->fd = init_socket(PORT, myCam->IPaddress);
    read(myCam->fd, &(myCam->status), sizeof(int));
    return myCam->status;
}

int cameraAPI_destroy(CAMERA* myCam)
{
    int command = 1;
    write(myCam->fd, &command, sizeof(int));
    close(myCam->fd);
    free(myCam->lastImage);
    return myCam->status;
}

