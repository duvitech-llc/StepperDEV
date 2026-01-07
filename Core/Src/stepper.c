#include "stepper.h"
#include <string.h>

/* External stepper configuration - must be defined in stepper_config.c */
extern Stepper *stepper_config_get_stepper(uint8_t index);
extern void stepper_config_set_acceleration(Stepper *stepper, uint32_t acceleration);
extern void stepper_config_stop(Stepper *stepper);
extern uint8_t stepper_config_get_count(void);

/* Static position array for Stepper_positions() */
static int32_t stepper_positions_array[4];  /* Max 4 steppers */

/* -------------------------------------------------------------------------- */
/*                              Stepper API                                   */
/* -------------------------------------------------------------------------- */

void stepper_init(Stepper *stepper,
                  uint8_t stepper_id,
                  const StepperDriver *driver,
                  void *hw_context)
{
    if (!stepper)
        return;

    stepper->stepper_id = stepper_id;
    stepper->driver = driver;
    stepper->hw_context = hw_context;

    stepper->position = 0;
    stepper->steps_remaining = 0;
    stepper->direction = true;

    stepper->us_per_step = 1000;   /* default: 1 ms per step */
    stepper->us_accumulator = 0;

    stepper->done_cb = 0;
    
    /* Initialize limit switch support */
    stepper->limits_enabled = false;
    stepper->limit_hit = false;
    stepper->limit_cb = 0;

    /* Driver-specific initialization hook */
    if (stepper->driver && stepper->driver->init)
        stepper->driver->init(stepper);
}

void stepper_enable(Stepper *stepper, bool enable)
{
    if (!stepper || !stepper->driver)
        return;

    if (stepper->driver->set_enable)
        stepper->driver->set_enable(stepper, enable);
}

void stepper_set_speed(Stepper *stepper, uint32_t us_per_step)
{
    if (!stepper)
        return;

    if (us_per_step == 0)
        us_per_step = 1;

    stepper->us_per_step = us_per_step;
}

uint32_t stepper_get_speed(const Stepper *stepper)
{
    if (!stepper)
        return 0;

    return stepper->us_per_step;
}

void stepper_set_steps(Stepper *stepper, int32_t steps)
{
    if (!stepper)
        return;

    stepper->steps_remaining = steps;
    stepper->us_accumulator = 0;

    bool dir = (steps >= 0);
    stepper->direction = dir;

    if (stepper->driver && stepper->driver->set_dir)
        stepper->driver->set_dir(stepper, dir);
}

int32_t stepper_get_steps(const Stepper *stepper)
{
    if (!stepper)
        return 0;

    return stepper->steps_remaining;
}

void stepper_set_done_callback(Stepper *stepper,
                               StepperDoneCallback cb)
{
    if (!stepper)
        return;

    stepper->done_cb = cb;
}

/* -------------------------------------------------------------------------- */
/*                         Time-Based Step Processing                          */
/* -------------------------------------------------------------------------- */

/*
 * stepper_update()
 *
 * Call periodically (task loop or timer ISR) with elapsed microseconds.
 * Returns true if a step pulse was issued.
 */
bool stepper_update(Stepper *stepper, uint32_t delta_us)
{
    if (!stepper)
        return false;

    if (stepper->steps_remaining == 0)
        return false;

    stepper->us_accumulator += delta_us;

    if (stepper->us_accumulator < stepper->us_per_step)
        return false;

    stepper->us_accumulator -= stepper->us_per_step;

    /* Issue one step */
    if (stepper->driver && stepper->driver->step_pulse)
        stepper->driver->step_pulse(stepper);

    if (stepper->steps_remaining > 0)
    {
        stepper->steps_remaining--;
        stepper->position++;
    }
    else
    {
        stepper->steps_remaining++;
        stepper->position--;
    }

    /* Notify completion */
    if (stepper->steps_remaining == 0 && stepper->done_cb)
        stepper->done_cb(stepper);

    return true;
}

/* -------------------------------------------------------------------------- */
/*                        Position Control Functions                          */
/* -------------------------------------------------------------------------- */

void stepper_move_to_position(Stepper *stepper, int32_t position)
{
    if (!stepper || !stepper->driver)
        return;

    if (stepper->driver->move_to)
        stepper->driver->move_to(stepper, position);
}

int32_t stepper_get_position(Stepper *stepper)
{
    if (!stepper || !stepper->driver)
        return 0;

    if (stepper->driver->get_position)
        return stepper->driver->get_position(stepper);

    /* Fallback to internal position if driver doesn't implement get_position */
    return stepper->position;
}

bool stepper_position_reached(Stepper *stepper)
{
    if (!stepper || !stepper->driver)
        return true;

    if (stepper->driver->position_reached)
        return stepper->driver->position_reached(stepper);

    /* Fallback: assume reached if no steps remaining */
    return (stepper->steps_remaining == 0);
}

/* -------------------------------------------------------------------------- */
/*                            Stepper Group API                                */
/* -------------------------------------------------------------------------- */

void stepper_group_init(StepperGroup *group)
{
    if (!group)
        return;

    group->count = 0;
}

bool stepper_group_add(StepperGroup *group, Stepper *stepper)
{
    if (!group || !stepper)
        return false;

    if (group->count >= STEPPER_GROUP_MAX)
        return false;

    group->steppers[group->count++] = stepper;
    return true;
}

