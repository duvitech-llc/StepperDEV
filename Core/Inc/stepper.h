#ifndef STEPPER_H
#define STEPPER_H

#include <stdint.h>
#include <stdbool.h>

/* ============================================================================
 *  Forward Declarations
 * ========================================================================== */

struct Stepper;

/* ============================================================================
 *  Callbacks
 * ========================================================================== */

/* Motion completed callback */
typedef void (*StepperDoneCallback)(struct Stepper *stepper);

/* Limit switch callback */
typedef void (*StepperLimitCallback)(struct Stepper *stepper, void *sw);

/* ============================================================================
 *  Driver Capability Flags
 * ========================================================================== */

typedef enum
{
    STEPPER_CAP_STEP_DIR      = (1u << 0), /* STEP/DIR pulse driven */
    STEPPER_CAP_MOVE_TO       = (1u << 1), /* Driver supports absolute move */
    STEPPER_CAP_POSITION_FB   = (1u << 2), /* Driver reports position */
    STEPPER_CAP_LIMITS        = (1u << 3)  /* Driver handles limit switches */
} StepperCaps;

/* ============================================================================
 *  Hardware Driver Interface
 * ========================================================================== */

typedef struct
{
    uint32_t caps;

    /* Optional lifecycle hook */
    void (*init)(struct Stepper *stepper);

    /* Enable / disable driver */
    void (*set_enable)(struct Stepper *stepper, bool enable);

    /* STEP/DIR mode (required if STEPPER_CAP_STEP_DIR is set) */
    void (*set_dir)(struct Stepper *stepper, bool dir);
    void (*step_pulse)(struct Stepper *stepper);

    /* Smart-driver motion (required if STEPPER_CAP_MOVE_TO is set) */
    void (*move_to)(struct Stepper *stepper, int32_t position);

    /* Driver-owned position feedback (required if STEPPER_CAP_POSITION_FB) */
    int32_t (*get_position)(struct Stepper *stepper);

    /* Completion check (required if STEPPER_CAP_MOVE_TO) */
    bool (*position_reached)(struct Stepper *stepper);

} StepperDriver;

/* ============================================================================
 *  Stepper Instance
 * ========================================================================== */

typedef struct Stepper
{
    /* Identity */
    uint8_t stepper_id;

    /* Driver binding */
    const StepperDriver *driver;
    void *hw_context;

    /* Motion request state */
    int32_t target_position;     /* Absolute target */
    int32_t steps_remaining;     /* STEP/DIR only */
    bool direction;

    /* Timing (STEP/DIR only) */
    uint32_t us_per_step;
    uint32_t us_accumulator;

    /* State flags */
    bool enabled;
    bool busy;

    /* Callbacks */
    StepperDoneCallback done_cb;

    /* Limit switch handling */
    bool limits_enabled;
    bool limit_hit;
    StepperLimitCallback limit_cb;

} Stepper;

/* ============================================================================
 *  Stepper Group
 * ========================================================================== */

#define STEPPER_GROUP_MAX  4

typedef struct
{
    Stepper *steppers[STEPPER_GROUP_MAX];
    uint8_t count;
    bool synch_capable; // true if all steppers are on unique SPI busses
} StepperGroup;

/* ============================================================================
 *  Low-Level Stepper API (ISR-safe, non-blocking)
 * ========================================================================== */

/*
 * Initialize a stepper instance
 */
void stepper_init(Stepper *stepper,
                  uint8_t stepper_id,
                  const StepperDriver *driver,
                  void *hw_context);

/*
 * Enable or disable the motor driver
 */
void stepper_enable(Stepper *stepper, bool enable);

/*
 * Set step rate (STEP/DIR drivers only)
 */
void stepper_set_speed(Stepper *stepper, uint32_t us_per_step);
uint32_t stepper_get_speed(const Stepper *stepper);

/*
 * Command absolute move
 * - Uses driver internal motion if supported
 * - Falls back to STEP/DIR stepping otherwise
 */
void stepper_move_to_position(Stepper *stepper, int32_t position);

/*
 * Update motor state
 * - Call periodically with elapsed microseconds
 * - Returns true if motor is still moving
 */
bool stepper_update(Stepper *stepper, uint32_t delta_us);

/*
 * Query driver-owned position
 */
int32_t stepper_get_position(Stepper *stepper);

/*
 * Check if motion is complete
 */
bool stepper_position_reached(Stepper *stepper);

/*
 * Register completion callback
 */
void stepper_set_done_callback(Stepper *stepper,
                               StepperDoneCallback cb);

/* ============================================================================
 *  High-Level Stepper API (Application / Blocking)
 * ========================================================================== */

/*
 * Generic initialization wrapper
 */
void Stepper_init(void *context);

/*
 * Set acceleration (implementation-defined units)
 */
void Stepper_setAcceleration(volatile Stepper *s, float accel);

/*
 * Check if motor is currently moving
 */
bool Stepper_isMoving(volatile Stepper *s);

/*
 * Start motion (enable motor if needed)
 */
void Stepper_start(volatile Stepper *s);

/*
 * Stop motor motion immediately
 */
void Stepper_stop(volatile Stepper *s);

/*
 * Disable motor driver
 */
void Stepper_disable(volatile Stepper *s);

/*
 * Move to absolute position
 */
void Stepper_move(volatile Stepper *s, int32_t position);

/*
 * Wait for motor to stop
 * timeout_ms = 0 → wait forever
 */
void Stepper_awaitStop(volatile Stepper *s, uint32_t timeout_ms);

/*
 * Enable limit switch handling
 */
void Stepper_enableLimits(volatile Stepper *s);

/*
 * Limit switch event handler
 */
void Stepper_hitLimit(volatile Stepper *s, void *sw);

/*
 * Wait for limit switch event
 * Returns true if limit was hit before timeout
 */
bool Stepper_awaitLimit(volatile Stepper *s, uint32_t timeout_ms);

/*
 * Get array of current positions for all steppers
 */
int32_t *Stepper_positions(void);

/* ============================================================================
 *  Stepper Group API
 * ========================================================================== */

void stepper_group_init(StepperGroup *group);
bool stepper_group_add(StepperGroup *group, Stepper *stepper);

void stepper_group_enable(StepperGroup *group, bool enable);
void stepper_group_move_to(StepperGroup *group, int32_t position);
bool stepper_group_update(StepperGroup *group, uint32_t delta_us);

#endif /* STEPPER_H */
