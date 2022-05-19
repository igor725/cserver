#include "core.h"
#include "platform.h"
#include "platforms/shared.c"

#if defined(CORE_USE_WINDOWS)
#include "platforms/windows.c"
#elif defined(CORE_USE_UNIX)
#include "platforms/unix.c"
#endif
