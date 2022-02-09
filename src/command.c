#include "core.h"
#include "str.h"
#include "list.h"
#include "log.h"
#include "platform.h"
#include "client.h"
#include "command.h"
#include "strstor.h"

static KListField *headCmd = NULL;

Command *Command_Register(cs_str name, cs_str descr, cmdFunc func, cs_byte flags) {
	if(Command_GetByName(name)) return NULL;
	Command *tmp = Memory_Alloc(1, sizeof(Command));
	tmp->flags = flags;
	tmp->descr = descr;
	tmp->func = func;
	KList_AddField(&headCmd, (void *)String_AllocCopy(name), tmp);
	return tmp;
}

cs_bool Command_SetAlias(Command *cmd, cs_str alias) {
	if(String_Length(alias) > 6) return false;
	String_Copy(cmd->alias, 7, alias);
	return true;
}

Command *Command_GetByName(cs_str name) {
	KListField *field;
	List_Iter(field, headCmd) {
		if(String_CaselessCompare(field->key.str, name))
			return field->value.ptr;

		Command *cmd = field->value.ptr;
		if(String_CaselessCompare(cmd->alias, name))
			return field->value.ptr;
	}
	return NULL;
}

void Command_Unregister(Command *cmd) {
	KListField *field;
	List_Iter(field, headCmd) {
		if(field->value.ptr == cmd) {
			Memory_Free(field->key.ptr);
			KList_Remove(&headCmd, field);
			Memory_Free(cmd);
			break;
		}
	}
}

void Command_UnregisterByFunc(cmdFunc func) {
	KListField *field;
	List_Iter(field, headCmd) {
		Command *cmd = field->value.ptr;
		if(cmd->func == func) {
			Memory_Free(field->key.ptr);
			KList_Remove(&headCmd, field);
			Memory_Free(cmd);
			break;
		}
	}
}

INL static void SendOutput(Client *caller, cs_char *ret) {
	if(caller) {
		cs_char *tmp = ret;
		while(*tmp) {
			cs_char *nlptr = (cs_char *)String_FirstChar(tmp, '\r');
			if(nlptr) *nlptr++ = '\0';
			else nlptr = tmp;
			nlptr = (cs_char *)String_FirstChar(nlptr, '\n');
			if(nlptr) *nlptr++ = '\0';
			Client_Chat(caller, 0, tmp);
			if(!nlptr) break;
			tmp = nlptr;
		}
	} else
		Log_Info(ret);
}

cs_bool Command_Handle(cs_char *str, Client *caller) {
	if(*str == '/') ++str;

	cs_char ret[MAX_CMD_OUT];
	cs_char *args = str;

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

	Command *cmd = Command_GetByName(str);
	if(cmd) {
		if(cmd->flags & CMDF_CLIENT && !caller) {
			Log_Error(Sstor_Get("CMD_NOCON"));
			return true;
		}

		if(cmd->flags & CMDF_OP && (caller && !Client_IsOP(caller))) {
			Client_Chat(caller, MESSAGE_TYPE_CHAT, Sstor_Get("CMD_NOPERM"));
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

static cs_str helpheader = "&eList of available commands:";
COMMAND_FUNC(Help) {
	KListField *tmp;
	if(ccdata->caller)
		Client_Chat(ccdata->caller, MESSAGE_TYPE_CHAT, helpheader);
	else
		Log_Info(helpheader + 2); // TODO: Не делать вот так

	List_Iter(tmp, headCmd) {
		cs_str name = tmp->key.str;
		Command *cmd = (Command *)tmp->value.ptr;
		cs_str descr = cmd->descr;
		if(ccdata->caller) {
			if(cmd->flags & CMDF_OP && !Client_IsOP(ccdata->caller))
				continue;
			
			String_FormatBuf(ccdata->out, MAX_CMD_OUT, "%s - %s", name, descr);
			Client_Chat(ccdata->caller, MESSAGE_TYPE_CHAT, ccdata->out);
		} else Log_Info("%s - %s", name, descr);
	}

	return false;
}

void Command_RegisterDefault(void) {
	COMMAND_ADD(Help, CMDF_NONE, "Prints this message");
}

void Command_UnregisterAll(void) {
	while(headCmd) {
		Memory_Free(headCmd->value.ptr);
		Memory_Free(headCmd->key.ptr);
		KList_Remove(&headCmd, headCmd);
	}
}
