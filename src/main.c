#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "libfreenect.h"


int main(void) {
    freenect_context *ctx = NULL;
    freenect_device *dev = NULL;
    int ret;

    printf("Initializing freenect...\n");

    // Initialize freenect context
    ret = freenect_init(&ctx, NULL);
    if (ret < 0) {
        fprintf(stderr, "Error initializing freenect context: %d\n", ret);
        return EXIT_FAILURE;
    }
    printf("Freenect context initialized successfully\n");

    // Set log level for debugging
    freenect_set_log_level(ctx, FREENECT_LOG_INFO);

    // Get number of connected devices
    int num_devices = freenect_num_devices(ctx);
    printf("Number of devices found: %d\n", num_devices);

    if (num_devices > 0) {
        // Open the first device
        ret = freenect_open_device(ctx, &dev, 0);
        if (ret < 0) {
            fprintf(stderr, "Error opening device: %d\n", ret);
            freenect_shutdown(ctx);
            return EXIT_FAILURE;
        }
        printf("Device opened successfully\n");

        // Close the device
        freenect_close_device(dev);
    }

    // Shutdown freenect context
    ret = freenect_shutdown(ctx);
    if (ret < 0) {
        fprintf(stderr, "Error shutting down freenect: %d\n", ret);
        return EXIT_FAILURE;
    }
    printf("Freenect shutdown successfully\n");

    return EXIT_SUCCESS;
}