#pragma once
#include "vl53l0x_types.hpp"
#include "i2c_slave.hpp"

// Default I2C address for VL53L0X
#define ADDRESS_DEFAULT 0b0101001

/*
    VL53L0X class
    Provides a high-level interface to the VL53L0X time-of-flight distance sensor
*/
class VL53L0X : public I2C_slave
{
public:
    // Constructeur avec adresse I2C
    VL53L0X(uint8_t address = ADDRESS_DEFAULT);

    // Destructeur par défaut
    ~VL53L0X() = default;

    // Initialise le capteur (vérifie MODEL_ID)
    bool init(bool io_2v8 = true);

    // Reset logiciel du capteur
    bool reset();

    bool setSignalRateLimit(float limit_Mcps);
    float getSignalRateLimit();

    bool setMeasurementTimingBudget(uint32_t budget_us);
    uint32_t getMeasurementTimingBudget();

    bool setVcselPulsePeriod(vcselPeriodType type, uint8_t period_pclks);
    uint8_t getVcselPulsePeriod(vcselPeriodType type);

    void startContinuous(uint32_t period_ms = 0);
    void stopContinuous();
    uint16_t readRangeContinuousMillimeters();
    uint16_t readRangeSingleMillimeters();

    inline void setTimeout(uint16_t timeout) { io_timeout = timeout; }
    inline uint16_t getTimeout() { return io_timeout; }
    bool timeoutOccurred();

    void writeReg(uint8_t reg, uint8_t value);
    void writeReg16Bit(uint8_t reg, uint16_t value);
    void writeReg32Bit(uint8_t reg, uint32_t value);
    uint8_t readReg(uint8_t reg);
    uint16_t readReg16Bit(uint8_t reg);
    uint32_t readReg32Bit(uint8_t reg);

    void writeMulti(uint8_t reg, uint8_t const *src, uint8_t count);
    void readMulti(uint8_t reg, uint8_t *dst, uint8_t count);

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

    // Helpers d'initialisation (implémentés dans le cpp)
    bool configure_settings();
    bool check_temperature();
    bool calibrate_spad();
    bool perform_ref_calibration();
    bool set_timing();

    uint16_t io_timeout;
    bool did_timeout;
    uint16_t timeout_start_ms;

    uint8_t stop_variable; // read by init and used when starting measurement; is StopVariable field of VL53L0X_DevData_t structure in API
    uint32_t measurement_timing_budget_us;

    bool getSpadInfo(uint8_t *count, bool *type_is_aperture);

    void getSequenceStepEnables(SequenceStepEnables *enables);
    void getSequenceStepTimeouts(SequenceStepEnables const *enables, SequenceStepTimeouts *timeouts);

    bool performSingleRefCalibration(uint8_t vhv_init_byte);

    static uint16_t decodeTimeout(uint16_t value);
    static uint16_t encodeTimeout(uint32_t timeout_mclks);
    static uint32_t timeoutMclksToMicroseconds(uint16_t timeout_period_mclks, uint8_t vcsel_period_pclks);
    static uint32_t timeoutMicrosecondsToMclks(uint32_t timeout_period_us, uint8_t vcsel_period_pclks);
    // Calcule l'offset moyen en fonction d'une distance réelle connue
    // target_dist_mm : la distance réelle où tu as placé ta cible (ex: 100mm)
    int32_t calibrateOffset(uint16_t target_dist_mm);

    // Applique un offset manuellement (en mm)
    void setOffset(int16_t offset_mm);

};

