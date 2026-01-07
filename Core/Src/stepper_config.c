/**
 * @file stepper_config.c
 * @brief TMC5240 driver implementation for the generic stepper API
 *
 * This file provides the hardware-specific driver implementation that bridges
 * the generic stepper.h API with the TMC5240 stepper motor controller.
 */
#include "main.h"
#include "stepper_config.h"
#include "stepper.h"
#include "tmc5240.h"
#include "util.h"
#include <stdio.h>

/* -------------------------------------------------------------------------- */
/*                         TMC5240 Hardware Context                           */
/* -------------------------------------------------------------------------- */

typedef struct
{
    uint16_t icID;          /* TMC5240 IC identifier (0 or 1) */
    uint32_t vmax;          /* Maximum velocity setting */
    uint32_t amax;          /* Maximum acceleration */
    uint32_t dmax;          /* Maximum deceleration */
} TMC5240_Context;

/* Hardware contexts for each motor */
static TMC5240_Context tmc5240_ctx[STEPPER_COUNT] = {
    { .icID = 0, .vmax = 0x2710, .amax = 0x0F8D, .dmax = 0x0F8D },
    { .icID = 1, .vmax = 0x2710, .amax = 0x0F8D, .dmax = 0x0F8D }
};

/* Stepper instances */
static Stepper steppers[STEPPER_COUNT];

/* Stepper group for coordinated motion */
static StepperGroup stepper_group;

/* -------------------------------------------------------------------------- */
/*                      TMC5240 Driver Implementation                         */
/* -------------------------------------------------------------------------- */

/** @brief Active bus type for TMC5240 communication (SPI or UART) */
static TMC5240BusType activeBus = IC_BUS_SPI;

/** @brief UART node address for TMC5240 (used in UART mode only) */
static uint8_t nodeAddress = 1;

/* -------------------------------------------------------------------------- */
/*                    TMC5240 External Interface Functions                    */
/* -------------------------------------------------------------------------- */
/*
 * These functions are required by tmc5240.c as extern declarations.
 * They provide the hardware abstraction layer for SPI/UART communication
 * with the TMC5240 stepper motor driver ICs.
 */

/**
 * @brief Perform SPI read/write transaction with TMC5240
 *
 * This function handles the low-level SPI communication with TMC5240 ICs.
 * It manages chip select signals for multiple ICs and performs full-duplex
 * SPI transfers. The TMC5240 uses a 40-bit (5-byte) SPI frame format.
 *
 * @param icID       IC identifier (0 = first TMC5240, 1 = second TMC5240)
 * @param data       Pointer to TX data buffer; receives RX data on return
 * @param dataLength Number of bytes to transfer (typically 5 for TMC5240)
 *
 * @note This function is called by tmc5240_readRegister() and tmc5240_writeRegister()
 * @note The function includes timing delays for SPI signal integrity
 */
void tmc5240_readWriteSPI(uint16_t icID, uint8_t *data, size_t dataLength)
{
    HAL_StatusTypeDef status;
    uint8_t ret_data[5] = { 0 };
    
    /* Add small delay to ensure previous transaction completed */
    delay_us(500);
    
    /* Select the TMC5240 by pulling CS low */
    switch(icID)
    {
        case 0:
            HAL_GPIO_WritePin(TMC5240_CS_GPIO_Port, TMC5240_CS_Pin, GPIO_PIN_RESET);
            break;
        case 1:
            HAL_GPIO_WritePin(STEP2_CS_GPIO_Port, STEP2_CS_Pin, GPIO_PIN_RESET);
            break;
        default:
            printf("TMC5240 SPI Error: Invalid IC ID %d\r\n", icID);
            return;
    }
    
    /* Small delay after CS assert (minimum CSN low time) */
    for(volatile int i = 0; i < 20; i++);
    
    /* Perform SPI transmission and reception */
    status = HAL_SPI_TransmitReceive(&hspi1, data, ret_data, dataLength, 100);
    if(status != HAL_OK) {
        printf("SPI Error during TransmitReceive: %d\r\n", status);
    }
    
    /* Small delay before CS deassert */
    for(volatile int i = 0; i < 20; i++);
    
    /* Deselect the TMC5240 by pulling CS high */
    switch(icID)
    {
        case 0:
            HAL_GPIO_WritePin(TMC5240_CS_GPIO_Port, TMC5240_CS_Pin, GPIO_PIN_SET);
            break;
        case 1:
            HAL_GPIO_WritePin(STEP2_CS_GPIO_Port, STEP2_CS_Pin, GPIO_PIN_SET);
            break;
        default:
            printf("TMC5240 SPI Error: Invalid IC ID %d\r\n", icID);
            return;
    }
    
    /* Copy received data back to the original buffer */
    for(size_t i = 0; i < dataLength; i++) {
        data[i] = ret_data[i];
    }

    if(status != HAL_OK) {
        printf("SPI Error: %d\r\n", status);
    }
}

