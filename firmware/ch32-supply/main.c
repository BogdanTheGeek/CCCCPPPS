//------------------------------------------------------------------------------
//       Filename: main.c
//------------------------------------------------------------------------------
//       Bogdan Ionescu (c) 2024
//------------------------------------------------------------------------------
//       Purpose : Application entry point
//------------------------------------------------------------------------------
//       Notes : None
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Module includes
//------------------------------------------------------------------------------
#include <inttypes.h>
#include <stdbool.h>

#include "RingBuffer.h"
#include "boost.h"
#include "ch32v003fun.h"
#include "log.h"
#include "rv003usb.h"

//------------------------------------------------------------------------------
// Module constant defines
//------------------------------------------------------------------------------

#define TAG "main"

#define array_size(x) (sizeof(x) / sizeof(x[0]))
//------------------------------------------------------------------------------
// External variables
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// External functions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Module type definitions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Module static variables
//------------------------------------------------------------------------------
static volatile uint32_t s_systickCount = 0;
static RingBuffer_t printRingBuffer = {0};

//------------------------------------------------------------------------------
// Module static function prototypes
//------------------------------------------------------------------------------
static void SysTick_Init(void);
static void WDT_Init(uint16_t reload_val, uint8_t prescaler);
static void WDT_Pet(void);

//------------------------------------------------------------------------------
// Module externally exported functions
//------------------------------------------------------------------------------

/**
 * @brief  Application entry point
 * @param  None
 * @return None
 */
int main(void)
{
    SystemInit();

#if FUNCONF_USE_DEBUGPRINTF
    WaitForDebuggerToAttach();
#endif

    SysTick_Init();

#ifdef CONFIG_USE_USB
    static volatile uint8_t printBuffer[64] = {0};
    (void)RingBuffer_Init(&printRingBuffer, (uint8_t *)printBuffer, array_size(printBuffer));
    usb_setup();
#endif

    LOG_Init(eLOG_LEVEL_DEBUG, (uint32_t *)&s_systickCount);

    BoostPWM_Init();

#if 0

    for (size_t i = 1000; i < 5001; i += 200)
    {
        BoostPWM_SetVoltageTarget(i);
        LOGD(TAG, "Target: %dmV", i);
        Delay_Ms(100);
    }
#endif

    WDT_Init(0x0FFF, IWDG_Prescaler_128);

    static uint32_t voltage = 0;
    static uint32_t current = 0;
    static uint32_t power = 0;
    static uint32_t voltageTarget = 0;
    static uint32_t currentLimit = 0;
    static bool ccMode = false;

    while (1)
    {
        WDT_Pet();

        int c = getchar();

        switch (c)
        {
            case '0':
                BoostPWM_SetVoltageTarget(0);
                BoostPWM_SetCurrentLimit(0);
                voltageTarget = 0;
                currentLimit = 0;
                break;
            case '+':
            case '=':
                if (ccMode)
                {
                    currentLimit += 25;
                    BoostPWM_SetCurrentLimit(currentLimit);
                }
                else
                {
                    voltageTarget += 50;
                    BoostPWM_SetVoltageTarget(voltageTarget);
                }
                break;
            case '-':
                if (ccMode)
                {
                    currentLimit -= 25;
                    BoostPWM_SetCurrentLimit(currentLimit);
                }
                else
                {
                    voltageTarget -= 50;
                    BoostPWM_SetVoltageTarget(voltageTarget);
                }
                break;
            case -1:
                voltage = BoostPWM_GetVoltageMillivolts();
                current = BoostPWM_GetCurrentMilliamps();
                power = voltage * current;
                LOGI(TAG, "CC: %d, Voltage: %dmV, Current: %dmA, Power: %dmW",
                     ccMode,
                     voltage,
                     current,
                     power / 1000);
                break;
            case 'c':
                ccMode = true;
                break;
            case 'v':
                ccMode = false;
                currentLimit = 0;
                BoostPWM_SetCurrentLimit(0);
                break;
            default:
                if (c <= '0' || c > '9') break;
                if (ccMode)
                {
                    currentLimit = (c - '0') * 100;
                    BoostPWM_SetCurrentLimit(currentLimit);
                }
                else
                {
                    voltageTarget = (c - '0') * 1000;
                    BoostPWM_SetVoltageTarget(voltageTarget);
                }
                break;
        }

        Delay_Ms(100);
    }
}

#if !FUNCONF_USE_DEBUGPRINTF

/**
 * @brief  Write string to console
 * @param      fd - the file descriptor(ignored)
 * @param[in]  buf - the buffer
 * @param      size - buffer size in bytes
 * @return the number of bytes written
 */
int _write(int fd, const char *buf, int size)
{
    (void)fd;
    (void)RingBuffer_Put(&printRingBuffer, (uint8_t *)buf, size);
    return size;
}
#endif

//------------------------------------------------------------------------------
// Module static functions
//------------------------------------------------------------------------------

