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

Command* headCmd;

Command* Command_Register(cs_str name, cmdFunc func, cs_uint8 flags) {
	if(Command_GetByName(name)) return NULL;
	Command* tmp = Memory_Alloc(1, sizeof(Command));

	tmp->name = String_AllocCopy(name);
	tmp->flags = flags;
	tmp->func = func;
	if(headCmd)
		headCmd->prev = tmp;
	tmp->next = headCmd;
	headCmd = tmp;

	return tmp;
}

void Command_SetAlias(Command* cmd, cs_str alias) {
	if(cmd->alias) Memory_Free((void*)cmd->alias);
	cmd->alias = String_AllocCopy(alias);
}

Command* Command_GetByName(cs_str name) {
	Command* ptr = headCmd;

	while(ptr) {
		if(String_CaselessCompare(ptr->name, name) ||
		(ptr->alias && String_CaselessCompare(ptr->alias, name)))
			return ptr;
		ptr = ptr->next;
	}

	return NULL;
}

Command* Command_GetByFunc(cmdFunc func) {
	Command* ptr = headCmd;

	while(ptr) {
		if(ptr->func == func)
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
		headCmd = cmd->next;
	} else
		headCmd = NULL;
	Memory_Free((void*)cmd->name);
	Memory_Free(cmd);
	return true;
}

cs_bool Command_UnregisterByName(cs_str name) {
	Command* cmd = Command_GetByName(name);
	if(!cmd) return false;
	return Command_Unregister(cmd);
}

cs_bool Command_UnregisterByFunc(cmdFunc func) {
	Command* cmd = Command_GetByFunc(func);
	if(!cmd) return false;
	return Command_Unregister(cmd);
}

COMMAND_FUNC(Info) {
	static cs_str format =
	"== Some info about server ==\r\n"
	"OS: "
	#if defined(WINDOWS)
	"Windows\r\n"
	#elif defined(POSIX)
	"Unix-like\r\n"
	#endif
	"Server core: %s\r\n"
	"PluginAPI version: %d\r\n"
	"zlib version: %s (build flags: %d)";
	Command_Printf(format,
		GIT_COMMIT_SHA,
		PLUGIN_API_NUM,
		zlibVersion(),
		zlibCompileFlags()
	);
}

COMMAND_FUNC(OP) {
	Command_SetUsage("/op <playername>");

	char clientname[64];
	if(Command_GetArg(clientname, 64, 0)) {
		Client* tg = Client_GetByName(clientname);
		if(tg) {
			PlayerData* pd = tg->playerData;
			cs_str name = pd->name;
			pd->isOP ^= 1;
			Command_Printf("Player %s %s", name, pd->isOP ? "opped" : "deopped");
		} else {
			Command_Print(Lang_Get(LANG_CMDPLNF));
		}
	}
	Command_PrintUsage;
}

COMMAND_FUNC(Uptime) {
	cs_uint64 msec, d, h, m, s, ms;
	msec = Time_GetMSec() - Server_StartTime;
	d = msec / 86400000;
	h = (msec % 86400000) / 3600000;
	m = (msec / 60000) % 60000;
	s = (msec / 1000) % 60;
	ms = msec % 1000;
	Command_Printf(
		"Server uptime: %03d:%02d:%02d:%02d.%03d",
		d, h, m, s, ms
	);
}

COMMAND_FUNC(CFG) {
	Command_SetUsage("/cfg <set/get/print> [key] [value]");
	char subcommand[8], key[MAX_CFG_LEN], value[MAX_CFG_LEN];

	if(Command_GetArg(subcommand, 8, 0)) {
		if(String_CaselessCompare(subcommand, "set")) {
			if(!Command_GetArg(key, MAX_CFG_LEN, 1)) {
				Command_PrintUsage;
			}
			CEntry* ent = Config_GetEntry(Server_Config, key);
			if(!ent) {
				Command_Print("This entry not found in \"server.cfg\" store.");
			}
			if(!Command_GetArg(value, MAX_CFG_LEN, 2)) {
				Command_PrintUsage;
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
					Command_Print("Can't detect entry type.");
			}
			Command_Print("Entry value changed successfully.");
		} else if(String_CaselessCompare(subcommand, "get")) {
			if(!Command_GetArg(key, MAX_CFG_LEN, 1)) {
				Command_PrintUsage;
			}

			CEntry* ent = Config_GetEntry(Server_Config, key);
			if(ent) {
				if(!Config_ToStr(ent, value, MAX_CFG_LEN)) {
					Command_Print("Can't detect entry type.");
				}
				Command_Printf("%s = %s (%s)", key, value, Config_TypeName(ent->type));
			}
			Command_Print("This entry not found in \"server.cfg\" store.");
		} else if(String_CaselessCompare(subcommand, "print")) {
			CEntry* ent = Server_Config->firstCfgEntry;
			Command_Append("Server config entries:");

			while(ent) {
				if(Config_ToStr(ent, value, MAX_CFG_LEN)) {
					cs_str type = Config_TypeName(ent->type);
					Command_Appendf(key, MAX_CFG_LEN, "\r\n%s = %s (%s)", ent->key, value, type);
				}
				ent = ent->next;
			}

			return true;
		}
	}

	Command_PrintUsage;
}

