#include "core.h"
#include "platform.h"
#include "str.h"
#include "config.h"
#include "error.h"

#define CFG_SETERROR(type, error, linenum) \
store->etype = ET_SERVER; \
store->ecode = error; \
store->eline = linenum;

#define CFG_SYSERROR CFG_SETERROR(ET_SYS, 0, 0);

#define CFG_LOADCHECKINT \
if(*value < '0' || *value > '9') { \
	CFG_SETERROR(ET_SERVER, EC_CFGLINEPARSE, linenum); \
	return false; \
}

#define CFG_TYPE(expectedType) \
if(ent->type != expectedType) { \
	Error_PrintF2(ET_SERVER, EC_CFGINVGET, true, ent->key, ent->store->path, Config_TypeName(expectedType), Config_TypeName(ent->type)); \
}

CStore *Config_NewStore(cs_str path) {
	CStore *store = Memory_Alloc(1, sizeof(CStore));
	store->path = String_AllocCopy(path);
	return store;
}

CEntry *Config_GetEntry(CStore *store, cs_str key) {
	CEntry *ent = store->firstCfgEntry;

	while(ent) {
		if(String_CaselessCompare(ent->key, key))
			return ent;
		ent = ent->next;
	}

	return NULL;
}

CEntry *Config_CheckEntry(CStore *store, cs_str key) {
	CEntry *ent = Config_GetEntry(store, key);
	if(!ent) {
		Error_PrintF2(ET_SERVER, EC_CFGUNK, true, key, store->path);
	}
	return ent;
}

CEntry *Config_NewEntry(CStore *store, cs_str key, cs_int32 type) {
	CEntry *ent = Config_GetEntry(store, key);
	if(ent) return ent;

	ent = Memory_Alloc(1, sizeof(CEntry));
	ent->key = String_AllocCopy(key);
	ent->store = store;
	ent->type = type;

	if(store->firstCfgEntry)
		store->lastCfgEntry->next = ent;
	else
		store->firstCfgEntry = ent;

	store->lastCfgEntry = ent;
	return ent;
}

NOINL static void ClearEntry(CEntry *ent) {
	if(ent->type == CFG_TSTR && ent->value.vchar)
		Memory_Free((void *)ent->value.vchar);

	ent->flags &= ~CFG_FCHANGED;
	ent->value.vchar = NULL;
}

cs_str Config_TypeName(CETypes type) {
	switch (type) {
		case CFG_TSTR:
			return "string";
		case CFG_TINT32:
			return "int32";
		case CFG_TINT16:
			return "int16";
		case CFG_TINT8:
			return "int8";
		case CFG_TBOOL:
			return "boolean";
		case CFG_TINVALID:
		default:
			return "unknownType";
	}
}

CETypes Config_TypeNameToEnum(cs_str name) {
	if(String_CaselessCompare(name, "string")) {
		return CFG_TSTR;
	} else if(String_CaselessCompare(name, "int32")) {
		return CFG_TINT32;
	} else if(String_CaselessCompare(name, "int16")) {
		return CFG_TINT16;
	} else if(String_CaselessCompare(name, "int8")) {
		return CFG_TINT8;
	} else if(String_CaselessCompare(name, "boolean")) {
		return CFG_TBOOL;
	}
	return CFG_TINVALID;
}

cs_bool Config_ToStr(CEntry *ent, cs_char *value, cs_byte len) {
	switch (ent->type) {
		case CFG_TINT32:
			String_FormatBuf(value, len, "%d", Config_GetInt32(ent));
			break;
		case CFG_TINT16:
			String_FormatBuf(value, len, "%d", Config_GetInt16(ent));
			break;
		case CFG_TINT8:
			String_FormatBuf(value, len, "%d", Config_GetInt8(ent));
			break;
		case CFG_TBOOL:
			String_Copy(value, len, Config_GetBool(ent) ? "True" : "False");
			break;
		case CFG_TSTR:
			String_Copy(value, len, Config_GetStr(ent));
			break;
		case CFG_TINVALID:
			return false;
	}
	return true;
}

void Config_PrintError(CStore *store) {
	switch (store->etype) {
		case ET_SERVER:
			if(store->eline > 0) {
				Error_PrintF2(store->etype, store->ecode, false, store->eline, store->path);
			} else {
				Error_PrintF2(store->etype, store->ecode, false, store->path);
			}
			break;
		case ET_SYS:
			Error_PrintSys(false);
			break;
	}
}