/**
 * @brief Perform UART read/write transaction with TMC5240
 *
 * This function would handle UART communication with TMC5240 ICs.
 * Currently not implemented as SPI mode is used.
 *
 * @param icID        IC identifier
 * @param data        Pointer to data buffer
 * @param writeLength Number of bytes to write
 * @param readLength  Number of bytes to read
 * @return true if successful, false otherwise
 *
 * @note Not implemented - UART mode not supported in this configuration
 */
bool tmc5240_readWriteUART(uint16_t icID, uint8_t *data, size_t writeLength, size_t readLength)
{
    /* Suppress unused parameter warnings */
    (void)icID;
    (void)data;
    (void)writeLength;
    (void)readLength;
    return false;
}

/**
 * @brief Get the communication bus type for a TMC5240 IC
 *
 * Returns whether SPI or UART is used for communication with the specified IC.
 *
 * @param icID IC identifier
 * @return TMC5240BusType indicating IC_BUS_SPI or IC_BUS_UART
 */
TMC5240BusType tmc5240_getBusType(uint16_t icID)
{
    UNUSED(icID);
    return activeBus;
}

/**
 * @brief Get the UART node address for a TMC5240 IC
 *
 * Returns the configured UART address for the specified IC.
 * Only relevant when using UART communication mode.
 *
 * @param icID IC identifier
 * @return UART node address (0-255)
 */
uint8_t tmc5240_getNodeAddress(uint16_t icID)
{
    UNUSED(icID);
    return nodeAddress;
}

/* -------------------------------------------------------------------------- */
/*                      TMC5240 Stepper Driver Callbacks                      */
/* -------------------------------------------------------------------------- */

/**
 * @brief Initialize TMC5240 for the given stepper
 */
static void tmc5240_driver_init(Stepper *stepper)
{
    if (!stepper || !stepper->hw_context)
        return;

    TMC5240_Context *ctx = (TMC5240_Context *)stepper->hw_context;
    uint16_t icID = ctx->icID;

    /* Enable driver, internal motion controller */
    tmc5240_writeRegister(icID, TMC5240_GCONF, 0x00000008);

    /* Current range selection */
    tmc5240_writeRegister(icID, TMC5240_DRV_CONF, 0x00000020);

    /* Run/Hold current: IHOLD=3, IRUN=10, IHOLDDELAY=7 */
    tmc5240_writeRegister(icID, TMC5240_IHOLD_IRUN, 0x00070A03);

    /* Minimal valid SpreadCycle config */
    tmc5240_writeRegister(icID, TMC5240_CHOPCONF, 0x00010053);

    /* Positioning mode */
    tmc5240_writeRegister(icID, TMC5240_RAMPMODE, TMC5240_MODE_POSITION);

    /* Reset logical position */
    tmc5240_writeRegister(icID, TMC5240_XACTUAL, 0x00000000);

    /* Acceleration / deceleration */
    tmc5240_writeRegister(icID, TMC5240_AMAX, ctx->amax);
    tmc5240_writeRegister(icID, TMC5240_DMAX, ctx->dmax);

    /* Max velocity */
    tmc5240_writeRegister(icID, TMC5240_VMAX, ctx->vmax);
}