#define GetPluginName \
if(!Command_GetArg(name, 64, 1)) { \
	Command_Print(Lang_Get(LANG_CPINVNAME)); \
} \
cs_str lc = String_LastChar(name, '.'); \
if(!lc || !String_CaselessCompare(lc, "." DLIB_EXT)) { \
	String_Append(name, 64, "." DLIB_EXT); \
}

COMMAND_FUNC(Plugins) {
	Command_SetUsage("/plugins <load/unload/print> [pluginName]");
	char subcommand[8], name[64];
	Plugin* plugin;

	if(Command_GetArg(subcommand, 8, 0)) {
		if(String_CaselessCompare(subcommand, "load")) {
			GetPluginName;
			if(!Plugin_Get(name)) {
				if(Plugin_Load(name)) {
					Command_Printf(
						Lang_Get(LANG_CPINF0),
						name,
						Lang_Get(LANG_CPLD)
					);
				} else {
					Command_Print("Plugin_Init() = false, plugin unloaded.");
				}
			}
			Command_Print("This plugin already loaded.");
		} else if(String_CaselessCompare(subcommand, "unload")) {
			GetPluginName;
			plugin = Plugin_Get(name);
			if(!plugin) {
				Command_Printf(
					Lang_Get(LANG_CPINF0),
					name,
					Lang_Get(LANG_CPNL)
				);
			}
			if(Plugin_Unload(plugin)) {
				Command_Printf(
					Lang_Get(LANG_CPINF0),
					name,
					Lang_Get(LANG_CPUNLD)
				);
			}
			else {
				Command_Printf(
					Lang_Get(LANG_CPINF1),
					name,
					Lang_Get(LANG_CPCB),
					Lang_Get(LANG_CPUNLD)
				);
			}
		} else if(String_CaselessCompare(subcommand, "list")) {
			cs_int32 idx = 1;
			char pluginfo[64];
			Command_Append("Plugins list:");

			for(cs_int32 i = 0; i < MAX_PLUGINS; i++) {
				plugin = Plugins_List[i];
				if(plugin) {
					if(idx > 10) {
						Command_Append("\r\n(Can't show full plugins list)");
						break;
					}
					Command_Appendf(pluginfo, 64, "\r\n%d.%s v%d", idx++, plugin->name, plugin->version);
				}
			}

			return true;
		}
	}

	Command_PrintUsage;
}

COMMAND_FUNC(Stop) {
	(void)ccdata;
	Server_Active = false;
	return false;
}

COMMAND_FUNC(Announce) {
	Client_Chat(
		!ccdata->caller ? Broadcast : ccdata->caller,
		MT_ANNOUNCE,
		!ccdata->args ? "Test announcement" : ccdata->args
	);
	return false;
}

COMMAND_FUNC(Kick) {
	Command_SetUsage("/kick <player> [reason]");

	char playername[64];
	if(Command_GetArg(playername, 64, 0)) {
		Client* tg = Client_GetByName(playername);
		if(tg) {
			cs_str reason = String_FromArgument(ccdata->args, 1);
			Client_Kick(tg, reason);
			Command_Printf("Player %s kicked", playername);
		} else {
			Command_Print(Lang_Get(LANG_CMDPLNF));
		}
	}

	Command_PrintUsage;
}

COMMAND_FUNC(SetModel) {
	Command_SetUsage("/model <modelname/blockid>");

	char modelname[64];
	if(Command_GetArg(modelname, 64, 0)) {
		if(!Client_SetModelStr(ccdata->caller, modelname)) {
			Command_Print("Invalid model name.");
		}
		Command_Print("Model changed successfully.");
	}

	Command_PrintUsage;
}

COMMAND_FUNC(ChgWorld) {
	Command_SetUsage("/chgworld <worldname>");

	char worldname[64];
	Command_ArgToWorldName(worldname, 0);
	World* world = World_GetByName(worldname);
	if(world) {
		if(Client_IsInWorld(ccdata->caller, world)) {
			Command_Print("You already in this world.");
		}
		if(Client_ChangeWorld(ccdata->caller, world)) return false;
	}
	Command_Print("World not found.");
}

