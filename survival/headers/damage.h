#ifndef SURV_DAMAGE_H
#define SURV_DAMAGE_H

#define SURV_DEFAULT_HIT 0.5

void SurvDmg_Hurt(SURVDATA target, SURVDATA attacker, float damage);
void SurvDmg_Tick(void);
#endif
