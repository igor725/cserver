#include "core.h"
#include "command.h"
#include "packets.h"
#include "server.h"

void Command_Register(const char* cmd, cmdFunc func) {
	COMMAND* tmp = Memory_Alloc(1, sizeof(COMMAND));

	tmp->name = String_AllocCopy(cmd);
	tmp->func = func;
	if(Command_Head)
		Command_Head->prev = tmp;
	tmp->next = Command_Head;
	Command_Head = tmp;
}

static void Command_Free(COMMAND* cmd) {
	if(cmd->prev)
		cmd->prev->next = cmd->prev;

	if(cmd->next) {
		cmd->next->prev = cmd->next;
		Command_Head = cmd->next->prev;
	} else
		Command_Head = NULL;

	Memory_Free((void*)cmd->name);
	Memory_Free(cmd);
}

void Command_Unregister(const char* cmd) {
	COMMAND* tmp = Command_Head;

	while(tmp) {
		if(String_CaselessCompare(tmp->name, cmd))
			Command_Free(tmp);
		tmp = tmp->next;
	}
}

static bool CHandler_OP(const char* args, CLIENT caller, char* out) {
	Command_OnlyForOP;

	char clientname[64];
	if(String_GetArgument(args, clientname, 64, 0)) {
		CLIENT tg = Client_FindByName(clientname);
		if(tg) {
			bool newtype = !Client_GetType(tg);
			const char* name = tg->playerData->name;
			Client_SetType(tg, newtype);
			String_FormatBuf(out, CMD_MAX_OUT, "Player %s %s", name, newtype ? "opped" : "deopped");
		} else {
			String_Copy(out, CMD_MAX_OUT, "Player not found");
		}
	}
	return true;
}

static bool CHandler_Stop(const char* args, CLIENT caller, char* out) {
	Command_OnlyForOP;

	Server_Active = false;
	return false;
}

static bool CHandler_Test(const char* args, CLIENT caller, char* out) {
	sprintf(out, "Command \"test\" called by %s with args: %s",
		caller ? caller->playerData->name : "console", args
	);
	return true;
}

static bool CHandler_Announce(const char* args, CLIENT caller, char* out) {
	Command_OnlyForOP;

	if(!caller) caller = Broadcast;
	Packet_WriteChat(caller, CPE_ANNOUNCE, !args ? "Test announcement" : args);
	return false;
}

static bool CHandler_ChangeWorld(const char* args, CLIENT caller, char* out) {
	Command_OnlyForClient;

	WORLD world = World_FindByName(args);
	Client_ChangeWorld(caller, world);
	return false;
}

static bool CHandler_Kick(const char* args, CLIENT caller, char* out) {
	Command_OnlyForOP;
	char playername[64];

	if(String_GetArgument(args, playername, 64, 0)) {
		CLIENT tg = Client_FindByName(playername);
		if(tg) {
			const char* reason = String_FromArgument(args, 1);
			Client_Kick(tg, reason);
			String_FormatBuf(out, CMD_MAX_OUT, "Player %s kicked", playername);
		} else {
			String_Copy(out, CMD_MAX_OUT, "Player not found");
		}
	} else {
		String_Copy(out, CMD_MAX_OUT, "Invalid argument #1");
	}

	return true;
}

static bool CHandler_Model(const char* args, CLIENT caller, char* out) {
	Command_OnlyForOP;
	Command_OnlyForClient;

	char modelname[64];
	if(String_GetArgument(args, modelname, 64, 0)) {
		if(!Client_SetModel(caller, modelname)) {
			String_Copy(out, CMD_MAX_OUT, "Model changing error");
			return true;
		}
	}
	return false;
}

void Command_RegisterDefault() {
	Command_Register("op", CHandler_OP);
	Command_Register("stop", CHandler_Stop);
	Command_Register("test", CHandler_Test);
	Command_Register("announce", CHandler_Announce);
	Command_Register("cw", CHandler_ChangeWorld);
	Command_Register("kick", CHandler_Kick);
	Command_Register("setmodel", CHandler_Model);
}

bool Command_Handle(char* cmd, CLIENT caller) {
	char ret[CMD_MAX_OUT] = {0};
	char* args = cmd;

	while(1) {
		++args;
		if(*args == 0) {
			args = NULL;
			break;
		} else if (*args == 32) {
			*args = 0;
			++args;
			break;
		}
	}

	COMMAND* tmp = Command_Head;

	while(tmp) {
		if(String_CaselessCompare(tmp->name, cmd)) {
			if(tmp->func((const char*)args, caller, ret)) {
				if(caller)
					Packet_WriteChat(caller, 0, ret);
				else
					Log_Info(ret);
			}
			return true;
		}
		tmp = tmp->next;
	}

	return false;
}
