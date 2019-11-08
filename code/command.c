#include "core.h"
#include "str.h"
#include "log.h"
#include "platform.h"
#include "error.h"
#include "client.h"
#include "server.h"
#include "generators.h"
#include "command.h"
#include "config.h"
#include "plugin.h"
#include "lang.h"

COMMAND HeadCmd;

void Command_Register(const char* cmd, cmdFunc func) {
	COMMAND tmp = Memory_Alloc(1, sizeof(struct command));

	tmp->name = String_AllocCopy(cmd);
	tmp->func = func;
	if(HeadCmd)
		HeadCmd->prev = tmp;
	tmp->next = HeadCmd;
	HeadCmd = tmp;
}

void Command_Unregister(const char* cmd) {
	COMMAND curr, tmp = HeadCmd;

	while(tmp) {
		curr = tmp;
		tmp = curr->next;

		if(String_CaselessCompare(curr->name, cmd)) {
			if(curr->prev)
				curr->prev->next = curr->prev;

			if(curr->next) {
				curr->next->prev = curr->next;
				HeadCmd = curr->next->prev;
			} else
				HeadCmd = NULL;

			Memory_Free((void*)curr->name);
			Memory_Free(curr);
		}
	}
}

static cs_bool CHandler_OP(const char* args, Client caller, char* out) {
	const char* cmdUsage = "/op <playername>";
	Command_OnlyForOP;

	char clientname[64];
	if(String_GetArgument(args, clientname, 64, 0)) {
		Client tg = Client_GetByName(clientname);
		if(tg) {
			PlayerData pd = tg->playerData;
			const char* name = pd->name;
			pd->isOP ^= 1;
			String_FormatBuf(out, MAX_CMD_OUT, "Player %s %s", name, pd->isOP ? "opped" : "deopped");
			return true;
		} else {
			Command_Print(Lang_Get(LANG_CMDPLNF));
		}
	}
	Command_PrintUsage;
}

static cs_bool CHandler_CFG(const char* args, Client caller, char* out) {
	const char* cmdUsage = "/cfg <set/get/print> [key] [value]";
	Command_OnlyForOP;

	char subcommand[8], key[MAX_CFG_LEN], value[MAX_CFG_LEN];

	if(String_GetArgument(args, subcommand, 8, 0)) {
		if(String_CaselessCompare(subcommand, "set")) {
			if(!String_GetArgument(args, key, MAX_CFG_LEN, 1)) {
				Command_PrintUsage;
			}
			CFGEntry ent = Config_GetEntry(Server_Config, key);
			if(!ent) {
				Command_Print("This entry not found in \"server.cfg\" store.");
			}
			if(!String_GetArgument(args, value, MAX_CFG_LEN, 2)) {
				Command_PrintUsage;
			}

			switch (ent->type) {
				case CFG_INT:
					Config_SetInt(ent, String_ToInt(value));
					break;
				case CFG_INT16:
					Config_SetInt16(ent, (cs_int16)String_ToInt(value));
					break;
				case CFG_INT8:
					Config_SetInt8(ent, (cs_int8)String_ToInt(value));
					break;
				case CFG_BOOL:
					Config_SetBool(ent, String_CaselessCompare(value, "True"));
					break;
				case CFG_STR:
					Config_SetStr(ent, value);
					break;
				default:
					Command_Print("Can't detect entry type.");
			}
			Command_Print("Entry value changed successfully.");
		} else if(String_CaselessCompare(subcommand, "get")) {
			if(!String_GetArgument(args, key, MAX_CFG_LEN, 1)) {
				Command_PrintUsage;
			}

			CFGEntry ent = Config_GetEntry(Server_Config, key);
			if(ent) {
				if(!Config_ToStr(ent, value, MAX_CFG_LEN)) {
					Command_Print("Can't detect entry type.");
				}
				String_FormatBuf(out, MAX_CMD_OUT, "%s = %s (%s)", key, value, Config_TypeName(ent->type));
				return true;
			}
			Command_Print("This entry not found in \"server.cfg\" store.");
		} else if(String_CaselessCompare(subcommand, "print")) {
			CFGEntry ent = Server_Config->firstCfgEntry;
			String_Copy(out, MAX_CMD_OUT, "Server config entries:");

			while(ent) {
				if(Config_ToStr(ent, value, MAX_CFG_LEN)) {
					String_FormatBuf(key, MAX_CFG_LEN, "\r\n%s = %s (%s)", ent->key, value, Config_TypeName(ent->type));
					String_Append(out, MAX_CMD_OUT, key);
				}
				ent = ent->next;
			}

			return true;
		}
	}

	Command_PrintUsage;
}

