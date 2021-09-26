#include "core.h"
#include "config.h"
#include "tests.h"

cs_bool Tests_Config(void) {
	Tests_NewTask("Create config store");
	CStore *store;
	CEntry *ent;
	Tests_Assert((store = Config_NewStore("__test.cfg")) != NULL, "create config store");
	Tests_Assert((ent = Config_NewEntry(store, "test-key", CONFIG_TYPE_BOOL)) != NULL, "create boolean entry");
	Tests_Assert(Config_TypeNameToEnum(Config_TypeName(ent->type)) == ent->type, "");
	Config_SetComment(ent, "_test_CoMment1");
	Tests_Assert(Config_SetDefaultBool(ent, true), "set default boolean value");

	Tests_Assert((ent = Config_NewEntry(store, "test-key-i32", CONFIG_TYPE_INT32)) != NULL, "create i32 entry");
	Tests_Assert(Config_TypeNameToEnum(Config_TypeName(ent->type)) == ent->type, "");
	Config_SetComment(ent, "_test_CoMment1-i32_");
	Config_SetDefaultInt32(ent, 40);
	Config_SetLimit(ent, 10, 80);

	Tests_Assert((ent = Config_NewEntry(store, "test-key-i16", CONFIG_TYPE_INT32)) != NULL, "create i16 entry");
	Tests_Assert(Config_TypeNameToEnum(Config_TypeName(ent->type)) == ent->type, "");
	Config_SetComment(ent, "_test_CoMment1-i16-=_");
	Config_SetDefaultInt32(ent, 40);
	Config_SetLimit(ent, 10, 80);
	
	Tests_NewTask("Check main config module functions");
	ent = Config_GetEntry(store, "test-key");
	Tests_Assert(Config_SetDefaultStr(ent, "true") == false, "set default string value to boolean entry");
	Tests_Assert(Config_SetStr(ent, "true") == false, "set string value to boolean entry");
	Tests_Assert(Config_SetDefaultInt32(ent, 40) == false, "set default int32 value to boolean entry");
	Tests_Assert(Config_SetLimit(ent, 10, 20) == false, "set limit for boolean entry");

	Tests_NewTask("Save config store");
	store->modified = true;
	Tests_Assert(Config_Save(store), "save config store");
	return true;
}
