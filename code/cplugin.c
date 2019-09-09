#ifdef CP_ENABLED
#include "core.h"
#include "event.h"
#include "client.h"
#include "server.h"
#include "cplugin.h"

void CPlugin_Start() {
	Directory_Ensure("plugins");
	
}

void CPlugin_Stop() {

}
#endif
