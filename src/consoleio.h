#ifndef CONSOLEIO_H
#define CONSOLEIO_H
#include "core.h"

#if defined(WINDOWS)
#define CONSOLEIO_TERMINATE CTRL_C_EVENT
#elif defined(UNIX)
#include <signal.h>
#define CONSOLEIO_TERMINATE SIGINT
#endif

cs_bool ConsoleIO_Init(void);
#endif
