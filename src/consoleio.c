#include "core.h"
#include "log.h"
#include "server.h"
#include "command.h"
#include "consoleio.h"

THREAD_FUNC(ConsoleIOThread) {
	(void)param;
	cs_char buf[192];

	while(Server_Active) {
		if(File_ReadLine(stdin, buf, 192))
			if(!Command_Handle(buf, NULL))
				Log_Info("Unknown command.");
	}

	return 0;
}

static TSHND_RET ConsoleIO_Handler(TSHND_PARAM signal) {
  if(signal == CONSOLEIO_TERMINATE) Server_Active = false;
  return TSHND_OK;
}

cs_bool ConsoleIO_Init(void) {
  Thread_Create(ConsoleIOThread, NULL, true);
	return Console_BindSignalHandler(ConsoleIO_Handler);
}
