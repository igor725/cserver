#ifndef BOT_H
#define BOT_H
#include "core.h"
#include "types/client.h"

API Client *Bot_New(void);
API cs_bool Bot_IsBot(Client *bot);
API cs_bool Bot_Spawn(Client *bot, World *world);
#endif