#define GetPluginName \
if(!String_GetArgument(args, name, 64, 1)) { \
	String_Copy(out, MAX_CMD_OUT, Lang_Get(LANG_CPINVNAME)); \
	return true; \
} \
const char* lc = String_LastChar(name, '.'); \
if(!lc || !String_CaselessCompare(lc, "." DLIB_EXT)) { \
	String_Append(name, 64, "." DLIB_EXT); \
}

static cs_bool CHandler_Plugins(const char* args, Client caller, char* out) {
	const char* cmdUsage = "/plugins <load/unload/print> [pluginName]";
	char subcommand[64], name[64];
	Plugin plugin;
	(void)caller;

	if(String_GetArgument(args, subcommand, 64, 0)) {
		if(String_CaselessCompare(subcommand, "load")) {
			GetPluginName;
			if(!Plugin_Get(name)) {
				if(Plugin_Load(name)) {
					String_FormatBuf(out, MAX_CMD_OUT,
						Lang_Get(LANG_CPINF0),
						name,
						Lang_Get(LANG_CPLD)
					);
					return true;
				} else {
					Command_Print("Plugin_Init() = false, plugin unloaded.");
				}
			}
			Command_Print("This plugin already loaded.");
		} else if(String_CaselessCompare(subcommand, "unload")) {
			GetPluginName;
			plugin = Plugin_Get(name);
			if(!plugin) {
				String_FormatBuf(out, MAX_CMD_OUT,
					Lang_Get(LANG_CPINF0),
					name,
					Lang_Get(LANG_CPNL)
				);
				return true;
			}
			if(Plugin_Unload(plugin))
				String_FormatBuf(out, MAX_CMD_OUT,
					Lang_Get(LANG_CPINF0),
					name,
					Lang_Get(LANG_CPUNLD)
				);
			else
				String_FormatBuf(out, MAX_CMD_OUT,
					Lang_Get(LANG_CPINF1),
					name,
					Lang_Get(LANG_CPCB),
					Lang_Get(LANG_CPUNLD)
				);

			return true;
		} else if(String_CaselessCompare(subcommand, "list")) {
			cs_int32 idx = 1;
			char pluginfo[64];
			String_Copy(out, MAX_CMD_OUT, "Plugins list:");

			for(cs_int32 i = 0; i < MAX_PLUGINS; i++) {
				plugin = Plugins_List[i];
				if(plugin) {
					String_FormatBuf(pluginfo, 64, "\r\n%d. %s", idx++, plugin->name);
					String_Append(out, MAX_CMD_OUT, pluginfo);
				}
			}

			return true;
		} else {
			Command_PrintUsage;
		}
	}

	Command_PrintUsage;
}

static cs_bool CHandler_Stop(const char* args, Client caller, char* out) {
	Command_OnlyForOP;
	(void)args;

	Server_Active = false;
	return false;
}

static cs_bool CHandler_Announce(const char* args, Client caller, char* out) {
	Command_OnlyForOP;

	if(!caller) caller = Broadcast;
	Client_Chat(caller, MT_ANNOUNCE, !args ? "Test announcement" : args);
	return false;
}

static cs_bool CHandler_Kick(const char* args, Client caller, char* out) {
	const char* cmdUsage = "/kick <player> [reason]";
	Command_OnlyForOP;

	char playername[64];
	if(String_GetArgument(args, playername, 64, 0)) {
		Client tg = Client_GetByName(playername);
		if(tg) {
			const char* reason = String_FromArgument(args, 1);
			Client_Kick(tg, reason);
			String_FormatBuf(out, MAX_CMD_OUT, "Player %s kicked", playername);
			return true;
		} else {
			Command_Print(Lang_Get(LANG_CMDPLNF));
		}
	}

	Command_PrintUsage;
}

static cs_bool CHandler_SetModel(const char* args, Client caller, char* out) {
	const char* cmdUsage = "/model <modelname/blockid>";
	Command_OnlyForOP;
	Command_OnlyForClient;

	char modelname[64];
	if(String_GetArgument(args, modelname, 64, 0)) {
		if(!Client_SetModelStr(caller, modelname)) {
			Command_Print("Invalid model name.");
		}
		Command_Print("Model changed successfully.");
	}

	Command_PrintUsage;
}

static cs_bool CHandler_ChgWorld(const char* args, Client caller, char* out) {
	const char* cmdUsage = "/chgworld <worldname>";
	Command_OnlyForClient;

	char worldname[64];
	Command_ArgToWorldName(worldname, 0);
	World world = World_GetByName(worldname);
	if(world) {
		if(Client_IsInWorld(caller, world)) {
			Command_Print("You already in this world.");
		}
		if(Client_ChangeWorld(caller, world)) return false;
	}
	Command_Print("World not found.");
}

