#include <stdio.h>
#include <libfreenect_sync.h>
#include <unistd.h>

static int main_matrix() {
    uint16_t *depth_buffer = NULL;
    uint32_t timestamp;

    printf("\e[1;1H\e[2J"); // Efface l'écran du terminal

    while (1) {
        int ret = freenect_sync_get_depth((void**)&depth_buffer, &timestamp, 0, FREENECT_DEPTH_11BIT);
        if (ret != 0) break;

        printf("\e[H"); // Ramène le curseur en haut à gauche sans effacer (évite le scintillement)
        
        // On parcourt l'image avec un pas de 32 (640/32 = 20 colonnes)
        // et un pas de 32 en hauteur (480/32 = 15 lignes)
        for (int y = 0; y < 480; y += 32) {
            for (int x = 0; x < 640; x += 32) {
                uint16_t d = depth_buffer[y * 640 + x];
                
                if (d >= 2047) printf("  . "); // Trop loin / Erreur
                else if (d < 800) printf(" ###"); // Très proche
                else if (d < 1500) printf(" ooo"); // Milieu
                else printf(" ...");              // Loin
            }
            printf("\n");
        }
        usleep(33000); // ~30 FPS
    }
    return 0;
}