cs_bool Config_Load(CStore *store) {
	cs_file fp = File_Open(store->path, "r");
	if(!fp) {
		if(errno == ENOENT) return true;
		CFG_SYSERROR
		return false;
	}

	cs_bool haveComment = false;
	cs_char line[MAX_CFG_LEN * 2 + 2];
	cs_char comment[MAX_CFG_LEN];
	cs_int32 lnret = 0, linenum = 0;

	while((lnret = File_ReadLine(fp, line, 256)) > 0 && ++linenum) {
		if(!haveComment && *line == '#') {
			haveComment = true;
			String_Copy(comment, MAX_CFG_LEN, line + 1);
			continue;
		}
		cs_char *value = (cs_char *)String_FirstChar(line, '=');
		if(!value) {
			CFG_SETERROR(ET_SERVER, EC_CFGLINEPARSE, linenum)
			return false;
		}
		*value++ = '\0';
		CEntry *ent = Config_CheckEntry(store, line);
		ent->flags |= CFG_FREADED;

		if(haveComment) {
			Config_SetComment(ent, comment);
			haveComment = false;
		}

		switch (ent->type) {
			case CFG_TSTR:
				Config_SetStr(ent, value);
				break;
			case CFG_TINT32:
				CFG_LOADCHECKINT;
				Config_SetInt32(ent, String_ToInt(value));
				break;
			case CFG_TINT8:
				CFG_LOADCHECKINT;
				Config_SetInt8(ent, (cs_int8)String_ToInt(value));
				break;
			case CFG_TINT16:
				CFG_LOADCHECKINT;
				Config_SetInt16(ent, (cs_int16)String_ToInt(value));
				break;
			case CFG_TBOOL:
				Config_SetBool(ent, String_Compare(value, "True"));
				break;
			case CFG_TINVALID: // Eh??
				break;
		}
	}

	if(lnret == -1) {
		CFG_SETERROR(ET_SERVER, EC_CFGEND, 0)
		return false;
	}

	File_Close(fp);
	CFG_SETERROR(ET_NOERR, 0, 0);

	CEntry *ent = store->firstCfgEntry;
	while(ent) {
		if((ent->flags & CFG_FREADED) == 0) {
			store->modified = true;
			break;
		}
		ent = ent->next;
	}

	return true;
}

cs_bool Config_Save(CStore *store) {
	if(!store->modified) return true;

	cs_char tmpname[256];
	String_FormatBuf(tmpname, 256, "%s.tmp", store->path);

	cs_file fp = File_Open(tmpname, "w");
	if(!fp) {
		CFG_SYSERROR
		return false;
	}

	CEntry *ptr = store->firstCfgEntry;

	while(ptr) {
		if(ptr->commentary)
			if(!File_WriteFormat(fp, "#%s\n", ptr->commentary)) {
				CFG_SYSERROR;
				return false;
			}
		if(!File_Write(ptr->key, String_Length(ptr->key), 1, fp)) {
			CFG_SYSERROR;
			return false;
		}

		cs_char *vchar;
		cs_int32 vint;
		cs_bool vbool;

		switch (ptr->type) {
			case CFG_TSTR:
				vchar = (cs_char *)Config_GetStr(ptr);
				if(!File_WriteFormat(fp, "=%s\n", vchar)) {
					CFG_SYSERROR;
					return false;
				}
				break;
			case CFG_TINT32:
				vint = Config_GetInt32(ptr);
				goto cfg_write_int;
			case CFG_TINT16:
				vint = Config_GetInt16(ptr);
				goto cfg_write_int;
			case CFG_TINT8:
				vint = Config_GetInt8(ptr);
				cfg_write_int:
				if(!File_WriteFormat(fp, "=%d\n", vint)) {
					CFG_SYSERROR;
					return false;
				}
				break;
			case CFG_TBOOL:
				vbool = Config_GetBool(ptr);
				if(!File_WriteFormat(fp, "=%s\n", vbool ? "True" : "False")) {
					CFG_SYSERROR;
					return false;
				}
				break;
			case CFG_TINVALID:
				if(!File_Write("=Unknown value\n", 16, 1, fp)) {
					CFG_SYSERROR;
					return false;
				}
				break;
		}
		ptr = ptr->next;
	}

	File_Close(fp);
	store->modified = false;
	if(!File_Rename(tmpname, store->path)) {
		CFG_SYSERROR;
		return false;
	}

	CFG_SETERROR(ET_NOERR, 0, 0);
	return true;
}

void Config_SetComment(CEntry *ent, cs_str commentary) {
	if(ent->commentary)
		Memory_Free((void *)ent->commentary);
	ent->commentary = String_AllocCopy(commentary);
}

void Config_SetLimit(CEntry *ent, cs_int32 min, cs_int32 max) {
	ent->flags |= CFG_FHAVELIMITS;
	ent->limits[1] = min;
	ent->limits[0] = max;
}

void Config_SetDefaultInt32(CEntry *ent, cs_int32 value) {
	CFG_TYPE(CFG_TINT32);
	ent->defvalue.vint = value;
}

void Config_SetDefaultInt8(CEntry *ent, cs_int8 value) {
	CFG_TYPE(CFG_TINT8);
	ent->defvalue.vint8 = value;
}

void Config_SetDefaultInt16(CEntry *ent, cs_int16 value) {
	CFG_TYPE(CFG_TINT16);
	ent->defvalue.vint16 = value;
}

