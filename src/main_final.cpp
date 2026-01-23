#include <libfreenect/libfreenect_sync.h>
#include <stdio.h>
#include <unistd.h>
#include <algorithm>
#include <vector>
#include "pca9685.hpp"

// MODIFIE CES VALEURS SELON TA MATRICE RÉELLE
#define COLS 2 // Nombre de colonnes de moteurs
#define ROWS 2 // Nombre de lignes de moteurs
#define TOTAL_MOTORS (COLS * ROWS)

// Paramètres de la Kinect (Résolution standard)
const int K_WIDTH = 640;
const int K_HEIGHT = 480;

// Dimensions d'une zone de détection
const int ZONE_W = K_WIDTH / COLS;
const int ZONE_H = K_HEIGHT / ROWS;

// Paramètres de mouvement
const float VITESSE_MM_S = 8.0; // À ajuster selon tes N20
const float COURSE_MAX = 100.0;

struct MotorState
{
    float current_pos = 0;
    float target_pos = 0;
};

MotorState moteurs[TOTAL_MOTORS];
PCA9685 pca;

static void process_kinect_logic(uint16_t *depth_buffer)
{
    for (int r = 0; r < ROWS; r++)
    {
        for (int c = 0; c < COLS; c++)
        {
            int motor_idx = r * COLS + c;

            // On calcule la moyenne de profondeur au centre de la zone
            // pour éviter qu'un seul pixel bruité ne fasse bouger le moteur.
            long sum_depth = 0;
            int samples = 0;

            // On scanne un petit carré de 20x20 pixels au centre de la zone
            int centerX = c * ZONE_W + (ZONE_W / 2);
            int centerY = r * ZONE_H + (ZONE_H / 2);

            for (int y = centerY - 10; y < centerY + 10; y++)
            {
                for (int x = centerX - 10; x < centerX + 10; x++)
                {
                    uint16_t d = depth_buffer[y * K_WIDTH + x];
                    if (d > 0 && d < 2047)
                    {
                        sum_depth += d;
                        samples++;
                    }
                }
            }

            if (samples > 0)
            {
                float avg_d = sum_depth / samples;

                // Mapping : Objet proche (600mm) -> Cible 100mm | Objet loin (1200mm) -> Cible 0mm
                float target = 100.0f - ((avg_d - 600.0f) * (100.0f / 600.0f));
                moteurs[motor_idx].target_pos = std::clamp(target, 0.0f, COURSE_MAX);
            }
        }
    }
}

static void drive_motors()
{
    for (int i = 0; i < TOTAL_MOTORS; i++)
    {
        float diff = moteurs[i].target_pos - moteurs[i].current_pos;

        // Canaux pour le pont en H (Moteur 0 utilise 0 et 1, Moteur 1 utilise 2 et 3...)
        int chA = i * 2;
        int chB = i * 2 + 1;

        // Seuil de 3mm pour éviter que les moteurs ne forcent inutilement
        if (std::abs(diff) > 3.0)
        {
            if (diff > 0)
            {
                // MONTER
                pca.set_pwm(chA, 4095); // 100% duty
                pca.set_pwm(chB, 0);    // 0% duty
            }
            else
            {
                // DESCENDRE
                pca.set_pwm(chA, 0);
                pca.set_pwm(chB, 4095);
            }
            // Odométrie : on simule le mouvement
            // Si VITESSE_MM_S = 8 et boucle à 50Hz, le pin bouge de 0.16mm par itération
            moteurs[i].current_pos += (diff > 0 ? 0.16f : -0.16f);
        }
        else
        {
            // STOP
            pca.set_pwm(chA, 0);
            pca.set_pwm(chB, 0);
        }
    }
}

static int main_final()
{
    uint16_t *depth_buffer = NULL;
    uint32_t timestamp;
    pca.init();

    printf("Matrice %dx%d prête. %d moteurs configurés.\n", COLS, ROWS, TOTAL_MOTORS);

    while (1)
    {
        int ret = freenect_sync_get_depth((void **)&depth_buffer, &timestamp, 0, FREENECT_DEPTH_11BIT);
        if (ret == 0)
        {
            process_kinect_logic(depth_buffer);
            drive_motors();
        }
        usleep(20000); // Boucle à 50Hz
    }
    return 0;
}