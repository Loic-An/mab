#include <libfreenect/libfreenect_sync.h>
#include <stdio.h>
#include <unistd.h>
#include <algorithm>
#include <vector>
#include <cmath>
#include "pca9685.hpp"

#define COLS 2 // Ajuste selon ton besoin
#define ROWS 2
#define TOTAL_MOTORS (COLS * ROWS)

const int K_WIDTH = 640;
const int K_HEIGHT = 480;
const int ZONE_W = K_WIDTH / COLS;
const int ZONE_H = K_HEIGHT / ROWS;

const float VITESSE_MM_S = 8.0;
const float COURSE_MAX = 100.0;

struct MotorState
{
    float current_pos = 0;
    float target_pos = 0;
};

static MotorState moteurs[TOTAL_MOTORS];
static PCA9685 pca;

// Fonction de rendu ASCII pour le debug
static void render_ui()
{
    printf("\e[H"); // Retour en haut à gauche
    printf("===== SHAPE DISPLAY DEBUG =====\n");
    printf("Config: %dx%d Matrice | Vitesse: %.1f mm/s\n", COLS, ROWS, VITESSE_MM_S);
    printf("--------------------------------\n");

    for (int r = 0; r < ROWS; r++)
    {
        for (int c = 0; c < COLS; c++)
        {
            int i = r * COLS + c;
            // On dessine une barre de progression verticale ou horizontale
            printf("M%d [%3.0f -> %3.0f mm] ", i, moteurs[i].current_pos, moteurs[i].target_pos);

            // Visualisation graphique simple
            int bars = (int)(moteurs[i].current_pos / 5.0f);
            printf("|");
            for (int b = 0; b < 20; b++)
                printf(b < bars ? "#" : " ");
            printf("|  ");
        }
        printf("\n");
    }
    printf("--------------------------------\n");
    printf("Appuyez sur Ctrl+C pour quitter\n");
}

static void process_kinect_logic(uint16_t *depth_buffer)
{
    for (int r = 0; r < ROWS; r++)
    {
        for (int c = 0; c < COLS; c++)
        {
            int motor_idx = r * COLS + c;
            long sum_depth = 0;
            int samples = 0;
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
                float avg_d = (float)sum_depth / samples;
                // Mapping : 600mm -> 100mm | 1200mm -> 0mm
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
        int chA = i * 2;
        int chB = i * 2 + 1;

        if (std::abs(diff) > 3.0)
        {
            if (diff > 0)
            {
                pca.set_pwm(chA, 4095);
                pca.set_pwm(chB, 0);
            }
            else
            {
                pca.set_pwm(chA, 0);
                pca.set_pwm(chB, 4095);
            }
            // 0.16mm par itération (8mm/s / 50Hz)
            moteurs[i].current_pos += (diff > 0 ? 0.16f : -0.16f);
        }
        else
        {
            pca.set_pwm(chA, 0);
            pca.set_pwm(chB, 0);
        }
    }
}

int main_final()
{
    uint16_t *depth_buffer = NULL;
    uint32_t timestamp;

    if (!pca.init())
    {
        printf("Erreur: PCA9685 non trouvé sur le bus I2C\n");
        return 1;
    }

    printf("\e[2J"); // Efface l'écran au démarrage

    while (1)
    {
        int ret = freenect_sync_get_depth((void **)&depth_buffer, &timestamp, 0, FREENECT_DEPTH_11BIT);
        if (ret == 0)
        {
            process_kinect_logic(depth_buffer);
            drive_motors();
            render_ui(); // Mise à jour de l'affichage
        }
        usleep(20000);
    }
    return 0;
}