void Config_SetInt32(CEntry *ent, cs_int32 value) {
	CFG_TYPE(CFG_TINT32);
	if(Config_GetInt32(ent) != value) {
		ent->flags |= CFG_FCHANGED;
		if(ent->flags & CFG_FHAVELIMITS)
			value = min(max(value, ent->limits[1]), ent->limits[0]);
		ent->value.vint = value;
	}
}

void Config_SetInt16(CEntry *ent, cs_int16 value) {
	CFG_TYPE(CFG_TINT16);
	if(Config_GetInt16(ent) != value) {
		ent->flags |= CFG_FCHANGED;
		if(ent->flags & CFG_FHAVELIMITS)
			value = (cs_int16)min(max(value, ent->limits[1]), ent->limits[0]);
		ent->value.vint16 = value;
		ent->store->modified = true;
	}
}

void Config_SetInt8(CEntry *ent, cs_int8 value) {
	CFG_TYPE(CFG_TINT8);
	if(Config_GetInt8(ent) != value) {
		ent->flags |= CFG_FCHANGED;
		if(ent->flags & CFG_FHAVELIMITS)
			value = (cs_int8)min(max(value, ent->limits[1]), ent->limits[0]);
		ent->value.vint8 = value;
		ent->store->modified = true;
	}
}

cs_int32 Config_GetInt32(CEntry *ent) {
	CFG_TYPE(CFG_TINT32);
	return ent->flags & CFG_FCHANGED ? ent->value.vint : ent->defvalue.vint;
}

cs_int32 Config_GetInt32ByKey(CStore *store, cs_str key) {
	return Config_GetInt32(Config_CheckEntry(store, key));
}

cs_int8 Config_GetInt8(CEntry *ent) {
	CFG_TYPE(CFG_TINT8);
	return ent->flags & CFG_FCHANGED ? ent->value.vint8 : ent->defvalue.vint8;
}

cs_int8 Config_GetInt8ByKey(CStore *store, cs_str key) {
	return Config_GetInt8(Config_CheckEntry(store, key));
}

cs_int16 Config_GetInt16(CEntry *ent) {
	CFG_TYPE(CFG_TINT16);
	return ent->flags & CFG_FCHANGED ? ent->value.vint16 : ent->defvalue.vint16;
}

cs_int16 Config_GetInt16ByKey(CStore *store, cs_str key) {
	return Config_GetInt16(Config_CheckEntry(store, key));
}

void Config_SetDefaultStr(CEntry *ent, cs_str value) {
	CFG_TYPE(CFG_TSTR);
	if(ent->defvalue.vchar)
		Memory_Free((void *)ent->defvalue.vchar);
	ent->defvalue.vchar = String_AllocCopy(value);
}

void Config_SetStr(CEntry *ent, cs_str value) {
	CFG_TYPE(CFG_TSTR);
	if(!value) {
		ClearEntry(ent);
		ent->store->modified = true;
	}else if(!String_Compare(value, Config_GetStr(ent))) {
		if(ent->value.vchar&& String_Compare(value, ent->value.vchar))
			return;
		ClearEntry(ent);
		ent->flags |= CFG_FCHANGED;
		ent->value.vchar = String_AllocCopy(value);
		ent->store->modified = true;
	}
}

cs_str Config_GetStr(CEntry *ent) {
	CFG_TYPE(CFG_TSTR);
	return ent->flags & CFG_FCHANGED ? ent->value.vchar: ent->defvalue.vchar;
}

cs_str Config_GetStrByKey(CStore *store, cs_str key) {
	return Config_GetStr(Config_CheckEntry(store, key));
}

void Config_SetDefaultBool(CEntry *ent, cs_bool value) {
	CFG_TYPE(CFG_TBOOL);
	ent->defvalue.vbool = value;
}

void Config_SetBool(CEntry *ent, cs_bool value) {
	CFG_TYPE(CFG_TBOOL);
	if(Config_GetBool(ent) != value) {
		ent->flags |= CFG_FCHANGED;
		ent->value.vbool = value;
		ent->store->modified = true;
	}
}

cs_bool Config_GetBool(CEntry *ent) {
	CFG_TYPE(CFG_TBOOL);
	return ent->flags & CFG_FCHANGED ? ent->value.vbool : ent->defvalue.vbool;
}

cs_bool Config_GetBoolByKey(CStore *store, cs_str key) {
	return Config_GetBool(Config_CheckEntry(store, key));
}

void Config_EmptyStore(CStore *store) {
	CEntry *prev, *ent = store->firstCfgEntry;

	while(ent) {
		prev = ent;
		ent = ent->next;
		if(prev->commentary)
			Memory_Free((void *)prev->commentary);
		if(prev->type == CFG_TSTR)
			Memory_Free((void *)prev->defvalue.vchar);
		ClearEntry(prev);
		Memory_Free(prev);
	}

	store->modified = true;
	store->firstCfgEntry = NULL;
	store->lastCfgEntry = NULL;
}

void Config_DestroyStore(CStore *store) {
	Memory_Free((void *)store->path);
	Config_EmptyStore(store);
	Memory_Free(store);
}
