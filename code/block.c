#include "core.h"
#include "server.h"
#include "block.h"

static const char* DefaultBlockNames[] = {
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


cs_bool Block_IsValid(BlockID id) {
	return id < 66;
}

const char* Block_GetName(BlockID id) {
	if(Block_IsValid(id))
		return DefaultBlockNames[id];
	return "Unknown block";
}
