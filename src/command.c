#include "core.h"
#include "str.h"
#include "list.h"
#include "log.h"
#include "platform.h"
#include "client.h"
#include "config.h"
#include "command.h"
#include "strstor.h"
#include "server.h"
#include "plugin.h"
#include "event.h"
#include "pager.h"

AListField *Command_Head = NULL;

Command *Command_Register(cs_str name, cs_str descr, cmdFunc func, cs_byte flags) {
	if(Command_GetByName(name)) return NULL;
	Command *tmp = Memory_Alloc(1, sizeof(Command));
	tmp->name = String_AllocCopy(name);
	tmp->flags = flags;
	tmp->descr = descr;
	tmp->func = func;
	AList_AddField(&Command_Head, tmp);
	return tmp;
}

cs_bool Command_RegisterBunch(CommandRegBunch *bunch) {
	for(cs_int32 i = 0; bunch[i].func; i++) {
		CommandRegBunch *cmd = &bunch[i];
		if(!Command_Register(cmd->name, cmd->descr, cmd->func, cmd->flags))
			return false;
	}
	return true;
}

cs_str Command_GetName(Command *cmd) {
	return cmd->name;
}

cs_bool Command_SetAlias(Command *cmd, cs_str alias) {
	Command *tcmd = Command_GetByName(alias);
	if(tcmd && tcmd != cmd) return false;
	String_Copy(cmd->alias, 7, alias);
	return true;
}

void Command_SetUserData(Command *cmd, void *ud) {
	cmd->data = ud;
}

void *Command_GetUserData(Command *cmd) {
	return cmd->data;
}

Command *Command_GetByName(cs_str name) {
	AListField *field;
	List_Iter(field, Command_Head) {
		Command *cmd = field->value.ptr;
		if(String_CaselessCompare(cmd->name, name))
			return field->value.ptr;
		if(String_CaselessCompare(cmd->alias, name))
			return field->value.ptr;
	}
	return NULL;
}

void Command_Unregister(Command *cmd) {
	AListField *field;
	List_Iter(field, Command_Head) {
		if(field->value.ptr == cmd) {
			AList_Remove(&Command_Head, field);
			Memory_Free((void *)cmd->name);
			Memory_Free(cmd);
			break;
		}
	}
}

void Command_UnregisterByFunc(cmdFunc func) {
	AListField *field;
	List_Iter(field, Command_Head) {
		Command *cmd = field->value.ptr;
		if(cmd->func == func) {
			AList_Remove(&Command_Head, field);
			Memory_Free((void *)cmd->name);
			Memory_Free(cmd);
			break;
		}
	}
}

void Command_UnregisterBunch(CommandRegBunch *bunch) {
	for(cs_int32 i = 0; bunch[i].func; i++)
		Command_UnregisterByFunc(bunch[i].func);
}

INL static void SendOutput(Client *caller, cs_char *ret) {
	if(caller) {
		cs_char *tmp = ret;
		while(*tmp) {
			cs_char *nlptr = String_FirstChar(tmp, '\r');
			if(nlptr) *nlptr++ = '\0';
			else nlptr = tmp;
			nlptr = String_FirstChar(nlptr, '\n');
			if(nlptr) *nlptr++ = '\0';
			Client_Chat(caller, 0, tmp);
			if(!nlptr) break;
			tmp = nlptr;
		}
	} else
		Log_Info(ret);
}

cs_bool Command_Handle(cs_char *str, Client *caller) {
	if(*str == '/' && *++str == '\0') return false;

	cs_char ret[MAX_CMD_OUT] = {0};
	cs_char *args = String_FirstChar(str, ' ');
	if(args) *args++ = '\0';

	Command *cmd = Command_GetByName(str);
	if(cmd) {
		if(cmd->flags & CMDF_CLIENT && !caller) {
			Log_Error(Sstor_Get("CMD_NOCON"));
			return true;
		}

		preCommand params = {
			.command = cmd,
			.caller = caller,
			.args = (cs_str)args,
			.allowed = ((cmd->flags & CMDF_OP) == 0) ||
				!caller || Client_IsOP(caller)
		};

		Event_Call(EVT_PRECOMMAND, &params);

		if(!params.allowed) {
			String_Copy(ret, MAX_CMD_OUT, Sstor_Get("CMD_NOPERM"));
			SendOutput(caller, ret);
			return true;
		}

		CommandCallData ccdata = {
			.args = (cs_str)args,
			.caller = caller,
			.command = cmd,
			.out = ret
		};

		if(cmd->func(&ccdata))
			SendOutput(caller, ret);

		return true;
	}

	return false;
}

COMMAND_FUNC(Help) {
	AListField *tmp;

	cs_char temparg[64];
	cs_int32 startPage = 1;
	if(COMMAND_GETARG(temparg, 6, 0))
		startPage = String_ToInt(temparg);
	Pager pager = Pager_Init(startPage, PAGER_DEFAULT_PAGELEN);

	COMMAND_APPEND("&eList of commands:");

	List_Iter(tmp, Command_Head) {
		Pager_Step(pager);

		Command *cmd = (Command *)tmp->value.ptr;
		COMMAND_APPENDF(temparg, 64, "\r\n  &b%s&f - %s", cmd->name, cmd->descr);
	}

	if(Pager_IsDirty(pager))
		COMMAND_APPENDF(temparg, 64, "\r\nPage %d/%d shown",
			Pager_CurrentPage(pager), Pager_CountPages(pager)
		);

	return true;
}

