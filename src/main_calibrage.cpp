#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include "pca9685.hpp"

#define VMAX 4095
#define VOFF 0

// Fonction pour lire les touches sans attendre "Entrée"
int getch()
{
    struct termios oldt, newt;
    int ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

int main_calibrage()
{
    PCA9685 pca(0x40);
    if (!pca.init())
    {
        printf("Erreur d'initialisation du PCA9685\n");
        return 1;
    }

    int motor_selected = 0;
    bool running = true;

    printf("\e[2J\e[H"); // Efface l'écran
    printf("=== CONTROLE MANUEL DES MOTEURS ===\n");
    printf("Utilisez les fleches GAUCHE/DROITE pour choisir le moteur\n");
    printf("Utilisez les fleches HAUT/BAS pour faire tourner\n");
    printf("Appuyez sur 'q' pour quitter\n\n");

    while (running)
    {
        printf("\rMoteur actif : [%d] (Canaux PCA %d & %d)          ",
               motor_selected, motor_selected * 2, motor_selected * 2 + 1);
        fflush(stdout);

        int c = getch();

        if (c == 'q')
        {
            running = false;
        }
        else if (c == 27)
        {            // Séquence d'échappement pour les flèches
            getch(); // ignore [
            switch (getch())
            {
            case 'A': // HAUT
                pca.set_pwm(motor_selected * 2, VMAX);
                pca.set_pwm(motor_selected * 2 + 1, VOFF);
                usleep(100000); // Tourne pendant 100ms
                break;
            case 'B': // BAS
                pca.set_pwm(motor_selected * 2, VOFF);
                pca.set_pwm(motor_selected * 2 + 1, VMAX);
                usleep(100000);
                break;
            case 'C': // DROITE
                motor_selected = (motor_selected + 1) % 4;
                break;
            case 'D': // GAUCHE
                motor_selected = (motor_selected - 1 + 4) % 4;
                break;
            }
            // Arrêt automatique après le mouvement
            pca.set_pwm(motor_selected * 2, 0);
            pca.set_pwm(motor_selected * 2 + 1, 0);
        }
    }

    // Sécurité : tout éteindre avant de quitter
    for (int i = 0; i < 16; i++)
        pca.set_pwm(i, 0);
    printf("\nFin du contrôle manuel.\n");
    return 0;
}