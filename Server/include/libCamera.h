#ifndef RPI_CAMERA_IP_LIBCAMERA_H
#define RPI_CAMERA_IP_LIBCAMERA_H

static unsigned int width = 640;
static unsigned int height = 480;

typedef struct {
    int status;
    int fd;
    unsigned char* lastImage;
} CAMERA;

void initCamera(CAMERA* myCam);
void capture_image(CAMERA* myCam, int stream);

#endif //RPI_CAMERA_IP_LIBCAMERA_H
