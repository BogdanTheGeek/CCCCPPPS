//------------------------------------------------------------------------------
//       Filename: boost.c
//------------------------------------------------------------------------------
//       Bogdan Ionescu (c) 2024
//------------------------------------------------------------------------------
//       Purpose : Implements the Boost Converter API
//------------------------------------------------------------------------------
//       Notes : None
//------------------------------------------------------------------------------
//
// PWM_OUT = PC0 = T1CH3
// FEEDBACK = PD6 = A6
// CURRENT = PD4 = A7
//
//------------------------------------------------------------------------------
// Module includes
//------------------------------------------------------------------------------
#include "boost.h"
#include "ch32v003fun.h"
#include "log.h"

//------------------------------------------------------------------------------
// Module constant defines
//------------------------------------------------------------------------------
#define TAG "boost"

// Feedback Resistors in 10 Ohm units
#define Rf  390
#define Rin 100
#define Rt  (Rf + Rin)

#define INTERNAL_VREF  1200
#define ADC_RESOLUTION 10
#define ADC_MAX        (1 << ADC_RESOLUTION)

#define MIN_DUTY    0
#define MAX_DUTY    250
#define ADC_SAMPLES (3)

// PID terms
#define KP(eP) ((eP) >> 0)
#define KD(eD) ((eD) >> 3)
#define KI(eI) ((eI) >> 6)

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef INLINE
#define INLINE __attribute__((always_inline))
#endif

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
static uint16_t s_feedbackVRaw = 0;
static uint16_t s_feedbackIRaw = 0;
static int16_t s_currentOffset = 0;
static uint16_t s_vref = 0;
static uint8_t s_pwmDuty = 0;
static uint8_t s_ccMode = 0;
static volatile uint32_t s_targetVRaw = 0;
static volatile uint32_t s_targetIRaw = 0;

//------------------------------------------------------------------------------
// Module static function prototypes
//------------------------------------------------------------------------------
static int GetVRefMillivolts(void);
static uint16_t GetVoltageMillivolts(void);
static uint16_t MillivoltsToADC(uint32_t millivolts);
static uint16_t GetCurrentMilliamps(void);
static uint16_t MilliampsToADC(uint32_t milliamps);

static void SetupOpAmp(void);
static void SetupADC(void);

static void BoostControllerPID(void);
static void SetDuty(uint8_t duty);

static void Calibrate(void);

//------------------------------------------------------------------------------
// Module externally exported functions
//------------------------------------------------------------------------------
/**
 * @brief  Initialize the Boost converter
 * @param  None
 * @return None
 */
void BoostPWM_Init(void)
{
    SetupOpAmp();

    SetupADC();

    RCC->APB2PCENR |= RCC_APB2Periph_TIM1 | RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOC;

    AFIO->PCFR1 |= GPIO_PartialRemap1_TIM1;

    // PC0 is T1CH3, 10MHz Output alt func, push-pull
    GPIOC->CFGLR &= ~(0xf << (0 << 2));
    GPIOC->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP_AF) << (0 << 2);

    // Reset TIM1 to init all regs
    RCC->APB2PRSTR |= RCC_APB2Periph_TIM1;
    RCC->APB2PRSTR &= ~RCC_APB2Periph_TIM1;

    // CTLR1: default is up, events generated, edge align
    // SMCFGR: default clk input is CK_INT

    // Prescaler
    TIM1->PSC = 0x0001;

    // Auto Reload - sets period
    TIM1->ATRLR = 255 + 10;

    // Reload immediately
    TIM1->SWEVGR |= TIM_UG;

    // Enable CH3 output, normal polarity
    TIM1->CCER |= TIM_CC3E | TIM_CC3NP;

    // CH3 Mode is output, PWM1 (CC3S = 00, OC3M = 110)
    TIM1->CHCTLR2 |= TIM_OC3M_2 | TIM_OC3M_1;

    // Set the Capture Compare Register value to off
    TIM1->CH3CVR = 0;

    // Setup TRGO for ADC.  This makes is to the ADC will trigger on timer
    // reset, so we trigger at the same position every time relative to the
    // FET turning on.
    TIM1->CTLR2 = TIM_MMS_1;

    // Enable TIM1 outputs
    TIM1->BDTR |= TIM_MOE;

    // Enable TIM1
    TIM1->CTLR1 |= TIM_CEN;

    Calibrate();

#if 0

    LOGD(TAG, "VRef: (%d) %dmV", s_vref, GetVRefMillivolts());
    LOGD(TAG, "Vout: (%d) %dmV", s_feedbackRaw, GetVoltageMillivolts());
    LOGD(TAG, "Current: (%d) %dmA", s_current, GetCurrentMilliamps());
    LOGD(TAG, "Target: %d", s_targetADC);
    puts("");
    Delay_Ms(1000);
