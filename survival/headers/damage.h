#ifndef SURV_DAMAGE_H
#define SURV_DAMAGE_H
void SurvDmg_Hurt(SURVDATA target, SURVDATA attacker, uint8_t damage);
void SurvDmg_Tick(SURVDATA data, uint32_t delta);
#endif
