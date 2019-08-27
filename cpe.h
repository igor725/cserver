#ifndef CPE_H
#define CPE_H

void CPEPacket_WriteInfo(CLIENT *cl);
void CPE_StartHandshake(CLIENT* client);
bool CPEHandler_ExtInfo(CLIENT* client, char* data);
bool CPEHandler_ExtEntry(CLIENT* client, char* data);

void CPEPacket_WriteInfo(CLIENT *client);
void CPEPacket_WriteExtEntry(CLIENT* client, EXT* ext);
void CPEPacket_WriteInventoryOrder(CLIENT* client, uchar order, BlockID block);
void CPEPacket_WriteHoldThis(CLIENT* client, BlockID block, bool preventChange);

EXT* firstExtension;
EXT* tailExtension;
int  extensionsCount;
#endif