#endif
}

/**
 * @brief  Set the target voltage for the boost converter
 * @param millivolts - The target voltage in millivolts
 * @return None
 */
void BoostPWM_SetVoltageTarget(uint32_t millivolts)
{
    s_targetVRaw = MillivoltsToADC(millivolts);
}

/**
 * @brief  Set the current limit for the boost converter
 * @param  milliamps - The current limit in milliamps, 0 to disable
 * @return None
 */
void BoostPWM_SetCurrentLimit(uint32_t milliamps)
{
    if (milliamps > 0)
    {
        s_targetIRaw = MilliampsToADC(milliamps);
    }
    else
    {
        s_targetIRaw = 0;
    }
}

/**
 * @brief  Get the state of the boost converter
 * @param[out] state - The state of the boost converter
 * @return None
 */
void BoostPWM_GetState(BoostState_t *state)
{
    *state = (BoostState_t){
        .voltage = GetVoltageMillivolts(),
        .current = GetCurrentMilliamps(),
        .duty = s_pwmDuty,
        .ccMode = s_ccMode,
    };
}

/**
 * @brief  ADC1 IRQ Handler
 * @param  None
 * @return None
 */
void ADC1_IRQHandler(void) __attribute__((section(".srodata"))) __attribute__((interrupt));
void ADC1_IRQHandler(void)
{
    // Values come in reverse order.
    s_vref = ADC1->IDATAR2;
    s_feedbackIRaw = ADC1->IDATAR1;

    s_feedbackVRaw = ADC1->RDATAR;
    BoostControllerPID();

    // Acknowledge pending interrupts.
    ADC1->STATR = 0;
}

//------------------------------------------------------------------------------
// Module static functions
//------------------------------------------------------------------------------

/**
 * @brief  Get the VRef voltage in millivolts
 * @param  None
 * @return The VRef voltage in millivolts
 */
static int GetVRefMillivolts(void)
{
    return ((INTERNAL_VREF * ADC_MAX) / s_vref);
}

/**
 * @brief  Get the output voltage in millivolts
 * @param  None
 * @return The output voltage in millivolts
 */
static uint16_t GetVoltageMillivolts(void)
{
    const int vref = GetVRefMillivolts();
    return (s_feedbackVRaw * vref * Rt) / (Rin * ADC_MAX);
}

/**
 * @brief  Convert millivolts to ADC target value
 * @param  millivolts: target voltage in millivolts
 * @return The ADC target value
 */
static uint16_t MillivoltsToADC(uint32_t millivolts)
{
    return (millivolts * ADC_MAX * Rin) / (Rt * GetVRefMillivolts());
}

/**
 * @brief  Get the current in milliamps
 * @param  None
 * @return The output current in milliamps
 */
static uint16_t GetCurrentMilliamps(void)
{
    const int current = (int)s_feedbackIRaw - s_currentOffset;
    if (current < 0)
    {
        return 0;
    }
    return (uint16_t)current;
}

/**
 * @brief  Convert milliamps to ADC target value
 * @param  milliamps: target current in milliamps
 * @return The ADC target value
 */
static uint16_t MilliampsToADC(uint32_t milliamps)
{
    return (uint16_t)(milliamps + s_currentOffset);
}

/**
 * @brief  Set up the ADC for the voltage and current feedback
 * @param  None
 * @return None
 */
