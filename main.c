/*******************************************************************************
* File Name: main.c
* Description: Smart Door Lock - Keypad, TM1637, Access LED, UART and PIN Change
*******************************************************************************/

#include "cy_pdl.h"
#include "cyhal.h"
#include "cybsp.h"

#include "matrix_keypad.h"
#include "tm1637.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>

/*******************************************************************************
* Configuration
*******************************************************************************/

#define PIN_LENGTH                  4
#define MAX_ATTEMPTS                3

#define ACCESS_LED_PIN              (P9_2)

#define LED_ON_TIME_MS              5000

#define LOCKDOWN_TIME_MS            30000
#define LED_BLINK_INTERVAL_MS       250

#define UART_BAUD_RATE              115200

/* Keypad debounce timing */
#define KEY_DEBOUNCE_DELAY_MS       50

/*******************************************************************************
* PIN Configuration
*******************************************************************************/

/* Default PIN */
static char correct_pin[PIN_LENGTH + 1] = "1234";

/*******************************************************************************
* Global Variables
*******************************************************************************/

/* Normal door unlock PIN */
static char entered_pin[PIN_LENGTH + 1];

static uint8_t entered_count = 0;
static uint8_t failed_attempts = 0;

/*******************************************************************************
* Password Change Buffers
*******************************************************************************/

static char current_pin_entry[PIN_LENGTH + 1];
static char new_pin[PIN_LENGTH + 1];
static char confirm_pin[PIN_LENGTH + 1];

static uint8_t password_entry_count = 0;

/*******************************************************************************
* Password Change State
*******************************************************************************/

typedef enum
{
    PIN_MODE_NORMAL,
    PIN_MODE_VERIFY_CURRENT,
    PIN_MODE_ENTER_NEW,
    PIN_MODE_CONFIRM_NEW

} pin_mode_t;

static pin_mode_t pin_mode = PIN_MODE_NORMAL;

/*******************************************************************************
* UART Configuration
*******************************************************************************/

static cyhal_uart_t console_uart;

/*******************************************************************************
* Function Prototypes
*******************************************************************************/

static cy_rslt_t uart_init_console(void);
static void uart_printf(const char *format, ...);

static void clear_pin(void);
static void clear_password_buffers(void);
static void clear_password_entry(void);

static bool verify_pin(void);
static bool verify_current_pin(void);

static void access_granted(void);
static void access_denied(void);
static void security_alert(void);

static void led_on(void);
static void led_off(void);

static void start_password_change(void);
static void cancel_password_change(void);
static void process_password_change(char key);

/*******************************************************************************
* UART Initialization
*******************************************************************************/

static cy_rslt_t uart_init_console(void)
{
    const cyhal_uart_cfg_t uart_config =
    {
        .data_bits = 8,
        .stop_bits = 1,
        .parity = CYHAL_UART_PARITY_NONE,
        .rx_buffer = NULL,
        .rx_buffer_size = 0
    };

    cy_rslt_t result;

    result = cyhal_uart_init(
        &console_uart,
        CYBSP_DEBUG_UART_TX,
        CYBSP_DEBUG_UART_RX,
        CYBSP_DEBUG_UART_CTS,
        CYBSP_DEBUG_UART_RTS,
        NULL,
        &uart_config
    );

    if (result != CY_RSLT_SUCCESS)
    {
        return result;
    }

    result = cyhal_uart_set_baud(
        &console_uart,
        UART_BAUD_RATE,
        NULL
    );

    if (result != CY_RSLT_SUCCESS)
    {
        cyhal_uart_free(&console_uart);
        return result;
    }

    return CY_RSLT_SUCCESS;
}

/*******************************************************************************
* UART Print Function
*******************************************************************************/

static void uart_printf(const char *format, ...)
{
    char buffer[256];

    va_list args;

    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    for (size_t i = 0; buffer[i] != '\0'; i++)
    {
        cyhal_uart_putc(&console_uart, (uint32_t)buffer[i]);
    }
}

/*******************************************************************************
* LED Functions
*******************************************************************************/

/* LED is active LOW */

static void led_on(void)
{
    cyhal_gpio_write(ACCESS_LED_PIN, false);
}

static void led_off(void)
{
    cyhal_gpio_write(ACCESS_LED_PIN, true);
}

