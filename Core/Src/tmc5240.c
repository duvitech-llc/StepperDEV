#include "tmc5240.h"

/* -------------------------------------------------------------------------
 *  Helper: pack/unpack 32‑bit values to/from byte arrays (big‑endian as in
 *  the datasheet)
 * ------------------------------------------------------------------------- */
static void pack_u32_be(uint32_t v, uint8_t *buf)
{
    buf[0] = (uint8_t)(v >> 24);
    buf[1] = (uint8_t)(v >> 16);
    buf[2] = (uint8_t)(v >>  8);
    buf[3] = (uint8_t)(v);
}

static uint32_t unpack_u32_be(const uint8_t *buf)
{
    return ((uint32_t)buf[0] << 24) |
           ((uint32_t)buf[1] << 16) |
           ((uint32_t)buf[2] <<  8) |
            (uint32_t)buf[3];
}

/* -------------------------------------------------------------------------
 *  Public API
 * ------------------------------------------------------------------------- */
int8_t tmc5240_init(void)
{
    /* According to the datasheet all registers are reset to 0 on power‑up
     * (unless noted otherwise) [1].  Here we simply verify communication
     * by reading a known register (e.g. GSTAT) and checking that the device
     * responds. */
    uint8_t dummy[4];
    int8_t ret = tmc5240_read_reg(TMC5240_REG_GSTAT, dummy, 4);
    return ret;   /* 0 = success, <0 = error */
}

/* Write an arbitrary number of bytes to a register */
int8_t tmc5240_write_reg(tmc5240_reg_t reg, const uint8_t *buf, size_t len)
{
    if (len == 0 || buf == NULL) return -1;
    uint8_t addr = TMC5240_REG_ADDR_WRITE(reg);   /* add 0x80 for write [1] */
    return platform_spi_write(addr, buf, len);
}

/* Read an arbitrary number of bytes from a register */
int8_t tmc5240_read_reg(tmc5240_reg_t reg, uint8_t *buf, size_t len)
{
    if (len == 0 || buf == NULL) return -1;
    uint8_t addr = TMC5240_REG_ADDR_READ(reg);
    return platform_spi_read(addr, buf, len);
}

/* -------------------------------------------------------------------------
 *  Convenience wrappers for 32‑bit registers
 * ------------------------------------------------------------------------- */
int8_t tmc5240_set_gconf(uint32_t value)
{
    uint8_t data[4];
    pack_u32_be(value, data);
    return tmc5240_write_reg(TMC5240_REG_GCONF, data, 4);
}

int8_t tmc5240_get_gconf(uint32_t *value)
{
    uint8_t data[4];
    int8_t ret = tmc5240_read_reg(TMC5240_REG_GCONF, data, 4);
    if (ret == 0 && value)
        *value = unpack_u32_be(data);
    return ret;
}

int8_t tmc5240_get_gstat(uint32_t *value)
{
    uint8_t data[4];
    int8_t ret = tmc5240_read_reg(TMC5240_REG_GSTAT, data, 4);
    if (ret == 0 && value)
        *value = unpack_u32_be(data);
    return ret;
}
