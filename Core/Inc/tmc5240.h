#ifndef TMC5240_H_
#define TMC5240_H_

#include <stdint.h>
#include <stddef.h>

/* -------------------------------------------------------------------------
 *  Register map (excerpt – full map is in the datasheet)
 *  The TMC5240 uses 8‑bit register addresses.  For a write access the
 *  address must be OR‑ed with 0x80 (add 0x80) [1].
 * ------------------------------------------------------------------------- */
typedef enum {
    TMC5240_REG_GCONF      = 0x00,   /* General configuration          */
    TMC5240_REG_GSTAT      = 0x01,   /* Global status flags            */
    TMC5240_REG_IFCNT      = 0x02,   /* Interface counter              */
    TMC5240_REG_NODECONF   = 0x03,   /* Node configuration (UART)      */
    TMC5240_REG_IOIN       = 0x04,   /* Input/Output pins status       */
    /* … add other registers as required … */
} tmc5240_reg_t;

/* -------------------------------------------------------------------------
 *  Platform abstraction – the user must provide these functions in a
 *  separate file (e.g. platform_stm32.c) that implements the low‑level
 *  SPI/UART peripheral handling for the STM32F4.
 * ------------------------------------------------------------------------- */
extern int8_t tmc5240_platform_spi_write(uint8_t address, const uint8_t *data,
                                         size_t length);
extern int8_t tmc5240_platform_spi_read (uint8_t address, uint8_t *data,
                                         size_t length);

/* -------------------------------------------------------------------------
 *  High‑level driver API
 * ------------------------------------------------------------------------- */
int8_t  tmc5240_init(void);                                   /* reset & verify */
int8_t  tmc5240_write_reg(tmc5240_reg_t reg, const uint8_t *buf,
                          size_t len);
int8_t  tmc5240_read_reg (tmc5240_reg_t reg, uint8_t *buf,
                          size_t len);

/* Convenience wrappers for the most common registers (32‑bit values) */
int8_t  tmc5240_set_gconf(uint32_t value);
int8_t  tmc5240_get_gconf(uint32_t *value);
int8_t  tmc5240_get_gstat(uint32_t *value);

/* -------------------------------------------------------------------------
 *  Miscellaneous
 * ------------------------------------------------------------------------- */
#define TMC5240_REG_ADDR_WRITE(reg)   ((uint8_t)((reg) | 0x80))   /* [1] */
#define TMC5240_REG_ADDR_READ(reg)    ((uint8_t)(reg))

#endif /* TMC5240_H_ */