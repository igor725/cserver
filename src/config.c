#include "core.h"
#include "platform.h"
#include "str.h"
#include "config.h"

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
	store->error.code = CONFIG_ERROR_PARSE; \
	store->error.extra = CONFIG_EXTRA_PARSE_NUMBER; \
	store->error.line = linenum; \
	File_Close(fp); \
	return false; \
}

CStore *Config_NewStore(cs_str name) {
	if(!String_IsSafe(name)) return false;
	CStore *store = Memory_Alloc(1, sizeof(CStore));
	store->name = String_TrimExtension(String_AllocCopy(name));
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

ECTypes Config_GetEntryType(CEntry *ent) {
	return ent->type;
}

cs_str Config_GetEntryTypeName(CEntry *ent) {
	return Config_TypeName(ent->type);
}

cs_str Config_GetEntryKey(CEntry *ent) {
	return ent->key;
}

CEntry *Config_NewEntry(CStore *store, cs_str key, ECTypes type) {
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
	if(ent->type == CONFIG_TYPE_STR && ent->value.vchar)
		Memory_Free((void *)ent->value.vchar);

	ent->flags &= ~CFG_FCHANGED;
	ent->value.vchar = NULL;
}

static cs_str TypeNames[CONFIG_MAX_TYPE] = {
	"CONFIG_TYPE_BOOL",
	"CONFIG_TYPE_INT32",
	"CONFIG_TYPE_INT16",
	"CONFIG_TYPE_INT8",
	"CONFIG_TYPE_STR"
};

cs_str Config_TypeName(ECTypes type) {
	if(type < CONFIG_MAX_TYPE)
		return TypeNames[type];
	else
		return NULL;
}

ECTypes Config_TypeNameToEnum(cs_str name) {
	for(ECTypes i = 0; i < CONFIG_MAX_TYPE; i++) {
		if(String_CaselessCompare(name, TypeNames[i]))
			return i;
	}
	return -1;
}

INL static cs_byte ToStr(CEntry *ent, cs_char *value, cs_byte len) {
	cs_byte written = 0;
	*value = '\0';

	switch (ent->type) {
		case CONFIG_TYPE_INT32:
			written = (cs_byte)String_FormatBuf(value, len, "%i", Config_GetInt32(ent));
			break;
		case CONFIG_TYPE_INT16:
			written = (cs_byte)String_FormatBuf(value, len, "%hi", Config_GetInt16(ent));
			break;
		case CONFIG_TYPE_INT8:
			written = (cs_byte)String_FormatBuf(value, len, "%hhi", Config_GetInt8(ent));
			break;
		case CONFIG_TYPE_BOOL:
			written = (cs_byte)String_Copy(value, len, Config_GetBool(ent) ? "True" : "False");
			break;
		case CONFIG_TYPE_STR:
			written = (cs_byte)String_Copy(value, len, Config_GetStr(ent));
			break;

		case CONFIG_MAX_TYPE:
		default: return 0;
	}

	return written;
}

cs_bool Config_Load(CStore *store) {
	cs_char path[MAX_PATH_LEN];
	if(String_FormatBuf(path, MAX_PATH_LEN, "configs/%s.cfg", store->name) < 1) {
		store->error.code = CONFIG_ERROR_INTERNAL;
		store->error.extra = CONFIG_EXTRA_NOINFO;
		return false;
	}
	cs_file fp = File_Open(path, "r");
	if(!fp) {
		store->error.code = CONFIG_ERROR_IOFAIL;
		store->error.extra = CONFIG_EXTRA_IO_LINEASERROR;
		store->error.line = Thread_GetError();
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
		cs_char *value = String_FirstChar(line, '=');
		if(!value) {
			store->error.code = CONFIG_ERROR_PARSE;
			store->error.extra = CONFIG_EXTRA_PARSE_LINEFORMAT;
			store->error.line = linenum;
			File_Close(fp);
			return false;
		}
		*value++ = '\0';
		CEntry *ent = Config_GetEntry(store, line);
		if(!ent) {
			haveComment = false;
			continue;
		}
		ent->flags |= CFG_FREADED;

		if(haveComment) {
			Config_SetComment(ent, comment);
			haveComment = false;
		}

		switch (ent->type) {
			case CONFIG_TYPE_STR:
				Config_SetStr(ent, value);
				break;
			case CONFIG_TYPE_INT32:
				CFG_LOADCHECKINT;
				Config_SetInt32(ent, String_ToInt(value));
				break;
			case CONFIG_TYPE_INT8:
				CFG_LOADCHECKINT;
				Config_SetInt8(ent, (cs_int8)String_ToInt(value));
				break;
			case CONFIG_TYPE_INT16:
				CFG_LOADCHECKINT;
				Config_SetInt16(ent, (cs_int16)String_ToInt(value));
				break;
			case CONFIG_TYPE_BOOL:
				Config_SetBool(ent, String_Compare(value, "True"));
				break;

			case CONFIG_MAX_TYPE:
			default:
				store->error.code = CONFIG_ERROR_INTERNAL;
				store->error.extra = CONFIG_EXTRA_NOINFO;
				File_Close(fp);
				return false;
		}
	}
	if(lnret == -1) {
		store->error.code = CONFIG_ERROR_PARSE;
		store->error.extra = CONFIG_EXTRA_PARSE_END;
		File_Close(fp);
		return false;
	}

	File_Close(fp);
	store->error.code = CONFIG_ERROR_SUCCESS;
	store->error.extra = CONFIG_EXTRA_NOINFO;
	store->error.line = 0;
	store->modified = false;

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

cs_bool Config_Save(CStore *store, cs_bool force) {
	if(!store->modified && !force) return true;

	cs_char tmppath[MAX_PATH_LEN]; cs_char path[MAX_PATH_LEN];
	if(String_FormatBuf(tmppath, MAX_PATH_LEN, "configs" PATH_DELIM "%s.tmp", store->name) < 1) {
		store->error.code = CONFIG_ERROR_INTERNAL;
		store->error.extra = CONFIG_EXTRA_NOINFO;
		return false;
	}

	Directory_Ensure("configs");
	cs_file fp = File_Open(tmppath, "w");
	if(!fp) {
		store->error.code = CONFIG_ERROR_IOFAIL;
		store->error.extra = CONFIG_EXTRA_IO_LINEASERROR;
		store->error.line = Thread_GetError();
		return false;
	}

	CEntry *ptr = store->firstCfgEntry;

	while(ptr) {
		if(ptr->commentary && File_WriteFormat(fp, "#%s\n", ptr->commentary) < 0) {
			store->error.code = CONFIG_ERROR_IOFAIL;
			store->error.extra = CONFIG_EXTRA_IO_LINEASERROR;
			store->error.line = Thread_GetError();
			File_Close(fp);
			return false;
		}

		if(!File_Write(ptr->key, 1, String_Length(ptr->key), fp)) {
			store->error.code = CONFIG_ERROR_IOFAIL;
			store->error.extra = CONFIG_EXTRA_IO_LINEASERROR;
			store->error.line = Thread_GetError();
			File_Close(fp);
			return false;
		}

		if(!File_Write("=", 1, 1, fp)) {
			store->error.code = CONFIG_ERROR_IOFAIL;
			store->error.extra = CONFIG_EXTRA_IO_LINEASERROR;
			store->error.line = Thread_GetError();
			File_Close(fp);
			return false;
		}

		cs_char value[CFG_MAX_LEN];
		cs_byte written = ToStr(ptr, value, CFG_MAX_LEN);
		value[written++] = '\n';
		if(written > 0) {
			if(!File_Write(value, 1, written, fp)) {
				store->error.code = CONFIG_ERROR_IOFAIL;
				store->error.extra = CONFIG_EXTRA_IO_LINEASERROR;
				store->error.line = Thread_GetError();
				File_Close(fp);
				return false;
			}
		}
		ptr = ptr->next;
	}

	File_Close(fp);
	store->modified = false;
	if(String_FormatBuf(path, MAX_PATH_LEN, "configs/%s.cfg", store->name) > 0) {
		if(!File_Rename(tmppath, path)) {
			store->error.code = CONFIG_ERROR_IOFAIL;
			store->error.extra = CONFIG_EXTRA_IO_FRENAME;
			store->error.line = 0;
			return false;
		}
	} else {
		store->error.code = CONFIG_ERROR_INTERNAL;
		store->error.extra = CONFIG_EXTRA_NOINFO;
		return false;
	}


	store->error.code = CONFIG_ERROR_SUCCESS;
	store->error.extra = CONFIG_EXTRA_NOINFO;
	store->error.line = 0;
	return true;
}

void Config_SetComment(CEntry *ent, cs_str commentary) {
	if(ent->commentary)
		Memory_Free((void *)ent->commentary);
	ent->commentary = String_AllocCopy(commentary);
}

cs_bool Config_SetLimit(CEntry *ent, cs_int32 min, cs_int32 max) {
	if(min > max || ent->type < CONFIG_TYPE_INT32 || ent->type > CONFIG_TYPE_INT8) return false;
	ent->flags |= CFG_FHAVELIMITS;
	ent->limits[1] = min;
	ent->limits[0] = max;
	return true;
}

cs_bool Config_SetDefaultInt32(CEntry *ent, cs_int32 value) {
	if(ent->type < CONFIG_TYPE_INT32 || ent->type > CONFIG_TYPE_INT8) return false;
	ent->defvalue.vint = value;
	return true;
}

cs_bool Config_SetDefaultInt8(CEntry *ent, cs_int8 value) {
	if(ent->type < CONFIG_TYPE_INT32 || ent->type > CONFIG_TYPE_INT8) return false;
	ent->defvalue.vint8 = value;
	return true;
}

cs_bool Config_SetDefaultInt16(CEntry *ent, cs_int16 value) {
	if(ent->type < CONFIG_TYPE_INT32 || ent->type > CONFIG_TYPE_INT8) return false;
	ent->defvalue.vint16 = value;
	return true;
}

cs_bool Config_SetInt32(CEntry *ent, cs_int32 value) {
	if(ent->type < CONFIG_TYPE_INT32 || ent->type > CONFIG_TYPE_INT8) return false;
	if(Config_GetInt32(ent) != value) {
		ent->flags |= CFG_FCHANGED;
		if(ent->flags & CFG_FHAVELIMITS)
			value = min(max(value, ent->limits[1]), ent->limits[0]);
		ent->value.vint = value;
	}
	return true;
}

cs_bool Config_SetInt16(CEntry *ent, cs_int16 value) {
	if(ent->type < CONFIG_TYPE_INT32 || ent->type > CONFIG_TYPE_INT8) return false;
	if(Config_GetInt16(ent) != value) {
		ent->flags |= CFG_FCHANGED;
		if(ent->flags & CFG_FHAVELIMITS)
			value = (cs_int16)min(max(value, ent->limits[1]), ent->limits[0]);
		ent->value.vint16 = value;
		ent->store->modified = true;
	}
	return true;
}

cs_bool Config_SetInt8(CEntry *ent, cs_int8 value) {
	if(ent->type < CONFIG_TYPE_INT32 || ent->type > CONFIG_TYPE_INT8) return false;
	if(Config_GetInt8(ent) != value) {
		ent->flags |= CFG_FCHANGED;
		if(ent->flags & CFG_FHAVELIMITS)
			value = (cs_int8)min(max(value, ent->limits[1]), ent->limits[0]);
		ent->value.vint8 = value;
		ent->store->modified = true;
	}
	return true;
}

cs_int32 Config_GetInt32(CEntry *ent) {
	if(ent->type < CONFIG_TYPE_INT32 || ent->type > CONFIG_TYPE_INT8) return 0;
	return ent->flags & CFG_FCHANGED ? ent->value.vint : ent->defvalue.vint;
}

cs_int32 Config_GetInt32ByKey(CStore *store, cs_str key) {
	CEntry *ent = Config_GetEntry(store, key);
	return ent != NULL ? Config_GetInt32(ent) : 0;
}

cs_int8 Config_GetInt8(CEntry *ent) {
	if(ent->type < CONFIG_TYPE_INT32 || ent->type > CONFIG_TYPE_INT8) return 0;
	return ent->flags & CFG_FCHANGED ? ent->value.vint8 : ent->defvalue.vint8;
}

cs_int8 Config_GetInt8ByKey(CStore *store, cs_str key) {
	CEntry *ent = Config_GetEntry(store, key);
	return ent != NULL ? Config_GetInt8(ent) : 0;
}

cs_int16 Config_GetInt16(CEntry *ent) {
	if(ent->type < CONFIG_TYPE_INT32 || ent->type > CONFIG_TYPE_INT8) return 0;
	return ent->flags & CFG_FCHANGED ? ent->value.vint16 : ent->defvalue.vint16;
}

cs_int16 Config_GetInt16ByKey(CStore *store, cs_str key) {
	CEntry *ent = Config_GetEntry(store, key);
	return ent != NULL ? Config_GetInt16(ent) : 0;
}

cs_bool Config_SetDefaultStr(CEntry *ent, cs_str value) {
	if(ent->type != CONFIG_TYPE_STR) return false;
	if(ent->defvalue.vchar)
		Memory_Free((void *)ent->defvalue.vchar);
	ent->defvalue.vchar = String_AllocCopy(value);
	return true;
}

cs_bool Config_SetStr(CEntry *ent, cs_str value) {
	if(ent->type != CONFIG_TYPE_STR) return false;
	if(!value) {
		ClearEntry(ent);
		ent->store->modified = true;
	} else if(!String_Compare(value, Config_GetStr(ent))) {
		if(ent->value.vchar && String_Compare(value, ent->value.vchar))
			return true;
		ClearEntry(ent);
		ent->flags |= CFG_FCHANGED;
		ent->value.vchar = String_AllocCopy(value);
		ent->store->modified = true;
	}
	return true;
}

cs_str Config_GetStr(CEntry *ent) {
	if(ent->type != CONFIG_TYPE_STR) return NULL;
	return ent->flags & CFG_FCHANGED ? ent->value.vchar: ent->defvalue.vchar;
}

cs_str Config_GetStrByKey(CStore *store, cs_str key) {
	CEntry *ent = Config_GetEntry(store, key);
	return ent != NULL ? Config_GetStr(ent) : NULL;
}

cs_bool Config_SetDefaultBool(CEntry *ent, cs_bool value) {
	if(ent->type != CONFIG_TYPE_BOOL) return false;
	ent->defvalue.vbool = value;
	return true;
}

cs_bool Config_SetBool(CEntry *ent, cs_bool value) {
	if(ent->type != CONFIG_TYPE_BOOL) return false;
	if(Config_GetBool(ent) != value) {
		ent->flags |= CFG_FCHANGED;
		ent->value.vbool = value;
		ent->store->modified = true;
	}
	return true;
}

cs_bool Config_GetBool(CEntry *ent) {
	if(ent->type != CONFIG_TYPE_BOOL) return false;
	return ent->flags & CFG_FCHANGED ? ent->value.vbool : ent->defvalue.vbool;
}

void Config_SetGeneric(CEntry *ent, cs_str value) {
	switch (ent->type) {
		case CONFIG_TYPE_BOOL:
			Config_SetBool(ent, *value == '1' || String_CaselessCompare(value, "True"));
			break;
		case CONFIG_TYPE_INT32:
			Config_SetInt32(ent, String_ToInt(value));
			break;
		case CONFIG_TYPE_INT16:
			Config_SetInt16(ent, (cs_int16)String_ToInt(value));
			break;
		case CONFIG_TYPE_INT8:
			Config_SetInt8(ent, (cs_int8)String_ToInt(value));
			break;
		case CONFIG_TYPE_STR:
			Config_SetStr(ent, value);
			break;
		case CONFIG_MAX_TYPE: break;
	}
}

cs_int32 Config_Parse(CEntry *ent, cs_char *buf, cs_size len) {
	switch (ent->type) {
		case CONFIG_TYPE_BOOL: return String_FormatBuf(buf, len, "%s", Config_GetBool(ent) ? "True" : "False");
		case CONFIG_TYPE_INT32: case CONFIG_TYPE_INT16: case CONFIG_TYPE_INT8:
			return String_FormatBuf(buf, len, "%i", Config_GetInt32(ent));
		case CONFIG_TYPE_STR: return String_FormatBuf(buf, len, "%s", Config_GetStr(ent));
		default: case CONFIG_MAX_TYPE: return 0;
	}
}

void Config_ResetToDefault(CStore *store) {
	CEntry *ent = store->firstCfgEntry;
	store->modified = true;

	while(ent) {
		ent->flags &= ~CFG_FCHANGED;
		ent = ent->next;
	}
}

cs_bool Config_GetBoolByKey(CStore *store, cs_str key) {
	return Config_GetBool(Config_GetEntry(store, key));
}

static cs_str ErrorStrings[CONFIG_MAX_ERROR] = {
	"CONFIG_ERROR_SUCCESS",
	"CONFIG_ERROR_INTERNAL",
	"CONFIG_ERROR_IOFAIL",
	"CONFIG_ERROR_PARSE"
};

cs_str Config_ErrorToString(ECError code) {
	if(code < CONFIG_MAX_ERROR)
		return ErrorStrings[code];
	else
		return NULL;
}

static cs_str ExtraStrings[CONFIG_MAX_EXTRA] = {
	"CONFIG_EXTRA_NOINFO",
	"CONFIG_EXTRA_IO_LINEASERROR",
	"CONFIG_EXTRA_IO_FRENAME",
	"CONFIG_EXTRA_PARSE_LINEFORMAT",
	"CONFIG_EXTRA_PARSE_NUMBER",
	"CONFIG_EXTRA_PARSE_END"
};

cs_str Config_ExtraToString(ECExtra extra) {
	if(extra < CONFIG_MAX_EXTRA)
		return ExtraStrings[extra];
	else
		return NULL;
}

ECError Config_PopError(CStore *store, ECExtra *extra, cs_int32 *line) {
	if(extra) *extra = store->error.extra;
	if(line) *line = store->error.line;
	return store->error.code;
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
		if(prev->type == CONFIG_TYPE_STR)
			Memory_Free((void *)prev->defvalue.vchar);
		ClearEntry(prev);
		Memory_Free(prev);
	}

	store->modified = true;
	store->firstCfgEntry = NULL;
	store->lastCfgEntry = NULL;
}

void Config_DestroyStore(CStore *store) {
	Memory_Free((void *)store->name);
	Config_EmptyStore(store);
	Memory_Free(store);
}
