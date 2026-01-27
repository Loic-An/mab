#include <libfreenect_sync.h>
#include <stdio.h>
#include <unistd.h>
#include <algorithm>
#include <vector>
#include <cmath>
#include "pca9685.hpp"

// Configuration Matrice
#define COLS 1
#define ROWS 1
#define TOTAL_MOTORS (COLS * ROWS)

// Constantes Physiques
const float VITESSE_MM_S = 8.0f;
const float COURSE_MAX = 100.0f;
const float DT = 0.02f; // 20ms (50Hz)
const float STEP_DIST = VITESSE_MM_S * DT;

// Plage Kinect (en mm)
const float K_NEAR = 600.0f; // Objet proche -> Extension max
const float K_FAR = 1200.0f; // Objet loin -> Extension min

extern volatile int should_exit;

struct MotorState
{
    float current_pos = 0.0f;
    float target_pos = 0.0f;
};

// Variables globales statiques
static MotorState moteurs[TOTAL_MOTORS];
static PCA9685 *pca = nullptr;

/**
 * Affiche l'état des moteurs dans la console
 */
static void render_ui()
{
    printf("\e[H"); // Reset curseur
    printf("===== SHAPE DISPLAY ACTIVE =====\n");
    printf("Moteurs: %d | Freq: 50Hz\n", TOTAL_MOTORS);
    printf("--------------------------------\n");

    for (int i = 0; i < TOTAL_MOTORS; i++)
    {
        printf("M%02d [%5.1f -> %5.1f mm] ", i, moteurs[i].current_pos, moteurs[i].target_pos);

        // Barre de progression visuelle
        int bars = (int)(moteurs[i].current_pos / (COURSE_MAX / 20.0f));
        printf("[");
        for (int b = 0; b < 20; b++)
            printf(b < bars ? "#" : " ");
        printf("]\n");
    }
    printf("--------------------------------\n");
    printf("Ctrl+C pour quitter\n");
}

/**
 * Analyse les données de profondeur et définit les cibles
 */
static void process_kinect_logic(uint16_t *depth_buffer)
{
    const int K_WIDTH = 640;
    const int K_HEIGHT = 480;
    const int ZONE_W = K_WIDTH / COLS;
    const int ZONE_H = K_HEIGHT / ROWS;

    for (int r = 0; r < ROWS; r++)
    {
        for (int c = 0; c < COLS; c++)
        {
            int motor_idx = r * COLS + c;

            // Échantillonnage au centre de la zone
            long sum_depth = 0;
            int samples = 0;
            int centerX = c * ZONE_W + (ZONE_W / 2);
            int centerY = r * ZONE_H + (ZONE_H / 2);

            for (int y = centerY - 15; y < centerY + 15; y++)
            {
                for (int x = centerX - 15; x < centerX + 15; x++)
                {
                    uint16_t d = depth_buffer[y * K_WIDTH + x];
                    if (d > 0 && d < 2047)
                    { // Filtre données valides
                        sum_depth += d;
                        samples++;
                    }
                }
            }

            if (samples > 20)
            {
                float avg_raw = (float)sum_depth / samples;

                // Conversion brute vers mm (approximation libfreenect)
                // d_mm = 1000 / (raw * -0.00307 + 3.33)
                float depth_mm = 1.0f / (avg_raw * -0.00307f + 3.33f) * 1000.0f;

                // Mapping Linéaire inverse :
                // K_NEAR (600mm) -> COURSE_MAX (100mm)
                // K_FAR (1200mm) -> 0mm
                float val = COURSE_MAX * (1.0f - (depth_mm - K_NEAR) / (K_FAR - K_NEAR));
                moteurs[motor_idx].target_pos = std::clamp(val, 0.0f, COURSE_MAX);
            }
        }
    }
}

/**
 * Commande les ponts en H via le PCA9685
 */
static void drive_motors()
{
    for (int i = 0; i < TOTAL_MOTORS; i++)
    {
        float diff = moteurs[i].target_pos - moteurs[i].current_pos;

        // Canaux : Moteur 0 -> Pin 0 & 1 | Moteur 1 -> Pin 2 & 3...
        int chA = i * 2;
        int chB = i * 2 + 1;

        if (std::abs(diff) > 1.5f)
        { // Zone morte de 1.5mm pour éviter les oscillations
            if (diff > 0)
            {
                pca->set_pwm(chA, 4095); // Monter
                pca->set_pwm(chB, 0);
                moteurs[i].current_pos += STEP_DIST;
            }
            else
            {
                pca->set_pwm(chA, 0);
                pca->set_pwm(chB, 4095); // Descendre
                moteurs[i].current_pos -= STEP_DIST;
            }
        }
        else
        {
            // Stop moteur
            pca->set_pwm(chA, 0);
            pca->set_pwm(chB, 0);
        }
    }
}

/**
 * Point d'entrée principal (à appeler depuis ton main.cpp)
 */
int main_final2()
{
    uint16_t *depth_buffer = nullptr;
    uint32_t timestamp;

    // 1. Initialisation propre de l'objet PCA sur le tas
    pca = new PCA9685(0x40);

    printf("Initialisation I2C (0x40)...\n");
    if (!pca->init())
    {
        printf("ERREUR CRITIQUE : PCA9685 introuvable sur le bus I2C.\n");
        delete pca;
        return 1;
    }

    printf("\e[2J"); // Efface l'écran une fois

    while (!should_exit)
    {
        // 2. Capture Kinect
        int ret = freenect_sync_get_depth((void **)&depth_buffer, &timestamp, 0, FREENECT_DEPTH_11BIT);

        if (ret == 0)
        {
            process_kinect_logic(depth_buffer);
            drive_motors();
            render_ui();
        }
        else
        {
            printf("\r[!] Erreur Kinect : Verifiez le branchement USB...          ");
            fflush(stdout);
        }

        usleep(20000); // 20ms = 50Hz
    }

    // 3. Cleanup
    printf("\nArrêt des moteurs et nettoyage...\n");
    for (int i = 0; i < 16; i++)
    {
        pca->set_pwm(i, 0);
    }

    freenect_sync_stop();
    delete pca; // Libération mémoire
    pca = nullptr;

    return 0;
}