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
	Config_SetDefaultBool(ent, true);
	store->modified = true;
	
	Tests_NewTask("Save config store");
	Tests_Assert(Config_Save(store), "save config store");
	return true;
}