void stepper_group_enable(StepperGroup *group, bool enable)
{
    if (!group)
        return;

    for (uint8_t i = 0; i < group->count; i++)
        stepper_enable(group->steppers[i], enable);
}

void stepper_group_set_steps(StepperGroup *group, int32_t steps)
{
    if (!group)
        return;

    for (uint8_t i = 0; i < group->count; i++)
        stepper_set_steps(group->steppers[i], steps);
}

void stepper_group_set_speed(StepperGroup *group, uint32_t us_per_step)
{
    if (!group)
        return;

    for (uint8_t i = 0; i < group->count; i++)
        stepper_set_speed(group->steppers[i], us_per_step);
}

void stepper_group_move_by(StepperGroup *group, int32_t steps)
{
    if (!group)
        return;
    for (uint8_t i = 0; i < group->count; i++) {
        Stepper *stepper = group->steppers[i];
        int32_t currentPos = stepper_get_position(stepper);
        int32_t targetPos = currentPos + steps;
        stepper_move_to_position(stepper, targetPos);
    }
}

/*
 * Returns true if ANY stepper in the group stepped.
 */
bool stepper_group_update(StepperGroup *group, uint32_t delta_us)
{
    if (!group)
        return false;

    bool stepped = false;

    for (uint8_t i = 0; i < group->count; i++)
        stepped |= stepper_update(group->steppers[i], delta_us);

    return stepped;
}

/* -------------------------------------------------------------------------- */
/*                        Generalized Stepper API                             */
/* -------------------------------------------------------------------------- */

void Stepper_init(void *context)
{
    /* Generic initialization - context could be config structure */
    /* In this implementation, initialization is handled by stepper_config_init() */
    (void)context;  /* Unused in this implementation */
}

void Stepper_setAcceleration(volatile Stepper *s, float a)
{
    if (!s)
        return;
    
    /* Convert float acceleration to driver-specific units */
    /* For TMC5240, typical acceleration range is 0-65535 */
    uint32_t accel = (uint32_t)a;
    
    /* Forward to driver-specific implementation */
    stepper_config_set_acceleration((Stepper *)s, accel);
}

bool Stepper_isMoving(volatile Stepper *s)
{
    if (!s)
        return false;
    
    /* Check if position has been reached (inverse of is_moving) */
    return !stepper_position_reached((Stepper *)s);
}

void Stepper_move(volatile Stepper *s, int32_t position)
{
    if (!s)
        return;
    
    /* Move to absolute position */
    stepper_move_to_position((Stepper *)s, position);
}

void Stepper_stop(volatile Stepper *s)
{
    if (!s)
        return;
    
    /* Stop motor motion */
    stepper_config_stop((Stepper *)s);
}

void Stepper_start(volatile Stepper *s)
{
    if (!s)
        return;
    
    /* Enable motor */
    stepper_enable((Stepper *)s, true);
}

void Stepper_awaitStop(volatile Stepper *s, uint32_t timeout)
{
    if (!s)
        return;
    
    uint32_t elapsed = 0;
    const uint32_t poll_interval = 10;  /* 10ms polling interval */
    
    /* Wait until motor stops or timeout */
    while (Stepper_isMoving(s))
    {
        /* Simple delay - in real implementation, use HAL_Delay or similar */
        for (volatile uint32_t i = 0; i < 10000; i++);
        
        if (timeout > 0)
        {
            elapsed += poll_interval;
            if (elapsed >= timeout)
                break;
        }
    }
}

bool Stepper_awaitLimit(volatile Stepper *s, uint32_t timeout)
{
    if (!s)
        return false;
    
    uint32_t elapsed = 0;
    const uint32_t poll_interval = 10;  /* 10ms polling interval */
    
    /* Wait until limit is hit or timeout */
    while (!((Stepper *)s)->limit_hit)
    {
        /* Simple delay */
        for (volatile uint32_t i = 0; i < 10000; i++);
        
        if (timeout > 0)
        {
            elapsed += poll_interval;
            if (elapsed >= timeout)
                return false;
        }
    }
    
    return true;
}

void Stepper_enableLimits(volatile Stepper *s)
{
    if (!s)
        return;
    
    ((Stepper *)s)->limits_enabled = true;
}

void Stepper_hitLimit(volatile Stepper *s, void *sw)
{
    if (!s)
        return;
    
    Stepper *stepper = (Stepper *)s;
    
    if (stepper->limits_enabled)
    {
        stepper->limit_hit = true;
        
        /* Stop motor when limit is hit */
        Stepper_stop(s);
        
        /* Call limit callback if registered */
        if (stepper->limit_cb)
            stepper->limit_cb(stepper, sw);
    }
}

void Stepper_disable(volatile Stepper *s)
{
    if (!s)
        return;
    
    /* Disable motor */
    stepper_enable((Stepper *)s, false);
}

int32_t *Stepper_positions(void)
{
    /* Get count of steppers in system */
    uint8_t count = stepper_config_get_count();
    
    /* Update positions array */
    for (uint8_t i = 0; i < count && i < 4; i++)
    {
        Stepper *stepper = stepper_config_get_stepper(i);
        if (stepper)
            stepper_positions_array[i] = stepper_get_position(stepper);
        else
            stepper_positions_array[i] = 0;
    }
    
    return stepper_positions_array;
}
