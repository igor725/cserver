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
#include <zlib.h>

Command* HeadCmd;

Command* Command_Register(const char* name, cmdFunc func) {
	if(Command_Get(name)) return NULL;
	Command* tmp = Memory_Alloc(1, sizeof(Command));

	tmp->name = String_AllocCopy(name);
	tmp->func = func;
	if(HeadCmd)
		HeadCmd->prev = tmp;
	tmp->next = HeadCmd;
	HeadCmd = tmp;

	return tmp;
}

void Command_SetAlias(Command* cmd, const char* alias) {
	if(cmd->alias) Memory_Free((void*)cmd->alias);
	cmd->alias = String_AllocCopy(alias);
}

Command* Command_Get(const char* name) {
	Command* ptr = HeadCmd;

	while(ptr) {
		if(String_CaselessCompare(ptr->name, name) ||
		(ptr->alias && String_CaselessCompare(ptr->alias, name)))
			return ptr;
		ptr = ptr->next;
	}

	return NULL;
}

cs_bool Command_Unregister(Command* cmd) {
	if(cmd->prev)
		cmd->prev->next = cmd->next;

	if(cmd->next) {
		cmd->next->prev = cmd->prev;
		HeadCmd = cmd->next;
	} else
		HeadCmd = NULL;

	Memory_Free((void*)cmd->name);
	Memory_Free(cmd);
	return true;
}

cs_bool Command_UnregisterByName(const char* name) {
	Command* cmd = Command_Get(name);
	if(!cmd) return false;
	return Command_Unregister(cmd);
}

static cs_bool CHandler_Info(CommandCallData* ccdata) {
	Command_OnlyForOP(ccdata);

	const char* zver = zlibVersion();
	uLong zflags = zlibCompileFlags();
	const char* format =
	"Some info about server:\r\n"
	"OS: "
	#if defined(WINDOWS)
	"Windows\r\n"
	#elif defined(POSIX)
	"Unix-like\r\n"
	#endif
	"Modules:\r\n"
	"  Server core: %s\r\n"
	"  PluginAPI version: %d\r\n"
	"  zlib version: %s (build flags: %d)";
	Command_Printf(ccdata, format,
		GIT_COMMIT_SHA,
		PLUGIN_API_NUM,
		zver, zflags
	);
}

static cs_bool CHandler_OP(CommandCallData* ccdata) {
	const char* cmdUsage = "/op <playername>";
	Command_OnlyForOP(ccdata);

	char clientname[64];
	if(String_GetArgument(ccdata->args, clientname, 64, 0)) {
		Client* tg = Client_GetByName(clientname);
		if(tg) {
			PlayerData* pd = tg->playerData;
			const char* name = pd->name;
			pd->isOP ^= 1;
			Command_Printf(ccdata, "Player %s %s", name, pd->isOP ? "opped" : "deopped");
		} else {
			Command_Print(ccdata, Lang_Get(LANG_CMDPLNF));
		}
	}
	Command_PrintUsage(ccdata);
}

static cs_bool CHandler_Uptime(CommandCallData* ccdata) {
	cs_uint64 msec, d, h, m, s, ms;
	msec = Time_GetMSec() - Server_StartTime;
	d = msec / 86400000;
	h = (msec % 86400000) / 3600000;
	m = (msec / 60000) % 60000;
	s = (msec / 1000) % 60;
	ms = msec % 1000;
	Command_Printf(ccdata,
		"Server uptime: %03d:%02d:%02d:%02d.%03d",
		d, h, m, s, ms
	);
}

static cs_bool CHandler_CFG(CommandCallData* ccdata) {
	const char* cmdUsage = "/cfg <set/get/print> [key] [value]";
	Command_OnlyForOP(ccdata);

	char subcommand[8], key[MAX_CFG_LEN], value[MAX_CFG_LEN];

	if(String_GetArgument(ccdata->args, subcommand, 8, 0)) {
		if(String_CaselessCompare(subcommand, "set")) {
			if(!String_GetArgument(ccdata->args, key, MAX_CFG_LEN, 1)) {
				Command_PrintUsage(ccdata);
			}
			CEntry ent = Config_GetEntry(Server_Config, key);
			if(!ent) {
				Command_Print(ccdata, "This entry not found in \"server.cfg\" store.");
			}
			if(!String_GetArgument(ccdata->args, value, MAX_CFG_LEN, 2)) {
				Command_PrintUsage(ccdata);
			}

			switch (ent->type) {
				case CFG_INT32:
					Config_SetInt32(ent, String_ToInt(value));
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
					Command_Print(ccdata, "Can't detect entry type.");
			}
			Command_Print(ccdata, "Entry value changed successfully.");
		} else if(String_CaselessCompare(subcommand, "get")) {
			if(!String_GetArgument(ccdata->args, key, MAX_CFG_LEN, 1)) {
				Command_PrintUsage(ccdata);
			}

			CEntry ent = Config_GetEntry(Server_Config, key);
			if(ent) {
				if(!Config_ToStr(ent, value, MAX_CFG_LEN)) {
					Command_Print(ccdata, "Can't detect entry type.");
				}
				Command_Printf(ccdata, "%s = %s (%s)", key, value, Config_TypeName(ent->type));
			}
			Command_Print(ccdata, "This entry not found in \"server.cfg\" store.");
		} else if(String_CaselessCompare(subcommand, "print")) {
			CEntry ent = Server_Config->firstCfgEntry;
			String_Copy(ccdata->out, MAX_CMD_OUT, "Server config entries:");

			while(ent) {
				if(Config_ToStr(ent, value, MAX_CFG_LEN)) {
					String_FormatBuf(key, MAX_CFG_LEN, "\r\n%s = %s (%s)", ent->key, value, Config_TypeName(ent->type));
					String_Append(ccdata->out, MAX_CMD_OUT, key);
				}
				ent = ent->next;
			}

			return true;
		}
	}

	Command_PrintUsage(ccdata);
}

