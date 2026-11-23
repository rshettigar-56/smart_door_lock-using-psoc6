/*******************************************************************************
* File Name: tm1637.c
* Description: TM1637 4-digit 7-segment display driver implementation.
*******************************************************************************/

#include "tm1637.h"

static void tm1637_start(void)
{
    cyhal_gpio_write(TM1637_CLK_PIN, true);
    cyhal_gpio_write(TM1637_DIO_PIN, true);
    cyhal_system_delay_us(5);
    cyhal_gpio_write(TM1637_DIO_PIN, false);
    cyhal_system_delay_us(5);
    cyhal_gpio_write(TM1637_CLK_PIN, false);
    cyhal_system_delay_us(5);
}

static void tm1637_stop(void)
{
    cyhal_gpio_write(TM1637_CLK_PIN, false);
    cyhal_gpio_write(TM1637_DIO_PIN, false);
    cyhal_system_delay_us(5);
    cyhal_gpio_write(TM1637_CLK_PIN, true);
    cyhal_system_delay_us(5);
    cyhal_gpio_write(TM1637_DIO_PIN, true);
    cyhal_system_delay_us(5);
}

static void tm1637_write_byte(uint8_t data)
{
    for (int i = 0; i < 8; i++)
    {
        cyhal_gpio_write(TM1637_CLK_PIN, false);
        cyhal_system_delay_us(5);
        cyhal_gpio_write(TM1637_DIO_PIN, (data & (1 << i)) ? true : false);
        cyhal_system_delay_us(5);
        cyhal_gpio_write(TM1637_CLK_PIN, true);
        cyhal_system_delay_us(5);
    }

    /* Wait for ACK cycle */
    cyhal_gpio_write(TM1637_CLK_PIN, false);
    cyhal_gpio_write(TM1637_DIO_PIN, true);
    cyhal_system_delay_us(5);
    cyhal_gpio_write(TM1637_CLK_PIN, true);
    cyhal_system_delay_us(5);
    cyhal_gpio_write(TM1637_CLK_PIN, false);
}

void tm1637_init(void)
{
    cyhal_gpio_init(TM1637_CLK_PIN, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_OPENDRAINDRIVESLOW, true);
    cyhal_gpio_init(TM1637_DIO_PIN, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_OPENDRAINDRIVESLOW, true);
}

void tm1637_update_display(uint8_t dashes_count)
{
    uint8_t segs[4] = {SEG_BLANK, SEG_BLANK, SEG_BLANK, SEG_BLANK};
    for (int i = 0; i < dashes_count && i < 4; i++)
    {
        segs[i] = SEG_DASH;
    }

    tm1637_start();
    tm1637_write_byte(TM1637_CMD_DATA_AUTO);
    tm1637_stop();

    tm1637_start();
    tm1637_write_byte(TM1637_CMD_ADDR_START);
    for (int i = 0; i < 4; i++)
    {
        tm1637_write_byte(segs[i]);
    }
    tm1637_stop();

    tm1637_start();
    tm1637_write_byte(TM1637_CMD_DISP_ON);
    tm1637_stop();
}