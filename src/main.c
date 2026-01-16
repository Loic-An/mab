#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "libfreenect.h"
#include "vl53l0x_api.h"
#include "vl53l0x_platform.h"

// Callback appelé à chaque nouvelle image de profondeur
void depth_cb(freenect_device *dev, void *v_depth, uint32_t timestamp)
{
    // Exemple : lire la distance du pixel central (320x240)
    printf("Profondeur au centre : %d mm\n", ((uint16_t *)v_depth)[320 + 240 * 640]);
}

int test_freenect_initialization(freenect_context *ctx, freenect_device *dev)
{

    // 1. Initialisation
    if (freenect_init(&ctx, NULL) < 0)
        return 1;

    // 2. Ouverture du premier périphérique trouvé
    if (freenect_open_device(ctx, &dev, 0) < 0)
        return 1;

    // 3. Configuration de la profondeur
    freenect_set_depth_callback(dev, depth_cb);
    freenect_set_depth_mode(dev, freenect_find_depth_mode(FREENECT_RESOLUTION_MEDIUM, FREENECT_DEPTH_11BIT));

    // 4. Démarrage du flux
    freenect_start_depth(dev);

    printf("Lecture des données... Appuyez sur Ctrl+C pour arrêter.\n");

    // 5. Boucle principale pour traiter les événements USB
    while (freenect_process_events(ctx) >= 0)
    {
        // Le programme tourne ici et appelle depth_cb automatiquement
    }

    freenect_stop_depth(dev);
    freenect_close_device(dev);
    freenect_shutdown(ctx);

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

    (void)test_freenect_initialization(ctx, dev);
    /*
        // test VL53L0X initialization
        VL53L0X_DEV vl53l0x_dev = NULL;
        (void)test_vl53l0x_initialization(vl53l0x_dev);
        return 0;
        */
}