#define GetPluginName \
if(!String_GetArgument(ccdata->args, name, 64, 1)) { \
	Command_Print(ccdata, Lang_Get(LANG_CPINVNAME)); \
} \
const char* lc = String_LastChar(name, '.'); \
if(!lc || !String_CaselessCompare(lc, "." DLIB_EXT)) { \
	String_Append(name, 64, "." DLIB_EXT); \
}

static cs_bool CHandler_Plugins(CommandCallData* ccdata) {
	const char* cmdUsage = "/plugins <load/unload/print> [pluginName]";
	char subcommand[64], name[64];
	Plugin* plugin;

	if(String_GetArgument(ccdata->args, subcommand, 64, 0)) {
		if(String_CaselessCompare(subcommand, "load")) {
			GetPluginName;
			if(!Plugin_Get(name)) {
				if(Plugin_Load(name)) {
					Command_Printf(ccdata,
						Lang_Get(LANG_CPINF0),
						name,
						Lang_Get(LANG_CPLD)
					);
				} else {
					Command_Print(ccdata, "Plugin_Init() = false, plugin unloaded.");
				}
			}
			Command_Print(ccdata, "This plugin already loaded.");
		} else if(String_CaselessCompare(subcommand, "unload")) {
			GetPluginName;
			plugin = Plugin_Get(name);
			if(!plugin) {
				Command_Printf(ccdata,
					Lang_Get(LANG_CPINF0),
					name,
					Lang_Get(LANG_CPNL)
				);
			}
			if(Plugin_Unload(plugin)) {
				Command_Printf(ccdata,
					Lang_Get(LANG_CPINF0),
					name,
					Lang_Get(LANG_CPUNLD)
				);
			}
			else {
				Command_Printf(ccdata,
					Lang_Get(LANG_CPINF1),
					name,
					Lang_Get(LANG_CPCB),
					Lang_Get(LANG_CPUNLD)
				);
			}
		} else if(String_CaselessCompare(subcommand, "list")) {
			cs_int32 idx = 1;
			char pluginfo[64];
			String_Copy(ccdata->out, MAX_CMD_OUT, "Plugins list:");

			for(cs_int32 i = 0; i < MAX_PLUGINS; i++) {
				plugin = Plugins_List[i];
				if(plugin) {
					String_FormatBuf(pluginfo, 64, "\r\n%d. %s v%d", idx++, plugin->name, plugin->version);
					String_Append(ccdata->out, MAX_CMD_OUT, pluginfo);
				}
			}

			return true;
		} else {
			Command_PrintUsage(ccdata);
		}
	}

	Command_PrintUsage(ccdata);
}

static cs_bool CHandler_Stop(CommandCallData* ccdata) {
	Command_OnlyForOP(ccdata);
	Server_Active = false;
	return false;
}

static cs_bool CHandler_Announce(CommandCallData* ccdata) {
	Command_OnlyForOP(ccdata);

	Client_Chat(
		!ccdata->caller ? Broadcast : ccdata->caller,
		MT_ANNOUNCE,
		!ccdata->args ? "Test announcement" : ccdata->args
	);
	return false;
}

static cs_bool CHandler_Kick(CommandCallData* ccdata) {
	const char* cmdUsage = "/kick <player> [reason]";
	Command_OnlyForOP(ccdata);

	char playername[64];
	if(String_GetArgument(ccdata->args, playername, 64, 0)) {
		Client* tg = Client_GetByName(playername);
		if(tg) {
			const char* reason = String_FromArgument(ccdata->args, 1);
			Client_Kick(tg, reason);
			Command_Printf(ccdata, "Player %s kicked", playername);
		} else {
			Command_Print(ccdata, Lang_Get(LANG_CMDPLNF));
		}
	}

	Command_PrintUsage(ccdata);
}

