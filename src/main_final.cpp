#include <libfreenect_sync.h>
#include <stdio.h>
#include <unistd.h>
#include <algorithm>
#include <vector>
#include <cmath>
#include "pca9685.hpp"

// Ajuste ici le nombre de moteurs réels
#define COLS 1
#define ROWS 1
#define TOTAL_MOTORS (COLS * ROWS)

extern volatile int should_exit;

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
// On utilise un pointeur pour une initialisation propre dans le main
static PCA9685 *pca = nullptr;

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
            printf("M%d [%3.1f -> %3.1f mm] ", i, moteurs[i].current_pos, moteurs[i].target_pos);

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
                // Mapping : Objet proche (600mm) -> 100mm | Loin (1200mm) -> 0mm
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
        int chA = i * 2 + 14;
        int chB = i * 2 + 15;

        if (std::abs(diff) > 1.0) // Seuil de tolérance réduit à 2mm
        {
            if (diff > 0)
            {
                pca->set_pwm(chA, 4095); // Monter
                pca->set_pwm(chB, 0);
            }
            else
            {
                pca->set_pwm(chA, 0);
                pca->set_pwm(chB, 4095); // Descendre
            }
            // Odométrie : 8mm/s divisé par 50 itérations/sec = 0.16mm par itération
            moteurs[i].current_pos += (diff > 0 ? 0.16f : -0.16f);
        }
        else
        {
            pca->set_pwm(chA, 0);
            pca->set_pwm(chB, 0);
        }
    }
}

static int main_final() // Utilise int main() ou appelle main_final depuis ton vrai main
{
    uint16_t *depth_buffer = NULL;
    uint32_t timestamp;

    // 1. Initialisation dynamique sur l'adresse détectée (0x41)
    double a = 0x41;
    pca = new PCA9685(a);

    printf("Recherche du PCA9685 sur %lf...\n", a);
    if (!pca->init())
    {
        printf("ERREUR: PCA9685 non trouvé\n");
        return 1;
    }

    printf("\e[2J"); // Clear screen

    while (!should_exit)
    {
        // Récupération de la profondeur
        int ret = freenect_sync_get_depth((void **)&depth_buffer, &timestamp, 0, FREENECT_DEPTH_11BIT);

        if (ret == 0)
        {
            process_kinect_logic(depth_buffer);
            drive_motors();
            render_ui();
        }
        else
        {
            // Si la Kinect ne répond pas, on affiche un message sans effacer l'UI
            printf("\rAttente données Kinect...          ");
            fflush(stdout);
        }

        usleep(20000); // 50 Hz
    }

    // Nettoyage avant sortie
    printf("\nArrêt en cours...\n");
    freenect_sync_stop();

    // Éteindre tous les moteurs
    for (int i = 0; i < 16; i++)
        pca->set_pwm(i, 0);

    delete pca;
    return 0;
}