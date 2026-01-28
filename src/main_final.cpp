#include <libfreenect_sync.h>
#include <stdio.h>
#include <unistd.h>
#include <algorithm>
#include <vector>
#include <cmath>
#include "pca9685.hpp"

#define COLS 2
#define ROWS 2
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
const int VMOY = 2500;

const float DIST_SOL = 900.0f;
const float DIST_OBJ_MAX = 500.0f;

struct MotorState
{
    float current_pos = 0;
    float target_pos = 0;
    float avg_depth_mm = 0; // Stocke la distance moyenne vue par la Kinect pour cette zone
};

static MotorState moteurs[TOTAL_MOTORS];
static PCA9685 pca;

static void render_ui()
{
    printf("\e[H");
    printf("===== SHAPE DISPLAY SYSTEM =====\n");
    printf("Config: %dx%d | Sol: %.0fmm | Seuil Max: %.0fmm\n", COLS, ROWS, DIST_SOL, DIST_OBJ_MAX);
    printf("------------------------------------------------------------\n");

    for (int i = 0; i < TOTAL_MOTORS; i++)
    {
        // Affichage : Index, Distance Kinect (mm), Position actuelle -> Cible (mm)
        printf("M%d | Kinect: %4.0fmm | Pos: %4.1f -> %4.1fmm ",
               i, moteurs[i].avg_depth_mm, moteurs[i].current_pos, moteurs[i].target_pos);

        int bars = (int)(moteurs[i].current_pos / (COURSE_MAX / 15.0f));
        printf("|");
        for (int b = 0; b < 15; b++)
            printf(b < bars ? "#" : " ");
        printf("|\n");
    }
}

static void show_matrix_viewport(uint16_t *depth_buffer, unsigned char *color_buffer)
{
    printf("\n--- VUE KINECT (Distances en cm) ---\n");
    int stepX = K_WIDTH / 40;
    int stepY = K_HEIGHT / 20;

    for (int y = 0; y < K_HEIGHT; y += stepY)
    {
        for (int x = 0; x < K_WIDTH; x += stepX)
        {
            uint16_t d = depth_buffer[y * K_WIDTH + x];
            // if (color_buffer)
            //{
            //     int pix = (y * K_WIDTH + x) * 3;
            //     unsigned char r = color_buffer[pix + 0];
            //     unsigned char g = color_buffer[pix + 1];
            //     unsigned char b = color_buffer[pix + 2];
            //     // print a 4-char wide colored block using background RGB
            //     printf("\033[48;2;%u;%u;%um    \033[0m", r, g, b);
            // }
            // else
            //{
            if (d == 0)
                printf("  . ");
            else if (d > 2500)
                printf(" -- ");
            else
                printf("%3d ", d / 10);
            //}
        }
        printf("\n");
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

            // Zone d'échantillonnage de 40x40 pixels
            for (int y = centerY - 20; y < centerY + 20; y++)
            {
                for (int x = centerX - 20; x < centerX + 20; x++)
                {
                    if (x < 0 || x >= K_WIDTH || y < 0 || y >= K_HEIGHT)
                        continue;
                    uint16_t d = depth_buffer[y * K_WIDTH + x];
                    if (d > 400 && d < 2400)
                    { // Filtre les données aberrantes
                        sum_depth += d;
                        samples++;
                    }
                }
            }

            if (samples > 0)
            {
                moteurs[motor_idx].avg_depth_mm = (float)sum_depth / samples;

                // Calcul du ratio de sortie du pin
                float ratio = (DIST_SOL - moteurs[motor_idx].avg_depth_mm) / (DIST_SOL - DIST_OBJ_MAX);
                moteurs[motor_idx].target_pos = std::clamp(ratio * COURSE_MAX, 0.0f, COURSE_MAX);
            }
            else
            {
                // Si aucun pixel valide n'est trouvé, on stabilise à 0 (sol supposé)
                moteurs[motor_idx].avg_depth_mm = DIST_SOL;
                moteurs[motor_idx].target_pos = 0;
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

static int main_final()
{
    uint16_t *depth_buffer = NULL;
    unsigned char *color_buffer = NULL;
    uint32_t timestamp;

    pca = PCA9685(0x40);
    if (!pca.init())
        return 1;

    printf("\e[2J");

    while (!should_exit)
    {
        // freenect_sync_get_video((void **)&color_buffer, &timestamp, 0, FREENECT_VIDEO_RGB);
        if (freenect_sync_get_depth((void **)&depth_buffer, &timestamp, 0, FREENECT_DEPTH_MM) == 0)
        {
            process_kinect_logic(depth_buffer);
            drive_motors();

            render_ui();
            show_matrix_viewport(depth_buffer, color_buffer);
        }
        usleep(20000);
    }
    freenect_sync_stop();
    reset_pins_to_zero();
    return 0;
}