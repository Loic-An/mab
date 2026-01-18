#pragma once

#include "i2c_slave.hpp"

class PCA9685 : public I2C_slave
{
public:
    // Constructor
    PCA9685(uint8_t address = 0x40);

    // Destructor
    ~PCA9685() = default;

    // Initialize the PCA9685
    bool init();

    // Set PWM frequency in Hz
    bool set_frequency(uint16_t frequency);

    // Set on/off cycle for a specific channel (0-15)
    bool set_time(uint8_t channel, uint16_t on_time, uint16_t off_time);

    // Set on/off cycle for all channels
    bool set_time_burst(uint16_t *on_time, uint16_t *off_time);

    // Set PWM duty cycle as percentage (0-MAX_PWM)
    bool set_pwm(uint8_t channel, uint16_t duty);

    // Reset the device
    bool reset();

private:
    // config bits
    static constexpr uint8_t SLEEP = 0b00010000;
    static constexpr uint8_t RESTART = 0b10000000;

    // PCA9685 register addresses
    static constexpr uint8_t MODE1 = 0x00;
    static constexpr uint8_t MODE2 = 0x01;

    static constexpr uint8_t LED0_ON_L = 0x06;
    static constexpr uint8_t LED0_ON_H = 0x07;
    static constexpr uint8_t LED0_OFF_L = 0x08;
    static constexpr uint8_t LED0_OFF_H = 0x09;
    static constexpr uint8_t PRE_SCALE = 0xFE;

    static constexpr uint16_t MAX_PWM = 4095;
};