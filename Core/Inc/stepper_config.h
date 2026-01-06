/**
 * @file stepper_config.h
 * @brief Configuration and initialization for stepper motors using TMC5240
 *
 * This header exposes the configured stepper instances and provides
 * TMC5240-specific helper functions for motion control.
 */

#ifndef STEPPER_CONFIG_H
#define STEPPER_CONFIG_H

#include "stepper.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------- */
/*                              Configuration                                 */
/* -------------------------------------------------------------------------- */

/** Number of stepper motors in the system */
#define STEPPER_COUNT 2

/** Stepper motor identifiers */
#define STEPPER_0   0
#define STEPPER_1   1

/* -------------------------------------------------------------------------- */
/*                            Initialization API                              */
/* -------------------------------------------------------------------------- */

/**
 * @brief Initialize all stepper motors and the stepper group
 *
 * This function initializes the TMC5240 driver for each motor,
 * configures the hardware, and sets up the stepper group.
 * Call this once during system initialization.
 */
void stepper_config_init(void);

/**
 * @brief Enable or disable the global driver output
 *
 * Controls the shared DRV_EN pin that enables/disables all motor drivers.
 *
 * @param enable true to enable drivers, false to disable
 */
void stepper_config_enable_drivers(bool enable);

/**
 * @brief Get pointer to a stepper instance by index
 *
 * @param index Stepper index (0 to STEPPER_COUNT-1)
 * @return Pointer to Stepper instance, or NULL if index is invalid
 */
Stepper *stepper_config_get_stepper(uint8_t index);

/**
 * @brief Get pointer to the stepper group
 *
 * @return Pointer to the StepperGroup containing all motors
 */
StepperGroup *stepper_config_get_group(void);

/* -------------------------------------------------------------------------- */
/*                      TMC5240-Specific Motion API                           */
/* -------------------------------------------------------------------------- */

/**
 * @brief Move stepper to an absolute position
 *
 * Uses TMC5240's internal ramp generator for smooth motion.
 *
 * @param stepper Pointer to stepper instance
 * @param position Target position in microsteps
 */
void stepper_config_move_to(Stepper *stepper, int32_t position);

/**
 * @brief Move stepper by a relative number of steps
 *
 * Uses TMC5240's internal ramp generator for smooth motion.
 *
 * @param stepper Pointer to stepper instance
 * @param steps Number of steps to move (positive or negative)
 */
void stepper_config_move_by(Stepper *stepper, int32_t steps);

/**
 * @brief Rotate motor continuously at specified velocity
 *
 * @param stepper Pointer to stepper instance
 * @param velocity Velocity value (positive = forward, negative = reverse)
 */
void stepper_config_rotate(Stepper *stepper, int32_t velocity);

/**
 * @brief Stop motor motion
 *
 * @param stepper Pointer to stepper instance
 */
void stepper_config_stop(Stepper *stepper);

/**
 * @brief Get current actual position from TMC5240
 *
 * @param stepper Pointer to stepper instance
 * @return Current position in microsteps
 */
int32_t stepper_config_get_position(Stepper *stepper);

/**
 * @brief Check if motor has reached target position
 *
 * @param stepper Pointer to stepper instance
 * @return true if position reached, false otherwise
 */
bool stepper_config_position_reached(Stepper *stepper);

/**
 * @brief Set maximum velocity for motion
 *
 * @param stepper Pointer to stepper instance
 * @param velocity Maximum velocity (TMC5240 VMAX register units)
 */
void stepper_config_set_velocity(Stepper *stepper, uint32_t velocity);

/**
 * @brief Set acceleration/deceleration for motion
 *
 * @param stepper Pointer to stepper instance
 * @param acceleration Acceleration value (TMC5240 AMAX register units)
 */
void stepper_config_set_acceleration(Stepper *stepper, uint32_t acceleration);

/**
 * @brief Print TMC5240 register values for debugging
 *
 * Prints key register values including GCONF, GSTAT, DRV_CONF,
 * CHOPCONF, IHOLD_IRUN, AMAX, VMAX, RAMPMODE, position, and status.
 *
 * @param stepper Pointer to stepper instance
 */
void stepper_config_print_registers(Stepper *stepper);

#ifdef __cplusplus
}
#endif

#endif /* STEPPER_CONFIG_H */