/**
 * @brief  Enable the SysTick module
 * @param  None
 * @return None
 */
static void SysTick_Init(void)
{
    // Disable default SysTick behavior
    SysTick->CTLR = 0;

    // Enable the SysTick IRQ
    NVIC_EnableIRQ(SysTicK_IRQn);

    // Set the tick interval to 1ms for normal op
    SysTick->CMP = (FUNCONF_SYSTEM_CORE_CLOCK / 1000) - 1;

    // Start at zero
    SysTick->CNT = 0;
    s_systickCount = 0;

    // Enable SysTick counter, IRQ, HCLK/1
    SysTick->CTLR = SYSTICK_CTLR_STE | SYSTICK_CTLR_STIE | SYSTICK_CTLR_STCLK;
}

/**
 * @brief  Initialize the watchdog timer
 * @param reload_val - the value to reload the counter with
 * @param prescaler - the prescaler to use
 * @return None
 */
static void WDT_Init(uint16_t reload_val, uint8_t prescaler)
{
    IWDG->CTLR = 0x5555;
    IWDG->PSCR = prescaler;

    IWDG->CTLR = 0x5555;
    IWDG->RLDR = reload_val & 0xfff;

    IWDG->CTLR = 0xCCCC;
}

/**
 * @brief  Pet the watchdog timer
 * @param  None
 * @return None
 */
static void WDT_Pet(void)
{
    IWDG->CTLR = 0xAAAA;
}

/**
 * @brief  SysTick interrupt handler
 * @param  None
 * @return None
 * @note   __attribute__((interrupt)) syntax is crucial!
 */
void SysTick_Handler(void) __attribute__((interrupt));
void SysTick_Handler(void)
{
    // Set the next interrupt to be in 1/1000th of a second
    SysTick->CMP += (FUNCONF_SYSTEM_CORE_CLOCK / 1000);

    // Clear IRQ
    SysTick->SR = 0;

    // Update counter
    s_systickCount++;
}

/**
 * @brief  Handle USB user in requests
 * @param  e - the endpoint
 * @param  scratchpad - the scratchpad buffer
 * @param  endp - the endpoint number
 * @param  sendtok - the token to send
 * @param  ist - the internal state
 * @return None
 * @note usb_hande_interrupt_in is OBLIGATED to call usb_send_data or usb_send_empty.
 */
void usb_handle_user_in_request(struct usb_endpoint *e, uint8_t *scratchpad, int endp, uint32_t sendtok, struct rv003usb_internal *ist)
{
    if (endp == 3)
    {
        size_t bytes = RingBuffer_Peek(&printRingBuffer);
        if (bytes > 7)
        {
            static uint8_t c[8];
            if (eRING_BUFFER_STATUS_SUCCESS == RingBuffer_Get(&printRingBuffer, c, 8, &bytes))
            {
                usb_send_data(c, 8, 0, sendtok);
                return;
            }
        }
    }
    // If it's a control transfer, don't send anything.
    usb_send_empty(sendtok);
}

/**
 * @brief  Handle USB control messages
 * @param  e - the endpoint
 * @param  s - the URB
 * @param  ist - the internal state
 * @return None
 */
void usb_handle_other_control_message(struct usb_endpoint *e, struct usb_urb *s, struct rv003usb_internal *ist)
{
    LogUEvent(SysTick->CNT, s->wRequestTypeLSBRequestMSB, s->lValueLSBIndexMSB, s->wLength);
    e->opaque = (uint8_t *)1;
}

/**
 * @brief  Handle USB user data
 * @param      e - the endpoint
 * @param      current_endpoint - the current endpoint
 * @param[in]  data - the data
 * @param      len - the length
 * @param      ist - the internal state
 * @return None
 */
void usb_handle_user_data(struct usb_endpoint *e, int current_endpoint, uint8_t *data, int len, struct rv003usb_internal *ist)
{
    LogUEvent(SysTick->CNT, 0xffffffff, current_endpoint, len);
    printf("Got %d bytes\r\n", len);
}

static uint8_t newByte = 0;
static uint32_t count = 0;
static uint32_t countLast = 0;

/**
 * @brief  Debugger input handler
 * @param  numbytes - the number of bytes received
 * @param  data - the data (8 bytes)
 * @return None
 */
void handle_debug_input(int numbytes, uint8_t *data)
{
    newByte = data[0];
    count += numbytes;
}

/**
 * @brief  Get the next character from the debugger
 * @param  None
 * @return the next character or -1 if no character is available
 * @note   This function will block for up to 100ms waiting for a character
 */
int getchar(void)
{
    const uint32_t timeout = 100;
    const uint32_t end = s_systickCount + timeout;
    while (count == countLast && (s_systickCount < end))
    {
        poll_input();
        putchar(0);
    }

    if (count == countLast)
    {
        return -1;
    }

    countLast = count;
    return newByte;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
