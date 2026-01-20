#include "pca9685.hpp"
#include <unistd.h>
#include <cmath>

PCA9685::PCA9685(uint8_t address) : I2C_slave(address)
{
}

/**
 * Initialise le PCA9685
 * - Reset du périphérique
 * - Configure MODE1 : auto-increment, mode normal
 * - Configure MODE2 : sorties totem pole
 * @return true si succès, false sinon
 */
bool PCA9685::init()
{
    // Reset de tous les registres aux valeurs par défaut
    if (!reset())
        return false;

    // Configure MODE1 : AI=1 (auto-increment), SLEEP=0 (mode normal)
    if (!write(MODE1, (uint8_t)0x20)) // Bit AI = bit 5
        return false;

    // Configure MODE2 : OUTDRV=1 (totem pole, pas open-drain)
    if (!write(MODE2, (uint8_t)0x04)) // Bit OUTDRV = bit 2
        return false;

    // Fréquence par défaut à 50Hz (courant pour les servos)
    return set_frequency(50);
}

/**
 * Définit la fréquence PWM pour tous les canaux
 * Plage de fréquence : 24-1526 Hz (pratique : 40-1000 Hz)
 * Formule : prescale = round(25MHz / (4096 * fréquence)) - 1
 * @param frequency Fréquence désirée en Hz
 * @return true si succès, false sinon
 */
bool PCA9685::set_frequency(uint16_t frequency)
{
    if (frequency < 24 || frequency > 1526)
        return false;

    // Calcule la valeur du prescaler
    // Oscillateur interne = 25 MHz
    float prescale_val = 25000000.0f / (4096.0f * frequency) - 1.0f;
    uint8_t prescale = (uint8_t)round(prescale_val);

    // Lit le MODE1 actuel
    uint8_t old_mode;
    if (!read(MODE1, &old_mode))
        return false;

    // Entre en mode sommeil pour changer le prescaler
    uint8_t sleep_mode = (old_mode & 0x7F) | SLEEP; // Active le bit SLEEP
    if (!write(MODE1, sleep_mode))
        return false;

    // Écrit le prescaler
    if (!write(PRE_SCALE, prescale))
        return false;

    // Restaure l'ancien mode (réveil)
    if (!write(MODE1, old_mode))
        return false;

    // Attend 500µs pour que l'oscillateur se stabilise
    usleep(500);

    // Redémarre avec auto-increment activé
    if (!write(MODE1, (uint8_t)(old_mode | 0xA0))) // Active les bits RESTART + AI
        return false;

    return true;
}

/**
 * Définit les temps on/off du PWM pour un canal spécifique
 * @param channel Numéro du canal (0-15)
 * @param on_time Moment d'activation (0-4095)
 * @param off_time Moment de désactivation (0-4095)
 * @return true si succès, false sinon
 * Exemple 1: ON=0,    OFF=2000  → ████████░░░░░░░░
 * Exemple 2: ON=1000, OFF=3000  → ░░░░████████░░░░
 * Exemple 3: ON=2000, OFF=4000  → ░░░░░░░░████████
 */
bool PCA9685::set_time(uint8_t channel, uint16_t on_time, uint16_t off_time)
{
    // vérifie si le canal est valide
    if (channel > 15)
        return false;

    // vérifie si les deux temps ne sont pas inversées
    if (on_time > off_time)
    {
        on_time = on_time ^ off_time;
        off_time = on_time ^ off_time;
        on_time = on_time ^ off_time;
    }

    // Vérifie si le on_time est valide (si la valeur est sur 12bits)
    if (on_time > MAX_PWM)
        on_time = MAX_PWM; // on considère la valeur max s

    // Vérifie si le off_time est valide
    if (off_time > MAX_PWM)
        off_time = MAX_PWM;

    // Calcule les adresses des registres pour ce canal
    uint8_t base = LED0_ON_L + (4 * channel);

    // compactage dans un buffer pour écrire en mode auto-incrémentage
    // uint8_t valeurs[4] = {on_time & 0xFF, on_time >> 8, off_time & 0xFF, off_time >> 8};
    // return write(base, valeurs, 4);
    uint16_t valeurs[2] = {on_time, off_time};
    return write(base, valeurs, 2);
}

bool PCA9685::set_time_burst(uint16_t *on_time, uint16_t *off_time)
{
    // TODO: utiliser le mode burst avec auto-increment (le seul interet mdr)
    for (uint8_t channel = 0; channel < 16; channel++)
    {
        if (!set_time(channel, on_time[channel], off_time[channel]))
            return false;
    }
    return true;
}

/**
 * Définit le rapport cyclique du PWM comme une valeur (0-4095)
 * @param channel Numéro du canal (0-15)
 * @param duty Valeur du rapport cyclique (0 = 0%, 4095 = 100%)
 * @attention on_time = 0, a eviter si plusieurs charges inductives, qui entrainent des pics de courant
 * @return true si succès, false sinon
 */
bool PCA9685::set_pwm(uint8_t channel, uint16_t duty)
{
    if (duty > MAX_PWM)
        duty = MAX_PWM;

    // Complètement éteint
    if (duty == 0)
        return set_time(channel, 0, 0);

    // Complètement allumé
    if (duty == MAX_PWM)
        return set_time(channel, MAX_PWM, 0);
    // PWM normal : commence à 0, se termine à la valeur duty
    return set_time(channel, 0, duty);
}

/**
 * Réinitialise le PCA9685 à son état par défaut (soft reset via RESTART bit)
 * Procédure selon datasheet PCA9685:
 * 1. Mettre le chip en SLEEP (bit 4)
 * 2. Attendre que le RESTART bit (bit 7) passe à 1
 * 3. Désactiver SLEEP
 * 4. Attendre 500µs pour stabilisation de l'oscillateur
 * 5. Activer RESTART pour redémarrer tous les canaux PWM
 * @return true si succès, false sinon
 */
bool PCA9685::reset()
{
    uint8_t mode = 0;

    // Étape 1: Lire le MODE1 actuel
    if (!read(MODE1, &mode))
        return false;

    // Étape 2: Mettre le chip en SLEEP (bit 4)
    if (!write(MODE1, (uint8_t)(mode | SLEEP)))
        return false;

    // Attendre que le RESTART bit se mette à 1 automatiquement
    // (généralement ~1ms après le SLEEP)
    usleep(1000);

    // Vérifier que le bit RESTART est bien passé à 1
    if (!read(MODE1, &mode))
        return false;

    if ((mode & RESTART) == 0)
    {
        fprintf(stderr, "Erreur reset PCA9685: RESTART bit not set\n");
        return false;
    }

    // Étape 3: Désactiver SLEEP (bit 4 = 0)
    // Ceci garde le RESTART bit à 1
    if (!write(MODE1, (uint8_t)(mode & ~SLEEP)))
        return false;

    // Étape 4: Attendre 500µs minimum pour que l'oscillateur se stabilise
    usleep(500);

    // Étape 5: Écrire 1 dans le bit RESTART pour redémarrer tous les canaux PWM
    // Le bit RESTART se remet à 0 automatiquement après écriture
    if (!write(MODE1, (uint8_t)(mode | RESTART)))
        return false;

    return true;
}