COMMAND_FUNC(GenWorld) {
	Command_SetUsage("/genworld <name> <x> <y> <z>");

	char worldname[64], x[6], y[6], z[6];
	if(Command_GetArg(x, 6, 1) &&
	Command_GetArg(y, 6, 2) &&
	Command_GetArg(z, 6, 3)) {
		cs_uint16 _x = (cs_uint16)String_ToInt(x),
		_y = (cs_uint16)String_ToInt(y),
		_z = (cs_uint16)String_ToInt(z);

		if(_x > 0 && _y > 0 && _z > 0) {
			Command_ArgToWorldName(worldname, 0);
			World* tmp = World_Create(worldname);
			SVec vec;
			Vec_Set(vec, _x, _y, _z);
			World_SetDimensions(tmp, &vec);
			World_AllocBlockArray(tmp);
			Generator_Flat(tmp);

			if(World_Add(tmp)) {
				Command_Printf("World \"%s\" created.", worldname);
			} else {
				World_Free(tmp);
				Command_Print("Too many worlds already loaded.");
			}
		}
	}

	Command_PrintUsage;
}

COMMAND_FUNC(UnlWorld) {
	Command_SetUsage("/unlworld <worldname>");

	char worldname[64];
	Command_ArgToWorldName(worldname, 0);
	World* tmp = World_GetByName(worldname);
	if(tmp) {
		if(tmp->id == 0) {
			Command_Print("Can't unload world with id 0.");
		}
		for(ClientID i = 0; i < MAX_CLIENTS; i++) {
			Client* c = Clients_List[i];
			if(c && Client_IsInWorld(c, tmp)) Client_ChangeWorld(c, Worlds_List[0]);
		}
		if(World_Save(tmp, true)) {
			Command_Print("World unloaded.");
		} else {
			Command_Print("Can't start world saving process, try again later.");
		}
	}
	Command_Print("World not found.");
}

COMMAND_FUNC(SavWorld) {
	Command_SetUsage("/savworld <worldname>");

	char worldname[64];
	Command_ArgToWorldName(worldname, 0);
	World* tmp = World_GetByName(worldname);
	if(tmp) {
		if(World_Save(tmp, false)) {
			Command_Print("World saving scheduled.");
		} else {
			Command_Print("Can't start world saving process, try again later.");
		}
	}
	Command_Print("World not found.");
}

void Command_RegisterDefault(void) {
	COMMAND_ADD(Info, CMDF_OP);
	COMMAND_ADD(OP, CMDF_OP);
	COMMAND_ADD(Uptime, CMDF_NONE);
	COMMAND_ADD(CFG, CMDF_OP);
	COMMAND_ADD(Plugins, CMDF_OP);
	COMMAND_ADD(Stop, CMDF_OP);
	COMMAND_ADD(Announce, CMDF_OP);
	COMMAND_ADD(Kick, CMDF_OP);
	COMMAND_ADD(SetModel, CMDF_OP | CMDF_CLIENT);
	COMMAND_ADD(ChgWorld, CMDF_OP | CMDF_CLIENT);
	COMMAND_ADD(GenWorld, CMDF_OP);
	COMMAND_ADD(UnlWorld, CMDF_OP);
	COMMAND_ADD(SavWorld, CMDF_OP);
}

/*
** По задумке эта функция делит строку с ньлайнами
** на несколько и по одной шлёт их клиенту, вроде
** как оно так и работает, но у меня есть сомнения
** на счёт надёжности данной функции.
** TODO: Разобраться, может ли здесь произойти краш.
*/
static void SendOutput(Client* caller, cs_str ret) {
	if(caller) {
		while(*ret != '\0') {
			char* nlptr = (char*)String_FirstChar(ret, '\r');
			if(nlptr)
				*nlptr++ = '\0';
			else
				nlptr = (char*)ret;
			nlptr = (char*)String_FirstChar(nlptr, '\n');
			if(nlptr) *nlptr++ = '\0';
			Client_Chat(caller, 0, ret);
			if(!nlptr) break;
			ret = nlptr;
		}
	} else
		Log_Info(ret);
}

cs_bool Command_Handle(char* str, Client* caller) {
	if(*str == '/') ++str;

	char ret[MAX_CMD_OUT];
	char* args = str;

	while(1) {
		++args;
		if(*args == '\0') {
			args = NULL;
			break;
		} else if(*args == 32) {
			*args++ = '\0';
			break;
		}
	}

	Command* cmd = Command_GetByName(str);
	if(cmd) {
		if(cmd->flags & CMDF_CLIENT && !caller) {
			SendOutput(caller, Lang_Get(LANG_CMDONLYCL));
			return true;
		}
		if(cmd->flags & CMDF_OP && (caller && !Client_IsOP(caller))) {
			SendOutput(caller, Lang_Get(LANG_CMDAD));
			return true;
		}

		CommandCallData ccdata;
		ccdata.args = (cs_str)args;
		ccdata.caller = caller;
		ccdata.command = cmd;
		ccdata.out = ret;
		*ret = '\0';

		if(cmd->func(&ccdata))
			SendOutput(caller, ret);

		return true;
	}

	return false;
}
