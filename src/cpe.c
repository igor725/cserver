#include "core.h"
#include "client.h"
#include "types/world.h"
#include "block.h"
#include "str.h"
#include "cpe.h"

static cs_str originalModelNames[16] = {
	"humanoid",
	"chicken",
	"creeper",
	"pig",
	"sheep",
	"skeleton",
	"sheep",
	"sheep_nofur",
	"skeleton",
	"spider",
	"zombie",
	"head",
	"sit",
	"chibi",
	NULL
};

static CPEModel *customModels[256] = {NULL};

cs_bool CPE_IsModelDefined(cs_byte id) {
	return customModels[id] != NULL || id < 15;
}

cs_bool CPE_IsModelDefinedPtr(CPEModel *model) {
	for(cs_int16 i = 0; i < 256; i++) {
		if(customModels[i] == model)
			return true;
	}
	return false;
}

cs_str CPE_GetDefaultModelName(void) {
	return customModels[0] ? customModels[0]->name : originalModelNames[0];
}

CPEModel *CPE_GetModel(cs_byte id) {
	return customModels[id];
}

cs_bool CPE_DefineModel(cs_byte id, CPEModel *model) {
	if(!model->part || !model->partsCount) return false;
	if(CPE_IsModelDefinedPtr(model)) return false;
	customModels[id] = model;
	for(ClientID i = 0; i < MAX_CLIENTS; i++) {
		Client *client = Clients_List[i];
		if(!client) continue;
		if(Client_GetExtVer(client, EXT_CUSTOMMODELS))
			Client_DefineModel(client, id, model);
	}
	return true;
}

cs_bool CPE_UndefineModel(cs_byte id) {
	if(!customModels[id]) return false;
	customModels[id] = NULL;
	for(ClientID i = 0; i < MAX_CLIENTS; i++) {
		Client *client = Clients_List[i];
		if(!client) continue;
		if(Client_GetExtVer(client, EXT_CUSTOMMODELS))
			Client_UndefineModel(client, id);
	}
	return true;
}

cs_bool CPE_UndefineModelPtr(CPEModel *mdl) {
	for(cs_int16 i = 0; i < 256 && mdl; i++) {
		if(customModels[i] == mdl)
			return CPE_UndefineModel((cs_byte)i);
	}
	return false;
}

cs_bool CPE_CheckModel(Client *client, cs_int16 model) {
	World *world = Client_GetWorld(client);
	if(world && model < 256)
		return Block_IsValid(world, (BlockID)model);
	return CPE_IsModelDefined(model % 256);
}

cs_int16 CPE_GetModelNum(cs_str model) {
	cs_int16 modelnum = -1;

	for(cs_int16 i = 0; i < 256; i++) {
		CPEModel *pmdl = customModels[i];
		if(pmdl && String_CaselessCompare(pmdl->name, model)) {
			modelnum = i + 256;
			break;
		}
	}

	if(modelnum == -1) {
		for(cs_int16 i = 0; originalModelNames[i]; i++) {
			cs_str cmdl = originalModelNames[i];
			if(!customModels[i] && String_CaselessCompare(model, cmdl)) {
				modelnum = i + 256;
				break;
			}
		}
	}

	if(modelnum == -1) {
		if(ISNUM(*model)) {
			cs_int32 tmp = String_ToInt(model);
			if(tmp >= 0 && tmp < 256)
				modelnum = (cs_int16)tmp;
		} else
			modelnum = 256;
	}

	return modelnum;
}

cs_uint32 CPE_GetModelStr(cs_int16 num, char *buffer, cs_uint32 buflen) {
	if(num > 255) { // За пределами 256 первых id находятся неблоковые модели
		cs_byte modelid = num % 256;
		cs_str mdl = NULL;
 		if(customModels[modelid])
			mdl = customModels[modelid]->name;
		else if(modelid < 15)
			mdl = originalModelNames[modelid];

		return mdl ? (cs_uint32)String_Copy(buffer, buflen, mdl) : 0;
	}

	cs_int32 ret = String_FormatBuf(buffer, buflen, "%d", num);
	return max(0, ret);
}

static void CubeNormalize(SVec *s, SVec *e) {
	cs_int16 tmp, *a = (cs_int16 *)s, *b = (cs_int16 *)e;
	for(int i = 0; i < 3; i++) {
		if(b[i] < a[i]) {
			tmp = b[i];
			b[i] = a[i];
			a[i] = tmp;
		}
		b[i]++;
	}
}

void Cuboid_SetPositions(CPECuboid *cub, SVec start, SVec end) {
	cub->pos[0] = start, cub->pos[1] = end;
	CubeNormalize(&cub->pos[0], &cub->pos[1]);
}

void Cuboid_SetColor(CPECuboid *cub, Color4 color) {
	cub->color = color;
}

cs_byte Cuboid_GetID(CPECuboid *cub) {
	return cub->id;
}

cs_uint32 Cuboid_GetSize(CPECuboid *cub) {
	return (cub->pos[1].x - cub->pos[0].x) *
	(cub->pos[1].y - cub->pos[0].y) *
	(cub->pos[1].z - cub->pos[0].z);
}

void Cuboid_GetPositions(CPECuboid *cub, SVec *start, SVec *end) {
	if(start) *start = cub->pos[0];
	if(end) *end = cub->pos[1];
}