static cs_bool CHandler_GenWorld(const char* args, Client caller, char* out) {
	const char* cmdUsage = "/genworld <name> <x> <y> <z>";
	Command_OnlyForOP;

	char worldname[64], x[6], y[6], z[6];
	if(String_GetArgument(args, x, 6, 1) &&
	String_GetArgument(args, y, 6, 2) &&
	String_GetArgument(args, z, 6, 3)) {
		cs_uint16 _x = (cs_uint16)String_ToInt(x),
		_y = (cs_uint16)String_ToInt(y),
		_z = (cs_uint16)String_ToInt(z);

		if(_x > 0 && _y > 0 && _z > 0) {
			Command_ArgToWorldName(worldname, 0);
			World tmp = World_Create(worldname);
			SVec vec;
			Vec_Set(vec, _x, _y, _z);
			World_SetDimensions(tmp, &vec);
			World_AllocBlockArray(tmp);
			Generator_Flat(tmp);

			if(World_Add(tmp)) {
				String_FormatBuf(out, MAX_CMD_OUT, "World \"%s\" created.", worldname);
			} else {
				World_Free(tmp);
				Command_Print("Too many worlds already loaded.");
			}

			return true;
		}
	}

	Command_PrintUsage;
}

static cs_bool CHandler_UnlWorld(const char* args, Client caller, char* out) {
	const char* cmdUsage = "/unlworld <worldname>";
	Command_OnlyForOP;

	char worldname[64];
	Command_ArgToWorldName(worldname, 0);
	World tmp = World_GetByName(worldname);
	if(tmp) {
		if(tmp->id == 0) {
			Command_Print("Can't unload world with id 0.");
		}
		for(ClientID i = 0; i < MAX_CLIENTS; i++) {
			Client c = Clients_List[i];
			if(c && Client_IsInWorld(c, tmp)) Client_ChangeWorld(c, Worlds_List[0]);
		}
		if(World_Save(tmp, true, true)) {
			Command_Print("World unloading scheduled.");
		} else {
			Command_Print("Can't start world saving process, try again later.");
		}
	}
	Command_Print("World not found.");
}

static cs_bool CHandler_SavWorld(const char* args, Client caller, char* out) {
	const char* cmdUsage = "/savworld <worldname>";
	Command_OnlyForOP;

	char worldname[64];
	Command_ArgToWorldName(worldname, 0);
	World tmp = World_GetByName(worldname);
	if(tmp) {
		if(World_Save(tmp, true, false)) {
			Command_Print("World saving scheduled.");
		} else {
			Command_Print("Can't start world saving process, try again later.");
		}
	}
	Command_Print("World not found.");
}

void Command_RegisterDefault(void) {
	Command_Register("op", CHandler_OP);
	Command_Register("cfg", CHandler_CFG);
	Command_Register("plugins", CHandler_Plugins);
	Command_Register("stop", CHandler_Stop);
	Command_Register("announce", CHandler_Announce);
	Command_Register("kick", CHandler_Kick);
	Command_Register("setmodel", CHandler_SetModel);
	Command_Register("chgworld", CHandler_ChgWorld);
	Command_Register("genworld", CHandler_GenWorld);
	Command_Register("unlworld", CHandler_UnlWorld);
	Command_Register("savworld", CHandler_SavWorld);
}

/*
** По задумке эта функция делит строку с ньлайнами
** на несколько и по одной шлёт их клиенту, вроде
** как оно так и работает, но у меня есть сомнения
** на счёт надёжности данной функции.
** TODO: Разобраться, может ли здесь произойти краш.
*/
static void SendOutputToClient(Client client, char* ret) {
	while(ret && *ret != '\0') {
		char* nlptr = (char*)String_FirstChar(ret, '\r');
		if(nlptr)
			*nlptr++ = '\0';
		else
			nlptr = ret;
		nlptr = (char*)String_FirstChar(nlptr, '\n');
		if(nlptr) *nlptr++ = '\0';
		Client_Chat(client, 0, ret);
		ret = nlptr;
	}
}

cs_bool Command_Handle(char* cmd, Client caller) {
	if(*cmd == '/') ++cmd;

	char ret[MAX_CMD_OUT] = {0};
	char* args = cmd;

	while(1) {
		++args;
		if(*args == '\0') {
			args = NULL;
			break;
		} else if(*args == 32) {
			*args++ = 0;
			break;
		}
	}

	COMMAND tmp = HeadCmd;

	while(tmp) {
		if(String_CaselessCompare(tmp->name, cmd)) {
			if(tmp->func((const char*)args, caller, ret)) {
				if(caller) {
					SendOutputToClient(caller, ret);
				} else
					Log_Info(ret);
			}
			return true;
		}
		tmp = tmp->next;
	}

	return false;
}
