#pragma once

#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <sys/ioctl.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <cerrno>

// spécifique au raspberry pi
#define I2C_DEVICE "/dev/i2c-1"

class I2C_slave
{
private:
    // variable faisant référence au descripteur de fichier du bus i2c, mis par défaut à -1 pour signalé que le bus n'est pas pret
    // c'est grace a cette variable que le programme peut, au travers d'appels systèmes, communiquer sur le bus i2c
    static int fd;

    // booléen indiquant si le programme peut communiquer sur le bus i2c
    static bool dev_initialized;
    // Compteur d'instance
    static uint8_t dev_ctn;
    // addresse i2c d'une instance
    uint8_t addr;
    // status of last I2C transmissions
    uint8_t last_status;

public:
    /* */
    I2C_slave(uint8_t addr);
    ~I2C_slave();

    /**
     * Initialise le bus I2C
     * @return true si la méthode a ouvert le bus (file descriptor valide), ou false quand il y a une erreur avec errno indiquant l'erreur
     */
    static bool bus_open();

    /**
     * Ferme le bus I2C
     * @return true si le bus est fermé, false sinon avec errno indique le status
     */
    static bool bus_close();

    /**
     * Modifie un registre a l'addresse reg par la valeur value
     * @return true si ACK, false si la moindre erreur avec errno modifié
     */
    bool write(uint8_t reg, uint8_t value);
    bool write(uint8_t reg, uint16_t value);
    bool write(uint8_t reg, uint32_t value);

    /**
     * Write multiple bytes to a register
     */
    bool write(uint8_t reg, uint8_t *data, uint32_t sdata);
    bool write(uint8_t reg, uint16_t *data, uint32_t sdata);
    bool write(uint8_t reg, uint32_t *data, uint32_t sdata);

    /**
     * Lit la valeur (sur un octet) d'un registre à l'addresse reg.
     * @attention la methode ne peut pas determiner la taille réelle de la valeur du registre (à determiner dans les datasheet). Si la taille ne correspond pas, comportement inattendu.
     * @return true si ACK (donc value a une valeur correcte), false si la moindre erreur avec errno modifié
     */
    bool read(uint8_t reg, uint8_t *value);
    bool read(uint8_t reg, uint16_t *value);
    bool read(uint8_t reg, uint32_t *value);

    /**
     * Read multiple bytes from a register
     */
    bool read(uint8_t reg, uint8_t *data, uint32_t sdata);
    bool read(uint8_t reg, uint16_t *data, uint32_t sdata);
    bool read(uint8_t reg, uint32_t *data, uint32_t sdata);
};
