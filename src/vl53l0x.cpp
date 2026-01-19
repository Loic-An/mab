#include "vl53l0x.hpp"
#include <unistd.h>
#include <cstdio>

VL53L0X::VL53L0X(uint8_t address) : I2C_slave(address)
{
}

/**
 * Initialise le capteur VL53L0X
 * Vérifie que le MODEL_ID correspond à la valeur attendue (0xEE)
 * @return true si le capteur répond correctement, false sinon
 */
bool VL53L0X::init()
{
    uint8_t model_id;

    // Lit et vérifie le MODEL_ID
    if (!read(REG_MODEL_ID, &model_id))
    {
        fprintf(stderr, "Erreur lecture MODEL_ID du VL53L0X\n");
        return false;
    }

    if (model_id != EXPECTED_MODEL_ID)
    {
        fprintf(stderr, "MODEL_ID inattendu: 0x%02X (attendu: 0x%02X)\n", 
                model_id, EXPECTED_MODEL_ID);
        return false;
    }

    printf("VL53L0X trouvé (MODEL_ID: 0x%02X). Début initialisation...\n", model_id);

    // Etapes d'initialisation
    if (!configure_settings()) {
        fprintf(stderr, "Erreur: configure_settings() a échoué\n");
        return false;
    }

    if (!check_temperature()) {
        fprintf(stderr, "Erreur: check_temperature() a échoué\n");
        return false;
    }

    if (!calibrate_spad()) {
        fprintf(stderr, "Erreur: calibrate_spad() a échoué\n");
        return false;
    }

    if (!perform_ref_calibration()) {
        fprintf(stderr, "Erreur: perform_ref_calibration() a échoué\n");
        return false;
    }

    if (!set_timing()) {
        fprintf(stderr, "Erreur: set_timing() a échoué\n");
        return false;
    }

    printf("VL53L0X initialisation complète\n");
    return true;
}

// Applique des réglages de base (arrêt des mesures, valeurs par défaut, ...)
bool VL53L0X::configure_settings()
{
    // Assure que le capteur n'est pas en mesure en cours
    if (!write(REG_SYSRANGE_START, 0x00)) {
        fprintf(stderr, "configure_settings: impossible d'arreter SYSRANGE_START\n");
        return false;
    }
    usleep(1000);

    // Ici on devrait écrire les registres de configuration recommandés
    // (timing, interleaving, filtres, etc.). Pour l'instant on vérifie
    // que le bus fonctionne et on laisse des points d'extension.
    printf("configure_settings: paramètres de base appliqués (placeholder)\n");
    return true;
}

// Vérifie que le capteur rapporte une réponse et permet de traiter la température
bool VL53L0X::check_temperature()
{
    // Le VL53L0X a des registres internes pour la température, mais leur
    // lecture/format dépend de l'implémentation; ici on vérifie simplement
    // la réactivité du capteur en lisant un registre connu.
    uint8_t status;
    if (!read(REG_RESULT_RANGE_STATUS, &status)) {
        fprintf(stderr, "check_temperature: lecture statut échouée\n");
        return false;
    }

    // Placeholder: normalement on corrigerait calibration selon température
    printf("check_temperature: statut lu 0x%02X (placeholder)\n", status);
    return true;
}

// Lance la calibration SPAD (Single Photon Avalanche Diodes)
bool VL53L0X::calibrate_spad()
{
    // La procédure complète de calibration SPAD nécessite une série
    // d'écritures/lectures spécifiques. Ici on pose une base et un point
    // d'extension pour l'implémentation détaillée.
    printf("calibrate_spad: calibration SPAD (placeholder)\n");
    usleep(2000);
    return true;
}

// Référence et calibrations (VHV, phase calibration, etc.)
bool VL53L0X::perform_ref_calibration()
{
    // Sequencement générique de calibrations de référence.
    printf("perform_ref_calibration: calibrations de référence (placeholder)\n");
    usleep(2000);
    return true;
}

// Configure le timing des mesures (timing budget, inter-measurement)
bool VL53L0X::set_timing()
{
    // Exemple: on pourrait régler le timing budget ici via les registres
    // dédiés. Pour l'instant on confirme la présence et ajoute un délai.
    printf("set_timing: configuration du timing (placeholder)\n");
    usleep(1000);
    return true;
}

/**
 * Démarre une mesure de distance
 * Écrit 0x01 dans le registre SYSRANGE_START
 * @return true si succès, false sinon
 */
bool VL53L0X::start_measurement()
{
    if (!write(REG_SYSRANGE_START, 0x01))
    {
        fprintf(stderr, "Erreur démarrage mesure VL53L0X\n");
        return false;
    }

    return true;
}

/**
 * Attend que la mesure soit terminée
 * Poll le registre de statut jusqu'à ce que le bit 0 soit à 0
 * @param timeout_ms Timeout en millisecondes (1ms par loop)
 * @return true si mesure terminée, false si timeout ou erreur
 */
bool VL53L0X::wait_for_completion(int timeout_ms)
{
    int max_loops = timeout_ms;
    int loop_count = 0;
    uint8_t range_status;

    while (loop_count < max_loops)
    {
        if (!read(REG_RESULT_RANGE_STATUS, &range_status))
        {
            fprintf(stderr, "Erreur lecture statut VL53L0X\n");
            return false;
        }

        // Vérifie si la mesure est prête (bit 0 à 0)
        if ((range_status & 0x01) == 0)
        {
            // Mesure terminée
            printf("Completed in %d loops (~%dms)\n", loop_count, loop_count);
            // Lecture de la distance
            uint16_t distance_mm;
            if (!read_distance(&distance_mm)) {
                printf("Erreur lecture distance après mesure\n");
                return false;
            }
            printf("Distance mesurée: %u mm\n", distance_mm);
            return true;
        }

        loop_count++;
        usleep(1000); // 1ms de délai entre chaque poll
    }

    fprintf(stderr, "Timeout attente mesure VL53L0X (%d ms)\n", timeout_ms);
    return false;
}

/**
 * Lit la distance mesurée
 * Lit 2 octets depuis le registre RESULT_RANGE_VALUE
 * @param distance_mm Pointeur vers la variable recevant la distance en mm
 * @return true si succès, false sinon
 */
bool VL53L0X::read_distance(uint16_t *distance_mm)
{
    if (!read(REG_RESULT_RANGE_VALUE, distance_mm, 2))
    {
        fprintf(stderr, "Erreur lecture distance VL53L0X\n");
        return false;
    }
    return true;
}

/**
 * Effectue une mesure complète
 * Enchaîne : start_measurement() -> wait_for_completion() -> read_distance()
 * @param distance_mm Pointeur vers la variable recevant la distance en mm
 * @param timeout_ms Timeout en millisecondes pour l'attente
 * @return true si succès, false sinon
 */
bool VL53L0X::measure(uint16_t *distance_mm, int timeout_ms)
{
    if (!start_measurement())
        return false;

    if (!wait_for_completion(timeout_ms))
        return false;

    if (!read_distance(distance_mm))
        return false;

    return true;
}

/**
 * Reset logiciel du capteur
 * Écrit 0x00 dans le registre SYSRANGE_START et attend la stabilisation
 * @return true si succès, false sinon
 */
bool VL53L0X::reset()
{
    if (!write(REG_SYSRANGE_START, 0x00))
    {
        fprintf(stderr, "Erreur reset VL53L0X\n");
        return false;
    }

    // Attend que l'oscillateur se stabilise
    usleep(500);

    printf("VL53L0X réinitialisé\n");
    return true;
}
