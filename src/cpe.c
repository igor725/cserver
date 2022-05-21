#include "core.h"
#include "client.h"
#include "protocol.h"
#include "types/world.h"
#include "block.h"
#include "str.h"
#include "cpe.h"

/**
 * CustomModel
 */

static cs_str originalModelNames[16] = {
	"humanoid", "chicken", "creeper",
	"pig", "sheep", "skeleton", "sheep",
	"sheep_nofur", "skeleton", "spider",
	"zombie", "head", "sit", "chibi",
	NULL
};

static CPEModel *customModels[CPE_MODELS_COUNT] = {NULL};

cs_bool CPE_IsModelDefined(cs_byte id) {
	if(id >= CPE_MODELS_COUNT) return false;
	return customModels[id] != NULL || id < 15;
}

cs_bool CPE_IsModelDefinedPtr(CPEModel *model) {
	for(cs_int16 i = 0; i < CPE_MODELS_COUNT; i++) {
		if(customModels[i] == model)
			return true;
	}
	return false;
}

cs_str CPE_GetDefaultModelName(void) {
	return customModels[0] ? customModels[0]->name : originalModelNames[0];
}

CPEModel *CPE_GetModel(cs_byte id) {
	if(id >= CPE_MODELS_COUNT) return NULL;
	return customModels[id];
}

void CPE_SendModel(Client *client, cs_int32 extVer, cs_byte id) {
	if(id >= CPE_MODELS_COUNT) return;
	CPEModel *model = customModels[id];
	if(!extVer || !model) return;
	CPE_WriteDefineModel(client, id, model);
	CPEModelPart *part = model->part;
	while(part) {
		CPE_WriteDefineModelPart(client, extVer, id, part);
		part = part->next;
	}
}

cs_bool CPE_DefineModel(cs_byte id, CPEModel *model) {
	if(id >= CPE_MODELS_COUNT) return false;
	if(!model->part || !model->partsCount) return false;
	if(CPE_IsModelDefinedPtr(model)) return false;
	customModels[id] = model;
	for(ClientID i = 0; i < MAX_CLIENTS; i++) {
		Client *client = Clients_List[i];
		if(!client) continue;
		cs_int32 extVer = Client_GetExtVer(client, EXT_CUSTOMMODELS);
		CPE_SendModel(client, extVer, id);
	}
	return true;
}

cs_bool CPE_UndefineModel(cs_byte id) {
	if(id >= CPE_MODELS_COUNT) return false;
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
	for(cs_int16 i = 0; i < CPE_MODELS_COUNT && mdl; i++) {
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

	for(cs_int16 i = 0; i < CPE_MODELS_COUNT; i++) {
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

cs_uint32 CPE_GetModelStr(cs_int16 num, cs_char *buffer, cs_uint32 buflen) {
	if(num > 255) { // За пределами 256 первых id находятся неблоковые модели
		cs_byte modelid = num % CPE_MODELS_COUNT;
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

/**
 * CustomParticles 
 */

static CPEParticle *customParticles[CPE_PARTICLES_COUNT] = {NULL};

cs_bool CPE_IsParticleDefined(cs_byte id) {
	if(id >= CPE_PARTICLES_COUNT) return false;
	return customParticles[id] != NULL;
}

cs_bool CPE_IsParticleDefinedPtr(CPEParticle *part) {
	for(cs_int16 i = 0; i < CPE_PARTICLES_COUNT && part; i++)
		if(part == customParticles[i])
			return true;
	return false;
}

CPEParticle *CPE_GetParticle(cs_byte id) {
	if(id >= CPE_PARTICLES_COUNT) return NULL;
	return customParticles[id];
}

void CPE_SendParticle(Client *client, cs_byte id) {
	if(id >= CPE_PARTICLES_COUNT) return;
	CPEParticle *part = customParticles[id];
	if(part) CPE_WriteDefineEffect(client, id, part);
}

cs_bool CPE_DefineParticle(cs_byte id, CPEParticle *part) {
	if(id >= CPE_PARTICLES_COUNT) return false;
	if(customParticles[id]) return false;
	if(CPE_IsParticleDefinedPtr(part)) return false;
	customParticles[id] = part;
	for(ClientID i = 0; i < MAX_CLIENTS; i++) {
		Client *client = Clients_List[i];
		if(!client) continue;
		if(Client_GetExtVer(client, EXT_CUSTOMPARTS))
			CPE_SendParticle(client, id);
	}
	return true;
}

cs_bool CPE_UndefineParticle(cs_byte id) {
	if(id >= CPE_PARTICLES_COUNT) return false;
	if(!customParticles[id]) return false;
	customParticles[id] = NULL;
	return true;
}

cs_bool CPE_UndefineParticlePtr(CPEParticle *ptr) {
	for(cs_int16 i = 0; i < CPE_PARTICLES_COUNT && ptr; i++) {
		if(customParticles[i] == ptr) {
			customParticles[i] = NULL;
			return true;
		}
	}
	return false;
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

/**
 * SelectionCuboid
 */

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
