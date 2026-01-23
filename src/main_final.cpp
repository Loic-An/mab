#include <libfreenect/libfreenect_sync.h>
#include <stdio.h>
#include <unistd.h>
#include <algorithm>
#include "pca9685.hpp"

// Paramètres de la matrice
#define NB_PINS_X 8
#define NB_PINS_Y 8
#define STEP_X (640 / NB_PINS_X)
#define STEP_Y (480 / NB_PINS_Y)

// Paramètres mécaniques (À CALIBRER)
const float MM_PAR_SECONDE = 5.0; // Vitesse de translation du pin à 100% PWM
const float COURSE_MAX = 100.0;   // 100mm de translation
const int PWM_VITESSE = 4095;     // 100% de puissance sur le PCA9685

struct Pin
{
    float current_pos = 0; // Position estimée en mm
    float target_pos = 0;  // Position demandée par la Kinect
};

Pin matrice[NB_PINS_X][NB_PINS_Y];
PCA9685 pca;

void update_motors()
{
    for (int y = 0; y < NB_PINS_Y; y++)
    {
        for (int x = 0; x < NB_PINS_X; x++)
        {
            float diff = matrice[x][y].target_pos - matrice[x][y].current_pos;

            if (abs(diff) > 2.0)
            { // Tolérance de 2mm pour éviter les vibrations
                // Calcul du temps pour bouger
                float temps_mouvement = abs(diff) / MM_PAR_SECONDE;

                // Direction (Selon ton câblage sur le pont en H)
                if (diff > 0)
                {
                    // Monter le pin
                    pca.set_pwm(x + (y * NB_PINS_X), PWM_VITESSE, 0);
                }
                else
                {
                    // Descendre le pin
                    pca.set_pwm(x + (y * NB_PINS_X), 0, PWM_VITESSE);
                }

                // Simulation du déplacement (Odométrie temps réel)
                // Dans un code parfait, on utiliserait un timer non-bloquant.
                // Ici pour demain, on peut simuler par petits pas :
                usleep(10000);
                matrice[x][y].current_pos += (diff > 0 ? 0.5 : -0.5);
            }
            else
            {
                // Arrêt du moteur
                pca.set_pwm(x + (y * NB_PINS_X), 0, 0);
            }
        }
    }
}

int main_kinect_to_motors()
{
    uint16_t *depth_buffer = NULL;
    uint32_t timestamp;
    pca.init();

    printf("Démarrage du mapping Kinect -> Moteurs...\n");

    while (1)
    {
        // 1. Récupération Depth
        int ret = freenect_sync_get_depth((void **)&depth_buffer, &timestamp, 0, FREENECT_DEPTH_11BIT);
        if (ret != 0)
            continue;

        // 2. Analyse de la profondeur pour chaque Pin
        for (int y = 0; y < NB_PINS_Y; y++)
        {
            for (int x = 0; x < NB_PINS_X; x++)
            {
                // On prend le pixel au centre de la zone du pin
                uint16_t d = depth_buffer[(y * STEP_Y + STEP_Y / 2) * 640 + (x * STEP_X + STEP_X / 2)];

                if (d > 0 && d < 2047)
                {
                    // Mapping : 600mm(proche) -> 100mm(pin haut) | 1200mm(loin) -> 0mm(pin bas)
                    float target = 100.0f - ((float)(d - 600) * (100.0f / 600.0f));
                    matrice[x][y].target_pos = std::clamp(target, 0.0f, 100.0f);
                }
            }
        }

        // 3. Mise à jour des moteurs
        update_motors();

        usleep(10000); // 100Hz
    }
    return 0;
}