COMMAND_FUNC(Stop) {
	(void)ccdata;
	Server_Active = false;
	return false;
}

COMMAND_FUNC(Say) {
	COMMAND_SETUSAGE("/say <message ...>")
	if(ccdata->args) {
		cs_char message[256];
		if(String_FormatBuf(message, 256, "&eServer&f: %s", ccdata->args)) {
			Client_Chat(CLIENT_BROADCAST, MESSAGE_TYPE_CHAT, message);
			return false;
		}
	}

	COMMAND_PRINTUSAGE;
}

COMMAND_FUNC(Plugin) {
	COMMAND_SETUSAGE("/plugin <load/unload/enable/disable/ifaces/list> [pluginName]");
	cs_char temparg1[64], temparg2[64];
	Plugin *plugin;

	if(COMMAND_GETARG(temparg1, 8, 0)) {
		if(String_CaselessCompare(temparg1, "list")) {
			cs_int32 idx = 0, startPage = 1;
			if(COMMAND_GETARG(temparg1, 8, 1))
				startPage = String_ToInt(temparg1);
			Pager pager = Pager_Init(startPage, PAGER_DEFAULT_PAGELEN);
			COMMAND_APPEND("Loaded plugins list:");

			for(cs_int32 i = 0; i < MAX_PLUGINS; i++) {
				plugin = Plugins_List[i];
				if(plugin) {
					idx += 1;
					Pager_Step(pager);
					COMMAND_APPENDF(
						temparg1, 64,
						"\r\n  %d.%.48s v%d", idx,
						plugin->name, plugin->version
					);
				}
			}

			if(Pager_IsDirty(pager))
				COMMAND_APPENDF(temparg2, 64, "\r\nPage %d/%d shown",
					Pager_CurrentPage(pager), Pager_CountPages(pager)
				);

			return true;
		} else {
			if(!COMMAND_GETARG(temparg2, 64, 1))
				COMMAND_PRINTUSAGE;

			cs_str lc = String_LastChar(temparg2, '.');
			if(!lc || !String_CaselessCompare(lc, "." DLIB_EXT)) {
				String_Append(temparg2, 64, "." DLIB_EXT);
			}

			if(String_CaselessCompare(temparg1, "load")) {
				if(!Plugin_Get(temparg2)) {
					if(Plugin_LoadDll(temparg2, Config_GetBoolByKey(Server_Config, CFG_IGNOREDEP_KEY)))
						COMMAND_PRINTF("Plugin \"%s\" loaded", temparg2);
					COMMAND_PRINT("Something went wrong");
				}
				COMMAND_PRINTF("Plugin \"%s\" is already loaded", temparg2);
			} else if(String_CaselessCompare(temparg1, "enable")) {
				if(Plugin_Enable(temparg2, true))
					COMMAND_PRINT("Plugin enabled successfully");
				else
					COMMAND_PRINT("Failed to enable specified plugin");
			} else if(String_CaselessCompare(temparg1, "disable")) {
				if(Plugin_PerformUnload(temparg2, true))
					COMMAND_PRINT("Plugin disabled successfully");
				else
					COMMAND_PRINT("Failed to disable specified plugin");
			} else {
				plugin = Plugin_Get(temparg2);
				if(!plugin)
					COMMAND_PRINTF("Plugin \"%s\" is not loaded.", temparg2);

				if(String_CaselessCompare(temparg1, "unload")) {
					if(Plugin_UnloadDll(plugin, false)) {
						COMMAND_PRINTF("Plugin \"%s\" successfully unloaded", temparg2);
					} else {
						COMMAND_PRINTF("Plugin \"%s\" cannot be unloaded", temparg2);
					}
				} else if(String_CaselessCompare(temparg1, "ifaces")) {
					COMMAND_APPENDF(temparg1, 64, "&e%s interfaces:", temparg2);
					PluginInterface *iface = plugin->ifaces;
					if(!iface && !plugin->ireqHead)
						COMMAND_PRINTF("&ePlugin %s has not yet interacted with ifaces API", temparg2);

					if(iface) {
						COMMAND_APPEND("\r\n  &bExported&f: ");
						for(; iface && iface->iname; iface++)
							COMMAND_APPENDF(temparg1, 64, "%.60s, ", iface->iname);
					}
					if(plugin->ireqHead) {
						COMMAND_APPEND("\r\n  &5Imported&f: ");
						AListField *tmp;
						List_Iter(tmp, plugin->ireqHead)
							COMMAND_APPENDF(temparg1, 64, "%.60s, ", tmp->value.str);
					}

					return true;
				} else if(String_CaselessCompare(temparg1, "url")) {
					COMMAND_PRINTF("Plugin's homepage: %s", plugin->url ? plugin->url() : "[empty]");
				}
			}
		}
	}

	COMMAND_PRINTUSAGE;
}

void Command_RegisterDefault(void) {
	COMMAND_ADD(Help, CMDF_NONE, "Prints this message");
	COMMAND_ADD(Stop, CMDF_OP, "Stops a server");
	COMMAND_ADD(Say, CMDF_OP, "Sends a message to all players");
	COMMAND_ADD(Plugin, CMDF_OP, "Server plugin manager");
}

void Command_UnregisterAll(void) {
	while(Command_Head) {
		Command *cmd = Command_Head->value.ptr;
		Memory_Free((void *)cmd->name);
		Memory_Free(cmd);
		AList_Remove(&Command_Head, Command_Head);
	}
}