/*******************************************************************************
* Clear Normal PIN Entry
*******************************************************************************/

static void clear_pin(void)
{
    entered_count = 0;

    for (uint8_t i = 0; i <= PIN_LENGTH; i++)
    {
        entered_pin[i] = '\0';
    }

    tm1637_update_display(0);
}

/*******************************************************************************
* Clear All Password Change Buffers
*******************************************************************************/

static void clear_password_buffers(void)
{
    for (uint8_t i = 0; i <= PIN_LENGTH; i++)
    {
        current_pin_entry[i] = '\0';
        new_pin[i] = '\0';
        confirm_pin[i] = '\0';
    }

    password_entry_count = 0;

    tm1637_update_display(0);
}

/*******************************************************************************
* Clear Password Entry Counter and Temporary Buffers
*
* IMPORTANT:
* This function does NOT clear new_pin.
*******************************************************************************/

static void clear_password_entry(void)
{
    password_entry_count = 0;

    for (uint8_t i = 0; i <= PIN_LENGTH; i++)
    {
        current_pin_entry[i] = '\0';
        confirm_pin[i] = '\0';
    }

    tm1637_update_display(0);
}

/*******************************************************************************
* PIN Verification - Normal Mode
*******************************************************************************/

static bool verify_pin(void)
{
    for (uint8_t i = 0; i < PIN_LENGTH; i++)
    {
        if (entered_pin[i] != correct_pin[i])
        {
            return false;
        }
    }

    return true;
}

/*******************************************************************************
* Current PIN Verification - Password Change
*******************************************************************************/

static bool verify_current_pin(void)
{
    for (uint8_t i = 0; i < PIN_LENGTH; i++)
    {
        if (current_pin_entry[i] != correct_pin[i])
        {
            return false;
        }
    }

    return true;
}

/*******************************************************************************
* Access Granted
*******************************************************************************/

static void access_granted(void)
{
    failed_attempts = 0;

    uart_printf("\r\n");
    uart_printf("[ACCESS GRANTED] Correct PIN entered.\r\n");
    uart_printf("[SECURITY] Door unlocked.\r\n");

    led_on();

    cyhal_system_delay_ms(LED_ON_TIME_MS);

    led_off();

    uart_printf("[SECURITY] Door locked again.\r\n");
}

/*******************************************************************************
* Access Denied
*******************************************************************************/

static void access_denied(void)
{
    failed_attempts++;

    uart_printf("\r\n");
    uart_printf("[ACCESS DENIED] Incorrect PIN entered.\r\n");

    uart_printf(
        "[SECURITY] Failed attempts: %u/%u\r\n",
        failed_attempts,
        MAX_ATTEMPTS
    );

    if (failed_attempts < MAX_ATTEMPTS)
    {
        led_off();

        uart_printf("[SECURITY] Try again.\r\n");
    }

    if (failed_attempts >= MAX_ATTEMPTS)
    {
        security_alert();
    }
}

/*******************************************************************************
* Security Alert / Lockdown
*******************************************************************************/

static void security_alert(void)
{
    uint32_t elapsed_time = 0;

    uart_printf("\r\n");
    uart_printf("========================================\r\n");
    uart_printf("[SECURITY ALERT]\r\n");
    uart_printf("[SECURITY] Maximum incorrect attempts reached!\r\n");
    uart_printf("[SECURITY] Lockdown activated.\r\n");
    uart_printf("[SECURITY] Duration: 30 seconds.\r\n");
    uart_printf("========================================\r\n");

    pin_mode = PIN_MODE_NORMAL;

    clear_password_buffers();
    clear_pin();

    while (elapsed_time < LOCKDOWN_TIME_MS)
    {
        led_on();

        cyhal_system_delay_ms(LED_BLINK_INTERVAL_MS);

        led_off();

        cyhal_system_delay_ms(LED_BLINK_INTERVAL_MS);

        elapsed_time += (LED_BLINK_INTERVAL_MS * 2);
    }

    led_off();

    uart_printf("[SECURITY] Lockdown ended.\r\n");
    uart_printf("[SECURITY] System ready for new PIN entry.\r\n");

    failed_attempts = 0;

    /* Consume a key held during lockdown */
    (void)keypad_scan();

    clear_pin();
}

/*******************************************************************************
* Start Password Change
*******************************************************************************/

