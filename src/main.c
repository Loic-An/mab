#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "libfreenect.h"
#include "vl53l0x_api.h"
#include "vl53l0x_platform.h"

int test_freenect_initialization(freenect_context *ctx, freenect_device *dev)
{
    int ret;

    // Initialize freenect context
    ret = freenect_init(&ctx, NULL);
    if (ret < 0)
    {
        fprintf(stderr, "Error initializing freenect context: %d\n", ret);
        return ret;
    }

    // Set log level for debugging
    freenect_set_log_level(ctx, FREENECT_LOG_INFO);

    // Get number of connected devices
    int num_devices = freenect_num_devices(ctx);
    printf("Number of devices found: %d\n", num_devices);

    if (num_devices > 0)
    {
        // Open the first device
        ret = freenect_open_device(ctx, &dev, 0);
        if (ret < 0)
        {
            fprintf(stderr, "Error opening device: %d\n", ret);
            freenect_shutdown(ctx);
            return ret;
        }
        printf("Device opened successfully\n");

        // Close the device
        freenect_close_device(dev);
    }

    // Shutdown freenect context
    ret = freenect_shutdown(ctx);
    if (ret < 0)
    {
        fprintf(stderr, "Error shutting down freenect: %d\n", ret);
        return ret;
    }

    return 0;
}

VL53L0X_Error test_vl53l0x_initialization(VL53L0X_DEV dev)
{
    VL53L0X_Error status;

    dev = (VL53L0X_DEV)malloc(sizeof(VL53L0X_Dev_t));
    if (dev == NULL)
    {
        fprintf(stderr, "Error allocating memory for VL53L0X device\n");
        return VL53L0X_ERROR_BUFFER_TOO_SMALL;
    }
    dev->I2cDevAddr = 29; // Default I2C address for VL53L0X
    dev->Data = (struct VL53L0X_DevData_t *)malloc(sizeof(struct VL53L0X_DevData_t));
    if (dev->Data == NULL)
    {
        fprintf(stderr, "Error allocating memory for VL53L0X device data\n");
        free(dev);
        return VL53L0X_ERROR_BUFFER_TOO_SMALL;
    }
    

    // Initialize the VL53L0X device
    status = VL53L0X_DataInit(dev);
    if (status != VL53L0X_ERROR_NONE)
    {
        fprintf(stderr, "Error initializing VL53L0X device: %d\n", status);
        return status;
    }
    printf("VL53L0X device initialized successfully\n");

    // Perform static initialization
    status = VL53L0X_StaticInit(dev);
    if (status != VL53L0X_ERROR_NONE)
    {
        fprintf(stderr, "Error in static initialization: %d\n", status);
        return status;
    }
    printf("VL53L0X static initialization completed successfully\n");

    return VL53L0X_ERROR_NONE;
}

int main(void)
{
    // test freenect initialization and device handling
    freenect_context *ctx = NULL;
    freenect_device *dev = NULL;

    (void)test_freenect_initialization(ctx, dev);

    // test VL53L0X initialization
    VL53L0X_DEV vl53l0x_dev = NULL;
    (void)test_vl53l0x_initialization(vl53l0x_dev);
    return 0;
}