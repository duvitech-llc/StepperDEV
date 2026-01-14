#include "tmc5240_driver.h"
#include "util.h"
#include <stdio.h>

/* --------------------------------------------------------------------------
 * Driver registry (IC ID → context)
 * -------------------------------------------------------------------------- */

#define TMC5240_MAX_IC  4

static TMC5240_Context *tmc_ctx_table[TMC5240_MAX_IC] = {0};

static inline TMC5240_Context *ctx_from_id(uint16_t icID)
{
    if (icID >= TMC5240_MAX_IC)
        return NULL;
    return tmc_ctx_table[icID];
}

/* --------------------------------------------------------------------------
 * Trinamic HAL required callbacks
 * -------------------------------------------------------------------------- */

void tmc5240_readWriteSPI(uint16_t icID, uint8_t *data, size_t len, bool cs_override)
{
    TMC5240_Context *ctx = ctx_from_id(icID);
    if (!ctx || !ctx->hspi)
        return;

    uint8_t rx[5] = {0};

    if (!cs_override)
    {
        HAL_GPIO_WritePin(ctx->cs_port, ctx->cs_pin, GPIO_PIN_RESET);
        for (volatile int i = 0; i < 20; i++);
    }

    HAL_SPI_TransmitReceive(ctx->hspi, data, rx, len, HAL_MAX_DELAY);

    if (!cs_override)
    {
        for (volatile int i = 0; i < 20; i++);
        HAL_GPIO_WritePin(ctx->cs_port, ctx->cs_pin, GPIO_PIN_SET);
    }

    for (size_t i = 0; i < len; i++)
        data[i] = rx[i];
}

void tmc5240_fast_writeSPI(uint16_t icID, uint8_t *data, size_t len)
{
    TMC5240_Context *ctx = ctx_from_id(icID);
    if (!ctx || !ctx->hspi)
        return;

    uint8_t rx[5] = {0};

    HAL_GPIO_WritePin(ctx->cs_port, ctx->cs_pin, GPIO_PIN_RESET);

    HAL_SPI_TransmitReceive(ctx->hspi, data, rx, len, HAL_MAX_DELAY);

    HAL_GPIO_WritePin(ctx->cs_port, ctx->cs_pin, GPIO_PIN_SET);
}

bool tmc5240_readWriteUART(uint16_t icID,
                           uint8_t *data,
                           size_t writeLength,
                           size_t readLength)
{
    (void)icID;
    (void)data;
    (void)writeLength;
    (void)readLength;
    return false;
}

TMC5240BusType tmc5240_getBusType(uint16_t icID)
{
    (void)icID;
    return IC_BUS_SPI;
}

uint8_t tmc5240_getNodeAddress(uint16_t icID)
{
    (void)icID;
    return 1;
}

/* --------------------------------------------------------------------------
 * StepperDriver callbacks
 * -------------------------------------------------------------------------- */

static void tmc5240_init(Stepper *s)
{
    TMC5240_Context *ctx = s->hw_context;
    tmc_ctx_table[ctx->icID] = ctx;

    /* Core driver configuration - matches working main.c */
    tmc5240_writeRegister(ctx->icID, TMC5240_GCONF, 0x00000008, false);
    tmc5240_writeRegister(ctx->icID, TMC5240_DRV_CONF, 0x00000020, false);
    tmc5240_writeRegister(ctx->icID, TMC5240_GLOBAL_SCALER, 0x00000000, false);
    
    /* Motor current configurations */
    tmc5240_writeRegister(ctx->icID, TMC5240_IHOLD_IRUN, 0x00070A03, false);
    tmc5240_writeRegister(ctx->icID, TMC5240_TPOWERDOWN, 0x0000000A, false);
    
    /* Chopper configuration - CRITICAL: use full value from working code */
    tmc5240_writeRegister(ctx->icID, TMC5240_CHOPCONF, 0x10410153, false);
    /* Motion parameters */
    tmc5240_writeRegister(ctx->icID, TMC5240_AMAX, ctx->amax, false);
    tmc5240_writeRegister(ctx->icID, TMC5240_DMAX, ctx->dmax, false);
    tmc5240_writeRegister(ctx->icID, TMC5240_VMAX, ctx->vmax, false);
    tmc5240_writeRegister(ctx->icID, TMC5240_TVMAX, 0x00000F8D, false);

    /* Position mode */
    tmc5240_writeRegister(ctx->icID, TMC5240_RAMPMODE, TMC5240_MODE_POSITION, false);
    
    /* Reset position */
    tmc5240_writeRegister(ctx->icID, TMC5240_XACTUAL, 0, false);
}

