#include <core.h>
#include <client.h>
#include <command.h>
#include <event.h>
#include <block.h>

#include "main.h"

static cs_uint16 WeAT;

static void CubeNormalize(SVec* s, SVec* e) {
	cs_int16 tmp, *a = (cs_int16*)s, *b = (cs_int16*)e;
	for(int i = 0; i < 3; i++) {
		if(a[i] < b[i]) {
			tmp = a[i];
			a[i] = b[i];
			b[i] = tmp;
		}
		a[i]++;
	}
}

static SVec* GetCuboid(Client* client) {
	return Assoc_GetPtr(client, WeAT);
}

static void clickhandler(void* param) {
	onPlayerClick a = param;
	if(Client_GetHeldBlock(a->client) != BLOCK_AIR || a->button == 0)
		return;

	SVec* vecs = GetCuboid(a->client);
	if(!vecs) return;

	cs_bool isVecInvalid = Vec_IsInvalid(a->pos);
	if(isVecInvalid && a->button == 2) {
		Vec_Set(vecs[0], -1, -1, -1);
		Client_RemoveSelection(a->client, 0);
	} else if(!isVecInvalid && a->button == 1) {
		if(vecs[0].x == -1)
			vecs[0] = *a->pos;
		else if(!SVec_Compare(&vecs[0], a->pos) && !SVec_Compare(&vecs[1], a->pos)){
			vecs[1] = *a->pos;
			SVec s = vecs[0], e = vecs[1];
			CubeNormalize(&s, &e);
			Client_MakeSelection(a->client, 0, &s, &e, &DefaultSelectionColor);
		}
	}
}

static cs_bool CHandler_Select(CommandCallData* ccdata) {
	SVec* ptr = GetCuboid(ccdata->caller);
	if(ptr) {
		Client_RemoveSelection(ccdata->caller, 0);
		Assoc_Remove(ccdata->caller, WeAT, true);
		Command_Print(ccdata, "Selection mode &cdisabled");
	}
	ptr = Memory_Alloc(1, sizeof(SVec) * 2);
	Memory_Fill(ptr, sizeof(SVec) * 2, 0xFF);
	Assoc_Set(ccdata->caller, WeAT, ptr);
	Command_Print(ccdata, "Selection mode &aenabled");
}

static cs_bool CHandler_Set(CommandCallData* ccdata) {
	const char* cmdUsage = "/set <blockid>";
	Client* client = ccdata->caller;
	SVec* ptr = GetCuboid(client);
	if(!ptr) {
		Command_Print(ccdata, "Select cuboid first.");
	}

	char blid[4];
	if(!String_GetArgument(ccdata->args, blid, 4, 0)) {
		Command_PrintUsage(ccdata);
	}

	BlockID block = (BlockID)String_ToInt(blid);
	World* world = Client_GetWorld(client);
	SVec s = ptr[0], e = ptr[1];
	CubeNormalize(&s, &e);
	cs_uint32 count = (s.x - e.x) * (s.y - e.y) * (s.z - e.z);
	BulkBlockUpdate bbu;
	Block_BulkUpdateClean(&bbu);
	bbu.world = world;
	bbu.autosend = true;

	for(cs_uint16 x = e.x; x < s.x; x++) {
		for(cs_uint16 y = e.y; y < s.y; y++) {
			for(cs_uint16 z = e.z; z < s.z; z++) {
				SVec pos; Vec_Set(pos, x, y, z);
				cs_uint32 offset = World_GetOffset(world, &pos);
				if(offset > 0) {
					Block_BulkUpdateAdd(&bbu, offset, block);
					World_SetBlockO(world, offset, block);
				}
			}
		}
	}

	Block_BulkUpdateSend(&bbu);
	Command_Printf(ccdata, "%d blocks filled with %d.", count, block);
}

static cs_bool CHandler_Replace(CommandCallData* ccdata) {
	const char* cmdUsage = "/repalce <from> <to>";
	Client* client = ccdata->caller;
	SVec* ptr = GetCuboid(client);
	if(!ptr) {
		Command_Print(ccdata, "Select cuboid first.");
	}

	char fromt[4], tot[4];
	if(!String_GetArgument(ccdata->args, fromt, 4, 0) ||
	!String_GetArgument(ccdata->args, tot, 4, 1)) {
		Command_PrintUsage(ccdata);
	}

	BlockID from = (BlockID)String_ToInt(fromt);
	BlockID to = (BlockID)String_ToInt(tot);
	World* world = Client_GetWorld(client);
	SVec s = ptr[0], e = ptr[1];
	CubeNormalize(&s, &e);
	cs_uint32 count = 0;
	BulkBlockUpdate bbu;
	Block_BulkUpdateClean(&bbu);
	bbu.world = world;
	bbu.autosend = true;

	for(cs_uint16 x = e.x; x < s.x; x++) {
		for(cs_uint16 y = e.y; y < s.y; y++) {
			for(cs_uint16 z = e.z; z < s.z; z++) {
				SVec pos; Vec_Set(pos, x, y, z);
				cs_uint32 offset = World_GetOffset(world, &pos);
				if(offset > 0 && world->data[offset] == from) {
					Block_BulkUpdateAdd(&bbu, offset, to);
					World_SetBlockO(world, offset, to);
					count++;
				}
			}
		}
	}

	Block_BulkUpdateSend(&bbu);
	Command_Printf(ccdata, "%d blocks replaced with %d.", count, to);
}

static void freeselvecs(void* param) {
	Assoc_Remove((Client*)param, WeAT, true);
}

Plugin_SetVersion(1);

cs_bool Plugin_Load(void) {
	WeAT = Assoc_NewType();
	Command* cmd;
	cmd = Command_Register("select", CHandler_Select);
	Command_SetAlias(cmd, "sel");
	cmd = Command_Register("fill", CHandler_Set);
	Command_SetAlias(cmd, "set");
	cmd = Command_Register("replace", CHandler_Replace);
	Command_SetAlias(cmd, "repl");
	Event_RegisterVoid(EVT_ONCLICK, clickhandler);
	Event_RegisterVoid(EVT_ONDISCONNECT, freeselvecs);
	return true;
}

cs_bool Plugin_Unload(void) {
	Assoc_DelType(WeAT, true);
	Command_UnregisterByName("select");
	Command_UnregisterByName("fill");
	Command_UnregisterByName("replace");
	Event_Unregister(EVT_ONCLICK, clickhandler);
	Event_Unregister(EVT_ONDISCONNECT, freeselvecs);
	return true;
}
