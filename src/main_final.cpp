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
const int K_HEIGHT = 320;
const int ZONE_W = K_WIDTH / COLS;
const int ZONE_H = K_HEIGHT / ROWS;

const float VITESSE_MM_S = 14.0;
const float COURSE_MAX = 55.0;
const int VMAX = 4095;
const int VOFF = 0;
const int VMOY = 2045;

struct MotorState
{
    float current_pos = 0;
    float target_pos = 0;
};

static MotorState moteurs[TOTAL_MOTORS];
// On utilise un pointeur pour une initialisation propre dans le main
static PCA9685 pca;

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
        int chA = i * 2;
        int chB = i * 2 + 1;

        if (std::abs(diff) > 1.0) // Seuil de tolérance réduit à 2mm
        {
            if (diff > 0)
            {
                if (diff > 25)
                {
                    pca.set_pwm(chA, VMAX); // Monter
                    pca.set_pwm(chB, VOFF);
                }
                else
                {
                    pca.set_pwm(chA, VMOY); // Monter
                    pca.set_pwm(chB, VOFF);
                }
            }
            else
            {
                if (diff < -25)
                {
                    pca.set_pwm(chA, VOFF); // Monter
                    pca.set_pwm(chB, VMAX);
                }
                else
                {
                    pca.set_pwm(chA, VOFF);
                    pca.set_pwm(chB, VMOY);
                } // Descendre
            }
            // Odométrie : 14mm/s divisé par 50 itérations/sec = 0.28mm par itération
            moteurs[i].current_pos += (diff > 0 ? 0.28f : -0.28f);
        }
        else
        {
            pca.set_pwm(chA, 0);
            pca.set_pwm(chB, 0);
        }
    }
}

static void reset_pins_to_zero()
{
    printf("\n[RESET] Remise à zéro des pins en cours...\n");

    // 1. On donne l'ordre de descendre à tous les moteurs
    // On utilise une puissance de 2500 (environ 60%) pour ne pas brûler les moteurs en butée
    for (int i = 0; i < TOTAL_MOTORS; i++)
    {
        pca.set_pwm(i * 2, 0);        // Canal A à 0
        pca.set_pwm(i * 2 + 1, 2500); // Canal B à 2500 (Descente)
    }

    // 2. Temps d'attente
    // Si la course est de 55mm à 14mm/s, il faut 4 secondes.
    // On attend 5 secondes pour être sûr à 100%.
    int wait_time = 5;
    for (int s = wait_time; s > 0; s--)
    {
        printf("\rFin du reset dans %d secondes...  ", s);
        fflush(stdout);
        usleep(1000000);
    }

    // 3. On coupe le courant et on remet les compteurs à zéro
    for (int i = 0; i < TOTAL_MOTORS; i++)
    {
        pca.set_pwm(i * 2, 0);
        pca.set_pwm(i * 2 + 1, 0);

        moteurs[i].current_pos = 0.0f; // Reset de l'odométrie
        moteurs[i].target_pos = 0.0f;  // Reset de la cible
    }
    printf("\n[RESET] Terminé. Les pins sont à 0mm.\n\n");
}

static int main_final() // Utilise int main() ou appelle main_final depuis ton vrai main
{
    uint16_t *depth_buffer = NULL;
    uint32_t timestamp;

    // 1. Initialisation dynamique sur l'adresse détectée (0x41)
    double a = 0x40;
    pca = PCA9685(a);

    printf("Recherche du PCA9685 sur %lf...\n", a);
    if (!pca.init())
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
    // On ramène tous les moteurs à la position zéro avant de quitter
    reset_pins_to_zero();

    // Nettoyage avant sortie
    printf("\nArrêt en cours...\n");
    freenect_sync_stop();

    // Éteindre tous les moteurs
    for (int i = 0; i < 16; i++)
        pca.set_pwm(i, 0);

    return 0;
}