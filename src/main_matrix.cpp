#include <stdio.h>
#include <libfreenect/libfreenect_sync.h>
#include <unistd.h>
#define step 1

static int main_matrix() {
    uint16_t *depth_buffer = NULL;
    uint32_t timestamp;

    printf("\e[1;1H\e[2J"); // Efface l'écran du terminal

    while (1) {
        int ret = freenect_sync_get_depth((void**)&depth_buffer, &timestamp, 0, FREENECT_DEPTH_11BIT);
        if (ret != 0) break;

        printf("\e[H"); // Ramène le curseur en haut à gauche sans effacer (évite le scintillement)
        
        // On parcourt l'image avec un pas de step 
        // et un pas de step en hauteur 
        for (int y = 0; y < 480; y += step){
            for (int x = 0; x < 640; x += step){
                uint16_t d = depth_buffer[y * 640 + x];
                
                if (d >= 2047) printf("  . "); // Trop loin / Erreur
                else if (d < 800) printf(" ###"); // Très proche
                else if (d < 1500) printf(" ooo"); // Milieu
                else printf(" ...");              // Loin
            }
            printf("\n");
        }
        usleep(100000); // ~20 FPS
    }
    return 0;
}

static int main_matrix_color() {
    uint16_t *depth_buffer = NULL;
    uint8_t  *video_buffer = NULL;
    uint32_t ts_d, ts_v;

    printf("\e[1;1H\e[2J"); // Efface l'écran

    while (1) {
        // 1. Récupérer la Profondeur
        freenect_sync_get_depth((void**)&depth_buffer, &ts_d, 0, FREENECT_DEPTH_11BIT);
        // 2. Récupérer l'image RGB
        freenect_sync_get_video((void**)&video_buffer, &ts_v, 0, FREENECT_VIDEO_RGB);

        printf("\e[H"); 
        
        for (int y = 0; y < 480; y += step) {
            for (int x = 0; x < 640; x += step) {
                int i = y * 640 + x;
                
                // Extraire la couleur RGB du pixel correspondant
                uint8_t r = video_buffer[i * 3];
                uint8_t g = video_buffer[i * 3 + 1];
                uint8_t b = video_buffer[i * 3 + 2];

                uint16_t d = depth_buffer[i];

                // Code ANSI TrueColor : \x1b[38;2;R;G;Bm
                printf("\x1b[38;2;%d;%d;%dm", r, g, b);

                // On garde votre logique de caractères pour la profondeur
                if (d >= 2047) printf(" . "); 
                else if (d < 800) printf("###"); 
                else if (d < 1500) printf("ooo"); 
                else printf("...");
            }
            printf("\x1b[0m\n"); // Reset couleur à chaque ligne
        }
        usleep(150000); 
    }
    return 0;
}