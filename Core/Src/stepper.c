#include "stepper.h"

#include <stdio.h>

/* ============================================================================
 *  Internal Helpers
 * ========================================================================== */

static inline bool stepper_driver_has(const Stepper *s, uint32_t cap)
{
    return (s->driver && (s->driver->caps & cap));
}

/* ============================================================================
 *  Low-Level Stepper API
 * ========================================================================== */

void stepper_init(Stepper *s,
                  uint8_t stepper_id,
                  const StepperDriver *driver,
                  void *hw_context)
{
    if (!s || !driver)
        return;

    s->stepper_id = stepper_id;
    s->driver = driver;
    s->hw_context = hw_context;

    s->target_position = 0;
    s->steps_remaining = 0;
    s->direction = false;

    s->us_per_step = 0;
    s->us_accumulator = 0;

    s->enabled = false;
    s->busy = false;

    s->done_cb = NULL;

    s->limits_enabled = false;
    s->limit_hit = false;
    s->limit_cb = NULL;

    if (driver->init)
        driver->init(s);
}

void stepper_enable(Stepper *s, bool enable)
{
    if (!s || !s->driver || !s->driver->set_enable)
        return;

    s->enabled = enable;
    s->driver->set_enable(s, enable);
}

void stepper_set_speed(Stepper *s, uint32_t us_per_step)
{
    if (!s)
        return;

    s->us_per_step = us_per_step;
}

uint32_t stepper_get_speed(const Stepper *s)
{
    return s ? s->us_per_step : 0;
}

int32_t stepper_get_position(Stepper *s)
{
    if (!s || !s->driver || !s->driver->get_position)
        return 0;

    return s->driver->get_position(s);
}

bool stepper_position_reached(Stepper *s)
{
    if (!s)
        return true;

    if (stepper_driver_has(s, STEPPER_CAP_MOVE_TO))
    {
        if (s->driver->position_reached)
            return s->driver->position_reached(s);
        return true;
    }

    return (s->steps_remaining == 0);
}

void stepper_set_done_callback(Stepper *s, StepperDoneCallback cb)
{
    if (!s)
        return;

    s->done_cb = cb;
}

void stepper_move_to_position(Stepper *s, int32_t position)
{
    if (!s || !s->driver)
        return;

    s->target_position = position;
    s->busy = true;
    s->limit_hit = false;

    /* Smart driver path */
    if (stepper_driver_has(s, STEPPER_CAP_MOVE_TO))
    {
        s->driver->move_to(s, position);
        return;
    }

    /* STEP/DIR fallback */
    if (!stepper_driver_has(s, STEPPER_CAP_STEP_DIR))
        return;

    int32_t current = stepper_get_position(s);
    int32_t delta = position - current;

    s->direction = (delta >= 0);
    s->steps_remaining = (delta >= 0) ? delta : -delta;

    if (s->driver->set_dir)
        s->driver->set_dir(s, s->direction);
}

bool stepper_update(Stepper *s, uint32_t delta_us)
{
    if (!s || !s->enabled || !s->busy)
        return false;

    /* Smart driver completion */
    if (stepper_driver_has(s, STEPPER_CAP_MOVE_TO))
    {
        if (s->driver->position_reached &&
            s->driver->position_reached(s))
        {
            s->busy = false;
            if (s->done_cb)
                s->done_cb(s);
        }
        return s->busy;
    }

    /* STEP/DIR stepping */
    if (!stepper_driver_has(s, STEPPER_CAP_STEP_DIR))
        return false;

    s->us_accumulator += delta_us;

    if (s->us_accumulator < s->us_per_step)
        return true;

    s->us_accumulator -= s->us_per_step;

    if (s->steps_remaining > 0)
    {
        s->driver->step_pulse(s);
        s->steps_remaining--;
    }

    if (s->steps_remaining == 0)
    {
        s->busy = false;
        if (s->done_cb)
            s->done_cb(s);
    }

    return s->busy;
}

/* ============================================================================
 *  High-Level Stepper API (Application Layer)
 * ========================================================================== */

void Stepper_init(void *context)
{
    Stepper *s = (Stepper *)context;
    if (!s)
        return;

    stepper_enable(s, true);
}

void Stepper_setAcceleration(volatile Stepper *s, float accel)
{
    /* Acceleration planning intentionally left driver- or app-specific */
    (void)s;
    (void)accel;
}

bool Stepper_isMoving(volatile Stepper *s)
{
    return s ? s->busy : false;
}

void Stepper_start(volatile Stepper *s)
{
    if (!s)
        return;

    stepper_enable((Stepper *)s, true);
}

void Stepper_stop(volatile Stepper *s)
{
    if (!s)
        return;

    s->busy = false;
    s->steps_remaining = 0;
}

void Stepper_disable(volatile Stepper *s)
{
    if (!s)
        return;

    stepper_enable((Stepper *)s, false);
}

void Stepper_move(volatile Stepper *s, int32_t position)
{
    if (!s)
        return;

    stepper_move_to_position((Stepper *)s, position);
}

void Stepper_awaitStop(volatile Stepper *s, uint32_t timeout_ms)
{
    //uint32_t start = 0; // HAL_GetTick();
    
    //while (s->busy) {
    //    stepper_update((Stepper*)s, 1000); // Update with 1ms delta
    //    
    //    if (timeout_ms && (HAL_GetTick() - start) >= timeout_ms)
    //        break;
    //        
    //    // HAL_Delay(1);
    //}
}

void Stepper_enableLimits(volatile Stepper *s)
{
    if (!s)
        return;

    s->limits_enabled = true;
    s->limit_hit = false;
}

void Stepper_hitLimit(volatile Stepper *s, void *sw)
{
    if (!s || !s->limits_enabled)
        return;

    s->limit_hit = true;
    s->busy = false;

    if (s->limit_cb)
        s->limit_cb((Stepper *)s, sw);
}

bool Stepper_awaitLimit(volatile Stepper *s, uint32_t timeout_ms)
{
    if (!s)
        return false;

    uint32_t elapsed = 0;

    while (!s->limit_hit)
    {
        if (timeout_ms && elapsed >= timeout_ms)
            return false;
        elapsed++;
    }

    return true;
}

int32_t *Stepper_positions(void)
{
    /* Application-defined global storage */
    static int32_t positions[STEPPER_GROUP_MAX];
    return positions;
}

/* ============================================================================
 *  Stepper Group API
 * ========================================================================== */

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

void stepper_group_move_to(StepperGroup *group, int32_t position)
{
    if (!group)
        return;

    for (uint8_t i = 0; i < group->count; i++)
        stepper_move_to_position(group->steppers[i], position);
}

bool stepper_group_update(StepperGroup *group, uint32_t delta_us)
{
    if (!group)
        return false;

    bool any_busy = false;

    for (uint8_t i = 0; i < group->count; i++)
        any_busy |= stepper_update(group->steppers[i], delta_us);

    return any_busy;
}
