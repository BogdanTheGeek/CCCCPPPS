//------------------------------------------------------------------------------
//       Filename: log.c
//------------------------------------------------------------------------------
//       Bogdan Ionescu (c) 2024
//------------------------------------------------------------------------------
//       Purpose : Implements the logging API
//------------------------------------------------------------------------------
//       Notes : None
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Module includes
//------------------------------------------------------------------------------
#include "log.h"

//------------------------------------------------------------------------------
// Module constant defines
//------------------------------------------------------------------------------
#define RED    "\033[31m"
#define GREEN  "\033[32m"
#define YELLOW "\033[33m"
#define BLUE   "\033[34m"
#define NORMAL "\033[0;39m"

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
static LogLevel_e s_maxLogLevel = eLOG_LEVEL_DEBUG;
static const uint32_t *s_systick = NULL;

//------------------------------------------------------------------------------
// Module static function prototypes
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Module externally exported functions
//------------------------------------------------------------------------------
/*
 * @brief       Initialize  UART Logging library.
 * @param[in]   level - logging level
 * @param[in]   systick - pointer to systick counter
 * @return      None
 */
void LOG_Init(const LogLevel_e level, const uint32_t *const systick)
{
    s_maxLogLevel = level;
    s_systick = systick;

    if (level != eLOG_LEVEL_NONE)
    {
        // clear the terminal
        printf("\033c");
        printf("Ready\r\n");
    }
}

/**
 * @brief  Custom Logging function with colour output and logging level selection
 * @param  level - logging level
 * @param  tag   - module identifying tag as null terminated string
 * @param  format - format string
 * @param  ... - variable argument list
 * @return None
 * */
#if (CONFIG_DEBUG_ENABLE_LOGS)
void LOG(const LogLevel_e level, const char *tag, char *format, ...)
{
    if (level < s_maxLogLevel)
    {
        return;
    }

    char *colour;
    char indicator;
    switch (level)
    {
        case eLOG_LEVEL_INFO:
            colour = GREEN;
            indicator = 'I';
            break;
        case eLOG_LEVEL_WARNING:
            colour = YELLOW;
            indicator = 'W';
            break;
        case eLOG_LEVEL_ERROR:
            colour = RED;
            indicator = 'E';
            break;
        case eLOG_LEVEL_DEBUG:
        default:
            colour = NORMAL;
            indicator = 'D';
            break;
    }
    printf(colour);
    printf("%lu %c %s: ", *s_systick, indicator, tag);
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\r\n%s", NORMAL);
}
#endif

//------------------------------------------------------------------------------
// Module static functions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