static void start_password_change(void)
{
    pin_mode = PIN_MODE_VERIFY_CURRENT;

    clear_pin();
    clear_password_buffers();

    uart_printf("\r\n");
    uart_printf("[PIN CHANGE] Password change initiated.\r\n");
    uart_printf("[PIN CHANGE] Enter current PIN.\r\n");
}

/*******************************************************************************
* Cancel Password Change
*******************************************************************************/

static void cancel_password_change(void)
{
    pin_mode = PIN_MODE_NORMAL;

    clear_pin();
    clear_password_buffers();

    uart_printf("[PIN CHANGE] Operation cancelled.\r\n");
    uart_printf("[SYSTEM] Enter your PIN.\r\n");
}

/*******************************************************************************
* Password Change Processing
*******************************************************************************/

static void process_password_change(char key)
{
    /* Cancel password change */

    if (key == '#')
    {
        cancel_password_change();
        return;
    }

    /* Clear current entry */

    if (key == '*')
    {
        clear_password_entry();

        uart_printf("[PIN CHANGE] Current entry cleared.\r\n");

        return;
    }

    /* Numeric input */

    if (key >= '0' && key <= '9')
    {
        if (password_entry_count >= PIN_LENGTH)
        {
            return;
        }

        /***********************************************************************
        * VERIFY CURRENT PIN
        ***********************************************************************/

        if (pin_mode == PIN_MODE_VERIFY_CURRENT)
        {
            current_pin_entry[password_entry_count] = key;

            password_entry_count++;

            current_pin_entry[password_entry_count] = '\0';

            tm1637_update_display(password_entry_count);

            if (password_entry_count == PIN_LENGTH)
            {
                tm1637_update_display(0);

                if (verify_current_pin())
                {
                    uart_printf("[PIN CHANGE] Current PIN verified.\r\n");
                    uart_printf("[PIN CHANGE] Enter new 4-digit PIN.\r\n");

                    pin_mode = PIN_MODE_ENTER_NEW;

                    clear_password_entry();
                }
                else
                {
                    uart_printf("[PIN CHANGE] Incorrect current PIN.\r\n");

                    pin_mode = PIN_MODE_NORMAL;

                    clear_password_buffers();
                    clear_pin();

                    access_denied();
                }
            }
        }

        /***********************************************************************
        * ENTER NEW PIN
        ***********************************************************************/

        else if (pin_mode == PIN_MODE_ENTER_NEW)
        {
            new_pin[password_entry_count] = key;

            password_entry_count++;

            new_pin[password_entry_count] = '\0';

            tm1637_update_display(password_entry_count);

            if (password_entry_count == PIN_LENGTH)
            {
                tm1637_update_display(0);

                uart_printf("[PIN CHANGE] New PIN entered.\r\n");
                uart_printf("[PIN CHANGE] Confirm new PIN.\r\n");

                pin_mode = PIN_MODE_CONFIRM_NEW;

                /* Preserve new_pin */
                clear_password_entry();
            }
        }

        /***********************************************************************
        * CONFIRM NEW PIN
        ***********************************************************************/

        else if (pin_mode == PIN_MODE_CONFIRM_NEW)
        {
            confirm_pin[password_entry_count] = key;

            password_entry_count++;

            confirm_pin[password_entry_count] = '\0';

            tm1637_update_display(password_entry_count);

            if (password_entry_count == PIN_LENGTH)
            {
                tm1637_update_display(0);

                bool match = true;

                for (uint8_t i = 0; i < PIN_LENGTH; i++)
                {
                    if (new_pin[i] != confirm_pin[i])
                    {
                        match = false;
                        break;
                    }
                }

                if (match)
                {
                    for (uint8_t i = 0; i < PIN_LENGTH; i++)
                    {
                        correct_pin[i] = new_pin[i];
                    }

                    correct_pin[PIN_LENGTH] = '\0';

                    uart_printf("\r\n");
                    uart_printf("[PIN CHANGE] Password changed successfully!\r\n");
                    uart_printf("[PIN CHANGE] New PIN is now active.\r\n");

                    failed_attempts = 0;

                    pin_mode = PIN_MODE_NORMAL;

                    clear_pin();
                    clear_password_buffers();

                    uart_printf("[SYSTEM] Enter your PIN.\r\n");
                }
                else
                {
                    uart_printf("\r\n");
                    uart_printf("[PIN CHANGE] Confirmation mismatch.\r\n");
                    uart_printf("[PIN CHANGE] Please enter the new PIN again.\r\n");

                    pin_mode = PIN_MODE_ENTER_NEW;

                    clear_password_buffers();
                    clear_pin();
                }
            }
        }
    }
}

