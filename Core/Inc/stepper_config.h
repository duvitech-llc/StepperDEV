#ifndef STEPPER_CONFIG_H
#define STEPPER_CONFIG_H

#include "stepper.h"

/* Logical stepper IDs for this product */
typedef enum
{
    STEPPER_0 = 0,
    STEPPER_1,
    STEPPER_COUNT
} StepperId;

/* Initialize all steppers for this product */
void stepper_config_init(void);

/* Accessors */
Stepper *stepper_config_get_stepper(StepperId id);
StepperGroup *stepper_config_get_group(void);

/* Debug: print driver registers for a stepper (if supported) */
void stepper_config_print_registers(Stepper *stepper);

#endif /* STEPPER_CONFIG_H */
