# smart_door_lock-using-psoc6
A secure embedded smart door lock system using PSoC 6, featuring keypad-based PIN authentication, PIN changing, TM1637 display feedback, LED access indication, UART monitoring, and a security lockdown mechanism after multiple incorrect attempts. Developed using modular C and ModusToolbox.
# Smart Door Lock System using PSoC 6

An embedded systems project implementing a secure electronic door lock using the **PSoC 6 CY8CPROTO-062-4343W** development board. The system uses a 4×4 matrix keypad for PIN-based authentication, a TM1637 seven-segment display for user feedback, and an LED to indicate access status.

The firmware is developed in modular C using **ModusToolbox** and includes password verification, configurable PIN changing, failed-attempt monitoring, and a security lockdown mechanism.

## Features

- **PIN-Based Authentication:** Unlock access using a four-digit password.
- **Matrix Keypad Interface:** Input passwords using a 4×4 keypad.
- **Display Feedback:** TM1637 seven-segment display indicates the number of digits entered.
- **Access Indicator:** LED activates for five seconds following successful authentication.
- **Failed Attempt Detection:** Tracks incorrect password attempts.
- **Security Lockdown:** Triggers a 30-second lockdown after three consecutive incorrect attempts, accompanied by LED blinking.
- **PIN Change Functionality:** Allows users to change their PIN after verifying the current password and confirming the new one.
- **UART Debugging:** Serial communication provides system status and authentication feedback.
- **Modular Firmware Architecture:** Separate modules for keypad scanning and display control.

## Hardware

- PSoC 6 CY8CPROTO-062-4343W
- 4×4 Matrix Keypad (HC-35S)
- TM1637 Four-Digit Seven-Segment Display
- LED (Access Indicator)
- USB-UART Interface

## Software & Technologies

- C Programming
- ModusToolbox
- PSoC 6 HAL
- UART Communication
- GPIO Interfacing
- Embedded Systems
- Finite State Machine (FSM)

## System Operation

1. The user enters a four-digit PIN using the matrix keypad.
2. The entered digit count is displayed on the TM1637 display.
3. The system compares the entered PIN with the stored PIN.
4. On successful authentication, access is granted and the indicator LED turns on for five seconds.
5. Incorrect attempts are counted, and three consecutive failures trigger a 30-second security lockdown.
6. Users can change their PIN through a dedicated password-change sequence involving current PIN verification, new PIN entry, and confirmation.

The project demonstrates embedded firmware development, peripheral interfacing, modular programming, state-machine implementation, and basic security mechanisms on a microcontroller platform.