/*******************************************************************************
* Main
*******************************************************************************/

int main(void)
{
    cy_rslt_t result;

#if defined(CY_DEVICE_SECURE)

    cyhal_wdt_t wdt_obj;

    result = cyhal_wdt_init(
        &wdt_obj,
        cyhal_wdt_get_max_timeout_ms()
    );

    CY_ASSERT(CY_RSLT_SUCCESS == result);

    cyhal_wdt_free(&wdt_obj);

#endif

    /* Initialize PSoC */

    result = cybsp_init();

    CY_ASSERT(result == CY_RSLT_SUCCESS);

    __enable_irq();

    /* Initialize UART */

    result = uart_init_console();

    CY_ASSERT(result == CY_RSLT_SUCCESS);

    /* Initialize hardware */

    keypad_init();

    tm1637_init();

    cyhal_gpio_init(
        ACCESS_LED_PIN,
        CYHAL_GPIO_DIR_OUTPUT,
        CYHAL_GPIO_DRIVE_STRONG,
        true
    );

    led_off();

    clear_pin();

    /* Startup messages */

    uart_printf("\r\n");
    uart_printf("========================================\r\n");
    uart_printf("       SMART DOOR LOCK SYSTEM\r\n");
    uart_printf("========================================\r\n");

    uart_printf("[SYSTEM] UART initialized successfully.\r\n");
    uart_printf("[SYSTEM] Baud Rate: 115200\r\n");
    uart_printf("[SYSTEM] Keypad initialized.\r\n");
    uart_printf("[SYSTEM] TM1637 display initialized.\r\n");
    uart_printf("[SYSTEM] Access LED initialized.\r\n");

    uart_printf("----------------------------------------\r\n");
    uart_printf("[SYSTEM] Enter your 4-digit PIN.\r\n");
    uart_printf("[SYSTEM] Press A to change PIN.\r\n");
    uart_printf("[SYSTEM] Press * to clear digits.\r\n");
    uart_printf("[SYSTEM] Press # to cancel.\r\n");
    uart_printf("========================================\r\n\r\n");

    /***************************************************************************
    * Main Loop
    ***************************************************************************/

    for (;;)
    {
        /*
         * Debounce delay:
         * Slows down keypad polling to reduce false detections
         * caused by mechanical button bouncing.
         */

        cyhal_system_delay_ms(KEY_DEBOUNCE_DELAY_MS);

        char key = keypad_scan();

        if (key == '\0')
        {
            continue;
        }

        /***********************************************************************
        * Password Change Mode
        ***********************************************************************/

        if (pin_mode != PIN_MODE_NORMAL)
        {
            process_password_change(key);
            continue;
        }

        /***********************************************************************
        * Start Password Change
        ***********************************************************************/

        if (key == 'A')
        {
            if (entered_count == 0)
            {
                start_password_change();
            }
            else
            {
                uart_printf("[INPUT] Clear entered PIN before changing password.\r\n");
            }

            continue;
        }

        /***********************************************************************
        * Numeric Input
        ***********************************************************************/

        if (key >= '0' && key <= '9')
        {
            if (entered_count < PIN_LENGTH)
            {
                entered_pin[entered_count] = key;

                entered_count++;

                entered_pin[entered_count] = '\0';

                tm1637_update_display(entered_count);
            }

            if (entered_count == PIN_LENGTH)
            {
                tm1637_update_display(0);

                if (verify_pin())
                {
                    access_granted();
                }
                else
                {
                    access_denied();
                }

                clear_pin();
            }
        }

        /***********************************************************************
        * Clear Key
        ***********************************************************************/

        else if (key == '*')
        {
            uart_printf("[INPUT] PIN entry cleared.\r\n");

            clear_pin();
        }

        /***********************************************************************
        * Cancel Key
        ***********************************************************************/

        else if (key == '#')
        {
            uart_printf("[INPUT] PIN entry cancelled.\r\n");

            clear_pin();
        }
    }
}