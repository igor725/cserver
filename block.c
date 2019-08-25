#include "core.h"
#include "block.h"

bool Block_IsValid(int id) {
	return id > 0 && id < 66;
}
