#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "lib/libfreenect.h"
#include "lib/pigpio.h"


int main(void) {
    if (gpioInitialise() < 0) {
        fprintf(stderr, "pigpio initialization failed\n");
        return 1;
    }

    // Your code to interact with libfreenect and pigpio goes here

    gpioTerminate();
    return 0;
}