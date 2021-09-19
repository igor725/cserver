#include "core.h"
#include "platform.h"

// #include "../../cs_memleak.c"

#include "platforms/shared.c"
#if defined(WINDOWS)
#include "platforms/windows.c"
#elif defined(UNIX)
#include "platforms/unix.c"
#endif
