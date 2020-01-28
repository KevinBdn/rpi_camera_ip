#ifndef GPIO_MANAGER

#define GPIO_MANAGER

#define LOW 0
#define HIGH 1

typedef struct GPIO{
    int fd;
    int num;
} GPIO;


GPIO* initGPIO(int num, char input_output_mode[3], int force_mode);

int writeGPIO(GPIO* gpio, int val);

int closeGPIO(GPIO* gpio);

#endif
