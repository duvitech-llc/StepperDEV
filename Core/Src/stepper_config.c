/*
 * stepper_config.c — per-product hardware mapping for stepper motors
 */

#include "stepper_config.h"
#include "stepper.h"
#include "tmc5240_driver.h"

#include <stdio.h>

/* --------------------------------------------------------------------------
 *  External hardware handles
 * -------------------------------------------------------------------------- */

extern SPI_HandleTypeDef hspi1;

/* --------------------------------------------------------------------------
 *  TMC5240 hardware contexts (driver-owned state)
 * -------------------------------------------------------------------------- */

static TMC5240_Context tmc5240_ctx[STEPPER_COUNT] =
{
    {
        .icID = 0,
        .hspi = &hspi1,
        .cs_port = STEP1_CS_GPIO_Port,
        .cs_pin  = STEP1_CS_Pin,
        .enable_port = DRV_EN_GPIO_Port,
        .enable_pin  = DRV_EN_Pin,
        .vmax = 0x2710,
        .amax = 0x0F8D,
        .dmax = 0x0F8D
    },
    {
        .icID = 1,
        .hspi = &hspi1,
        .cs_port = STEP2_CS_GPIO_Port,
        .cs_pin  = STEP2_CS_Pin,
        .enable_port = DRV_EN_GPIO_Port,
        .enable_pin  = DRV_EN_Pin,
        .vmax = 0x2710,
        .amax = 0x0F8D,
        .dmax = 0x0F8D
    }
};

/* --------------------------------------------------------------------------
 *  Stepper instances
 * -------------------------------------------------------------------------- */

static Stepper steppers[STEPPER_COUNT];
static StepperGroup stepper_group;

/* --------------------------------------------------------------------------
 *  Configuration table (like OPTICS_MAP)
 * -------------------------------------------------------------------------- */

typedef struct
{
    StepperId id;
    const StepperDriver *driver;
    void *context;
} StepperConfigEntry;

static const StepperConfigEntry STEPPER_MAP[] =
{
    {
        .id      = STEPPER_0,
        .driver  = &TMC5240_Driver,
        .context = &tmc5240_ctx[0]
    },
    {
        .id      = STEPPER_1,
        .driver  = &TMC5240_Driver,
        .context = &tmc5240_ctx[1]
    }
};

/* --------------------------------------------------------------------------
 *  Public API
 * -------------------------------------------------------------------------- */

void stepper_config_init(void)
{
    printf("Initializing stepper configuration...\r\n");

    stepper_group_init(&stepper_group);

    for (uint32_t i = 0; i < STEPPER_COUNT; i++)
    {
        const StepperConfigEntry *cfg = &STEPPER_MAP[i];
        Stepper *s = &steppers[cfg->id];

        stepper_init(s,
                     cfg->id,
                     cfg->driver,
                     cfg->context);

        /* Don't enable here - let main do it after printing registers */
        /* stepper_enable(s, true); */

        stepper_group_add(&stepper_group, s);

        printf("Stepper %lu configured\r\n", (unsigned long)cfg->id);
    }
}

Stepper *stepper_config_get_stepper(StepperId id)
{
    if (id >= STEPPER_COUNT)
        return NULL;

    return &steppers[id];
}

StepperGroup *stepper_config_get_group(void)
{
    return &stepper_group;
}


// tmc5240_driver_print_registers()

void stepper_config_print_registers(Stepper *stepper)
{
    if (!stepper || !stepper->hw_context)
        return;

    TMC5240_Context *ctx = (TMC5240_Context *)stepper->hw_context;
    uint16_t icID = ctx->icID;
    int32_t value;

    printf("\r\nStepper %u Configuration:\r\n", stepper->stepper_id);

    /* Helper to read, print and add a small CS spacing delay to avoid
     * SPI timing issues when querying multiple registers quickly. */
#define READ_PRINT(reg, label)                         \
    do {                                              \
        value = tmc5240_readRegister(icID, (reg));    \
        printf("  %-14s 0x%08lX\r\n", (label), (unsigned long)value); \
        for (volatile int _d = 0; _d < 50; _d++);     \
    } while (0)

    READ_PRINT(TMC5240_GCONF, "GCONF:");
    READ_PRINT(TMC5240_GSTAT, "GSTAT:");
    READ_PRINT(TMC5240_DRV_CONF, "DRV_CONF:");
    READ_PRINT(TMC5240_GLOBAL_SCALER, "GLOBAL_SCALER:");
    READ_PRINT(TMC5240_CHOPCONF, "CHOPCONF:");
    READ_PRINT(TMC5240_IHOLD_IRUN, "IHOLD_IRUN:");
    READ_PRINT(TMC5240_AMAX, "AMAX:");
    READ_PRINT(TMC5240_DMAX, "DMAX:");
    READ_PRINT(TMC5240_VMAX, "VMAX:");
    READ_PRINT(TMC5240_RAMPMODE, "RAMPMODE:");
    READ_PRINT(TMC5240_XACTUAL, "XACTUAL:");
    READ_PRINT(TMC5240_XTARGET, "XTARGET:");
    READ_PRINT(TMC5240_VACTUAL, "VACTUAL:");
    READ_PRINT(TMC5240_INP_OUT, "INP_OUT:");
    READ_PRINT(TMC5240_DRVSTATUS, "DRVSTATUS:");

#undef READ_PRINT
}