static void SetupADC(void)
{
    // Configure ADC.
    RCC->APB2PCENR |= RCC_APB2Periph_GPIOD | RCC_APB2Periph_ADC1;

    // PD6 is analog input ch 6
    GPIOD->CFGLR &= ~(0xf << (6 << 2)); // CNF = 00: Analog, MODE = 00: Input
    // PD4 is analog input ch 7
    GPIOD->CFGLR &= ~(0xf << (4 << 2)); // CNF = 00: Analog, MODE = 00: Input

    // Reset the ADC to init all regs
    RCC->APB2PRSTR |= RCC_APB2Periph_ADC1;
    RCC->APB2PRSTR &= ~RCC_APB2Periph_ADC1;

    // ADCCLK = 12 MHz => RCC_ADCPRE divide by 4
    RCC->CFGR0 &= ~RCC_ADCPRE;     // Clear out the bis in case they were set
    RCC->CFGR0 |= RCC_ADCPRE_DIV4; // set it to 010xx for /4.

    ADC1->RSQR1 = 0; // 1 channels in sequence
    ADC1->RSQR2 = 0;
    // Set up 1st conversion on ch6
    // 0-9 for 8 ext inputs and two internals
    ADC1->RSQR3 = (6 << 0);

    // Injection group is 8. NOTE: See note in 9.3.12 (ADC_ISQR) of TRM. The
    //  group numbers is actually 4-group numbers.
    ADC1->ISQR = (8 << 15) | (7 << 10); // ch8 as first conversion, ch7 as second
    ADC1->ISQR |= (1 << 20);            // set number of conversions to 1

    // Sampling time for channels. Careful: This has PID tuning implications.
    // Note that with 3 and 3,the full loop (and injection) runs at 138kHz.
    ADC1->SAMPTR2 = (ADC_SAMPLES << (3 * 7)) | (ADC_SAMPLES << (3 * 8)) | (ADC_SAMPLES << (3 * 1));
    // 0:7 => 3/9/15/30/43/57/73/241 cycles
    // (4 == 43 cycles), (6 = 73 cycles)  Note these are alrady /2, so
    // setting this to 73 cycles actually makes it wait 256 total cycles @ 48MHz.

    // Turn on ADC and set rule group to sw trig
    // 0 = Use TRGO event for Timer 1 to fire ADC rule.
    ADC1->CTLR2 = ADC_ADON | ADC_JEXTTRIG | ADC_JEXTSEL | ADC_EXTTRIG;

    // Reset calibration
    ADC1->CTLR2 |= ADC_RSTCAL;
    while (ADC1->CTLR2 & ADC_RSTCAL)
        ;

    // Calibrate ADC
    ADC1->CTLR2 |= ADC_CAL;
    while (ADC1->CTLR2 & ADC_CAL)
        ;

    // enable the ADC Conversion Complete IRQ
    NVIC_EnableIRQ(ADC_IRQn);

#if CONFIG_ENABLE_NESTED_INTERRUPTS
    __set_INTSYSCR(__get_INTSYSCR() | 2); // Enable interrupt nesting.

    // Ref. 6.3 Vector Table of Interrupts and Exceptions
    PFIC->IPRIOR[15] = 1 << 7; // Turn on preemption for ADC
#endif

    // ADC_JEOCIE: Enable the End-of-conversion interrupt.
    // ADC_JDISCEN | ADC_JAUTO: Force injection after rule conversion.
    // ADC_SCAN: Allow scanning.
    ADC1->CTLR1 = ADC_JEOCIE | ADC_JDISCEN | ADC_SCAN | ADC_JAUTO;
}

/**
 * @brief  Set up the Op-Amp for current sensing
 * @param  None
 * @return None
 */
static void SetupOpAmp(void)
{
    RCC->APB2PCENR = RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOA;

    // Set the Op-Amp Input Positive and Negative to Floating
    GPIOA->CFGLR &= ~(0xf << (1 << 2));
    GPIOA->CFGLR &= ~(0xf << (2 << 2));
    GPIOA->CFGLR |= (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (1 << 2);
    GPIOA->CFGLR |= (GPIO_Speed_In | GPIO_CNF_IN_ANALOG) << (2 << 2);

    // Set the Default Op-Amp pins to OPP0 and OPN0, then Enable the Op-Amp
    EXTEN->EXTEN_CTR &= ~(EXTEN_OPA_NSEL | EXTEN_OPA_PSEL);
    EXTEN->EXTEN_CTR |= EXTEN_OPA_EN;
}

/**
 * @brief  Boost controller PID algorithm
 * @param  None
 * @return None
 * @note   eP = P error, eI = I error, eD = D error
 */
static INLINE void BoostControllerPID(void)
{
    static int lastEP = 0;
    static int eI = 0;

    // Skip if the target is 0
    if (s_targetVRaw == 0 || s_targetIRaw == 0)
    {
        // Reset the algorithm
        lastEP = 0;
        eI = 0;
        SetDuty(0);
        return;
    }

    // Calculate the voltage and current errors
    const int ePv = s_targetVRaw - s_feedbackVRaw;
    const int ePi = s_targetIRaw - s_feedbackIRaw;
    s_ccMode = (ePv < ePi) ? 0 : 1;

    // Picking the smallest error gives current or voltage limiting
    const int eP = min(ePv, ePi);
    const int eD = eP - lastEP;
    eI += eP;

    int duty = KP(eP) + KD(eD) + KI(eI);

    // Limit the duty cycle for safety
    duty = max(duty, MIN_DUTY);
    duty = min(duty, MAX_DUTY);

    SetDuty(duty);
}

/**
 * @brief  Set PWM duty cycle for the boost converter
 * @param  None
 * @return None
 */
static INLINE void SetDuty(uint8_t duty)
{
    TIM1->CH3CVR = s_pwmDuty = duty;
}

/**
 * @brief  Calibrate the current sensor
 * @param  None
 * @return None
 */
static void Calibrate(void)
{
    // Calibrate the current sensor
    s_targetVRaw = 0;
    Delay_Ms(100);
    s_currentOffset = s_feedbackIRaw;

    LOGD(TAG, "Current offset: %d", s_currentOffset);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
