#include <stdio.h>
#include <libfreenect_sync.h>
#include <unistd.h>

#define stepX 12
#define stepY 12

static int main_matrix()
{
    freenect_context *ctx;
    if (freenect_init(&ctx, NULL) < 0)
    {
        printf("ERREUR : freenect_init a échoué. Problème de libfreenect.\n");
    }
    else
    {
        int num_devices = freenect_num_devices(ctx);
        printf("DEBUG : Nombre de Kinect détectées par le driver : %d\n", num_devices);
        freenect_shutdown(ctx);
    }
    printf("\e[1;1H\e[2J");

    uint16_t *depth_buffer = NULL;
    uint32_t timestamp;

    printf("Initialisation Kinect...\n");
    // Un premier appel pour "réveiller" le hardware
    freenect_sync_get_depth((void **)&depth_buffer, &timestamp, 0, FREENECT_DEPTH_11BIT);
    usleep(500000); // Pause

    while (1)
    {
        // On récupère la profondeur
        int ret = freenect_sync_get_depth((void **)&depth_buffer, &timestamp, 0, FREENECT_DEPTH_11BIT);
        if (ret != 0)
        {
            printf("Erreur de lecture...\n");
            usleep(500000);
            continue;
        }

        printf("\e[H");

        for (int y = 0; y < 480; y += stepY)
        {
            for (int x = 0; x < 640; x += stepX)
            {
                uint16_t d = depth_buffer[y * 640 + x];

                // 2047 est le code "sans donnée" ou "trop proche/loin".
                if (d >= 2047 || d == 0)
                {
                    printf("    ");
                }
                else if (d < 600)
                { // TRÈS PROCHE
                    printf(" ###");
                }
                else if (d < 950)
                { // PROCHE
                    printf(" ooo");
                }
                else if (d < 1300)
                { // MILIEU
                    printf(" ---");
                }
                else
                { // LOIN
                    printf("  . ");
                }
            }
            printf("\n");
        }
        // ~30 fps
        usleep(33000);
    }
    return 0;
}