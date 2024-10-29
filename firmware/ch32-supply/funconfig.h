#ifndef _FUNCONFIG_H
#define _FUNCONFIG_H

// Though this should be on by default we can extra force it on.
#ifndef CONFIG_USE_USB
#define FUNCONF_USE_DEBUGPRINTF 1
#define FUNCONF_DEBUGPRINTF_TIMEOUT (1 << 31) // Wait for a very very long time.
#else

#define FUNCONF_USE_DEBUGPRINTF 0
#define FUNCONF_USE_UARTPRINTF  0
#define FUNCONF_NULL_PRINTF     1
#endif

#define FUNCONF_SYSTICK_USE_HCLK 1
#define CH32V003                 1

#endif
