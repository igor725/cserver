#include "core.h"
#include "platform.h"
#include "str.h"
#include "config.h"
#include "error.h"

INL static void setError(CStore *store, cs_int32 etype, cs_int32 ecode, cs_int32 eline) {
	store->etype = etype;
	store->ecode = ecode;
	store->eline = eline;
}

INL static void resetError(CStore *store) {
	setError(store, ET_NOERR, 0, 0);
}

INL static void setSysError(CStore *store) {
	setError(store, ET_SYS, 0, 0);
}

INL static cs_bool checkNumber(cs_str n) {
	cs_bool valid = true;
	while(*n && valid) {
		cs_char c = *n++;
		if(c < '0' || c > '9') valid = false;
	}
	return valid;
}

#define CFG_LOADCHECKINT \
if(!checkNumber(value)) { \
	setError(store, ET_SERVER, EC_CFGLINEPARSE, linenum); \
	File_Close(fp); \
	return false; \
}

#define CFG_TYPE(expectedType) \
if(ent->type != expectedType) { \
	ERROR_PRINTF(ET_SERVER, EC_CFGINVGET, true, ent->key, ent->store->path, Config_TypeName(expectedType), Config_TypeName(ent->type)); \
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
		ERROR_PRINTF(ET_SERVER, EC_CFGUNK, true, key, store->path);
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

INL static void ClearEntry(CEntry *ent) {
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
		default:
			return NULL;
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
	return -1;
}

cs_byte Config_ToStr(CEntry *ent, cs_char *value, cs_byte len) {
	cs_byte written = 0;
	*value = '\0';

	switch (ent->type) {
		case CFG_TINT32:
			written = (cs_byte)String_FormatBuf(value, len, "%i", Config_GetInt32(ent));
			break;
		case CFG_TINT16:
			written = (cs_byte)String_FormatBuf(value, len, "%hi", Config_GetInt16(ent));
			break;
		case CFG_TINT8:
			written = (cs_byte)String_FormatBuf(value, len, "%hhi", Config_GetInt8(ent));
			break;
		case CFG_TBOOL:
			written = (cs_byte)String_Copy(value, len, Config_GetBool(ent) ? "True" : "False");
			break;
		case CFG_TSTR:
			written = (cs_byte)String_Copy(value, len, Config_GetStr(ent));
			break;
	}
	
	return written;
}

void Config_PrintError(CStore *store) {
	switch (store->etype) {
		case ET_SERVER:
			if(store->eline > 0) {
				ERROR_PRINTF(store->etype, store->ecode, false, store->eline, store->path);
			} else {
				ERROR_PRINTF(store->etype, store->ecode, false, store->path);
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
		setSysError(store);
		return false;
	}

	cs_bool haveComment = false;
	cs_int32 linenum = 0, lnret = 0;
	cs_char line[CFG_MAX_LEN], comment[CFG_MAX_LEN];

	while(++linenum && (lnret = File_ReadLine(fp, line, CFG_MAX_LEN)) > 0) {
		if(!haveComment && *line == '#') {
			haveComment = true;
			String_Copy(comment, CFG_MAX_LEN, line + 1);
			continue;
		}
		cs_char *value = (cs_char *)String_FirstChar(line, '=');
		if(!value) {
			setError(store, ET_SERVER, EC_CFGLINEPARSE, linenum);
			File_Close(fp);
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
		}
	}
	if(lnret == -1) {
		setError(store, ET_SERVER, EC_CFGEND, linenum);
		File_Close(fp);
		return false;
	}

	File_Close(fp);
	resetError(store);

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
		setSysError(store);
		return false;
	}

	CEntry *ptr = store->firstCfgEntry;

	while(ptr) {
		if(ptr->commentary)
			if(!File_WriteFormat(fp, "#%s\n", ptr->commentary)) {
				setSysError(store);
				File_Close(fp);
				return false;
			}
		if(!File_Write(ptr->key, 1, String_Length(ptr->key), fp)) {
			setSysError(store);
			File_Close(fp);
			return false;
		}

		if(!File_Write("=", 1, 1, fp)) {
			setSysError(store);
			File_Close(fp);
			return false;
		}

		cs_char value[CFG_MAX_LEN];
		cs_byte written = Config_ToStr(ptr, value, CFG_MAX_LEN);
		value[written++] = '\n';
		if(written > 0) {
			if(!File_Write(value, 1, written, fp)) {
				setSysError(store);
				File_Close(fp);
				return false;
			}
		}
		ptr = ptr->next;
	}

	File_Close(fp);
	store->modified = false;
	if(!File_Rename(tmpname, store->path)) {
		setSysError(store);
		return false;
	}

	resetError(store);
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
		if(prev->key)
			Memory_Free((void *)prev->key);
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
