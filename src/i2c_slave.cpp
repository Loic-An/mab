#include "i2c_slave.hpp"

uint8_t I2C_slave::dev_ctn = 0;
bool I2C_slave::dev_initialized = false;
int I2C_slave::fd = -1;

I2C_slave::I2C_slave(uint8_t addr) : addr(addr)
{
    if (!dev_initialized)
    {
        if (!bus_open())
            exit(errno); // on se permet de quitter directement car c'est une erreur fatale
        dev_initialized = true;
    };
    dev_ctn++;
};

I2C_slave::~I2C_slave()
{
    dev_ctn--;
    if (dev_ctn < 1)
    {
        if (!bus_close())
            exit(errno);
        dev_initialized = false;
    }
}

bool I2C_slave::bus_open()
{
    errno = 0;
    fd = open(I2C_DEVICE, O_RDWR);
    if (errno)
    {
        // Erreur quelconque à l'ouverture du bus
        perror("Error opening I2C device");
        return false;
    }
    printf("I2C initialized on %s\n", I2C_DEVICE);
    return true;
}

bool I2C_slave::bus_close()
{
    errno = 0;
    if (dev_initialized)
    {
        if (close(I2C_slave::fd) == 0)
        {
            printf("I2C bus closed\n");
            dev_initialized = false;
            fd = -1;
            return true;
        }
        perror("cannot close i2c bus");
    }
    return false;
}

bool I2C_slave::write(uint8_t reg, uint8_t value)
{
    uint8_t buf[2];
    struct i2c_msg msg;
    struct i2c_rdwr_ioctl_data ioctl_data;

    // envoie dans un premier temps de l'addresse à laquelle on va modifier des données (premier octet)
    // envoie dans un second temps de la nouvelle valeur de ce registre (deuxième octet)
    buf[0] = reg;
    buf[1] = value;
    msg.addr = this->addr;
    msg.flags = 0;
    msg.len = 2;
    msg.buf = buf;

    ioctl_data.msgs = &msg;
    ioctl_data.nmsgs = 1;

    errno = 0;
    if (ioctl(fd, I2C_RDWR, &ioctl_data) < 0)
    {
        perror("Error writing register");
        return false;
    }
    return true;
}

bool I2C_slave::write(uint8_t reg, uint8_t *data, uint32_t sdata)
{
    struct i2c_msg msg;
    struct i2c_rdwr_ioctl_data ioctl_data;

    // copie des données dans un nouveau buffer de taille sdata + 1 pour mettre en en-tête l'addresse du registre
    uint8_t buf[sdata + 1];
    buf[0] = reg;
    memcpy(&buf[1], data, sdata);

    // envoie de l'addresse puis des sdata octets
    msg.addr = this->addr;
    msg.flags = 0;
    msg.len = sdata + 1;
    msg.buf = buf;

    ioctl_data.msgs = &msg;
    ioctl_data.nmsgs = 1;

    errno = 0;
    if (ioctl(fd, I2C_RDWR, &ioctl_data) < 0)
    {
        perror("Error writing multiple bytes");
        return false;
    }
    return true;
}

bool I2C_slave::write(uint8_t reg, uint16_t *data, uint32_t sdata)
{
    // Cast direct sans copie - fonctionne car uint16_t* est compatible avec uint8_t*
    // sur architectures little-endian (ARM, x86)

    // uint16_t data[2] = {0x1234, 0x5678};
    //  En mémoire:
    //  data[0] = [0x34, 0x12]  → data[0] & 0xFF = 0x34, data[0] >> 8 = 0x12
    //  data[1] = [0x78, 0x56]  → data[1] & 0xFF = 0x78, data[1] >> 8 = 0x56

    // Quand castée en uint8_t*:
    // uint8_t buf[] = {0x34, 0x12, 0x78, 0x56}
    return write(reg, reinterpret_cast<uint8_t *>(data), 2 * sdata);
}

bool I2C_slave::write(uint8_t reg, uint32_t *data, uint32_t sdata)
{
    return write(reg, reinterpret_cast<uint8_t *>(data), 4 * sdata);
}

bool I2C_slave::read(uint8_t reg, uint8_t *value)
{
    uint8_t buf = reg;
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data ioctl_data;

    // Premier message: envoie de l'addresse du registre qu'on veut lire
    msgs[0].addr = this->addr;
    msgs[0].flags = 0;
    msgs[0].len = 1;
    msgs[0].buf = &buf;

    // Second message (en provenance de l'esclave !): lecture du registre
    msgs[1].addr = this->addr;
    msgs[1].flags = I2C_M_RD; // drapeau indiquant que le message ne provient pas du programme
    msgs[1].len = 1;
    msgs[1].buf = value;

    ioctl_data.msgs = msgs;
    ioctl_data.nmsgs = 2;

    errno = 0;
    if (ioctl(fd, I2C_RDWR, &ioctl_data) < 0)
    {
        perror("Error reading register");
        return false;
    }
    return true;
}

bool I2C_slave::read(uint8_t reg, uint8_t *data, uint32_t sdata)
{
    uint8_t buf = reg;
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data ioctl_data;

    // Premier message: envoie de l'addresse du registre qu'on veut lire
    msgs[0].addr = this->addr;
    msgs[0].flags = 0;
    msgs[0].len = 1;
    msgs[0].buf = &buf;

    // Deuxième message : lecture des données (sdata octets)
    msgs[1].addr = this->addr;
    msgs[1].flags = I2C_M_RD; // indique que le message provient d'un esclave
    msgs[1].len = sdata;
    msgs[1].buf = data;

    ioctl_data.msgs = msgs;
    ioctl_data.nmsgs = 2;

    errno = 0;
    if (ioctl(fd, I2C_RDWR, &ioctl_data) < 0)
    {
        perror("Error reading multiple bytes");
        return false;
    }
    return true;
}

bool I2C_slave::read(uint8_t reg, uint16_t *data, uint32_t sdata)
{
    return read(reg, reinterpret_cast<uint8_t *>(data), 2 * sdata);
}

bool I2C_slave::read(uint8_t reg, uint32_t *data, uint32_t sdata)
{
    return read(reg, reinterpret_cast<uint8_t *>(data), 4 * sdata);
}
