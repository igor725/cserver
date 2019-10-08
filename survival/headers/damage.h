#ifndef SURV_DAMAGE_H
#define SURV_DAMAGE_H

#define SURV_DEFAULT_HIT 0.5

void SurvDamage_Hurt(SURVDATA target, SURVDATA attacker, float damage);
void SurvDamage_Tick(void);
#endif
