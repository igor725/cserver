#include "core.h"
#include "command.h"
#include "packets.h"
#include "server.h"
#include "generators.h"

void Command_Register(const char* cmd, cmdFunc func) {
	COMMAND tmp = Memory_Alloc(1, sizeof(struct command));

	tmp->name = String_AllocCopy(cmd);
	tmp->func = func;
	if(Command_Head)
		Command_Head->prev = tmp;
	tmp->next = Command_Head;
	Command_Head = tmp;
}

static void Command_Free(COMMAND cmd) {
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
	COMMAND tmp = Command_Head;

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
		CLIENT tg = Client_GetByName(clientname);
		if(tg) {
			bool newtype = !Client_GetType(tg);
			const char* name = tg->playerData->name;
			Client_SetType(tg, newtype);
			String_FormatBuf(out, CMD_MAX_OUT, "Player %s %s", name, newtype ? "opped" : "deopped");
		} else {
			Command_Print("Player not found");
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

	if(!caller) caller = Client_Broadcast;
	Packet_WriteChat(caller, CPE_ANNOUNCE, !args ? "Test announcement" : args);
	return false;
}

static bool CHandler_ChangeWorld(const char* args, CLIENT caller, char* out) {
	const char* cmdUsage = "/chworld <worldname>";
	Command_OnlyForClient;

	char worldname[64];
	Command_ArgToWorldName(worldname, 0);
	WORLD world = World_GetByName(worldname);
	if(world) {
		if(Client_IsInWorld(caller, world)) {
			Command_Print("You already in this world.");
		}
		if(Client_ChangeWorld(caller, world)) return false;
	}
	Command_Print("World not found.");
}

static bool CHandler_Kick(const char* args, CLIENT caller, char* out) {
	const char* cmdUsage = "/kick <player> [reason]";
	Command_OnlyForOP;

	char playername[64];
	if(String_GetArgument(args, playername, 64, 0)) {
		CLIENT tg = Client_GetByName(playername);
		if(tg) {
			const char* reason = String_FromArgument(args, 1);
			Client_Kick(tg, reason);
			String_FormatBuf(out, CMD_MAX_OUT, "Player %s kicked", playername);
		} else {
			Command_Print("Player not found.");
		}
	}

	Command_PrintUsage;
}

static bool CHandler_Model(const char* args, CLIENT caller, char* out) {
	const char* cmdUsage = "/model <modelname/blockid>";
	Command_OnlyForOP;
	Command_OnlyForClient;

	char modelname[64];
	if(String_GetArgument(args, modelname, 64, 0)) {
		if(!Client_SetModel(caller, modelname)) {
			Command_Print("Invalid model name.");
		}
		Command_Print("Model changed successfully.");
	}

	Command_PrintUsage;
}

static bool CHandler_GenWorld(const char* args, CLIENT caller, char* out) {
	const char* cmdUsage = "/genworld <name> <x> <y> <z>";
	Command_OnlyForOP;
	Command_OnlyForClient;

	char worldname[64], x[6], y[6], z[6];
	if(String_GetArgument(args, x, 6, 1) &&
	String_GetArgument(args, y, 6, 2) &&
	String_GetArgument(args, z, 6, 3)) {
		uint16_t _x = (uint16_t)String_ToInt(x),
		_y = (uint16_t)String_ToInt(y),
		_z = (uint16_t)String_ToInt(z);

		if(_x > 0 && _y > 0 && _z > 0) {
			Command_ArgToWorldName(worldname, 0);
			WORLD tmp = World_Create(worldname);
			World_SetDimensions(tmp, _x, _y, _z);
			World_AllocBlockArray(tmp);
			Generator_Flat(tmp);

			if(World_Add(tmp)) {
				String_FormatBuf(out, CMD_MAX_OUT, "World \"%s\" created.", worldname);
			} else {
				World_Free(tmp);
				Command_Print("Too many worlds already loaded.");
			}

			return true;
		}
	}

	Command_PrintUsage;
}

static bool CHandler_UnlWorld(const char* args, CLIENT caller, char* out) {
	const char* cmdUsage = "/unlworld <worldname>";
	Command_OnlyForOP;

	char worldname[64];
	Command_ArgToWorldName(worldname, 0);
	WORLD tmp = World_GetByName(worldname);
	if(tmp) {
		if(tmp->id == 0) {
			Command_Print("Can't unload world with id 0.");
		}
		for(ClientID i = 0; i < MAX_CLIENTS; i++) {
			CLIENT c = Clients_List[i];
			if(c && Client_IsInWorld(c, tmp)) Client_ChangeWorld(c, Worlds_List[0]);
		}
		World_Save(tmp);
		World_Free(tmp);
		Command_Print("World unloaded");
	}
	Command_Print("World not found.");
}

void Command_RegisterDefault(void) {
	Command_Register("op", CHandler_OP);
	Command_Register("stop", CHandler_Stop);
	Command_Register("test", CHandler_Test);
	Command_Register("announce", CHandler_Announce);
	Command_Register("chworld", CHandler_ChangeWorld);
	Command_Register("kick", CHandler_Kick);
	Command_Register("setmodel", CHandler_Model);
	Command_Register("genworld", CHandler_GenWorld);
	Command_Register("unlworld", CHandler_UnlWorld);
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

	COMMAND tmp = Command_Head;

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
