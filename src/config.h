#ifndef CONFIG_H
#define CONFIG_H
#include "core.h"
#include "types/config.h"

API cs_bool Config_HasError(CStore *store);
API ECError Config_PopError(CStore *store, ECExtra *extra, cs_int32 *line);

API cs_str Config_TypeName(ECTypes type);
API ECTypes Config_TypeNameToEnum(cs_str name);
API cs_byte Config_ToStr(CEntry *ent, cs_char *value, cs_byte len);
API cs_str Config_ErrorToString(ECError code);
API cs_str Config_ExtraToString(ECExtra extra);

API CStore *Config_NewStore(cs_str name);
API void Config_EmptyStore(CStore *store);
API void Config_DestroyStore(CStore *store);
API void Config_ResetToDefault(CStore *store);

API CEntry *Config_NewEntry(CStore *store, cs_str key, ECTypes type);
API CEntry *Config_GetEntry(CStore *store, cs_str key);
API ECTypes Config_GetEntryType(CEntry *ent);
API cs_str Config_GetEntryTypeName(CEntry *ent);
API cs_str Config_GetEntryKey(CEntry *ent);

API cs_bool Config_Load(CStore *store);
API cs_bool Config_Save(CStore *store, cs_bool force);

API void Config_SetComment(CEntry *ent, cs_str commentary);
API cs_bool Config_SetLimit(CEntry *ent, cs_int32 min, cs_int32 max);

API cs_int32 Config_GetInt32(CEntry *ent);
API cs_int32 Config_GetInt32ByKey(CStore *store, cs_str key);
API cs_int16 Config_GetInt16(CEntry *ent);
API cs_int16 Config_GetInt16ByKey(CStore *store, cs_str key);
API cs_int8 Config_GetInt8(CEntry *ent);
API cs_int8 Config_GetInt8ByKey(CStore *store, cs_str key);

API cs_bool Config_SetDefaultInt32(CEntry *ent, cs_int32 value);
API cs_bool Config_SetDefaultInt16(CEntry *ent, cs_int16 value);
API cs_bool Config_SetDefaultInt8(CEntry *ent, cs_int8 value);
API cs_bool Config_SetInt32(CEntry *ent, cs_int32 value);
API cs_bool Config_SetInt16(CEntry *ent, cs_int16 value);
API cs_bool Config_SetInt8(CEntry *ent, cs_int8 value);

API cs_str Config_GetStr(CEntry *ent);
API cs_str Config_GetStrByKey(CStore *store, cs_str key);
API cs_bool Config_SetDefaultStr(CEntry *ent, cs_str value);
API cs_bool Config_SetStr(CEntry *ent, cs_str value);

API cs_bool Config_GetBool(CEntry *ent);
API cs_bool Config_GetBoolByKey(CStore *store, cs_str key);
API cs_bool Config_SetDefaultBool(CEntry *ent, cs_bool value);
API cs_bool Config_SetBool(CEntry *ent, cs_bool value);
#endif // CONFIG_H
