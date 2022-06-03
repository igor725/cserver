#ifdef CORE_BUILD_PLUGIN
#	error ConsoleIO cannot be used in plugins
#endif

#ifndef CONSOLEIO_H
#define CONSOLEIO_H
#include "core.h"

#if defined(CORE_USE_WINDOWS)
#	define CONSOLEIO_TERMINATE CTRL_C_EVENT
#elif defined(CORE_USE_UNIX)
#	include <signal.h>
#	define CONSOLEIO_TERMINATE SIGINT
#endif

void ConsoleIO_PrePrint(void);
void ConsoleIO_AfterPrint(void);
cs_bool ConsoleIO_Init(void);
void ConsoleIO_Uninit(void);
#endif
