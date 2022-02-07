#ifndef CONSOLEIO_H
#define CONSOLEIO_H
#include "core.h"

#if defined(CORE_USE_WINDOWS)
#define CONSOLEIO_TERMINATE CTRL_C_EVENT
#elif defined(CORE_USE_UNIX)
#include <signal.h>
#define CONSOLEIO_TERMINATE SIGINT
#endif

cs_bool ConsoleIO_Init(void);
#endif
