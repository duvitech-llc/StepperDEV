#ifndef TMC5240_H_
#define TMC5240_H_

// Uncomment if you want to save space.....
// and put the table into your own .c file
//#define TMC_API_EXTERNAL_CRC_TABLE 1

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "tmc5240_hw_abstraction.h"

// Amount of CRC tables available
// Each table takes ~260 bytes (257 bytes, one bool and structure padding)
#define CRC_TABLE_COUNT 2

typedef enum {
    IC_BUS_SPI,
    IC_BUS_UART,
} TMC5240BusType;

// => TMC-API wrapper
extern void tmc5240_readWriteSPI(uint16_t icID, uint8_t *data, size_t dataLength);
extern bool tmc5240_readWriteUART(uint16_t icID, uint8_t *data, size_t writeLength, size_t readLength);
extern TMC5240BusType tmc5240_getBusType(uint16_t icID);
extern uint8_t tmc5240_getNodeAddress(uint16_t icID);
// => TMC-API wrapper


int32_t tmc5240_readRegister(uint16_t icID, uint8_t address);
void tmc5240_writeRegister(uint16_t icID, uint8_t address, int32_t value);
void tmc5240_rotateMotor(uint16_t icID, uint8_t motor, int32_t velocity);

typedef struct
{
    uint32_t mask;
    uint8_t shift;
    uint8_t address;
    bool isSigned;
} RegisterField;


static inline uint32_t field_extract(uint32_t data, RegisterField field)
{
    uint32_t value = (data & field.mask) >> field.shift;

    if (field.isSigned)
    {
        // Apply signedness conversion
        uint32_t baseMask = field.mask >> field.shift;
        uint32_t signMask = baseMask & (~baseMask >> 1);
        value = (value ^ signMask) - signMask;
    }

    return value;
}

static inline uint32_t field_read(uint16_t icID, RegisterField field)
{
    uint32_t value = tmc5240_readRegister(icID, field.address);

    return field_extract(value, field);
}

static inline uint32_t field_update(uint32_t data, RegisterField field, uint32_t value)
{
    return (data & (~field.mask)) | ((value << field.shift) & field.mask);
}

static inline void field_write(uint16_t icID, RegisterField field, uint32_t value)
{
    uint32_t regValue = tmc5240_readRegister(icID, field.address);

    regValue = field_update(regValue, field, value);

    tmc5240_writeRegister(icID, field.address, regValue);
}

#endif /* TMC5240_H_ */