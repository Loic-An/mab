#include <stdio.h>
#include <libfreenect/libfreenect_sync.h>

#define ROWS 2  // Nombre de moteurs en hauteur
#define COLS 2  // Nombre de moteurs en largeur

static int main_rows() {
    uint16_t *depth_buffer = NULL;
    uint32_t timestamp;

    // Dimensions d'une zone (ex: pour 2x2, une zone fait 320x240 pixels)
    int zone_w = 640 / COLS;
    int zone_h = 480 / ROWS;

    while (1) {
        int ret = freenect_sync_get_depth((void**)&depth_buffer, &timestamp, 0, FREENECT_DEPTH_11BIT);
        if (ret != 0) continue;

        // Pour chaque moteur (Pin)
        for (int r = 0; r < ROWS; r++) {
            for (int c = 0; c < COLS; c++) {
                
                long long sum = 0;
                int count = 0;

                // Parcourir les pixels de la zone correspondante
                for (int y = r * zone_h; y < (r + 1) * zone_h; y++) {
                    for (int x = c * zone_w; x < (c + 1) * zone_w; x++) {
                        uint16_t d = depth_buffer[y * 640 + x];
                        if (d > 0 && d < 2047) { // Filtrer les erreurs
                            sum += d;
                            count++;
                        }
                    }
                }

                // Valeur moyenne pour ce moteur
                int avg_dist = (count > 0) ? (int)(sum / count) : 2047;

                // TODO: Envoyer avg_dist vers vos moteurs (via GPIO ou PWM)
                printf("Pin [%d,%d] : %d mm | ", r, c, avg_dist);
            }
            printf("\n");
        }
        printf("-----------\n");
    }
    return 0;
}