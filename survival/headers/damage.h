#ifndef SURV_DAMAGE_H
#define SURV_DAMAGE_H
void SurvDmg_Hurt(SURVDATA target, SURVDATA attacker, cs_uint8 damage);
void SurvDmg_Tick(SURVDATA data, cs_uint32 delta);
#endif // SURV_DAMAGE_H
