#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "libfreenect_sync.h"
#include "vl53l0x_api.h"
#include "vl53l0x_platform.h"

int test_freenect_sync()
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

// Callback appelé à chaque nouvelle image de profondeur
void depth_cb(freenect_device *dev, void *v_depth, uint32_t timestamp)
{
    // Exemple : lire la distance du pixel central (320x240)
    printf("Profondeur au centre : %d mm\n", ((uint16_t *)v_depth)[320 + 240 * 640]);
}

int test_freenect_async(freenect_context *ctx, freenect_device *dev)
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

int main(int argc, char **argv)
{
    if (argc > 1)
    {
        printf("Running freenect async test\n");
        freenect_context *ctx = NULL;
        freenect_device *dev = NULL;
        return test_freenect_async(ctx, dev);
    }
    printf("Running freenect sync test\n");
    return test_freenect();
}