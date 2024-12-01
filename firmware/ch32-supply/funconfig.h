#ifndef _FUNCONFIG_H
#define _FUNCONFIG_H

// Must be a multiple of 8
#define BOOST_REPORT_SIZE (8)

#define CONFIG_DEBUG_ENABLE_LOGS 1

// Though this should be on by default we can extra force it on.
#define FUNCONF_USE_DEBUGPRINTF 1
// #define FUNCONF_DEBUGPRINTF_TIMEOUT (1 << 31) // Wait for a very very long time.

#define CONFIG_USE_USB           1
#define FUNCONF_SYSTICK_USE_HCLK 1
#define CH32V003                 1

#endif
