#ifndef TMC5240_DRIVER_H
#define TMC5240_DRIVER_H

#include "main.h"
#include "stepper.h"
#include "tmc5240.h"
#include "tmc5240_hw_abstraction.h"
#include <stdint.h>

/* ============================================================================
 *  TMC5240 Driver Context (per motor)
 * ========================================================================== */

typedef struct
{
    /* IC identity */
    uint16_t icID;

    /* SPI interface */
    SPI_HandleTypeDef *hspi;
    GPIO_TypeDef *cs_port;
    uint16_t cs_pin;

    GPIO_TypeDef *enable_port;
    uint16_t enable_pin;

    /* Motion parameters */
    uint32_t vmax;
    uint32_t amax;
    uint32_t dmax;

    /* Cached state */
    int32_t last_target;

} TMC5240_Context;

/* ============================================================================
 *  Public Driver Descriptor
 * ========================================================================== */

/* Exposed driver instance */
extern const StepperDriver TMC5240_Driver;

/* Utility / debug */
void tmc5240_driver_print_registers(const TMC5240_Context *ctx);

/* --------------------------------------------------------------------------
 * Trinamic HAL hooks (called by tmc5240.c)
 * -------------------------------------------------------------------------- */
void tmc5240_readWriteSPI(uint16_t icID, uint8_t *data, size_t dataLength);
bool tmc5240_readWriteUART(uint16_t icID, uint8_t *data,
                           size_t writeLength, size_t readLength);
void tmc5240_fast_writeSPI(uint16_t icID, uint8_t *data, size_t len);

TMC5240BusType tmc5240_getBusType(uint16_t icID);
uint8_t tmc5240_getNodeAddress(uint16_t icID);

#endif /* TMC5240_DRIVER_H */
