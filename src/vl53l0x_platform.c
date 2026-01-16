#include "vl53l0x_platform.h"
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#define USE_I2C_2V8

static int i2c_fd = -1;

static int i2c_set_addr(uint8_t addr)
{
    if (i2c_fd < 0)
    {
        i2c_fd = open(I2C_DEVICE, O_RDWR);
        if (i2c_fd < 0)
            return -1;
    }
    return ioctl(i2c_fd, I2C_SLAVE, addr);
}

int VL53L0X_WriteMulti(VL53L0X_DEV dev, uint8_t reg, uint8_t *data, uint32_t count)
{
    int ret = i2c_set_addr(dev->I2cDevAddr);
    if (ret < 0)
        return -1;

    /* Use I2C_RDWR to perform a single write message (reg + payload) */
    uint8_t buf[count + 1];
    struct i2c_msg msg;
    struct i2c_rdwr_ioctl_data ioctl_data;

    buf[0] = reg;
    memcpy(&buf[1], data, count);

    msg.addr = dev->I2cDevAddr;
    msg.flags = 0;
    msg.len = count + 1;
    msg.buf = buf;

    ioctl_data.msgs = &msg;
    ioctl_data.nmsgs = 1;

    if (ioctl(i2c_fd, I2C_RDWR, &ioctl_data) < 0)
    {
        return -1;
    }
    return 0;
}

int VL53L0X_ReadMulti(VL53L0X_DEV dev, uint8_t reg, uint8_t *data, uint32_t count)
{
    int ret = i2c_set_addr(dev->I2cDevAddr);
    if (ret < 0)
        return -1;

    /* Two-message transaction: write register address, then read data */
    uint8_t reg_buf = reg;
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data ioctl_data;

    msgs[0].addr = dev->I2cDevAddr;
    msgs[0].flags = 0;
    msgs[0].len = 1;
    msgs[0].buf = &reg_buf;

    msgs[1].addr = dev->I2cDevAddr;
    msgs[1].flags = I2C_M_RD;
    msgs[1].len = count;
    msgs[1].buf = data;

    ioctl_data.msgs = msgs;
    ioctl_data.nmsgs = 2;

    if (ioctl(i2c_fd, I2C_RDWR, &ioctl_data) < 0)
    {
        return -1;
    }
    return 0;
}

int VL53L0X_WrByte(VL53L0X_DEV dev, uint8_t reg, uint8_t value)
{
    return VL53L0X_WriteMulti(dev, reg, &value, 1);
}

int VL53L0X_RdByte(VL53L0X_DEV dev, uint8_t reg, uint8_t *value)
{
    return VL53L0X_ReadMulti(dev, reg, value, 1);
}

void VL53L0X_DelayMs(uint32_t ms)
{
    usleep(ms * 1000);
}

int VL53L0X_WrWord(VL53L0X_DEV dev, uint8_t reg, uint16_t value)
{
    uint8_t data[2];
    data[0] = (value >> 8) & 0xFF; /* MSB first */
    data[1] = value & 0xFF;        /* LSB */
    return VL53L0X_WriteMulti(dev, reg, data, 2);
}

int VL53L0X_RdWord(VL53L0X_DEV dev, uint8_t reg, uint16_t *value)
{
    uint8_t data[2];
    int ret = VL53L0X_ReadMulti(dev, reg, data, 2);
    if (ret == 0)
    {
        *value = ((uint16_t)data[0] << 8) | data[1];
    }
    return ret;
}

int VL53L0X_WrDWord(VL53L0X_DEV dev, uint8_t reg, uint32_t value)
{
    uint8_t data[4];
    data[0] = (value >> 24) & 0xFF; /* MSB first */
    data[1] = (value >> 16) & 0xFF;
    data[2] = (value >> 8) & 0xFF;
    data[3] = value & 0xFF; /* LSB */
    return VL53L0X_WriteMulti(dev, reg, data, 4);
}

int VL53L0X_RdDWord(VL53L0X_DEV dev, uint8_t reg, uint32_t *value)
{
    uint8_t data[4];
    int ret = VL53L0X_ReadMulti(dev, reg, data, 4);
    if (ret == 0)
    {
        *value = ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) |
                 ((uint32_t)data[2] << 8) | data[3];
    }
    return ret;
}