/**
 * @brief Enable or disable motor driver output
 *
 * Note: The TMC5240 typically uses a shared enable pin (DRV_EN).
 * This implementation controls the per-motor enable via the GCONF register.
 */
static void tmc5240_driver_set_enable(Stepper *stepper, bool enable)
{
    if (!stepper || !stepper->hw_context)
        return;

    TMC5240_Context *ctx = (TMC5240_Context *)stepper->hw_context;
    uint16_t icID = ctx->icID;

    if (enable)
    {
        /* Enable motor - set shaft bit for internal ramp generator */
        tmc5240_writeRegister(icID, TMC5240_GCONF, 0x00000008);
    }
    else
    {
        /* Disable motor outputs */
        tmc5240_writeRegister(icID, TMC5240_GCONF, 0x00000000);
    }
}

/**
 * @brief Set motor direction
 *
 * For TMC5240 with internal ramp generator, direction is implicit
 * in the target position. This function sets up the ramp mode.
 */
static void tmc5240_driver_set_dir(Stepper *stepper, bool dir)
{
    if (!stepper || !stepper->hw_context)
        return;

    TMC5240_Context *ctx = (TMC5240_Context *)stepper->hw_context;
    uint16_t icID = ctx->icID;

    /* For velocity mode, set direction via RAMPMODE */
    /* VELPOS=1 (positive direction), VELNEG=2 (negative direction) */
    if (dir)
    {
        tmc5240_writeRegister(icID, TMC5240_RAMPMODE, TMC5240_MODE_VELPOS);
    }
    else
    {
        tmc5240_writeRegister(icID, TMC5240_RAMPMODE, TMC5240_MODE_VELNEG);
    }
}

/**
 * @brief Issue a step pulse (move to target position)
 *
 * For TMC5240 with internal ramp generator, we use position mode
 * and write to XTARGET. The steps_remaining in the Stepper struct
 * represents our high-level motion command.
 */
static void tmc5240_driver_step_pulse(Stepper *stepper)
{
    if (!stepper || !stepper->hw_context)
        return;

    /* TMC5240 handles stepping internally via the ramp generator.
     * This callback is invoked by stepper_update() for software-timed stepping.
     * For TMC5240, we don't need to pulse - the IC does it automatically.
     * This is a no-op for the internal motion controller. */
}

/**
 * @brief Move to absolute position using TMC5240's internal ramp generator
 *
 * This is the driver callback for stepper_move_to_position().
 */
static void tmc5240_driver_move_to(Stepper *stepper, int32_t position)
{
    if (!stepper || !stepper->hw_context)
        return;

    TMC5240_Context *ctx = (TMC5240_Context *)stepper->hw_context;
    uint16_t icID = ctx->icID;

    /* Set position mode */
    tmc5240_writeRegister(icID, TMC5240_RAMPMODE, TMC5240_MODE_POSITION);

    /* Set max velocity for position mode */
    tmc5240_writeRegister(icID, TMC5240_VMAX, ctx->vmax);

    /* Write target position */
    tmc5240_writeRegister(icID, TMC5240_XTARGET, position);
}

/**
 * @brief Get current actual position from TMC5240
 *
 * This is the driver callback for stepper_get_position().
 */
static int32_t tmc5240_driver_get_position(Stepper *stepper)
{
    if (!stepper || !stepper->hw_context)
        return 0;

    TMC5240_Context *ctx = (TMC5240_Context *)stepper->hw_context;
    return tmc5240_readRegister(ctx->icID, TMC5240_XACTUAL);
}

/**
 * @brief Check if motor has reached target position
 *
 * This is the driver callback for stepper_position_reached().
 */
