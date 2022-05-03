#ifndef CPE_H
#define CPE_H
#include "core.h"
#include "types/cpe.h"
#include "types/client.h"
#include "vector.h"

API cs_bool CPE_IsModelDefined(cs_byte id);
API cs_bool CPE_IsModelDefinedPtr(CPEModel *model);
API cs_str CPE_GetDefaultModelName(void);
API CPEModel *CPE_GetModel(cs_byte id);
API cs_bool CPE_DefineModel(cs_byte id, CPEModel *model);
API cs_bool CPE_UndefineModel(cs_byte id);
API cs_bool CPE_UndefineModelPtr(CPEModel *mdl);
API cs_bool CPE_CheckModel(Client *client, cs_int16 model);
API void CPE_RegisterExtension(cs_str name, cs_int32 version);
API cs_int16 CPE_GetModelNum(cs_str model);
API cs_uint32 CPE_GetModelStr(cs_int16 num, char *buffer, cs_uint32 buflen);

API void Cuboid_SetPositions(CPECuboid *cub, SVec start, SVec end);
API void Cuboid_SetColor(CPECuboid *cub, Color4 color);
API cs_byte Cuboid_GetID(CPECuboid *cub);
API cs_uint32 Cuboid_GetSize(CPECuboid *cub);
API void Cuboid_GetPositions(CPECuboid *cub, SVec *start, SVec *end);
#endif
