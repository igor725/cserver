#ifndef SURV_GUI_H
#define SURV_GUI_H

#define SURV_HEALTH_POS CPE_STATUS1
#define SURV_BREAK_POS CPE_ANNOUNCE
#define SURV_MAX_HEALTH 10

void SurvGui_DrawHealth(SURVDATA* data);
void SurvGui_DrawBreakProgress(SURVDATA* data);
void SurvGui_DrawAll(SURVDATA* data);
#endif