static bool tmc5240_driver_position_reached(Stepper *stepper)
{
    if (!stepper || !stepper->hw_context)
        return true;

    TMC5240_Context *ctx = (TMC5240_Context *)stepper->hw_context;
    uint32_t status = tmc5240_readRegister(ctx->icID, TMC5240_RAMPSTAT);
    return (status & TMC5240_POSITION_REACHED_MASK) != 0;
}

/* TMC5240 driver interface */
static const StepperDriver tmc5240_driver = {
    .init = tmc5240_driver_init,
    .set_enable = tmc5240_driver_set_enable,
    .set_dir = tmc5240_driver_set_dir,
    .step_pulse = tmc5240_driver_step_pulse,
    .move_to = tmc5240_driver_move_to,
    .get_position = tmc5240_driver_get_position,
    .position_reached = tmc5240_driver_position_reached
};

/* -------------------------------------------------------------------------- */
/*                    TMC5240-Specific Helper Functions                       */
/* -------------------------------------------------------------------------- */

/**
 * @brief Move stepper to absolute position using TMC5240's internal ramp generator
 *
 * This is a convenience wrapper around stepper_move_to_position().
 */
void stepper_config_move_to(Stepper *stepper, int32_t position)
{
    tmc5240_driver_move_to(stepper, position);
}

/**
 * @brief Move stepper by relative steps using TMC5240's internal ramp generator
 */
void stepper_config_move_by(Stepper *stepper, int32_t steps)
{
    int32_t current_pos = tmc5240_driver_get_position(stepper);
    tmc5240_driver_move_to(stepper, current_pos + steps);
}

/**
 * @brief Rotate motor continuously at specified velocity
 */
void stepper_config_rotate(Stepper *stepper, int32_t velocity)
{
    if (!stepper || !stepper->hw_context)
        return;

    TMC5240_Context *ctx = (TMC5240_Context *)stepper->hw_context;
    tmc5240_rotateMotor(ctx->icID, velocity);
}

/**
 * @brief Stop motor immediately
 */
void stepper_config_stop(Stepper *stepper)
{
    stepper_config_rotate(stepper, 0);
}

/**
 * @brief Get current actual position from TMC5240
 *
 * This is a convenience wrapper around stepper_get_position().
 */
int32_t stepper_config_get_position(Stepper *stepper)
{
    return tmc5240_driver_get_position(stepper);
}

/**
 * @brief Check if motor has reached target position
 *
 * This is a convenience wrapper around stepper_position_reached().
 */
bool stepper_config_position_reached(Stepper *stepper)
{
    return tmc5240_driver_position_reached(stepper);
}

/**
 * @brief Set velocity (VMAX) for TMC5240
 */
void stepper_config_set_velocity(Stepper *stepper, uint32_t velocity)
{
    if (!stepper || !stepper->hw_context)
        return;

    TMC5240_Context *ctx = (TMC5240_Context *)stepper->hw_context;
    ctx->vmax = velocity;
    tmc5240_writeRegister(ctx->icID, TMC5240_VMAX, velocity);
}

/**
 * @brief Set acceleration (AMAX) for TMC5240
 */
void stepper_config_set_acceleration(Stepper *stepper, uint32_t acceleration)
{
    if (!stepper || !stepper->hw_context)
        return;

    TMC5240_Context *ctx = (TMC5240_Context *)stepper->hw_context;
    ctx->amax = acceleration;
    ctx->dmax = acceleration;  /* Keep accel/decel symmetric */
    tmc5240_writeRegister(ctx->icID, TMC5240_AMAX, acceleration);
    tmc5240_writeRegister(ctx->icID, TMC5240_DMAX, acceleration);
}

/* -------------------------------------------------------------------------- */
/*                        Configuration / Initialization                      */
/* -------------------------------------------------------------------------- */

/**
 * @brief Initialize all stepper motors and the stepper group
 */