static cs_bool CHandler_SetModel(CommandCallData* ccdata) {
	const char* cmdUsage = "/model <modelname/blockid>";
	Command_OnlyForOP(ccdata);
	Command_OnlyForClient(ccdata);

	char modelname[64];
	if(String_GetArgument(ccdata->args, modelname, 64, 0)) {
		if(!Client_SetModelStr(ccdata->caller, modelname)) {
			Command_Print(ccdata, "Invalid model name.");
		}
		Command_Print(ccdata, "Model changed successfully.");
	}

	Command_PrintUsage(ccdata);
}

static cs_bool CHandler_ChgWorld(CommandCallData* ccdata) {
	const char* cmdUsage = "/chgworld <worldname>";
	Command_OnlyForClient(ccdata);

	char worldname[64];
	Command_ArgToWorldName(ccdata, worldname, 0);
	World* world = World_GetByName(worldname);
	if(world) {
		if(Client_IsInWorld(ccdata->caller, world)) {
			Command_Print(ccdata, "You already in this world.");
		}
		if(Client_ChangeWorld(ccdata->caller, world)) return false;
	}
	Command_Print(ccdata, "World not found.");
}

static cs_bool CHandler_GenWorld(CommandCallData* ccdata) {
	const char* cmdUsage = "/genworld <name> <x> <y> <z>";
	Command_OnlyForOP(ccdata);

	char worldname[64], x[6], y[6], z[6];
	if(String_GetArgument(ccdata->args, x, 6, 1) &&
	String_GetArgument(ccdata->args, y, 6, 2) &&
	String_GetArgument(ccdata->args, z, 6, 3)) {
		cs_uint16 _x = (cs_uint16)String_ToInt(x),
		_y = (cs_uint16)String_ToInt(y),
		_z = (cs_uint16)String_ToInt(z);

		if(_x > 0 && _y > 0 && _z > 0) {
			Command_ArgToWorldName(ccdata, worldname, 0);
			World* tmp = World_Create(worldname);
			SVec vec;
			Vec_Set(vec, _x, _y, _z);
			World_SetDimensions(tmp, &vec);
			World_AllocBlockArray(tmp);
			Generator_Flat(tmp);

			if(World_Add(tmp)) {
				Command_Printf(ccdata, "World \"%s\" created.", worldname);
			} else {
				World_Free(tmp);
				Command_Print(ccdata, "Too many worlds already loaded.");
			}
		}
	}

	Command_PrintUsage(ccdata);
}

static cs_bool CHandler_UnlWorld(CommandCallData* ccdata) {
	const char* cmdUsage = "/unlworld <worldname>";
	Command_OnlyForOP(ccdata);

	char worldname[64];
	Command_ArgToWorldName(ccdata, worldname, 0);
	World* tmp = World_GetByName(worldname);
	if(tmp) {
		if(tmp->id == 0) {
			Command_Print(ccdata, "Can't unload world with id 0.");
		}
		for(ClientID i = 0; i < MAX_CLIENTS; i++) {
			Client* c = Clients_List[i];
			if(c && Client_IsInWorld(c, tmp)) Client_ChangeWorld(c, Worlds_List[0]);
		}
		if(World_Save(tmp, true)) {
			Command_Print(ccdata, "World unloaded.");
		} else {
			Command_Print(ccdata, "Can't start world saving process, try again later.");
		}
	}
	Command_Print(ccdata, "World not found.");
}

static cs_bool CHandler_SavWorld(CommandCallData* ccdata) {
	const char* cmdUsage = "/savworld <worldname>";
	Command_OnlyForOP(ccdata);

	char worldname[64];
	Command_ArgToWorldName(ccdata, worldname, 0);
	World* tmp = World_GetByName(worldname);
	if(tmp) {
		if(World_Save(tmp, false)) {
			Command_Print(ccdata, "World saving scheduled.");
		} else {
			Command_Print(ccdata, "Can't start world saving process, try again later.");
		}
	}
	Command_Print(ccdata, "World not found.");
}

void Command_RegisterDefault(void) {
	Command_Register("info", CHandler_Info);
	Command_Register("op", CHandler_OP);
	Command_Register("uptime", CHandler_Uptime);
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
static void SendOutputToClient(Client* client, char* ret) {
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

cs_bool Command_Handle(char* cmd, Client* caller) {
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

	CommandCallData ccdata;
	ccdata.args = (const char*)args;
	ccdata.caller = caller;
	ccdata.out = ret;

	Command* _cmd = Command_Get(cmd);
	if(_cmd) {
		ccdata.command = _cmd;
		if(_cmd->func(&ccdata)) {
			if(caller) {
				SendOutputToClient(caller, ret);
			} else
				Log_Info(ret);
		}
		return true;
	}

	return false;
}
