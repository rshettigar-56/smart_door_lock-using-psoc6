
#ifndef MATRIX_KEYPAD_H
#define MATRIX_KEYPAD_H

#include "cyhal.h"
#include "cybsp.h"

/*******************************************************************************
 * HC-35S Matrix Keypad Pin Configuration
 ******************************************************************************/

/* Row pins */
#define KEYPAD_R1      P10_0
#define KEYPAD_R2      P10_1
#define KEYPAD_R3      P10_2
#define KEYPAD_R4      P10_3

/* Column pins */
#define KEYPAD_C1      P10_4
#define KEYPAD_C2      P10_5
#define KEYPAD_C3      P9_6
#define KEYPAD_C4      P9_7

/*******************************************************************************
 * Function Prototypes
 ******************************************************************************/

/* Initialize keypad GPIO pins */
void keypad_init(void);

/* Scan keypad and return detected character */
char keypad_scan(void);

#endif /* MATRIX_KEYPAD_H */