void stepper_config_init(void)
{
    /* Initialize stepper group */
    stepper_group_init(&stepper_group);

    /* Initialize each stepper with the TMC5240 driver */
    for (uint8_t i = 0; i < STEPPER_COUNT; i++)
    {
        stepper_init(&steppers[i], i, &tmc5240_driver, &tmc5240_ctx[i]);
        stepper_group_add(&stepper_group, &steppers[i]);
    }
}

/**
 * @brief Enable global driver output (shared DRV_EN pin)
 */
void stepper_config_enable_drivers(bool enable)
{
    if (enable)
    {
        HAL_GPIO_WritePin(DRV_EN_GPIO_Port, DRV_EN_Pin, GPIO_PIN_RESET);
    }
    else
    {
        HAL_GPIO_WritePin(DRV_EN_GPIO_Port, DRV_EN_Pin, GPIO_PIN_SET);
    }
}

/**
 * @brief Get pointer to stepper instance by index
 */
Stepper *stepper_config_get_stepper(uint8_t index)
{
    if (index >= STEPPER_COUNT)
        return NULL;

    return &steppers[index];
}

/**
 * @brief Get pointer to the stepper group
 */
StepperGroup *stepper_config_get_group(void)
{
    return &stepper_group;
}

/**
 * @brief Print TMC5240 register values for a stepper
 */
void stepper_config_print_registers(Stepper *stepper)
{
    if (!stepper || !stepper->hw_context)
        return;

    TMC5240_Context *ctx = (TMC5240_Context *)stepper->hw_context;
    uint16_t icID = ctx->icID;
    int32_t value;

    printf("\r\nStepper %u Configuration:\r\n", stepper->stepper_id);

    value = tmc5240_readRegister(icID, TMC5240_GCONF);
    printf("  GCONF:         0x%08lX\r\n", (unsigned long)value);

    value = tmc5240_readRegister(icID, TMC5240_GSTAT);
    printf("  GSTAT:         0x%08lX\r\n", (unsigned long)value);

    value = tmc5240_readRegister(icID, TMC5240_DRV_CONF);
    printf("  DRV_CONF:      0x%08lX\r\n", (unsigned long)value);

    value = tmc5240_readRegister(icID, TMC5240_GLOBAL_SCALER);
    printf("  GLOBAL_SCALER: 0x%08lX\r\n", (unsigned long)value);

    value = tmc5240_readRegister(icID, TMC5240_CHOPCONF);
    printf("  CHOPCONF:      0x%08lX\r\n", (unsigned long)value);

    value = tmc5240_readRegister(icID, TMC5240_IHOLD_IRUN);
    printf("  IHOLD_IRUN:    0x%08lX\r\n", (unsigned long)value);

    value = tmc5240_readRegister(icID, TMC5240_AMAX);
    printf("  AMAX:          0x%08lX\r\n", (unsigned long)value);

    value = tmc5240_readRegister(icID, TMC5240_DMAX);
    printf("  DMAX:          0x%08lX\r\n", (unsigned long)value);

    value = tmc5240_readRegister(icID, TMC5240_VMAX);
    printf("  VMAX:          0x%08lX\r\n", (unsigned long)value);

    value = tmc5240_readRegister(icID, TMC5240_RAMPMODE);
    printf("  RAMPMODE:      0x%08lX\r\n", (unsigned long)value);

    value = tmc5240_readRegister(icID, TMC5240_XACTUAL);
    printf("  XACTUAL:       0x%08lX\r\n", (unsigned long)value);

    value = tmc5240_readRegister(icID, TMC5240_XTARGET);
    printf("  XTARGET:       0x%08lX\r\n", (unsigned long)value);

    value = tmc5240_readRegister(icID, TMC5240_VACTUAL);
    printf("  VACTUAL:       0x%08lX\r\n", (unsigned long)value);

    value = tmc5240_readRegister(icID, TMC5240_INP_OUT);
    printf("  INP_OUT:       0x%08lX\r\n", (unsigned long)value);

    value = tmc5240_readRegister(icID, TMC5240_DRVSTATUS);
    printf("  DRVSTATUS:     0x%08lX\r\n", (unsigned long)value);
}
