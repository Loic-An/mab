#pragma once

#include "i2c_slave.hpp"

class VL53L0X : public I2C_slave
{
public:
    // Constructeur avec adresse I2C par défaut
    VL53L0X(uint8_t address = 0x29);

    // Destructeur par défaut
    ~VL53L0X() = default;

    // Initialise le capteur (vérifie MODEL_ID)
    bool init();

    // Démarre une mesure de distance
    bool start_measurement();

    // Attend la fin de la mesure (avec timeout en ms)
    bool wait_for_completion(int timeout_ms = 33);

    // Lit la distance mesurée en millimètres
    bool read_distance(uint16_t *distance_mm);

    // Effectue une mesure complète (start + wait + read)
    bool measure(uint16_t *distance_mm, int timeout_ms = 33);

    // Reset logiciel du capteur
    bool reset();

private:
    // Adresses des registres VL53L0X
    static constexpr uint8_t REG_MODEL_ID = 0xC0;
    static constexpr uint8_t REG_SYSRANGE_START = 0x00;
    static constexpr uint8_t REG_RESULT_RANGE_STATUS = 0x14;
    static constexpr uint8_t REG_RESULT_RANGE_VALUE = 0x1E;

    // Valeur attendue du MODEL_ID
    static constexpr uint8_t EXPECTED_MODEL_ID = 0xEE;

    // Timeout maximal en nombre de loops (1ms par loop)
    static constexpr int MAX_LOOPS = 2000;
};