#include "core.h"
#include "str.h"
#include "world.h"
#include "client.h"
#include "block.h"
#include "platform.h"
#include "list.h"

static cs_str defaultBlockNames[BLOCK_DEFAULT_COUNT] = {
	"Air", "Stone", "Grass", "Dirt",
	"Cobblestone", "Wood", "Sapling",
	"Bedrock", "Water", "Still water",
	"Lava", "Still lava", "Sand",
	"Gravel", "Gold ore", "Iron ore",
	"Coal ore", "Log", "Leaves",
	"Sponge", "Glass", "Red", "Orange",
	"Yellow", "Lime", "Green", "Teal",
	"Aqua", "Cyan", "Blue", "Indigo",
	"Violet", "Magenta", "Pink",
	"Black", "Gray", "White",
	"Dandelion", "Rose", "Brown mushroom",
	"Red mushroom", "Gold", "Iron",
	"Double slab", "Slab", "Brick",
	"TNT", "Bookshelf", "Mossy rocks",
	"Obsidian",

	"Cobblestone slab", "Rope", "Sandstone",
	"Snow", "Fire", "Light pink", "Forest green",
	"Brown", "Deep blue", "Turquoise", "Ice",
	"Ceramic tile", "Magma", "Pillar", "Crate",
	"Stone brick"
};

cs_bool Block_IsValid(World *world, BlockID id) {
	return id < BLOCK_DEFAULT_COUNT || world->info.bdefines[id] != NULL;
}

BlockID Block_GetFallbackFor(World *world, BlockID id) {
	if(world->info.bdefines[id])
		return world->info.bdefines[id]->fallback;

	switch(id) {
		case BLOCK_COBBLESLAB: return BLOCK_SLAB;
		case BLOCK_ROPE: return BLOCK_BROWN_SHROOM;
		case BLOCK_SANDSTONE: return BLOCK_SAND;
		case BLOCK_SNOW: return BLOCK_AIR;
		case BLOCK_FIRE: return BLOCK_LAVA;
		case BLOCK_LIGHTPINK: return BLOCK_PINK;
		case BLOCK_FORESTGREEN: return BLOCK_GREEN;
		case BLOCK_BROWN: return BLOCK_DIRT;
		case BLOCK_DEEPBLUE: return BLOCK_BLUE;
		case BLOCK_TURQUOISE: return BLOCK_CYAN;
		case BLOCK_ICE: return BLOCK_GLASS;
		case BLOCK_CERAMICTILE: return BLOCK_IRON;
		case BLOCK_MAGMA: return BLOCK_OBSIDIAN;
		case BLOCK_PILLAR: return BLOCK_WHITE;
		case BLOCK_CRATE: return BLOCK_WOOD;
		case BLOCK_STONEBRICK: return BLOCK_STONE;
		default: return id;
	}
}

cs_str Block_GetName(World *world, BlockID id) {
	if(!Block_IsValid(world, id))
		return "Unknown block";

	if(world->info.bdefines[id])
		return world->info.bdefines[id]->name;

	return defaultBlockNames[id];
}

BlockID Block_GetIDFor(World *world, BlockDef *bdef) {
	for(cs_uint16 i = 0; i < 256; i++) {
		if(world->info.bdefines[i] == bdef)
			return (BlockID)i;
	}
	return BLOCK_AIR;
}

cs_bool Block_Define(World *world, BlockID id, BlockDef *bdef) {
	if(id < 1 || world->info.bdefines[id]) return false;
	if(Block_GetIDFor(world, bdef) > 0) return false;
	bdef->flags &= ~(BDF_UPDATED | BDF_UNDEFINED);
	world->info.bdefines[id] = bdef;
	return true;
}

BlockDef *Block_GetDefinition(World *world, BlockID id) {
	return world->info.bdefines[id];
}

cs_bool Block_Undefine(World *world, BlockDef *bdef) {
	BlockID bid = Block_GetIDFor(world, bdef);
	if(bid < 1) return false;
	for(ClientID id = 0; id < MAX_CLIENTS; id++) {
		Client *client = Clients_List[id];
		if(client && Client_IsInWorld(client, world))
			Client_UndefineBlock(client, bid);
	}
	world->info.bdefines[bid] = NULL;
	return true;
}

void Block_UndefineGlobal(BlockDef *bdef) {
	if((bdef->flags & BDF_UNDEFINED) == 0) {
		bdef->flags |= BDF_UNDEFINED;
		bdef->flags &= ~BDF_UPDATED;
	}
}

void Block_UpdateDefinition(BlockDef *bdef) {
	if(bdef->flags & BDF_UPDATED) return;
	bdef->flags |= BDF_UPDATED;

	AListField *tmp;
	List_Iter(tmp, World_Head) {
		World *world = (World *)tmp->value.ptr;
		BlockID bid = Block_GetIDFor(world, bdef);
		if(bid > BLOCK_AIR) {
			if(bdef->flags & BDF_UNDEFINED) {
				for(ClientID id = 0; id < MAX_CLIENTS; id++) {
					Client *client = Clients_List[id];
					if(client && Client_IsInWorld(client, world))
						Client_UndefineBlock(client, bid);
				}
				world->info.bdefines[bid] = NULL;
			} else {
				for(ClientID id = 0; id < MAX_CLIENTS; id++) {
					Client *client = Clients_List[id];
					if(client && Client_IsInWorld(client, world))
						Client_DefineBlock(client, bid, bdef);
				}
			}
		}
	}
}

cs_bool Block_BulkUpdateAdd(BulkBlockUpdate *bbu, cs_uint32 offset, BlockID id) {
	((cs_uint32 *)bbu->data.offsets)[bbu->data.count] = htonl(offset);
	bbu->data.ids[bbu->data.count++] = id;

	if(bbu->data.count == 255) {
		if(bbu->autosend && bbu->world) {
			Block_BulkUpdateSend(bbu);
			Block_BulkUpdateClean(bbu);
		} else return false;
	}

	return true;
}

cs_bool Block_BulkUpdateSend(BulkBlockUpdate *bbu) {
	if(!bbu->world) return false;

	for(ClientID cid = 0; cid < MAX_CLIENTS; cid++) {
		Client *client = Clients_List[cid];
		if(client && Client_IsInWorld(client, bbu->world))
			Client_BulkBlockUpdate(client, bbu);
	}

	return true;
}

void Block_BulkUpdateClean(BulkBlockUpdate *bbu) {
	Memory_Zero(&bbu->data, sizeof(struct _BBUData));
}
