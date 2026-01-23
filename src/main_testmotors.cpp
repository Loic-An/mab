#include <libfreenect/libfreenect_sync.h>
#include <unistd.h>

static int main_testmotors()
{
    freenect_context *ctx;
    freenect_device *dev;

    if (freenect_init(&ctx, NULL) < 0)
        return 1;

    // Ouvre le device 0
    if (freenect_open_device(ctx, &dev, 0) < 0)
    {
        printf("Impossible d'ouvrir la Kinect\n");
        return 1;
    }

    printf("Test du moteur : Inclinaison à +15°\n");
    freenect_set_tilt_degs(dev, 15);
    sleep(2);

    printf("Inclinaison à -15°\n");
    freenect_set_tilt_degs(dev, -15);
    sleep(2);

    printf("Retour à 0°\n");
    freenect_set_tilt_degs(dev, 0);

    freenect_close_device(dev);
    freenect_shutdown(ctx);
    return 0;
}