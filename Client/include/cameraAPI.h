#ifndef RPI_CAMERA_IP_CAMERAAPI_H
#define RPI_CAMERA_IP_CAMERAAPI_H


#define BROADCAST_PORT 5678

static unsigned  int PORT=32424;

typedef struct {
    int status;
    char IPaddress[50];
    int fd;
    unsigned char* lastImage;
} CAMERA;

static unsigned int width = 640;
static unsigned int height = 480;


int init_socket(int port, char* address);

int cameraAPI_init(CAMERA* myCam);

int cameraAPI_snapshot(CAMERA* myCam);

int cameraAPI_video_init(CAMERA* myCam);

int cameraAPI_video(CAMERA* myCam, int stop);


void* cameraAPI_getIP(void* arg);

int cameraAPI_destroy(CAMERA* myCam);



#endif