static void tmc5240_enable(Stepper *s, bool en)
{
    TMC5240_Context *ctx = s->hw_context;

    HAL_GPIO_WritePin(ctx->enable_port, ctx->enable_pin, en ? GPIO_PIN_RESET : GPIO_PIN_SET);
    tmc5240_writeRegister(ctx->icID,
                          TMC5240_GCONF,
                          en ? 0x00000008 : 0x00000000, false);
}

static void tmc5240_set_dir(Stepper *s, bool dir)
{
    TMC5240_Context *ctx = s->hw_context;
    tmc5240_writeRegister(ctx->icID,
        TMC5240_RAMPMODE,
        dir ? TMC5240_MODE_VELPOS : TMC5240_MODE_VELNEG, false);
}

static void tmc5240_step_pulse(Stepper *s)
{
    (void)s;
    /* No-op: internal ramp generator */
}

static void tmc5240_move_to(Stepper *s, int32_t pos)
{
    TMC5240_Context *ctx = s->hw_context;
    tmc5240_writeRegister(ctx->icID, TMC5240_RAMPMODE, TMC5240_MODE_POSITION, false);
    tmc5240_writeRegister(ctx->icID, TMC5240_XTARGET, pos, false);
}

static int32_t tmc5240_get_position(Stepper *s)
{
    TMC5240_Context *ctx = s->hw_context;
    return tmc5240_readRegister(ctx->icID, TMC5240_XACTUAL, false);
}

static bool tmc5240_position_reached(Stepper *s)
{
    TMC5240_Context *ctx = s->hw_context;
    uint32_t st = tmc5240_readRegister(ctx->icID, TMC5240_RAMPSTAT, false);
    return (st & TMC5240_POSITION_REACHED_MASK) != 0;
}

/* --------------------------------------------------------------------------
 * Public StepperDriver instance
 * -------------------------------------------------------------------------- */

const StepperDriver TMC5240_Driver = {
    .caps = STEPPER_CAP_MOVE_TO | 
            STEPPER_CAP_POSITION_FB,
    .init             = tmc5240_init,
    .set_enable       = tmc5240_enable,
    .set_dir          = tmc5240_set_dir,
    .step_pulse       = tmc5240_step_pulse,
    .move_to          = tmc5240_move_to,
    .get_position     = tmc5240_get_position,
    .position_reached = tmc5240_position_reached,
};

/* --------------------------------------------------------------------------
 * Debug helpers
 * -------------------------------------------------------------------------- */

void tmc5240_driver_print_registers(const TMC5240_Context *ctx)
{
    if (!ctx)
        return;

    printf("\nTMC5240[%u] registers:\n", ctx->icID);

#define R(r) printf("  %-12s 0x%08lX\n", #r, \
    (unsigned long)tmc5240_readRegister(ctx->icID, r, false))

    R(TMC5240_GCONF);
    R(TMC5240_GSTAT);
    R(TMC5240_IHOLD_IRUN);
    R(TMC5240_CHOPCONF);
    R(TMC5240_AMAX);
    R(TMC5240_DMAX);
    R(TMC5240_VMAX);
    R(TMC5240_RAMPMODE);
    R(TMC5240_XACTUAL);
    R(TMC5240_XTARGET);
    R(TMC5240_VACTUAL);
    R(TMC5240_DRVSTATUS);

#undef R
}
