#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "libfreenect_sync.h"
#include "vl53l0x_api.h"
#include "vl53l0x_platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libfreenect.h>
#include <libfreenect_sync.h>

int test_freenect()
{

    uint16_t *depth_buffer = NULL;
    uint32_t timestamp;

    // Coordonnées du pixel central (640x480)
    int x = 320;
    int y = 240;
    int index = y * 640 + x;

    printf("Lecture de la Kinect (Mode Synchrone)...\n");

    while (1)
    {
        // Récupération synchrone de la frame de profondeur
        // Cette fonction attend qu'une nouvelle frame soit disponible
        int ret = freenect_sync_get_depth((void **)&depth_buffer, &timestamp, 0, FREENECT_DEPTH_11BIT);
        usleep(100000);
        if (ret != 0)
        {
            printf("Erreur : Impossible de récupérer les données (Kinect déconnectée ?)\n");
            break;
        }

        // Accès à la valeur brute du pixel central
        uint16_t raw_depth = depth_buffer[index];

        if (raw_depth >= 2047)
        {
            printf("Pixel [%d, %d] : Hors de portée / Trop proche\n", x, y);
        }
        else
        {
            printf("Pixel [%d, %d] : Distance brute = %d\n", x, y, raw_depth);
        }

        // Optionnel : un petit délai pour ne pas saturer le processeur
        // usleep(10000);
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
    dev->I2cDevAddr = 0x29; // Default I2C address for VL53L0X
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

    test_freenect();
    /*
        // test VL53L0X initialization
        VL53L0X_DEV vl53l0x_dev = NULL;
        (void)test_vl53l0x_initialization(vl53l0x_dev);
        return 0;
        */
}