/*******************************************************************************
* File Name: matrix_keypad.c
* Description: Driver for HC-35S 4x4 matrix keypad.
*******************************************************************************/

#include "matrix_keypad.h"

/* Row pins */
static const cyhal_gpio_t row_pins[4] =
{
    KEYPAD_R1,
    KEYPAD_R2,
    KEYPAD_R3,
    KEYPAD_R4
};

/* Column pins */
static const cyhal_gpio_t col_pins[4] =
{
    KEYPAD_C1,
    KEYPAD_C2,
    KEYPAD_C3,
    KEYPAD_C4
};

/*
 * Key mapping table.
 * This is the standard 4x4 mapping and can be modified
 * after confirming the physical keypad mapping.
 */
static const char key_map[4][4] =
{
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};


/*******************************************************************************
* Function Name: keypad_init
* Description: Initializes keypad GPIO pins.
*******************************************************************************/
void keypad_init(void)
{
    /* Initialize row pins as outputs */
    for (uint8_t i = 0; i < 4; i++)
    {
        cyhal_gpio_init(
            row_pins[i],
            CYHAL_GPIO_DIR_OUTPUT,
            CYHAL_GPIO_DRIVE_STRONG,
            true
        );
    }

    /* Initialize column pins as inputs with pull-ups */
    for (uint8_t i = 0; i < 4; i++)
    {
        cyhal_gpio_init(
            col_pins[i],
            CYHAL_GPIO_DIR_INPUT,
            CYHAL_GPIO_DRIVE_PULLUP,
            true
        );
    }
}


/*******************************************************************************
* Function Name: keypad_scan
* Description: Scans keypad and returns pressed key.
* Return: Character corresponding to pressed key, or '\0' if no key pressed.
*******************************************************************************/
char keypad_scan(void)
{
    char pressed_key = '\0';

    /* Scan each row */
    for (uint8_t row = 0; row < 4; row++)
    {
        /* Set all rows HIGH */
        for (uint8_t i = 0; i < 4; i++)
        {
            cyhal_gpio_write(row_pins[i], true);
        }

        /* Select current row */
        cyhal_gpio_write(row_pins[row], false);

        /* Small settling delay */
        cyhal_system_delay_ms(1);

        /* Check all columns */
        for (uint8_t col = 0; col < 4; col++)
        {
            if (cyhal_gpio_read(col_pins[col]) == false)
            {
                /* Debounce delay */
                cyhal_system_delay_ms(20);

                /* Confirm key is still pressed */
                if (cyhal_gpio_read(col_pins[col]) == false)
                {
                    pressed_key = key_map[row][col];

                    /* Wait until key is released */
                    while (cyhal_gpio_read(col_pins[col]) == false)
                    {
                        cyhal_system_delay_ms(10);
                    }

                    /* Release all rows */
                    for (uint8_t i = 0; i < 4; i++)
                    {
                        cyhal_gpio_write(row_pins[i], true);
                    }

                    return pressed_key;
                }
            }
        }
    }

    /* Release all rows */
    for (uint8_t i = 0; i < 4; i++)
    {
        cyhal_gpio_write(row_pins[i], true);
    }

    return '\0';
}