#include "core.h"
#include "str.h"
#include "list.h"
#include "log.h"
#include "platform.h"
#include "client.h"
#include "command.h"
#include "strstor.h"
#include "server.h"

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

cs_str Command_GetName(Command *cmd) {
	return cmd->name;
}

void Command_SetAlias(Command *cmd, cs_str alias) {
	String_Copy(cmd->alias, 7, alias);
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
	if(*str == '/' && *++str == '\0') return false;

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
	AListField *tmp;
	if(ccdata->caller)
		Client_Chat(ccdata->caller, MESSAGE_TYPE_CHAT, helpheader);
	else
		Log_Info(helpheader + 2); // TODO: Не делать вот так

	List_Iter(tmp, Command_Head) {
		Command *cmd = (Command *)tmp->value.ptr;
		if(ccdata->caller) {
			if(cmd->flags & CMDF_OP && !Client_IsOP(ccdata->caller))
				continue;
			
			String_FormatBuf(ccdata->out, MAX_CMD_OUT, "%s - %s", cmd->name, cmd->descr);
			Client_Chat(ccdata->caller, MESSAGE_TYPE_CHAT, ccdata->out);
		} else Log_Info("%s - %s", cmd->name, cmd->descr);
	}

	return false;
}

COMMAND_FUNC(Stop) {
	(void)ccdata;
	Server_Active = false;
	return false;
}

void Command_RegisterDefault(void) {
	COMMAND_ADD(Help, CMDF_NONE, "Prints this message");
	COMMAND_ADD(Stop, CMDF_OP, "Stops a server");
}

void Command_UnregisterAll(void) {
	while(Command_Head) {
		Command *cmd = (Command *)Command_Head->value.ptr;
		Memory_Free((void *)cmd->name);
		Memory_Free(cmd);
		AList_Remove(&Command_Head, Command_Head);
	}
}
