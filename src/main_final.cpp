#include <libfreenect_sync.h>
#include <stdio.h>
#include <unistd.h>
#include <algorithm>
#include <vector>
#include <cmath>
#include "pca9685.hpp"

#define COLS 1
#define ROWS 1
#define TOTAL_MOTORS (COLS * ROWS)

extern volatile int should_exit;

const int K_WIDTH = 640;
const int K_HEIGHT = 480;
const int ZONE_W = K_WIDTH / COLS;
const int ZONE_H = K_HEIGHT / ROWS;

const float VITESSE_MM_S = 14.0;
const float COURSE_MAX = 55.0;
const int VMAX = 4095;
const int VOFF = 0;
const int VMOY = 2500;

const float DIST_SOL = 2000.0f;
const float DIST_OBJ_MAX = 1400.0f;

struct MotorState
{
    float current_pos = 0;
    float target_pos = 0;
};

static MotorState moteurs[TOTAL_MOTORS];
static PCA9685 pca;

// Affiche une représentation visuelle de ce que voit la Kinect
static void show_matrix_viewport(uint16_t *depth_buffer)
{
    printf("\n--- VUE KINECT (Distances en cm) ---\n");
    // On échantillonne la grille (ex: 20x15 caractères pour que ça tienne à l'écran)
    int stepX = K_WIDTH / 40;
    int stepY = K_HEIGHT / 20;

    for (int y = 0; y < K_HEIGHT; y += stepY)
    {
        for (int x = 0; x < K_WIDTH; x += stepX)
        {
            uint16_t d = depth_buffer[y * K_WIDTH + x];
            if (d == 0)
                printf("  . "); // Pas de donnée
            else if (d > 2500)
                printf(" -- "); // Trop loin
            else
            {
                // Affiche la distance en centimètres pour gagner de la place
                printf("%3d ", d / 10);
            }
        }
        printf("\n");
    }
}

static void render_ui()
{
    printf("\e[H"); // Revient en haut de l'écran (sans effacer pour éviter le scintillement)
    printf("===== SHAPE DISPLAY SYSTEM =====\n");
    printf("Matrice: %dx%d | Vitesse: %.1f mm/s\n", COLS, ROWS, VITESSE_MM_S);

    for (int i = 0; i < TOTAL_MOTORS; i++)
    {
        printf("M%d [%3.1f -> %3.1f mm] ", i, moteurs[i].current_pos, moteurs[i].target_pos);
        int bars = (int)(moteurs[i].current_pos / (COURSE_MAX / 15.0f));
        printf("|");
        for (int b = 0; b < 15; b++)
            printf(b < bars ? "#" : " ");
        printf("|  \n");
    }
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

            for (int y = centerY - 20; y < centerY + 20; y++)
            {
                for (int x = centerX - 20; x < centerX + 20; x++)
                {
                    if (x < 0 || x >= K_WIDTH || y < 0 || y >= K_HEIGHT)
                        continue;
                    uint16_t d = depth_buffer[y * K_WIDTH + x];
                    if (d > 400 && d < 2500)
                    {
                        sum_depth += d;
                        samples++;
                    }
                }
            }

            if (samples > 0)
            {
                float avg_mm = (float)sum_depth / samples;
                float ratio = (DIST_SOL - avg_mm) / (DIST_SOL - DIST_OBJ_MAX);
                moteurs[motor_idx].target_pos = std::clamp(ratio * COURSE_MAX, 0.0f, COURSE_MAX);
            }
            else
            {
                moteurs[motor_idx].target_pos = 0; // Rien vu = Pin rentré
            }
        }
    }
}

static void drive_motors()
{
    const float step = VITESSE_MM_S / 50.0f;
    for (int i = 0; i < TOTAL_MOTORS; i++)
    {
        float diff = moteurs[i].target_pos - moteurs[i].current_pos;
        int chA = i * 2;
        int chB = i * 2 + 1;

        if (std::abs(diff) > 1.2f)
        {
            int pwr = (std::abs(diff) > 10) ? VMAX : VMOY;
            if (diff > 0)
            {
                pca.set_pwm(chA, pwr);
                pca.set_pwm(chB, VOFF);
                moteurs[i].current_pos += step;
            }
            else
            {
                pca.set_pwm(chA, VOFF);
                pca.set_pwm(chB, pwr);
                moteurs[i].current_pos -= step;
            }
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
    printf("\n[RESET] Descente des pins...\n");
    for (int i = 0; i < TOTAL_MOTORS; i++)
    {
        pca.set_pwm(i * 2, 0);
        pca.set_pwm(i * 2 + 1, 2500);
    }
    sleep(5);
    for (int i = 0; i < 16; i++)
        pca.set_pwm(i, 0);
}

int main_final()
{
    uint16_t *depth_buffer = NULL;
    uint32_t timestamp;

    pca = PCA9685(0x40);
    if (!pca.init())
        return 1;

    reset_pins_to_zero();
    printf("\e[2J"); // Clear screen une seule fois

    while (!should_exit)
    {
        if (freenect_sync_get_depth((void **)&depth_buffer, &timestamp, 0, FREENECT_DEPTH_MM) == 0)
        {
            process_kinect_logic(depth_buffer);
            drive_motors();

            render_ui();
            show_matrix_viewport(depth_buffer); // Affiche la matrice brute en dessous
        }
        usleep(20000);
    }

    reset_pins_to_zero();
    freenect_sync_stop();
    return 0;
}