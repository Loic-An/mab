#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "libfreenect.h"
#include "vl53l0x_api.h"

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

int main(void)
{
    // test freenect initialization and device handling
    freenect_context *ctx = NULL;
    freenect_device *dev = NULL;

    (void)test_freenect_initialization(ctx, dev);

    // test if vl53l0x sensor is present
    printf("Testing VL53L0X sensor measurement\n");
    VL53L0X_DEV vl53l0x_dev;
    VL53L0X_Error status;
    vl53l0x_dev = (VL53L0X_DEV)malloc(sizeof(VL53L0X_Dev_t));
    if (vl53l0x_dev == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for VL53L0X device\n");
        return EXIT_FAILURE;
    }
    vl53l0x_dev->I2cDevAddr = 0x29; // Default I2C address for VL53L0X
    status = VL53L0X_DataInit(vl53l0x_dev);
    if (status != VL53L0X_ERROR_NONE)
    {
        fprintf(stderr, "VL53L0X DataInit failed with error: %d\n", status);
        free(vl53l0x_dev);
        return EXIT_FAILURE;
    }
    printf("VL53L0X DataInit successful\n");
    // make a measurement
    VL53L0X_RangingMeasurementData_t ranging_data;
    status = VL53L0X_PerformSingleRangingMeasurement(vl53l0x_dev, &ranging_data);
    if (status != VL53L0X_ERROR_NONE)
    {
        fprintf(stderr, "VL53L0X measurement failed with error: %d\n", status);
        free(vl53l0x_dev);
        return EXIT_FAILURE;
    }
    printf("VL53L0X Measurement successful: Distance = %d mm\n", ranging_data.RangeMilliMeter);
    free(vl53l0x_dev);

    printf("VL53L0X sensor test completed\n");
    return EXIT_SUCCESS;
}