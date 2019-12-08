#ifndef SURV_DAMAGE_H
#define SURV_DAMAGE_H
void SurvDmg_Hurt(SurvivalData* target, SurvivalData* attacker, cs_uint8 damage);
void SurvDmg_Tick(SurvivalData* data, cs_uint32 delta);
#endif // SURV_DAMAGE_H
