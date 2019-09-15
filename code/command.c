#include "core.h"
#include "command.h"
#include "packets.h"
#include "server.h"

void Command_Register(const char* cmd, cmdFunc func, bool onlyOP) {
	COMMAND* tmp = Memory_Alloc(1, sizeof(COMMAND));

	tmp->name = String_AllocCopy(cmd);
	tmp->func = func;
	tmp->onlyOP = onlyOP;
	tmp->next = headCommand;
	headCommand = tmp;
}

static bool CHandler_OP(const char* args, CLIENT* caller, char* out) {
	char clientname[64];
	if(String_GetArgument(args, clientname, 64, 0)) {
		CLIENT* tg = Client_FindByName(clientname);
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

static bool CHandler_Stop(const char* args, CLIENT* caller, char* out) {
	serverActive = false;
	return false;
}

static bool CHandler_Test(const char* args, CLIENT* caller, char* out) {
	sprintf(out, "Command \"test\" called by %s with args: %s",
		caller ? caller->playerData->name : "console", args
	);
	return true;
}

static bool CHandler_Announce(const char* args, CLIENT* caller, char* out) {
	if(!caller) caller = Broadcast;
	Packet_WriteChat(caller, CPE_ANNOUNCE, !args ? "Test announcement" : args);
	return false;
}

static bool CHandler_ChangeWorld(const char* args, CLIENT* caller, char* out) {
	if(!caller) return false;
	WORLD* world = World_FindByName(args);
	Client_ChangeWorld(caller, world);
	return false;
}

static bool CHandler_Kick(const char* args, CLIENT* caller, char* out) {
	// ????
	return false;
}

static bool CHandler_Model(const char* args, CLIENT* caller, char* out) {
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
	Command_Register("op", CHandler_OP, true);
	Command_Register("stop", CHandler_Stop, true);
	Command_Register("test", CHandler_Test, false);
	Command_Register("announce", CHandler_Announce, false);
	Command_Register("cw", CHandler_ChangeWorld, false);
	Command_Register("kick", CHandler_Kick, true);
	Command_Register("setmodel", CHandler_Model, true);
}

static bool checkPermissions(COMMAND* cmd, CLIENT* client) {
	if(!client) return true;
	return cmd->onlyOP ? client->playerData->isOP : true;
}

bool Command_Handle(char* cmd, CLIENT* caller) {
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

	COMMAND* tmp = headCommand;

	while(tmp) {
		if(String_CaselessCompare(tmp->name, cmd)) {
			if(!checkPermissions(tmp, caller)) {
				Packet_WriteChat(caller, CPE_CHAT, "Access denied");
				return true;
			}
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
