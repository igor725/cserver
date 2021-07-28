#ifndef CONSOLEIO_H
#define CONSOLEIO_H
#if defined(WINDOWS)
#define CONSOLEIO_TERMINATE CTRL_C_EVENT
#elif defined(UNIX)
#include <signal.h>
#define CONSOLEIO_TERMINATE SIGINT
#endif

void ConsoleIO_Init(void);
cs_bool ConsoleIO_Handler(cs_uint32 signal);
#endif
