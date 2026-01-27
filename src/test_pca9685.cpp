#include <libfreenect_sync.h>
#include <stdio.h>
#include <unistd.h>
#include <algorithm>
#include <vector>
#include <cmath>
#include "pca9685.hpp"

extern volatile int should_exit;
static int main_pca9685()
{
    double a = 0x40;
    PCA9685 pca = PCA9685(a);

    printf("Recherche du PCA9685 sur %lf...\n", a);
    if (!pca.init())
    {
        printf("ERREUR: PCA9685 non trouvé\n");
        return 1;
    }
    // test du pca9685

    while (!should_exit)
    {
        for (int i = 0; i < 4; i++)
        {
            pca.set_pwm(i, 4095); // 50% duty cycle
            usleep(500000);       // wait 500ms
            pca.set_pwm(i, 0);    // turn off
        }
    }
}