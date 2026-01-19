#include <stdio.h>
#include <libfreenect/libfreenect_sync.h>
#include <unistd.h>

#define ROWS 2
#define COLS 2

static int main_motors() {
    uint16_t *depth_buffer = NULL;
    uint32_t timestamp;

    // Dimensions d'une zone 
    int zone_w = 640 / COLS;
    int zone_h = 480 / ROWS;

    printf("\e[1;1H\e[2J"); // Efface l'écran une fois au début

    while (1) {
        // Récupération de la profondeur
        int ret = freenect_sync_get_depth((void**)&depth_buffer, &timestamp, 0, FREENECT_DEPTH_11BIT);
        if (ret != 0) {
            printf("Erreur de connexion Kinect...\n");
            usleep(500000);
            continue;
        }

        printf("\e[H"); // Ramène le curseur en haut à gauche
        printf("--- MATRICE DE CONTROLE MOTEURS (%d x %d) ---\n\n", ROWS, COLS);

        for (int r = 0; r < ROWS; r++) {
            for (int c = 0; c < COLS; c++) {
                
                long long sum = 0;
                int count = 0;

                // Parcourir UNIQUEMENT les pixels de la zone du moteur r,c
                for (int y = r * zone_h; y < (r + 1) * zone_h; y++) {
                    for (int x = c * zone_w; x < (c + 1) * zone_w; x++) {
                        uint16_t d = depth_buffer[y * 640 + x];
                        
                        // On ignore 0 et 2047 (pixels morts / trop proches)
                        if (d > 0 && d < 2047) { 
                            sum += d;
                            count++;
                        }
                    }
                }

                // Calcul de la moyenne de la zone
                int avg_dist = (count > 0) ? (int)(sum / count) : 2047;

                // Affichage formaté pour le débug
                // On affiche la valeur brute moyenne
                printf("Moteur [%d,%d] : %4d | ", r, c, avg_dist);
            }
            printf("\n\n");
        }
        
        printf("------------------------------------------\n");
        usleep(50000); // 20 FPS
    }
    return 0;
}