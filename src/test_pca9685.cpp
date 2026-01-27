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
        pca.set_pwm(3, 2500); // 100% duty cycle
        usleep(1000000);      // wait 500ms
        pca.set_pwm(3, 0);    // turn off
        usleep(1000000);      // wait 500ms
    }
}