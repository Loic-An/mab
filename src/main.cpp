#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string>
#include <libfreenect/libfreenect_sync.h>
#include "vl53l0x.hpp"
#include "main_matrix.cpp"
#include "main_rows.cpp"
#include "pca9685.hpp"

int test_freenect_sync()
{

    uint16_t *depth_buffer = NULL;
    uint32_t timestamp;

    // Coordonnées du pixel central (ecran de 640x480)
    int x = 320;
    int y = 240;
    int index = y * 640 + x;

    printf("Lecture de la Kinect (Mode Synchrone)...\n");

    while (1)
    {
        // Récupération synchrone de la frame de profondeur
        // Cette fonction attend qu'une nouvelle frame soit disponible
        int ret = freenect_sync_get_depth((void **)&depth_buffer, &timestamp, 0, FREENECT_DEPTH_11BIT);
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
        usleep(100000);
    }

    return 0;
}

// Callback appelé à chaque nouvelle image de profondeur
void depth_cb(freenect_device *dev, void *v_depth, uint32_t timestamp)
{
    (void)dev;
    // Exemple : lire la distance du pixel central (320x240)
    printf("[%d]Profondeur au centre : %d mm\n", timestamp, ((uint16_t *)v_depth)[320 + 240 * 640]);
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

int test_vl53l0x()
{
    VL53L0X dev;
    if (!dev.init())
    {
        fprintf(stderr, "Erreur initialisation VL53L0X\n");
        return EXIT_FAILURE;
    }
    printf("VL53L0X initialisé avec succès\n");
    printf("Mesure de distance...\n");
    uint16_t distance = dev.readRangeSingleMillimeters();
    if (distance == 65535 || dev.timeoutOccurred()) {
        fprintf(stderr, "Erreur : Timeout ou échec de lecture\n");
        return EXIT_FAILURE;
    }
    if (errno)
    {
        fprintf(stderr, "Erreur mesure distance VL53L0X\n");
        return EXIT_FAILURE;
    }
    printf("Distance mesurée : %u mm\n", distance);
    return EXIT_SUCCESS;
}

int test_PCA9385(){
    PCA9685 dev;
    dev.init();
    dev.set_time(0,0,4095);
}

int test(int argc, char **argv)
{
    if (argc > 2)
    {
        fprintf(stderr, "Usage: %s <freenect_sync|freenect_async|vl53l0x|matrix|motors|all>\n", argv[0]);
        return EXIT_FAILURE;
    }
    if (argv[1] == std::string("freenect_sync"))
    {
        // Exemple de test : initialisation synchrone de la Kinect
        return test_freenect_sync();
    }
    else if (argv[1] == std::string("freenect_async"))
    {
        // Exemple de test : initialisation asynchrone de la Kinect
        freenect_context *ctx = nullptr;
        freenect_device *dev = nullptr;
        return test_freenect_async(ctx, dev);
    }
    else if (argv[1] == std::string("vl53l0x"))
    {
        // Exemple de test : initialisation du VL53L0X
        return test_vl53l0x();
    }
    else if (argv[1] == std::string("matrix"))
    {
        return main_matrix();
    }
    else if (argv[1] == std::string("motors"))
    {
        return main_motors();
    }
     else if (argv[1] == std::string("pca9685"))
    {
        return test_PCA9385();
    }
    else if (argv[1] == std::string("all"))
    {
        int status = 0;
        printf("===== Test Freenect Sync =====\n");
        status |= test_freenect_sync();
        printf("status=%d\n", status);
        printf("\n===== Test Freenect Async =====\n");
        freenect_context *ctx = nullptr;
        freenect_device *dev = nullptr;
        status |= test_freenect_async(ctx, dev);
        printf("status=%d\n", status);
        printf("\n===== Test VL53L0X =====\n");
        status |= test_vl53l0x();
        printf("status=%d\n", status);
        return status;
    }
    else
    {
        fprintf(stderr, "Usage: %s <freenect_sync|freenect_async|vl53l0x|matrix|motors|all>\n", argv[0]);
        return EXIT_FAILURE;
    }
}

int main(int argc, char **argv)
{
    if (argc > 1)
        return test(argc, argv);
    return 0;
}