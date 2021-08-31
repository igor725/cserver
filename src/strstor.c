#include "core.h"
#include "str.h"
#include "list.h"
#include "platform.h"
#include "strstor.h"

KListField *storage = NULL;

cs_bool Sstor_IsExists(cs_str key) {
	KListField *tmp;
	List_Iter(tmp, storage) {
		if(String_CaselessCompare(tmp->key.str, key))
			return true;
	}
	return false;
}

cs_str Sstor_Get(cs_str key) {
	KListField *tmp;
	List_Iter(tmp, storage) {
		if(String_CaselessCompare(tmp->key.str, key))
			return tmp->value.str;
	}
	return NULL;
}

cs_bool Sstor_Set(cs_str key, cs_str value) {
	if(String_Length(key) < 2) return false;
	if(Sstor_IsExists(key)) return false;
	cs_str _key = String_AllocCopy(key),
	_value = String_AllocCopy(value);
	return KList_Add(&storage, (void *)_key, (void *)_value) != NULL;
}

cs_bool Strstro_Remove(cs_str key) {
	KListField *tmp;
	List_Iter(tmp, storage) {
		if(String_CaselessCompare(tmp->key.str, key)) {
			Memory_Free(tmp->key.ptr);
			Memory_Free(tmp->value.ptr);
			KList_Remove(&storage, tmp);
			return true;
		}
	}
	return false;
}

void Sstor_Cleanup(void) {
	KListField *tmp;
	List_Iter(tmp, storage) {
		Memory_Free(tmp->key.ptr);
		Memory_Free(tmp->value.ptr);
		KList_Remove(&storage, tmp);
	}
}

cs_bool Sstor_Defaults(void) {
	Sstor_Set("WGEN_ERROR", "Oh! Error happened in the world generator for \"%s\".");
	Sstor_Set("WGEN_INVDIM", "Invalid dimensions specified for \"%s\".");
	Sstor_Set("WGEN_NOGEN", "Invalid generator specified for \"%s\".");

	Sstor_Set("SV_START", "Server started on %s:%d.");
	Sstor_Set("SV_NOWORLDS", "No worlds loaded.");
	Sstor_Set("SV_WLDONE", "%d world(-s) successfully loaded.");
	Sstor_Set("SV_STOPNOTE", "Press Ctrl+C to stop the server.");
	Sstor_Set("SV_BADTICK_BW", "Time ran backwards? Time between the last two ticks < 0ms.");
	Sstor_Set("SV_BADTICK", "Last server tick took %dms!");
	Sstor_Set("SV_STOP_PL", "Kicking players...");
	Sstor_Set("SV_STOP_SW", "Saving worlds...");
	Sstor_Set("SV_PERR", "Invalid packet 0x%02X from Client[%d]");

	Sstor_Set("CMD_UNK", "Unknown command.");
	Sstor_Set("CMD_NOCON", "This command can't be called from console.");
	Sstor_Set("CMD_NOPERM", "Access denied.");

	Sstor_Set("WS_NOTVALID", "Not a websocket connection.");
	Sstor_Set("WS_SHAERR", "SHA1_Init() returned false.");

	Sstor_Set("NONAME", "Unnamed");
	Sstor_Set("CL_NOKEY", "Not received");
	Sstor_Set("CL_VANILLA", "Vanilla client");

	Sstor_Set("KICK_NOREASON", "Kicked without reason");
	Sstor_Set("KICK_PROTOVER", "Invalid protocol version");
	Sstor_Set("KICK_NAMEINUSE", "This name is already in use");
	Sstor_Set("KICK_AUTHFAIL", "Authorization failed");
	Sstor_Set("KICK_MANYCONN", "Too many connections from one IP");
	Sstor_Set("KICK_FULL", "Server is full");
	Sstor_Set("KICK_PACKETSPAM", "Too many packets per second");
	Sstor_Set("KICK_PERR", "Packet reading error");
	Sstor_Set("KICK_UNKBID", "Invalid block ID");
	Sstor_Set("KICK_WCOMP", "World compression failed");
	Sstor_Set("KICK_STOP", "Server stopped");

	Sstor_Set("CFG_SVIP_COMM", "Bind server to specified IP address. \"0.0.0.0\" - means \"all available network adapters\".");
	Sstor_Set("CFG_SVIP_DVAL", "0.0.0.0");
	Sstor_Set("CFG_SVPORT_COMM", "Use specified port to accept clients. [1-65535]");
	Sstor_Set("CFG_SVNAME_COMM", "Server name and MOTD will be shown to the player during map loading.");
	Sstor_Set("CFG_SVNAME_DVAL", "Server name");
	Sstor_Set("CFG_SVMOTD_DVAL", "Server MOTD");
	Sstor_Set("CFG_LOGLVL_COMM", "I - Info, C - Chat, W - Warnings, D - Debug.");
	Sstor_Set("CFG_LOGLVL_DVAL", "ICWD");
	Sstor_Set("CFG_LOP_COMM", "Any player with ip address \"127.0.0.1\" will automatically become an operator.");
	Sstor_Set("CFG_MAXPL_COMM", "Max players on server. [1-127]");
	Sstor_Set("CFG_MAXCON_COMM", "Max connections per one IP. [1-5]");
	Sstor_Set("CFG_WORLDS_COMM", "List of worlds to load at startup. (Can be \"*\" it means load all worlds in the folder.)");
	Sstor_Set("CFG_WORLDS_DVAL", "world.cws:256x256x256:normal,flat_world.cws:256x256x256:flat");

	Sstor_Set("PLUG_DEPR_API", "Please upgrade your server software. Plugin \"%s\" compiled for PluginAPI v%03d, but server uses v%d.");
	Sstor_Set("PLUG_DEPR", "Plugin \"%s\" is deprecated. Server uses PluginAPI v%03d but plugin compiled for v%03d.");
	Sstor_Set("PLUG_LIBERR", "%s: %s");

	Sstor_Set("BLOCK_UNK", "Unknown block");

	Sstor_Set("Z_NOGZ", "Your zlib installation has no gzip support.");
	Sstor_Set("Z_LVL1", "Your zlib installation supports only one, lowest compression level!");
	Sstor_Set("Z_LVL2", "This means less CPU load in deflate tasks, but the worlds will take much more space on your disk.");
	Sstor_Set("Z_LVL3", "It also means a longer connection of players to the server.");

	return true;
}
