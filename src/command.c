#include "core.h"
#include "str.h"
#include "list.h"
#include "log.h"
#include "platform.h"
#include "client.h"
#include "command.h"
#include "lang.h"

KListField *headCmd = NULL;

Command *Command_Register(cs_str name, cmdFunc func, cs_byte flags) {
	if(Command_GetByName(name)) return NULL;
	Command *tmp = Memory_Alloc(1, sizeof(Command));
	tmp->flags = flags;
	tmp->func = func;
	KList_Add(&headCmd, (void *)String_AllocCopy(name), tmp);
	return tmp;
}

void Command_SetAlias(Command *cmd, cs_str alias) {
	if(cmd->alias) Memory_Free((void *)cmd->alias);
	cmd->alias = String_AllocCopy(alias);
}

Command *Command_GetByName(cs_str name) {
	KListField *field;
	List_Iter(field, headCmd) {
		if(String_CaselessCompare(field->key.str, name))
			return field->value.ptr;

		Command *cmd = field->value.ptr;
		if(cmd->alias && String_CaselessCompare(cmd->alias, name))
			return field->value.ptr;
	}
	return NULL;
}

void Command_Unregister(Command *cmd) {
	KListField *field;
	List_Iter(field, headCmd) {
		if(field->value.ptr == cmd) {
			if(cmd->alias) Memory_Free((void *)cmd->alias);
			Memory_Free(field->key.ptr);
			KList_Remove(&headCmd, field);
			Memory_Free(cmd);
		}
	}
}

void Command_UnregisterByFunc(cmdFunc func) {
	KListField *field;
	List_Iter(field, headCmd) {
		Command *cmd = field->value.ptr;
		if(cmd->func == func) {
			if(cmd->alias) Memory_Free((void *)cmd->alias);
			Memory_Free(field->key.ptr);
			KList_Remove(&headCmd, field);
			Memory_Free(cmd);
		}
	}
}

/*
** По задумке эта функция делит строку с ньлайнами
** на несколько и по одной шлёт их клиенту, вроде
** как оно так и работает, но у меня есть сомнения
** на счёт надёжности данной функции.
** TODO: Разобраться, может ли здесь произойти краш.
*/
static void SendOutput(Client *caller, cs_str ret) {
	if(caller) {
		while(*ret != '\0') {
			cs_char *nlptr = (cs_char *)String_FirstChar(ret, '\r');
			if(nlptr)
				*nlptr++ = '\0';
			else
				nlptr = (cs_char *)ret;
			nlptr = (cs_char *)String_FirstChar(nlptr, '\n');
			if(nlptr) *nlptr++ = '\0';
			Client_Chat(caller, 0, ret);
			if(!nlptr) break;
			ret = nlptr;
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
			SendOutput(caller, Lang_Get(Lang_CmdGrp, 4));
			return true;
		}
		if(cmd->flags & CMDF_OP && (caller && !Client_IsOP(caller))) {
			SendOutput(caller, Lang_Get(Lang_CmdGrp, 1));
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
