#include "core.h"
#include "log.h"
#include "lang.h"
#include "server.h"
#include "command.h"
#include "consoleio.h"

THREAD_FUNC(ConsoleIO_Thread) {
	(void)param;
	cs_char buf[192];

	while(Server_Active) {
		if(File_ReadLine(stdin, buf, 192))
			if(!Command_Handle(buf, NULL))
				Log_Info(Lang_Get(Lang_CmdGrp, 3));
	}
	return 0;
}

static TSHND_RET ConsoleIO_Handler(TSHND_PARAM signal) {
  if(signal == CONSOLEIO_TERMINATE)Server_Active = false;
  return TSHND_OK;
}

cs_bool ConsoleIO_Init(void) {
  Thread_Create(ConsoleIO_Thread, NULL, true);
	return Console_BindSignalHandler(ConsoleIO_Handler);
}
