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

#include "boost.h"
#include "ch32v003fun.h"
#include "log.h"
#include "rv003usb.h"

//------------------------------------------------------------------------------
// Module constant defines
//------------------------------------------------------------------------------

#define TAG "main"

#ifndef CONFIG_CURRENT_LIMIT
#define CONFIG_CURRENT_LIMIT 1000
#endif

#ifndef CONFIG_VOLTAGE_LIMIT
#define CONFIG_VOLTAGE_LIMIT 15000
#endif

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

typedef struct
{
    uint32_t voltage;
    uint32_t current;
} Command_t;

//------------------------------------------------------------------------------
// Module static variables
//------------------------------------------------------------------------------
static volatile uint32_t s_systickCount = 0;

static volatile size_t s_bytesReceived = 0;
static volatile Command_t s_command = {
    .voltage = 0,
    .current = CONFIG_CURRENT_LIMIT,
};
static volatile BoostState_t s_state = {
    .voltage = 0,
    .current = CONFIG_CURRENT_LIMIT,
    .duty = 0,
    .ccMode = false,
};

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

    SysTick_Init();

    const bool debuggerAttached = !WaitForDebuggerToAttach(1000);
    if (debuggerAttached)
    {
        LOG_Init(eLOG_LEVEL_INFO, (uint32_t *)&s_systickCount);
    }
    else
    {
        LOG_Init(eLOG_LEVEL_NONE, (uint32_t *)&s_systickCount);
    }

#ifdef CONFIG_USE_USB
    usb_setup();
#endif

    BoostPWM_Init();

    BoostPWM_SetVoltageTarget(s_command.voltage);
    BoostPWM_SetCurrentLimit(s_command.current);

#if 0

    for (size_t i = 1000; i < 5001; i += 200)
    {
        BoostPWM_SetVoltageTarget(i);
        LOGD(TAG, "Target: %dmV", i);
        Delay_Ms(100);
    }
#endif

    WDT_Init(0x0FFF, IWDG_Prescaler_128);

    static uint32_t power = 0;

    static size_t s_lastBytesReceived = 0;
    static uint32_t lastTime = 0;

    while (1)
    {
        WDT_Pet();

        if (s_bytesReceived != s_lastBytesReceived)
        {
            LOGD(TAG, "Received %d bytes", s_bytesReceived - s_lastBytesReceived);
            LOGI(TAG, "Setting Voltage: %dmV, Current: %dmA", s_command.voltage, s_command.current);

            if (s_command.voltage > CONFIG_VOLTAGE_LIMIT)
            {
                s_command.voltage = CONFIG_VOLTAGE_LIMIT;
            }

            if (s_command.current > CONFIG_CURRENT_LIMIT)
            {
                s_command.voltage = CONFIG_CURRENT_LIMIT;
            }

            BoostPWM_SetVoltageTarget(s_command.voltage);
            BoostPWM_SetCurrentLimit(s_command.current);
            s_lastBytesReceived = s_bytesReceived;
        }

        BoostPWM_GetState((BoostState_t *)&s_state);
        power = (s_state.voltage * s_state.current) / 1000;

        if (s_systickCount - lastTime > 1000)
        {
            lastTime = s_systickCount;
            LOGI(TAG, "CC: %d, Voltage: %5dmV, Current: %4dmA, Power: %5dmW, Duty: %3d",
                 s_state.ccMode,
                 s_state.voltage,
                 s_state.current,
                 power,
                 s_state.duty);
        }

        if (!debuggerAttached) continue;

        int c = getchar();

        switch (c)
        {
            case '0':
                BoostPWM_SetVoltageTarget(0);
                BoostPWM_SetCurrentLimit(CONFIG_CURRENT_LIMIT);
                s_command.voltage = 0;
                s_command.current = CONFIG_CURRENT_LIMIT;
                break;
            case '+':
            case '=':
                if (s_state.ccMode)
                {
                    s_command.current += 25;
                    BoostPWM_SetCurrentLimit(s_command.current);
                }
                else
                {
                    s_command.voltage += 50;
                    BoostPWM_SetVoltageTarget(s_command.voltage);
                }
                break;
            case '-':
                if (s_state.ccMode)
                {
                    s_command.current -= 25;
                    BoostPWM_SetCurrentLimit(s_command.current);
                }
                else
                {
                    s_command.voltage -= 50;
                    BoostPWM_SetVoltageTarget(s_command.voltage);
                }
                break;
            case -1:
                break;
            case 'c':
                s_state.ccMode = true;
                break;
            case 'v':
                s_state.ccMode = false;
                break;
            default:
                if (c <= '0' || c > '9') break;
                if (s_state.ccMode)
                {
                    s_command.current = (c - '0') * 100;
                    BoostPWM_SetCurrentLimit(s_command.current);
                }
                else
                {
                    s_command.voltage = (c - '0') * 1000;
                    BoostPWM_SetVoltageTarget(s_command.voltage);
                }
                break;
        }
        Delay_Ms(100);
    }
}

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
    // Make sure we only deal with control messages. Like get/set feature reports.
    if (endp)
    {
        usb_send_empty(sendtok);
    }
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
    // e->opaque = (uint8_t *)1;
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
#if 0
    int offset = e->count << 3;
    int torx = e->max_len - offset;
    if (torx > len) torx = len;
	if( torx > 0 )
	{
		memcpy( scratch + offset, data, torx );
		e->count++;

      s_bytesReceived += torx;
	}
#else
    if (len >= 6 && 0xAA == data[0])
    {
        const uint8_t cmd = data[1];
        switch (cmd)
        {
            case 1:
                s_command.voltage = *(uint32_t *)(data + 2);
                break;
            case 2:
                s_command.current = *(uint32_t *)(data + 2);
                break;
        }

        e->count++;
        s_bytesReceived += len;
    }
#endif
}

void usb_handle_hid_get_report_start(struct usb_endpoint *e, int reqLen, uint32_t lValueLSBIndexMSB)
{

    // You can check the lValueLSBIndexMSB word to decide what you want to do here
    // But, whatever you point this at will be returned back to the host PC where
    // it calls hid_get_feature_report.
    //
    // Please note, that on some systems, for this to work, your return length must
    // match the length defined in HID_REPORT_COUNT, in your HID report, in usb_config.h

    // if (reqLen > sizeof(s_state)) reqLen = sizeof(s_state);
    e->opaque = (void *)&s_state;
    e->max_len = sizeof(s_state);
}

void usb_handle_hid_set_report_start(struct usb_endpoint *e, int reqLen, uint32_t lValueLSBIndexMSB)
{
    // Here is where you get an alert when the host PC calls hid_send_feature_report.
    //
    // You can handle the appropriate message here.  Please note that in this
    // example, the data is chunked into groups-of-8-bytes.
    //
    // Note that you may need to make this match HID_REPORT_COUNT, in your HID
    // report, in usb_config.h

    // if (reqLen > sizeof(s_state)) reqLen = sizeof(s_state);
    e->max_len = sizeof(s_state);
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
