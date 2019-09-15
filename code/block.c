#include "core.h"
#include "block.h"

bool Block_IsValid(BlockID id) {
	return id >= 0 && id <= 49;
}
