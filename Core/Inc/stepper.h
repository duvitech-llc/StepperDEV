#ifndef STEPPER_H
#define STEPPER_H

#include <stdint.h>
#include <stdbool.h>


/* Forward declaration */
struct Stepper;

/* Completion callback */
typedef void (*StepperDoneCallback)(struct Stepper *stepper);

/* Hardware driver interface */
typedef struct
{
    /* Optional driver init hook */
    void (*init)(struct Stepper *stepper);

    void (*set_enable)(struct Stepper *stepper, bool enable);
    void (*set_dir)(struct Stepper *stepper, bool dir);
    void (*step_pulse)(struct Stepper *stepper);

    /* Optional: move to absolute position (for drivers with internal ramp generators) */
    void (*move_to)(struct Stepper *stepper, int32_t position);

    /* Optional: get current position from driver */
    int32_t (*get_position)(struct Stepper *stepper);

    /* Optional: check if position reached */
    bool (*position_reached)(struct Stepper *stepper);
} StepperDriver;

/* Stepper instance */
typedef struct Stepper
{
    uint8_t stepper_id;      /* User-defined ID */

    const StepperDriver *driver;
    void *hw_context;

    int32_t position;
    int32_t steps_remaining;

    bool direction;

    uint32_t us_per_step;
    uint32_t us_accumulator;

    StepperDoneCallback done_cb;
} Stepper;

/* Stepper group */
#define STEPPER_GROUP_MAX 4

typedef struct
{
    Stepper *steppers[STEPPER_GROUP_MAX];
    uint8_t count;
} StepperGroup;

/* --------------------------------------------------------------------------
 *  Stepper API – thin wrappers that forward to the driver implementation.
 * -------------------------------------------------------------------------- */

void stepper_init(Stepper *stepper,
                  uint8_t stepper_id,
                  const StepperDriver *driver,
                  void *hw_context);

void stepper_enable(Stepper *stepper, bool enable);

void stepper_set_steps(Stepper *stepper, int32_t steps);

void stepper_set_speed(Stepper *stepper, uint32_t us_per_step);
uint32_t stepper_get_speed(const Stepper *stepper);

int32_t stepper_get_steps(const Stepper *stepper);

void stepper_set_done_callback(Stepper *stepper,
                               StepperDoneCallback cb);

bool stepper_update(Stepper *stepper, uint32_t delta_us);

/* Move to absolute position (for drivers with internal motion controllers) */
void stepper_move_to_position(Stepper *stepper, int32_t position);

/* Get current position from driver */
int32_t stepper_get_position(Stepper *stepper);

/* Check if target position has been reached */
bool stepper_position_reached(Stepper *stepper);


/* Register completion callback */
void stepper_set_done_callback(Stepper *stepper,
                               StepperDoneCallback cb);


/* --------------------------------------------------------------------------
 *  Group API – applies the generic API to every motor in the group.
 * -------------------------------------------------------------------------- */

void stepper_group_init(StepperGroup *group);
bool stepper_group_add(StepperGroup *group, Stepper *stepper);

void stepper_group_enable(StepperGroup *group, bool enable);
void stepper_group_set_steps(StepperGroup *group, int32_t steps);
void stepper_group_set_speed(StepperGroup *group, uint32_t us_per_step);

bool stepper_group_update(StepperGroup *group, uint32_t delta_us);
void stepper_group_move_by(StepperGroup *group, int32_t steps);

#endif // STEPPER_H
