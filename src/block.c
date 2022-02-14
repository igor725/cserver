#include "core.h"
#include "str.h"
#include "client.h"
#include "block.h"
#include "strstor.h"

static cs_str defaultBlockNames[] = {
	"Air", "Stone", "Grass", "Dirt",
	"Cobblestone", "Planks", "Sapling",
	"Bedrock", "Water", "Still Water",
	"Lava", "Still Lava", "Sand",
	"Gravel", "Gold Ore", "Iron Ore",
	"Coal Ore", "Wood", "Leaves",
	"Sponge", "Glass", "Red Wool",
	"Orange Wool", "Yellow Wool",
	"Lime Wool", "Green Wool",
	"Teal Wool", "Aqua Wool",
	"Cyan Wool", "Blue Wool",
	"Indigo Wool", "Violen Wool",
	"Magenta Wool", "Pink Wool",
	"Black Wool", "Gray Wool",
	"White Wool", "Dandelion",
	"Rose", "Brown Mushroom",
	"Red Mushroom", "Gold Block",
	"Iron Block", "Double Slab",
	"Slab", "Brick", "TNT", "Bookshelf",
	"Mossy Stone", "Obsidian",
	"Cobblestone Slab", "Rope", "Sandstone",
	"Snow", "Fire", "Light-Pink Wool",
	"Forest-Green Wool", "Brown Wool",
	"Deep-Blue Wool", "Turquoise Wool", "Ice",
	"Ceramic Tile", "Magma", "Pillar", "Crate",
	"Stone Brick"
};

cs_bool Block_IsValid(World *world, BlockID id) {
	return id < 254 && (id < 50 || world->info.bdefines[id] != NULL);
}

cs_str Block_GetName(World *world, BlockID id) {
	if(!Block_IsValid(world, id))
		return Sstor_Get("BLOCK_UNK");

	if(world->info.bdefines[id])
		return world->info.bdefines[id]->name;

	return defaultBlockNames[id];
}

BlockDef *Block_New(BlockID id, cs_str name, cs_byte flags) {
	BlockDef *bdef = Memory_Alloc(1, sizeof(BlockDef));
	String_Copy(bdef->name, 65, name);
	bdef->flags = BDF_DYNALLOCED | flags;
	bdef->id = id;
	return bdef;
}

void Block_Free(BlockDef *bdef) {
	if(bdef->flags & BDF_DYNALLOCED)
		Memory_Free(bdef);
}

cs_bool Block_Define(World *world, BlockDef *bdef) {
	if(world->info.bdefines[bdef->id]) return false;
	bdef->flags &= ~(BDF_UPDATED | BDF_UNDEFINED);
	world->info.bdefines[bdef->id] = bdef;
	return true;
}

BlockDef *Block_GetDefinition(World *world, BlockID id) {
	return world->info.bdefines[id];
}

cs_bool Block_Undefine(World *world, BlockDef *bdef) {
	if(!world->info.bdefines[bdef->id]) return false;
	for(ClientID id = 0; id < MAX_CLIENTS; id++) {
		Client *client = Clients_List[id];
		if(client && Client_IsInWorld(client, world))
			Client_UndefineBlock(client, bdef->id);
	}
	world->info.bdefines[bdef->id] = NULL;
	return true;
}

void Block_UndefineGlobal(BlockDef *bdef) {
	bdef->flags |= BDF_UNDEFINED;
	bdef->flags &= ~BDF_UPDATED;
}

void Block_UpdateDefinition(BlockDef *bdef) {
	if(bdef->flags & BDF_UPDATED) return;
	bdef->flags |= BDF_UPDATED;

	AListField *tmp;
	List_Iter(tmp, World_Head) {
		World *world = (World *)tmp->value.ptr;
		if(world->info.bdefines[bdef->id] == bdef) {
			if(bdef->flags & BDF_UNDEFINED) {
				for(ClientID id = 0; id < MAX_CLIENTS; id++) {
					Client *client = Clients_List[id];
					if(client && Client_IsInWorld(client, world))
						Client_UndefineBlock(client, bdef->id);
				}
				world->info.bdefines[bdef->id] = NULL;
			} else {
				for(ClientID id = 0; id < MAX_CLIENTS; id++) {
					Client *client = Clients_List[id];
					if(client && Client_IsInWorld(client, world))
						Client_DefineBlock(client, bdef);
				}
			}
		}
	}
}

cs_bool Block_BulkUpdateAdd(BulkBlockUpdate *bbu, cs_uint32 offset, BlockID id) {
	if(bbu->data.count == 255) {
		if(bbu->autosend) {
			Block_BulkUpdateSend(bbu);
			Block_BulkUpdateClean(bbu);
		} else return false;
	}
	((cs_uint32 *)bbu->data.offsets)[bbu->data.count++] = htonl(offset);
	bbu->data.ids[bbu->data.count] = id;
	return true;
}

void Block_BulkUpdateSend(BulkBlockUpdate *bbu) {
	for(ClientID cid = 0; cid < MAX_CLIENTS; cid++) {
		Client *client = Clients_List[cid];
		if(client && Client_IsInWorld(client, bbu->world))
			Client_BulkBlockUpdate(client, bbu);
	}
}

void Block_BulkUpdateClean(BulkBlockUpdate *bbu) {
	Memory_Zero(&bbu->data, sizeof(struct _BBUData));
}
