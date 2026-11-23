/*******************************************************************************
* File Name: tm1637.h
* Description: Header file for TM1637 4-digit 7-segment display driver.
*******************************************************************************/

#ifndef TM1637_H
#define TM1637_H

#include "cyhal.h"
#include "cybsp.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* TM1637 Pin Definitions */
#define TM1637_CLK_PIN        (P9_0)
#define TM1637_DIO_PIN        (P9_1)

/* TM1637 Protocol Commands */
#define TM1637_CMD_DATA_AUTO  (0x40)
#define TM1637_CMD_ADDR_START (0xC0)
#define TM1637_CMD_DISP_ON    (0x88 | 0x07) /* Display ON, Max Brightness */

/* 7-Segment Character Masks */
#define SEG_DASH              (0x40)
#define SEG_BLANK             (0x00)

/* Function Prototypes */
void tm1637_init(void);
void tm1637_update_display(uint8_t dashes_count);

#ifdef __cplusplus
}
#endif

#endif /* TM1637_H */