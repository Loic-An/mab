#ifndef VL53L0X_PLATFORM_H_
#define VL53L0X_PLATFORM_H_

#include <stdint.h>
#include "vl53l0x_types.h"

// I2C device file path on the Raspberry Pi running Raspbian
#define I2C_DEVICE "/dev/i2c-1"
#define USE_I2C_2V8

/* Forward declaration - actual definition is in vl53l0x_def.h */
struct VL53L0X_DevData_t;

typedef struct
{
    uint8_t I2cDevAddr;
    struct VL53L0X_DevData_t *Data;
} VL53L0X_Dev_t;

typedef VL53L0X_Dev_t *VL53L0X_DEV;

/* PAL Device Data Access macros */
#define PALDevDataGet(Dev, field) (Dev->Data->field)
#define PALDevDataSet(Dev, field, value) (Dev->Data->field = value)

int VL53L0X_WriteMulti(VL53L0X_DEV dev, uint8_t reg, uint8_t *data, uint32_t count);
int VL53L0X_ReadMulti(VL53L0X_DEV dev, uint8_t reg, uint8_t *data, uint32_t count);

int VL53L0X_WrByte(VL53L0X_DEV dev, uint8_t reg, uint8_t value);
int VL53L0X_RdByte(VL53L0X_DEV dev, uint8_t reg, uint8_t *value);

int VL53L0X_WrWord(VL53L0X_DEV dev, uint8_t reg, uint16_t value);
int VL53L0X_RdWord(VL53L0X_DEV dev, uint8_t reg, uint16_t *value);

int VL53L0X_WrDWord(VL53L0X_DEV dev, uint8_t reg, uint32_t value);
int VL53L0X_RdDWord(VL53L0X_DEV dev, uint8_t reg, uint32_t *value);

void VL53L0X_DelayMs(uint32_t ms);

#endif
